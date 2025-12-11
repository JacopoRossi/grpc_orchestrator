# Fix: Task TIMED Eseguiti in Parallelo

## 🐛 Problema Identificato

Con la configurazione:
```cpp
task1.execution_mode = TASK_MODE_SEQUENTIAL;
task1.scheduled_time_us = 0;

task2.execution_mode = TASK_MODE_TIMED;
task2.scheduled_time_us = 0;  // Dovrebbe partire a tempo 0!

task3.execution_mode = TASK_MODE_SEQUENTIAL;
task3.wait_for_task_id = "task_1";
```

**Comportamento SBAGLIATO** (prima):
```
T=0ms:     Task 1 PARTE
T=500ms:   Task 1 FINISCE
           Task 2 PARTE  ← SBAGLIATO! Doveva partire a T=0!
           Task 3 PARTE
```

Task 2 partiva **dopo** Task 1, anche se era schedulato a tempo 0!

## 🔍 Causa del Problema

Lo scheduler processava i task **sequenzialmente nel loop**:

```cpp
// VECCHIO CODICE (SBAGLIATO):
for (size_t i = 0; i < schedule_.tasks.size(); i++) {
    const ScheduledTask& task = schedule_.tasks[i];
    
    // Lancia il task
    launch_task(task);
    
    // Se SEQUENTIAL, aspetta che finisca
    if (task.execution_mode == TASK_MODE_SEQUENTIAL) {
        wait_for_completion(task);  // ← BLOCCA QUI!
    }
}
```

Quando Task 1 (SEQUENTIAL) veniva lanciato, lo scheduler **si bloccava** aspettando il suo completamento prima di passare a Task 2.

## ✅ Soluzione Implementata

Lo scheduler ora funziona in **2 FASI**:

### FASE 1: Lancia tutti i task TIMED
```cpp
// FASE 1: Lancia TUTTI i task TIMED immediatamente
for (const auto& task : schedule_.tasks) {
    if (task.execution_mode == TASK_MODE_TIMED) {
        std::thread([this, task]() {
            // Aspetta il tempo schedulato NEL THREAD
            wait_until(task.scheduled_time_us);
            execute_task(task);
        }).detach();
    }
}
```

### FASE 2: Processa i task SEQUENTIAL
```cpp
// FASE 2: Processa i task SEQUENTIAL in ordine
for (const auto& task : schedule_.tasks) {
    if (task.execution_mode == TASK_MODE_SEQUENTIAL) {
        // Aspetta dipendenze
        if (!task.wait_for_task_id.empty()) {
            wait_for_task(task.wait_for_task_id);
        }
        
        // Lancia e aspetta completamento
        launch_task(task);
        wait_for_completion(task);
    }
}
```

## 📊 Comportamento Corretto Ora

Con la stessa configurazione:

```
T=0ms:     FASE 1: Task 2 (TIMED) viene lanciato in un thread
           FASE 2: Task 1 (SEQUENTIAL) PARTE
           → Task 2 e Task 1 partono INSIEME!
           
T=500ms:   Task 1 FINISCE
           Task 2 FINISCE (partito a T=0)
           Task 3 PARTE (dipendenza da Task 1 soddisfatta)
           
T=1000ms:  Task 3 FINISCE
```

## 🎯 Esempi di Configurazione

### Esempio 1: Task TIMED a tempi diversi

```cpp
task1.execution_mode = TASK_MODE_TIMED;
task1.scheduled_time_us = 0;        // Parte subito

task2.execution_mode = TASK_MODE_TIMED;
task2.scheduled_time_us = 2000000;  // Parte a 2 secondi

task3.execution_mode = TASK_MODE_TIMED;
task3.scheduled_time_us = 5000000;  // Parte a 5 secondi
```

**Timeline**:
```
T=0ms:     Task 1, 2, 3 vengono TUTTI lanciati (FASE 1)
           Task 1 parte subito (scheduled_time = 0)
T=2000ms:  Task 2 parte (scheduled_time = 2s)
T=5000ms:  Task 3 parte (scheduled_time = 5s)
```

### Esempio 2: Mix TIMED e SEQUENTIAL

```cpp
task1.execution_mode = TASK_MODE_SEQUENTIAL;  // Parte subito
task1.wait_for_task_id = "";

task2.execution_mode = TASK_MODE_TIMED;       // Parte a 1 secondo
task2.scheduled_time_us = 1000000;

task3.execution_mode = TASK_MODE_SEQUENTIAL;  // Parte dopo task_1
task3.wait_for_task_id = "task_1";
```

**Timeline**:
```
T=0ms:     FASE 1: Task 2 lanciato (aspetterà 1s nel suo thread)
           FASE 2: Task 1 parte
T=500ms:   Task 1 finisce
           Task 3 parte (dipendenza soddisfatta)
T=1000ms:  Task 2 parte (tempo schedulato raggiunto)
T=1500ms:  Task 3 finisce
T=1500ms:  Task 2 finisce
```

