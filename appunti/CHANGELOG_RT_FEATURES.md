# Changelog - RT Features Implementation

## Data: 2024-12-16

## Nuove Funzionalità

### 1. Configurazione Real-Time per Task

Ogni task può ora specificare la propria configurazione real-time tramite YAML:

- **`rt_policy`**: Policy di scheduling (`"none"`, `"fifo"`, `"rr"`, `"deadline"`)
- **`rt_priority`**: Priorità RT (1-99, dove 99 è la massima)
- **`cpu_affinity`**: Core CPU su cui eseguire il task (-1 per nessuna affinità)

### 2. Test delle Primitive RT

È possibile testare il comportamento delle primitive real-time di Linux, in particolare:
- Preemption basata sulla priorità
- Affinità CPU
- Scheduling FIFO/RR

## Modifiche ai File

### File Modificati

1. **`include/schedule.h`**
   - Aggiunti campi RT a `ScheduledTask`:
     - `std::string rt_policy`
     - `int32_t rt_priority`
     - `int32_t cpu_affinity`

2. **`src/schedule.cpp`**
   - Parser YAML esteso per leggere i nuovi campi RT
   - Aggiunti defaults per i campi RT
   - Inizializzazione campi RT nel test schedule

3. **`proto/orchestrator.proto`**
   - Esteso `StartTaskRequest` con campi RT:
     - `string rt_policy = 6`
     - `int32 rt_priority = 7`
     - `int32 cpu_affinity = 8`

4. **`src/orchestrator.cpp`**
   - Modificato `execute_task()` per passare configurazione RT al task
   - I campi RT vengono inviati tramite gRPC al task

5. **`src/task_wrapper.cpp`**
   - Modificato `task_execution_thread()` per applicare configurazione RT
   - Configurazione RT dal request ha priorità su quella del wrapper
   - Log dettagliati della configurazione RT applicata

### File Nuovi

1. **`schedules/example_rt_priority_test.yaml`**
   - Esempio completo per testare priorità RT
   - Due task sullo stesso core con priorità diverse
   - Documentazione del comportamento atteso

2. **`appunti/RT_PRIORITY_TESTING.md`**
   - Guida completa all'uso delle funzionalità RT
   - Esempi di configurazione
   - Istruzioni per debugging
   - Note sui requisiti e limitazioni

3. **`test_rt_priority.sh`**
   - Script per testare facilmente la funzionalità RT
   - Build, avvio e esecuzione automatizzata del test

### File Aggiornati

1. **`schedules/template_full.yaml`**
   - Aggiunti campi RT ai defaults
   - Aggiunti campi RT ai template dei task
   - Nuova sezione "REAL-TIME CONFIGURATION" nella documentazione
   - Aggiornata sezione FIELD REFERENCE
   - Aggiunto riferimento all'esempio RT

2. **`schedules/example_hybrid.yaml`**
   - Aggiunti campi RT ai defaults (con valori "none" per compatibilità)

## Compatibilità

### Backward Compatibility

Tutte le modifiche sono **backward compatible**:
- I campi RT sono opzionali
- Default: `rt_policy = "none"`, `rt_priority = 50`, `cpu_affinity = -1`
- Gli schedule esistenti continuano a funzionare senza modifiche

### Protobuf

Il protobuf è stato esteso con nuovi campi opzionali:
- I client vecchi possono ignorare i nuovi campi
- I client nuovi possono usare i nuovi campi
- Nessuna breaking change

## Testing

### Test Manuale

```bash
# Build e test
./test_rt_priority.sh

# Oppure manualmente
docker-compose build
docker-compose up -d
docker-compose exec orchestrator /app/bin/orchestrator_main \
    /app/schedules/example_rt_priority_test.yaml
```

### Verifica Comportamento RT

1. **Log del sistema**: Verificare che i task vengano eseguiti nell'ordine corretto
2. **Preemption**: Task con priorità maggiore deve completare prima
3. **CPU Affinity**: Verificare con `taskset -p <PID>` all'interno del container

### Requisiti

- Docker con capability `SYS_NICE` (già configurato in `docker-compose.yml`)
- Kernel Linux con supporto RT (PREEMPT_RT o CONFIG_PREEMPT)

## Esempio di Utilizzo

```yaml
schedule:
  name: "RT Priority Test"
  
  defaults:
    rt_policy: "fifo"
    rt_priority: 50
    cpu_affinity: 0
  
  tasks:
    - id: low_priority
      rt_policy: "fifo"
      rt_priority: 30
      cpu_affinity: 0
      parameters:
        iterations: "1000000"
    
    - id: high_priority
      rt_policy: "fifo"
      rt_priority: 80
      cpu_affinity: 0
      depends_on: low_priority
      parameters:
        iterations: "500000"
```

## Note Tecniche

### Policy RT Supportate

- **FIFO**: Strict priority, no time slicing
- **RR**: Round-robin con time slicing
- **DEADLINE**: Scheduling basato su deadline (supporto base)
- **NONE**: Scheduler normale (CFS)

### Priorità

- Range: 1-99 (99 = massima priorità)
- Priorità maggiore preempta priorità minore
- Solo su stesso core CPU

### CPU Affinity

- Specifica il core CPU (0, 1, 2, ...)
- -1 = nessuna affinità (kernel decide)
- Utile per testare preemption (stesso core) o parallelismo (core diversi)

## Prossimi Passi

Possibili miglioramenti futuri:
1. Supporto completo per SCHED_DEADLINE con parametri runtime/deadline/period
2. Monitoring delle metriche RT (latency, jitter, context switches)
3. Validazione dei parametri RT nel parser YAML
4. Test automatizzati per verificare il comportamento RT
5. Documentazione estesa con esempi di use case reali

## Riferimenti

- Linux Real-Time Scheduling: https://www.kernel.org/doc/html/latest/scheduler/sched-rt-group.html
- SCHED_DEADLINE: https://www.kernel.org/doc/html/latest/scheduler/sched-deadline.html
- CPU Affinity: https://man7.org/linux/man-pages/man2/sched_setaffinity.2.html
