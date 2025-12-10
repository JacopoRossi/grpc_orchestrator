// VERSIONE SEQUENZIALE - I task NON girano in parallelo
// Sostituisci la funzione scheduler_loop() in orchestrator.cpp con questa

void Orchestrator::scheduler_loop() {
    std::cout << "[Orchestrator] Scheduler loop started (SEQUENTIAL MODE)" << std::endl;
    
    for (size_t i = 0; i < schedule_.tasks.size() && running_; i++) {
        const ScheduledTask& task = schedule_.tasks[i];
        
        // Aspetta il tempo schedulato
        int64_t current_time_us = get_current_time_us() - start_time_us_;
        if (task.scheduled_time_us > current_time_us) {
            int64_t wait_us = task.scheduled_time_us - current_time_us;
            std::cout << "[Orchestrator] Waiting " << wait_us << " us before task " 
                      << task.task_id << std::endl;
            std::this_thread::sleep_for(std::chrono::microseconds(wait_us));
        }
        
        std::cout << "[Orchestrator] Starting task: " << task.task_id 
                  << " (SEQUENTIAL - will wait for completion)" << std::endl;
        
        // Registra task
        {
            std::lock_guard<std::mutex> lock(mutex_);
            TaskExecution exec;
            exec.task_id = task.task_id;
            exec.scheduled_time_us = task.scheduled_time_us;
            exec.actual_start_time_us = get_current_time_us();
            exec.state = TASK_STATE_STARTING;
            active_tasks_[task.task_id] = exec;
            pending_tasks_++;
        }
        
        // Crea connessione gRPC
        auto channel = grpc::CreateChannel(task.task_address, 
                                          grpc::InsecureChannelCredentials());
        auto stub = TaskService::NewStub(channel);
        
        // Prepara richiesta
        StartTaskRequest request;
        request.set_task_id(task.task_id);
        request.set_scheduled_time_us(task.scheduled_time_us);
        request.set_deadline_us(task.deadline_us);
        request.set_priority(task.priority);
        for (const auto& param : task.parameters) {
            (*request.mutable_parameters())[param.first] = param.second;
        }
        
        // Invia comando start
        StartTaskResponse response;
        grpc::ClientContext context;
        context.set_deadline(std::chrono::system_clock::now() + std::chrono::seconds(5));
        
        grpc::Status status = stub->StartTask(&context, request, &response);
        
        if (!status.ok() || !response.success()) {
            std::cerr << "[Orchestrator] Failed to start task " << task.task_id << std::endl;
            continue;
        }
        
        std::cout << "[Orchestrator] Task " << task.task_id 
                  << " started, WAITING for completion..." << std::endl;
        
        // *** QUESTO È IL PUNTO CHIAVE ***
        // ASPETTA che il task completi prima di continuare
        {
            std::unique_lock<std::mutex> lock(mutex_);
            completion_cv_.wait(lock, [this, &task]() {
                // Aspetta finché il task non è più in active_tasks_
                // (significa che ha completato e on_task_end() è stato chiamato)
                return active_tasks_.find(task.task_id) == active_tasks_.end();
            });
        }
        
        std::cout << "[Orchestrator] Task " << task.task_id 
                  << " COMPLETED, proceeding to next task" << std::endl;
    }
    
    std::cout << "[Orchestrator] All tasks completed (SEQUENTIAL MODE)" << std::endl;
}
```

**Cosa cambia:**
1. ❌ **Rimosso** `std::thread(...).detach()`
2. ✅ **Aggiunto** `completion_cv_.wait()` che BLOCCA fino al completamento
3. ✅ I task vengono eseguiti **uno alla volta**

## 📊 Timeline con Esecuzione Sequenziale:

```
Tempo   Scheduler Thread          Thread Task1        Thread Task2        Thread Task3
─────────────────────────────────────────────────────────────────────────────────────────
0.0s    [Avvia]
        |
1.0s    Invia start a task1 ────> [AVVIA]
        ASPETTA...                 Esegue (500ms)
        |                          |
1.5s    ASPETTA...                 [COMPLETA]
        |                          Invia NotifyEnd()
        Riceve notifica            |
        SBLOCCA!                   |
        |                          |
2.0s    Invia start a task2 ───────────────────────> [AVVIA]
        ASPETTA...                                    Esegue (500ms)
        |                                             |
2.5s    ASPETTA...                                    [COMPLETA]
        |                                             Invia NotifyEnd()
        Riceve notifica                               |
        SBLOCCA!                                      |
        |                                             |
3.0s    Invia start a task3 ──────────────────────────────────────────> [AVVIA]
        ASPETTA...                                                       Esegue (500ms)
        |                                                                |
3.5s    ASPETTA...                                                       [COMPLETA]
        Riceve notifica                                                  |
        SBLOCCA!                                                         |
        Fine!
```

**Risultato:** Un solo task alla volta! ✅

## 🎯 Riepilogo

**Situazione Attuale:**
```cpp
std::thread([this, task]() {
    execute_task(task);
}).detach();  // ← I task girano in PARALLELO
```

**Se Vuoi Sequenziale:**
```cpp
execute_task(task);  // Niente thread
completion_cv_.wait(...);  // Aspetta completamento
// ← I task girano UNO ALLA VOLTA
```

**Quale scegliere?**
- **Parallelo**: Più veloce, ma task possono interferire
- **Sequenziale**: Più lento, ma nessuna interferenza

Per un sistema satellitare, probabilmente vuoi **sequenziale** per evitare conflitti di risorse! 🛰️

Vuoi che implementi la versione sequenziale? 🚀
