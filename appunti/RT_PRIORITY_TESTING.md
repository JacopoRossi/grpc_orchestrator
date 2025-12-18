# Real-Time Priority Testing Guide

## Overview

Il sistema ora supporta la configurazione real-time per ogni task, permettendo di:
- Scegliere la policy di scheduling RT (`fifo`, `rr`, `deadline`)
- Impostare la priorità RT (1-99, dove 99 è la massima)
- Specificare l'affinità CPU (su quale core eseguire il task)

Questo permette di testare le primitive real-time di Linux, in particolare la preemption basata sulla priorità.

## Configurazione YAML

### Campi RT disponibili

Ogni task può specificare:

```yaml
- id: task_id
  # ... altri campi ...
  rt_policy: "fifo"      # Policy RT: "none", "fifo", "rr", "deadline"
  rt_priority: 80        # Priorità RT: 1-99 (99 = massima)
  cpu_affinity: 0        # Core CPU: 0, 1, 2, ... o -1 (nessuna affinità)
```

### Defaults globali

Puoi impostare valori di default per tutti i task:

```yaml
schedule:
  defaults:
    rt_policy: "fifo"
    rt_priority: 50
    cpu_affinity: -1
```

## Test delle Primitive RT

### Scenario: Due task sullo stesso core con priorità diverse

File: `schedules/example_rt_priority_test.yaml`

Questo esempio dimostra il comportamento RT corretto:

1. **Task 1**: Priorità RT 30, core 0, esecuzione lunga (3 secondi)
2. **Task 3**: Priorità RT 80, core 0, esecuzione breve (2 secondi)

**Comportamento atteso:**
1. Task 1 inizia per primo (priorità 30)
2. Task 3 inizia dopo che task 1 è partito (grazie a `depends_on`)
3. Task 3 **preempta** task 1 perché ha priorità maggiore (80 > 30)
4. Task 3 completa per primo (anche se è partito dopo)
5. Task 1 riprende e completa dopo task 3

Questo dimostra che `SCHED_FIFO` con priorità maggiore preempta task con priorità minore sullo stesso core.

## Policy RT

### SCHED_FIFO (`fifo`)
- **First In First Out** con priorità stretta
- Un task con priorità maggiore preempta sempre uno con priorità minore
- Nessun time slicing tra task della stessa priorità
- **Ideale per**: Test di preemption, task critici

### SCHED_RR (`rr`)
- **Round Robin** con time slicing
- Come FIFO ma con time slicing tra task della stessa priorità
- **Ideale per**: Task RT che devono condividere la CPU equamente

### SCHED_DEADLINE (`deadline`)
- Scheduling basato su deadline
- Richiede parametri aggiuntivi (runtime, deadline, period)
- **Nota**: Attualmente supportato solo a livello base

### None (`none`)
- Nessuno scheduling RT (default)
- Usa lo scheduler normale del kernel (CFS)

## Requisiti

### Capabilities Docker

Per usare le primitive RT in Docker, è necessario il capability `SYS_NICE`:

```yaml
# docker-compose.yml
services:
  orchestrator:
    cap_add:
      - SYS_NICE
  
  task1:
    cap_add:
      - SYS_NICE
```

Oppure da riga di comando:

```bash
docker run --cap-add=SYS_NICE ...
```

### Verifica capabilities

Controlla se il container ha i permessi necessari:

```bash
# All'interno del container
capsh --print | grep sys_nice
```

## Esempi di Configurazione

### Test 1: Priorità su stesso core

```yaml
tasks:
  - id: low_priority_task
    rt_policy: "fifo"
    rt_priority: 30
    cpu_affinity: 0
    
  - id: high_priority_task
    rt_policy: "fifo"
    rt_priority: 80
    cpu_affinity: 0
    depends_on: low_priority_task
```

### Test 2: Task su core diversi

```yaml
tasks:
  - id: task_core_0
    rt_policy: "fifo"
    rt_priority: 50
    cpu_affinity: 0
    
  - id: task_core_1
    rt_policy: "fifo"
    rt_priority: 50
    cpu_affinity: 1
```

### Test 3: Mix RT e non-RT

```yaml
tasks:
  - id: rt_task
    rt_policy: "fifo"
    rt_priority: 90
    cpu_affinity: 0
    
  - id: normal_task
    rt_policy: "none"    # Scheduler normale
    cpu_affinity: -1     # Nessuna affinità
```

## Debugging

### Log RT

Il sistema stampa informazioni sulla configurazione RT applicata:

```
[Task task_1] Applying RT config: policy=fifo, priority=30, cpu_affinity=0
```

### Verifica scheduling

All'interno del container, puoi verificare la policy e priorità:

```bash
# Trova il PID del task
ps aux | grep task_main

# Verifica policy e priorità
chrt -p <PID>
```

### Verifica affinità CPU

```bash
# Verifica su quale core sta girando
taskset -p <PID>
```

## Note Importanti

1. **Priorità RT vs Priority normale**: 
   - `priority` (0-100) è per la logica applicativa
   - `rt_priority` (1-99) è per lo scheduler RT del kernel

2. **Preemption**:
   - Funziona solo con task sullo stesso core
   - Richiede `SCHED_FIFO` o `SCHED_RR`
   - La priorità RT deve essere diversa

3. **Performance**:
   - Task RT possono monopolizzare la CPU
   - Usa con cautela in produzione
   - Ideale per test e sistemi embedded

4. **Limiti del sistema**:
   - Controlla `/proc/sys/kernel/sched_rt_runtime_us`
   - Controlla `/proc/sys/kernel/sched_rt_period_us`
   - Potrebbero limitare il tempo RT disponibile

## Riferimenti

- `schedules/example_rt_priority_test.yaml` - Esempio completo di test RT
- `schedules/template_full.yaml` - Template con tutti i campi RT
- `include/rt_utils.h` - Utility per configurazione RT
- `proto/orchestrator.proto` - Definizione protobuf con campi RT
