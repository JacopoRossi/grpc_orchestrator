# Time Measurement System

## Overview

Il sistema utilizza un **modello centralizzato di misurazione dei tempi** dove **solo l'Orchestrator** misura i tempi di inizio e fine di ogni task. Questo garantisce:
- ✅ Misurazione consistente e centralizzata
- ✅ Nessuna dipendenza dai clock dei task
- ✅ Semplicità e affidabilità

## Clock Selection

### `system_clock` - Global Synchronization
- **Used for**: All timestamp measurements (start, end, duration)
- **Precision**: Microseconds (µs)
- **Scope**: Global across all containers
- **Synchronization**: Via host system clock sharing

### Why `system_clock` instead of `steady_clock`?

| Aspect | `steady_clock` | `system_clock` |
|--------|----------------|----------------|
| **Monotonic** | ✅ Yes | ⚠️ Can be adjusted |
| **Cross-container** | ❌ No (independent per container) | ✅ Yes (synchronized) |
| **Comparable timestamps** | ❌ No | ✅ Yes |
| **Best for** | Single-process duration | Distributed system timing |

In a **multi-container environment**, `system_clock` is essential for:
- Comparing timestamps between orchestrator and tasks
- Measuring end-to-end latency including gRPC communication
- Global event ordering
- Accurate performance analysis

## Time Synchronization

### Docker Configuration

All containers share the host's system clock via volume mounts:

```yaml
volumes:
  - /etc/localtime:/etc/localtime:ro
  - /etc/timezone:/etc/timezone:ro
environment:
  - TZ=UTC
```

This ensures:
- All containers use the same time reference
- Timestamps are directly comparable
- No clock drift between containers

## Measured Metrics

### 1. Task Start Time
```cpp
// In orchestrator.cpp::execute_task()
int64_t task_start_time = get_current_time_us() - start_time_us_;
exec.actual_start_time_us = task_start_time;
```
- **Measured by**: Orchestrator
- **When**: IMMEDIATAMENTE prima di inviare il comando StartTask
- **Precision**: Microseconds
- **Includes**: Setup time + gRPC communication

### 2. Task End Time
```cpp
// In orchestrator.cpp::on_task_end()
int64_t task_end_time = get_current_time_us() - start_time_us_;
exec.end_time_us = task_end_time;
```
- **Measured by**: Orchestrator
- **When**: IMMEDIATAMENTE quando riceve la notifica TaskEnd
- **Precision**: Microseconds
- **Includes**: Task execution + gRPC communication back

### 3. Task Execution Duration (Measured by Orchestrator)
```cpp
execution_duration_us = end_time_us - actual_start_time_us
```
- **Calculated by**: Orchestrator
- **Represents**: Tempo totale dal momento in cui l'orchestrator invia il comando fino a quando riceve la notifica di completamento
- **Includes**: 
  - Latenza gRPC andata
  - Esecuzione del task
  - Latenza gRPC ritorno

## Timing Guarantees

### Vantaggi del Modello Centralizzato

✅ **Misurazione consistente** - Un solo punto di misurazione elimina discrepanze
✅ **Nessuna dipendenza dai task** - I task non devono preoccuparsi dei tempi
✅ **Semplificazione** - Logica di timing concentrata nell'orchestrator
✅ **Misurazione end-to-end** - Include latenza gRPC e overhead di comunicazione
✅ **Affidabilità** - Solo il clock dell'orchestrator è rilevante

### Cosa Misura

- **Start Time**: Momento in cui l'orchestrator decide di eseguire il task
- **End Time**: Momento in cui l'orchestrator riceve conferma di completamento
- **Duration**: Tempo totale percepito dall'orchestrator (include tutto l'overhead)

### Best Practices

1. **I task non misurano tempi** - Tutta la misurazione è gestita dall'orchestrator
2. **Duration include overhead** - Tieni conto che include comunicazione gRPC
3. **Tempi relativi** - Tutti i tempi sono relativi allo `start_time_us_` dell'orchestrator
4. **Analisi performance** - Usa i tempi misurati per confrontare esecuzioni diverse
5. **Logging** - L'orchestrator logga tutti i tempi in modo centralizzato

## Code References

### Orchestrator
```cpp
// src/orchestrator.cpp:450-454
int64_t Orchestrator::get_current_time_us() const {
    return std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
}
```

### Task Wrapper
```cpp
// src/task_wrapper.cpp:320-324
int64_t TaskWrapper::get_current_time_us() const {
    return std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
}
```

## Example Analysis

Con il modello centralizzato:

```cpp
// Orchestrator misura start time (prima di inviare StartTask)
task_start_time = 1000000 µs  // (relativo a start_time_us_)

// Orchestrator invia StartTask via gRPC
// ... task esegue ...
// Task invia TaskEnd notification

// Orchestrator misura end time (quando riceve TaskEnd)
task_end_time = 1500000 µs  // (relativo a start_time_us_)

// Duration calcolata dall'orchestrator
duration = 500000 µs (0.5 seconds)
// Questa include: gRPC latency + execution + gRPC return
```

Tutti i tempi sono misurati dall'orchestrator con un **singolo clock di riferimento**.

## Migration Notes

### Cambiamento: Da Task Measurement a Orchestrator-Only Measurement

**Prima**: 
- I task misuravano `start_time_us` e `end_time_us`
- Inviavano i tempi all'orchestrator via gRPC
- L'orchestrator usava i tempi ricevuti dai task

**Dopo**:
- Solo l'orchestrator misura i tempi
- Task start: misurato PRIMA di chiamare `StartTask()`
- Task end: misurato QUANDO si riceve `TaskEnd()` notification
- I task non devono più preoccuparsi della misurazione del tempo

**Impact**: 
- ✅ Semplificazione del codice dei task
- ✅ Misurazione più affidabile e centralizzata
- ✅ Nessuna dipendenza da sincronizzazione clock tra container
- ✅ Duration include l'overhead completo (più realistico)
