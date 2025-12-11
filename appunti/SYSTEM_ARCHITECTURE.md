# Architettura del Sistema Real-Time

## Schema Generale

```
┌─────────────────────────────────────────────────────────────────────────┐
│                         DOCKER ENVIRONMENT                               │
│                                                                          │
│  ┌────────────────────────────────────────────────────────────────┐    │
│  │  ORCHESTRATOR (grpc_orchestrator)                              │    │
│  │  ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━  │    │
│  │  🔴 Real-Time Mode: FIFO                                       │    │
│  │  ⚡ Priority: 80 (HIGHEST)                                      │    │
│  │  🎯 CPU Affinity: CPU 0 (dedicated)                            │    │
│  │  📡 Listen: 0.0.0.0:50050                                       │    │
│  │                                                                 │    │
│  │  [Scheduler Loop - SEQUENTIAL MODE]                            │    │
│  │         │                                                       │    │
│  │         ├─► 1. Launch Task 1 (thread)                          │    │
│  │         │   Wait for END signal ⏸                              │    │
│  │         │                                                       │    │
│  │         ├─► 2. Launch Task 2 (thread)                          │    │
│  │         │   Wait for END signal ⏸                              │    │
│  │         │                                                       │    │
│  │         └─► 3. Launch Task 3 (thread)                          │    │
│  │             Wait for END signal ⏸                              │    │
│  │                                                                 │    │
│  │  [gRPC Server - receives END notifications]                    │    │
│  └────────────────────────────────────────────────────────────────┘    │
│                                                                          │
│         │                    │                    │                     │
│         │ gRPC START         │ gRPC START         │ gRPC START          │
│         │ gRPC END ←         │ gRPC END ←         │ gRPC END ←          │
│         ↓                    ↓                    ↓                     │
│                                                                          │
│  ┌──────────────────┐  ┌──────────────────┐  ┌──────────────────┐    │
│  │  TASK 1          │  │  TASK 2          │  │  TASK 3          │    │
│  │  (grpc_task1)    │  │  (grpc_task2)    │  │  (grpc_task3)    │    │
│  │  ──────────────  │  │  ──────────────  │  │  ──────────────  │    │
│  │  🔴 RT: FIFO     │  │  🔴 RT: FIFO     │  │  🔴 RT: FIFO     │    │
│  │  ⚡ Priority: 75  │  │  ⚡ Priority: 75  │  │  ⚡ Priority: 75  │    │
│  │  🎯 CPU: 1       │  │  🎯 CPU: 2       │  │  🎯 CPU: 3       │    │
│  │  📡 Port: 50051  │  │  📡 Port: 50052  │  │  📡 Port: 50053  │    │
│  │                  │  │                  │  │                  │    │
│  │  [gRPC Server]   │  │  [gRPC Server]   │  │  [gRPC Server]   │    │
│  │  Waits for       │  │  Waits for       │  │  Waits for       │    │
│  │  START command   │  │  START command   │  │  START command   │    │
│  │                  │  │                  │  │                  │    │
│  │  [Task Function] │  │  [Task Function] │  │  [Task Function] │    │
│  │  Prints:         │  │  Prints:         │  │  Prints:         │    │
│  │  "/ciao"         │  │  " sono "        │  │  " Jacopo/"      │    │
│  │                  │  │                  │  │                  │    │
│  │  Sends END ✓     │  │  Sends END ✓     │  │  Sends END ✓     │    │
│  └──────────────────┘  └──────────────────┘  └──────────────────┘    │
│                                                                          │
└─────────────────────────────────────────────────────────────────────────┘
```

## Flusso di Esecuzione Sequenziale

```
TIME ──────────────────────────────────────────────────────────────────►

  ORCHESTRATOR     TASK 1           TASK 2           TASK 3
      │
      │ START ──────►│
      │              │ [RT Thread]
      │              │ CPU 1
      │              │ Priority 75
      │              │
      │              │ Print: "/ciao"
      │              │
      │              │ Execute...
      │              │
      │◄─── END ─────┤
      │              │
      │ WAIT ⏸       │
      │              │
      │ START ───────┼──────────────►│
      │              │               │ [RT Thread]
      │              │               │ CPU 2
      │              │               │ Priority 75
      │              │               │
      │              │               │ Print: " sono "
      │              │               │
      │              │               │ Execute...
      │              │               │
      │◄─── END ─────┼───────────────┤
      │              │               │
      │ WAIT ⏸       │               │
      │              │               │
      │ START ───────┼───────────────┼──────────────►│
      │              │               │               │ [RT Thread]
      │              │               │               │ CPU 3
      │              │               │               │ Priority 75
      │              │               │               │
      │              │               │               │ Print: " Jacopo/"
      │              │               │               │
      │              │               │               │ Execute...
      │              │               │               │
      │◄─── END ─────┼───────────────┼───────────────┤
      │              │               │               │
      │ DONE ✓       │               │               │
      │              │               │               │
   EXIT(0)           │               │               │
                  (idle)          (idle)          (idle)
```

