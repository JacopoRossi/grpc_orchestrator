# Esecuzione Sequenziale vs Parallela

## Comportamento Attuale: PARALLELO

I task vengono avviati al loro tempo schedulato **indipendentemente** dallo stato degli altri task.

```
Timeline:
0s ────────────────────────────────────────────────>
   |         |         |
   1s        2s        3s
   ↓         ↓         ↓
   Task1     Task2     Task3
   [────]    [────]    [────]
   
   ← Tutti e 3 girano contemporaneamente! →
```

## Opzione 1: Esecuzione Sequenziale Stretta

Ogni task aspetta che il precedente finisca.

### Modifica a `orchestrator.cpp`:

```cpp
void Orchestrator::scheduler_loop() {
    std::cout << "[Orchestrator] Scheduler loop started (SEQUENTIAL MODE)" << std::endl;
    
    for (size_t i = 0; i < schedule_.tasks.size() && running_; i++) {
        const ScheduledTask& task = schedule_.tasks[i];
        
        // Aspetta il tempo schedulato
        int64_t current_time_us = get_current_time_us() - start_time_us_;
        if (task.scheduled_time_us > current_time_us) {
            int64_t wait_us = task.scheduled_time_us - current_time_us;
            std::this_thread::sleep_for(std::chrono::microseconds(wait_us));
        }
        
        std::cout << "[Orchestrator] Executing task: " << task.task_id << std::endl;
        
        // Esegui task SINCRONAMENTE (blocca fino al completamento)
        execute_task_sync(task);
        
        std::cout << "[Orchestrator] Task " << task.task_id << " completed" << std::endl;
    }
}

// Nuova funzione sincrona
void Orchestrator::execute_task_sync(const ScheduledTask& task) {
    // Registra task
    {
        std::lock_guard<std::mutex> lock(mutex_);
        TaskExecution exec;
        exec.task_id = task.task_id;
        exec.scheduled_time_us = task.scheduled_time_us;
        exec.actual_start_time_us = get_current_time_us();
        exec.state = TASK_STATE_STARTING;
        active_tasks_[task.task_id] = exec;
    }
    
    // Invia start
    auto channel = grpc::CreateChannel(task.task_address, grpc::InsecureChannelCredentials());
    auto stub = TaskService::NewStub(channel);
    
    StartTaskRequest request;
    request.set_task_id(task.task_id);
    // ... setup request ...
    
    StartTaskResponse response;
    grpc::ClientContext context;
    stub->StartTask(&context, request, &response);
    
    // ASPETTA la notifica di completamento
    std::unique_lock<std::mutex> lock(mutex_);
    completion_cv_.wait(lock, [this, &task]() {
        return active_tasks_.find(task.task_id) == active_tasks_.end();
    });
}
```

**Risultato:**
```
Timeline:
0s ────────────────────────────────────────────────>
   |         
   1s        
   ↓         
   Task1     
   [────]    
        ↓    
        Task2
        [────]
             ↓
             Task3
             [────]
```

## Opzione 2: Esecuzione con Dipendenze

Task possono avere prerequisiti.

### Aggiungi a `schedule.h`:

```cpp
struct ScheduledTask {
    // ... campi esistenti ...
    std::vector<std::string> depends_on;  // Task che devono completare prima
};
```

### Modifica scheduler:

```cpp
void Orchestrator::scheduler_loop() {
    while (next_task_index_ < schedule_.tasks.size() && running_) {
        const ScheduledTask& task = schedule_.tasks[next_task_index_];
        
        // Verifica tempo
        int64_t current_time_us = get_current_time_us() - start_time_us_;
        if (task.scheduled_time_us > current_time_us) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            continue;
        }
        
        // Verifica dipendenze
        bool dependencies_met = true;
        for (const auto& dep_id : task.depends_on) {
            if (!is_task_completed(dep_id)) {
                dependencies_met = false;
                break;
            }
        }
        
        if (!dependencies_met) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            continue;
        }
        
        // Esegui task
        execute_task(task);
        next_task_index_++;
    }
}
```

**Esempio schedule:**
```cpp
task1.depends_on = {};              // Nessuna dipendenza
task2.depends_on = {"task_1"};      // Aspetta task_1
task3.depends_on = {"task_1", "task_2"};  // Aspetta entrambi
```

## Opzione 3: Limita Concorrenza

Massimo N task contemporaneamente.

```cpp
class Orchestrator {
private:
    std::atomic<int> concurrent_tasks_;
    int max_concurrent_tasks_ = 2;  // Max 2 task alla volta
};

void Orchestrator::scheduler_loop() {
    while (next_task_index_ < schedule_.tasks.size() && running_) {
        // Aspetta se troppi task attivi
        if (concurrent_tasks_ >= max_concurrent_tasks_) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }
        
        const ScheduledTask& task = schedule_.tasks[next_task_index_];
        
        if (task.scheduled_time_us <= get_current_time_us() - start_time_us_) {
            concurrent_tasks_++;
            
            std::thread([this, task]() {
                execute_task(task);
                concurrent_tasks_--;
            }).detach();
            
            next_task_index_++;
        }
    }
}
```

## Raccomandazione

**Per sistemi real-time satellitari:**
- Usa **Opzione 2 (dipendenze)** per flessibilità
- Aggiungi **timeout** per ogni task
- Implementa **retry logic** per task critici
- Monitora **deadline miss**

**Per testing:**
- Mantieni parallelo ma aggiungi logging dettagliato
- Usa Docker per isolamento completo
