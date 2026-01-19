# Context Switch Measurement

## Overview

Il sistema ora misura il **tempo di context switch** tra l'esecuzione di task consecutivi. Questo tempo rappresenta l'overhead dell'orchestrator tra la fine di un task e l'inizio del successivo.

## Cosa Misura il Context Switch Time

Il **context switch time** include tutto l'overhead necessario per passare da un task a quello successivo:

```
┌─────────────┐         ┌─────────────┐         ┌─────────────┐
│   Task N    │         │  OVERHEAD   │         │  Task N+1   │
│  Running    │ ───────►│             │ ───────►│  Starting   │
└─────────────┘         └─────────────┘         └─────────────┘
      ↑                        ↑                       ↑
      │                        │                       │
   end_time         Context Switch Time         start_time
                                                 (next task)
```

### Componenti del Context Switch

1. **Ricezione notifica TaskEnd** - latenza gRPC + deserialization
2. **Processing orchestrator** - aggiornamento stato, logging, mutex
3. **Wake-up thread sequenziale** - condition variable notify/wait
4. **Decisione scheduling** - controllo dipendenze, selezione task
5. **Setup nuovo task** - creazione stub gRPC, preparazione parametri
6. **Invio StartTask** - latenza gRPC + serialization

## Implementazione

### 1. Struttura Dati

```cpp
struct TaskExecution {
    std::string task_id;
    int64_t scheduled_time_us;
    int64_t actual_start_time_us;
    int64_t end_time_us;
    int64_t estimated_duration_us;
    int64_t context_switch_time_us;  // ← NUOVO CAMPO
    TaskState state;
    TaskResult result;
    std::string error_message;
    std::string output_data_json;
};
```

### 2. Misurazione

**Step 1**: Quando un task finisce, salviamo il tempo:

```cpp
void Orchestrator::on_task_end(const TaskEndNotification& notification) {
    int64_t task_end_time = get_current_time_us() - start_time_us_;
    
    // ... aggiorna stato task ...
    
    // Update last task end time for context switch measurement
    last_task_end_time_us_ = task_end_time;
}
```

**Step 2**: Quando il prossimo task inizia, calcoliamo la differenza:

```cpp
void Orchestrator::execute_task(const ScheduledTask& task) {
    int64_t task_start_time = get_current_time_us() - start_time_us_;
    
    // Calculate context switch time
    if (last_task_end_time_us_ > 0) {
        exec.context_switch_time_us = task_start_time - last_task_end_time_us_;
        
        std::cout << "[Orchestrator] ⏱️  Context Switch Time: " 
                  << exec.context_switch_time_us << " µs ("
                  << (exec.context_switch_time_us / 1000.0) << " ms)" << std::endl;
    } else {
        exec.context_switch_time_us = 0;  // First task, no context switch
    }
}
```

### 3. Formula

```
context_switch_time = task[n+1].actual_start_time_us - task[n].end_time_us
```

Dove:
- `task[n].end_time_us` = quando orchestrator riceve TaskEnd notification
- `task[n+1].actual_start_time_us` = quando orchestrator invia StartTask command

## Output e Statistiche

### Durante l'Esecuzione

Quando ogni task (tranne il primo) inizia:

```
[Orchestrator] ⏱️  Context Switch Time: 1234 µs (1.234 ms)
```

### Alla Fine dell'Esecuzione

Report completo con statistiche:

```
[Orchestrator] === Context Switch Statistics ===
[Orchestrator] Task task_2: 1234 µs (1.234 ms)
[Orchestrator] Task task_3: 987 µs (0.987 ms)
[Orchestrator] Task task_4: 1456 µs (1.456 ms)

[Orchestrator] Context Switch Summary:
  - Count: 3
  - Average: 1225.67 µs (1.226 ms)
  - Min: 987 µs (0.987 ms)
  - Max: 1456 µs (1.456 ms)
  - Total: 3677 µs (3.677 ms)
[Orchestrator] ===================================
```

## Analisi e Interpretazione

### Valori Tipici

In un ambiente Docker locale:
- **Context switch buono**: 500-2000 µs (0.5-2 ms)
- **Context switch medio**: 2000-5000 µs (2-5 ms)
- **Context switch alto**: > 5000 µs (> 5 ms)

### Fattori che Influenzano il Context Switch

#### 1. Modalità di Esecuzione