### Esempio 3: Tutti SEQUENTIAL (catena)

```cpp
task1.execution_mode = TASK_MODE_SEQUENTIAL;
task1.wait_for_task_id = "";

task2.execution_mode = TASK_MODE_SEQUENTIAL;
task2.wait_for_task_id = "task_1";

task3.execution_mode = TASK_MODE_SEQUENTIAL;
task3.wait_for_task_id = "task_2";
```

**Timeline**:
```
T=0ms:     FASE 1: Nessun task TIMED
           FASE 2: Task 1 parte
T=500ms:   Task 1 finisce, Task 2 parte
T=1000ms:  Task 2 finisce, Task 3 parte
T=1500ms:  Task 3 finisce
```

## 🔬 Verifica nei Log

### Log FASE 1 (TIMED):
```
[Orchestrator] === PHASE 1: Launching TIMED tasks ===

Launching TIMED task: task_2 (scheduled at 0 ms)
[Orchestrator] Task task_2 starting at scheduled time
```

### Log FASE 2 (SEQUENTIAL):
```
[Orchestrator] === PHASE 2: Processing SEQUENTIAL tasks ===

========================================
Processing SEQUENTIAL task: task_1
========================================

[Orchestrator] Task task_1 started successfully
[Orchestrator] Task task_1 completed

========================================
Processing SEQUENTIAL task: task_3 (waiting for task_1)
========================================

[Orchestrator] Dependency satisfied, starting task...
[Orchestrator] Task task_3 started successfully
[Orchestrator] Task task_3 completed
```

## 📈 Execution Summary

Ora i tempi sono corretti:

```
=== Execution Summary ===
Task: task_1
  Scheduled: 0 us
  Started: ~100 us          ← Parte subito
  Ended: ~500100 us
  Duration: ~500000 us

Task: task_2
  Scheduled: 0 us
  Started: ~200 us          ← Parte INSIEME a task_1!
  Ended: ~500200 us
  Duration: ~500000 us

Task: task_3
  Scheduled: 0 us
  Started: ~500300 us       ← Parte dopo task_1
  Ended: ~1000300 us
  Duration: ~500000 us
```

## 🎯 Vantaggi della Nuova Implementazione

1. **Parallelismo reale**: Task TIMED partono tutti insieme
2. **Timing preciso**: Ogni task TIMED aspetta il suo tempo nel proprio thread
3. **Dipendenze corrette**: Task SEQUENTIAL rispettano le dipendenze
4. **Flessibilità**: Puoi mescolare liberamente TIMED e SEQUENTIAL
5. **Efficienza**: Nessun blocco inutile dello scheduler

## 🚀 Come Testare

### Test 1: Due task a tempo 0

```cpp
task1.execution_mode = TASK_MODE_TIMED;
task1.scheduled_time_us = 0;

task2.execution_mode = TASK_MODE_TIMED;
task2.scheduled_time_us = 0;
```

**Verifica**: Entrambi devono partire a ~100 us (insieme!)

### Test 2: Task TIMED durante SEQUENTIAL

```cpp
task1.execution_mode = TASK_MODE_SEQUENTIAL;  // Dura 500ms

task2.execution_mode = TASK_MODE_TIMED;
task2.scheduled_time_us = 200000;  // 200ms
```

**Verifica**: Task 2 deve partire a 200ms, MENTRE task 1 è ancora in esecuzione

### Test 3: Verifica dipendenze

```cpp
task1.execution_mode = TASK_MODE_SEQUENTIAL;

task2.execution_mode = TASK_MODE_SEQUENTIAL;
task2.wait_for_task_id = "task_1";
```

**Verifica**: Task 2 deve partire SOLO dopo che task 1 finisce

## 🔧 File Modificato

- **`src/orchestrator.cpp`**: Funzione `scheduler_loop()` completamente riscritta

## 📝 Note Tecniche

- I task TIMED usano `std::this_thread::sleep_for()` per aspettare il tempo schedulato
- Ogni task TIMED ha il proprio thread che gestisce il timing
- I task SEQUENTIAL sono processati uno alla volta nel thread principale dello scheduler
- Il contatore `pending_tasks_` tiene traccia di TUTTI i task (TIMED + SEQUENTIAL)
- La condition variable `task_end_cv_` notifica sia il completamento che la registrazione dei task

## ✅ Conclusione

Ora il sistema supporta **veramente** l'esecuzione ibrida:
- ✅ Task TIMED partono al tempo schedulato (anche se 0)
- ✅ Task SEQUENTIAL rispettano le dipendenze
- ✅ Task TIMED e SEQUENTIAL possono eseguire in parallelo
- ✅ Timing preciso e prevedibile

Buon lavoro! 🚀