## Dettagli Real-Time

### CPU Mapping
```
┌─────────┬──────────────────┬──────────┬──────────┐
│  CPU    │   Component      │ Priority │  Policy  │
├─────────┼──────────────────┼──────────┼──────────┤
│  CPU 0  │  Orchestrator    │    80    │   FIFO   │
│  CPU 1  │  Task 1          │    75    │   FIFO   │
│  CPU 2  │  Task 2          │    75    │   FIFO   │
│  CPU 3  │  Task 3          │    75    │   FIFO   │
└─────────┴──────────────────┴──────────┴──────────┘
```

### Comunicazione gRPC

```
ORCHESTRATOR                              TASK
    │                                       │
    │  StartTaskRequest                     │
    │  ─────────────────────────────────►   │
    │  {                                    │
    │    task_id: "task_1"                  │
    │    parameters: {...}                  │
    │  }                                    │
    │                                       │
    │                                       │ [Executes]
    │                                       │ [Prints string]
    │                                       │
    │  TaskEndNotification                  │
    │  ◄─────────────────────────────────   │
    │  {                                    │
    │    task_id: "task_1"                  │
    │    result: SUCCESS                    │
    │    duration_us: 505954                │
    │  }                                    │
    │                                       │
    │  TaskEndResponse                      │
    │  ─────────────────────────────────►   │
    │  {                                    │
    │    acknowledged: true                 │
    │  }                                    │
    │                                       │
```

## Sincronizzazione

### Condition Variable Mechanism

```cpp
// Orchestrator Scheduler Loop
for (task in tasks) {
    // 1. Launch task on separate thread
    std::thread([task]() {
        execute_task(task);
    }).detach();
    
    // 2. Wait for task to be registered
    task_end_cv_.wait_for(..., [task]() {
        return active_tasks_.find(task.id) != end();
    });
    
    // 3. Wait for task to complete
    task_end_cv_.wait(..., [task]() {
        return active_tasks_.find(task.id) == end();
    });
    
    // 4. Proceed to next task
}
```

### State Transitions

```
TASK STATE MACHINE:

  IDLE ──► STARTING ──► RUNNING ──► COMPLETED ──► IDLE
   │                                      │
   │                                      │
   └──────────────────────────────────────┘
          (ready for next execution)
```

## Caratteristiche Chiave

### ✅ Real-Time Scheduling
- **FIFO Policy**: Scheduling deterministico
- **Fixed Priorities**: Orchestrator (80) > Tasks (75)
- **No Preemption**: Tasks run to completion
- **CPU Isolation**: Each component on dedicated CPU

### ✅ Sequential Execution
- **Strict Ordering**: Task N+1 starts ONLY after Task N completes
- **Signal-Based**: Uses END notifications for synchronization
- **Thread-Based**: Each task runs on separate thread
- **One-Shot**: No loops, no restarts

### ✅ Deterministic Behavior
- **Predictable Timing**: RT scheduling ensures consistency
- **No Context Switching**: CPU affinity reduces overhead
- **Priority Inversion Protection**: FIFO policy prevents issues
- **Bounded Latency**: Real-time guarantees

## Output Esempio

```bash
$ sudo docker-compose up

[Orchestrator] Scheduler loop started (SEQUENTIAL MODE)
[Orchestrator] Real-time configuration applied: FIFO, Priority 80, CPU 0

[Orchestrator] Starting task 1/3: task_1
[Task task_1] Real-time configuration applied: FIFO, Priority 75, CPU 1
[Task task_1] /ciao
[Orchestrator] Task task_1 completed and acknowledged

[Orchestrator] Starting task 2/3: task_2
[Task task_2] Real-time configuration applied: FIFO, Priority 75, CPU 2
[Task task_2]  sono 
[Orchestrator] Task task_2 completed and acknowledged

[Orchestrator] Starting task 3/3: task_3
[Task task_3] Real-time configuration applied: FIFO, Priority 75, CPU 3
[Task task_3]  Jacopo/
[Orchestrator] Task task_3 completed and acknowledged

[Orchestrator] All tasks completed successfully!
grpc_orchestrator exited with code 0
```

## Vantaggi del Design

1. **Determinismo**: Esecuzione prevedibile grazie a RT scheduling
2. **Isolamento**: Ogni componente su CPU dedicata
3. **Sincronizzazione Robusta**: Condition variables per coordinamento
4. **Scalabilità**: Facile aggiungere nuovi task
5. **Monitoraggio**: gRPC permette osservabilità completa
6. **Flessibilità**: Parametri configurabili via docker-compose

## Requisiti Sistema

- **Linux Kernel** con supporto PREEMPT_RT (opzionale ma consigliato)
- **Docker** con capabilities appropriate
- **Multi-core CPU** (minimo 4 core per questa configurazione)
- **Privilegi sudo** per applicare RT scheduling