**Sequential Mode**:
```yaml
- id: task_2
  mode: sequential
  depends_on: task_1
```
- Context switch più prevedibile
- Include wait su condition variable
- Overhead di sincronizzazione mutex

**Timed Mode**:
```yaml
- id: task_2
  mode: timed
  scheduled_time_us: 5000000
```
- Context switch può essere zero (task in parallelo)
- Non misura context switch se tasks non sono sequenziali

#### 2. Configurazione Real-Time

Con RT scheduling:
```yaml
rt_policy: "fifo"
rt_priority: 80
```
- Context switch generalmente più basso
- Maggiore priorità = meno preemption
- CPU affinity riduce cache miss

#### 3. Carico di Sistema

- Containers concorrenti aumentano context switch
- Contention su CPU/memoria
- Docker overhead
- Network latency gRPC

### Ottimizzazione

#### ✅ Ridurre Context Switch

1. **Usare RT scheduling**:
```yaml
defaults:
  rt_policy: "fifo"
  rt_priority: 80
```

2. **CPU Affinity**:
```yaml
defaults:
  cpu_affinity: 1  # Pin a CPU core
```

3. **Ridurre logging**:
- Logging I/O può introdurre latenza

4. **Batch processing**:
- Raggruppare task piccoli quando possibile

#### ⚠️ Trade-offs

- RT priority alta può affamare altri processi
- CPU affinity riduce load balancing
- Context switch basso ≠ performance migliore (dipende dal workload)

## Best Practices

### 1. Monitoraggio

Controlla sempre le statistiche alla fine:
```cpp
auto history = orchestrator.get_execution_history();
for (const auto& task : history) {
    std::cout << task.task_id << ": " 
              << task.context_switch_time_us << " µs" << std::endl;
}
```

### 2. Baseline

Stabilisci una baseline per il tuo sistema:
- Esegui 10+ run con stesso schedule
- Calcola media e deviazione standard
- Identifica outliers

### 3. Confronti

Per valutare ottimizzazioni:
```bash
# Before optimization
Context Switch Average: 3500 µs

# After enabling RT scheduling
Context Switch Average: 1200 µs  # ✅ 66% improvement
```

### 4. Task Dependency Chain

Per catene lunghe di task sequenziali:
```yaml
task_1 → task_2 → task_3 → task_4 → task_5
```

Total overhead = sum of all context switches:
```
Total = CS(1→2) + CS(2→3) + CS(3→4) + CS(4→5)
```

## Limitazioni

1. **Solo task sequenziali**: Context switch significativo solo per task con dipendenze
2. **Primo task**: Sempre `context_switch_time_us = 0`
3. **Task paralleli**: Non misura "context switch" (eseguono concorrentemente)
4. **Overhead incluso**: Include latenza gRPC (non solo CPU context switch puro)

## Esempio Pratico

### Schedule Configuration

```yaml
schedule:
  name: "Sequential Pipeline"
  
  tasks:
    - id: task_1
      mode: sequential
      # context_switch_time_us = 0 (primo task)
      
    - id: task_2
      mode: sequential
      depends_on: task_1
      # context_switch_time_us = T2_start - T1_end
      
    - id: task_3
      mode: sequential
      depends_on: task_2
      # context_switch_time_us = T3_start - T2_end
```

### Output Atteso

```
[Orchestrator] ⏱️  Context Switch Time: 0 µs (0.000 ms)       # task_1
[Orchestrator] ⏱️  Context Switch Time: 1523 µs (1.523 ms)   # task_2
[Orchestrator] ⏱️  Context Switch Time: 1298 µs (1.298 ms)   # task_3

Context Switch Summary:
  - Count: 2
  - Average: 1410.5 µs (1.411 ms)
  - Min: 1298 µs (1.298 ms)
  - Max: 1523 µs (1.523 ms)
  - Total: 2821 µs (2.821 ms)
```

## Riferimenti Codice

- **Header**: `include/orchestrator.h` - TaskExecution struct
- **Implementation**: `src/orchestrator.cpp`
  - `on_task_end()` - aggiorna `last_task_end_time_us_`
  - `execute_task()` - calcola e logga context switch
  - `scheduler_loop()` - stampa statistiche finali

## Conclusioni

La misurazione del context switch fornisce insight preziosi su:
- ✅ Overhead dell'orchestrator
- ✅ Efficienza della pipeline sequenziale
- ✅ Impatto della configurazione RT
- ✅ Identificazione di bottleneck

Usa queste metriche per ottimizzare la configurazione del tuo sistema real-time.
