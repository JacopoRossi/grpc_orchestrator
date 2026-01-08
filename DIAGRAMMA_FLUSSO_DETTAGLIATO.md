# 🔄 Diagramma di Flusso Dettagliato - gRPC Orchestrator

Questo documento fornisce una visualizzazione dettagliata di come funziona il sistema passo dopo passo.

---

## 📊 Flusso Completo del Sistema

### Fase 1: Avvio Sistema (Docker Compose)

```
┌─────────────────────────────────────────────────────────────────┐
│ UTENTE: docker-compose up                                       │
└────────────────────┬────────────────────────────────────────────┘
                     │
                     ▼
┌─────────────────────────────────────────────────────────────────┐
│ Docker Compose legge docker-compose.yml                         │
│ • Trova 4 servizi: orchestrator, task1, task2, task3           │
│ • Crea network virtuale: grpc_network                           │
└────────────────────┬────────────────────────────────────────────┘
                     │
                     ▼
┌─────────────────────────────────────────────────────────────────┐
│ Docker costruisce immagini (se necessario)                      │
│ • Dockerfile.orchestrator → immagine orchestrator              │
│ • Dockerfile.task → immagine task                              │
└────────────────────┬────────────────────────────────────────────┘
                     │
                     ▼
┌─────────────────────────────────────────────────────────────────┐
│ Docker avvia container in parallelo                             │
│ ┌──────────────┐  ┌──────────────┐  ┌──────────────┐          │
│ │ orchestrator │  │    task1     │  │    task2     │          │
│ │   :50050     │  │   :50051     │  │   :50052     │          │
│ └──────────────┘  └──────────────┘  └──────────────┘          │
│                   ┌──────────────┐                              │
│                   │    task3     │                              │
│                   │   :50053     │                              │
│                   └──────────────┘                              │
└─────────────────────────────────────────────────────────────────┘
```

---

### Fase 2: Inizializzazione Orchestrator

```
CONTAINER ORCHESTRATOR
│
├─> Esegue: ./orchestrator_main --address 0.0.0.0:50050 \
│                                --schedule schedules/example.yaml \
│                                --policy fifo --priority 80
│
├─> main() in orchestrator_main.cpp
│   │
│   ├─> 1. Parse argomenti linea di comando
│   │   • listen_address = "0.0.0.0:50050"
│   │   • schedule_file = "schedules/example.yaml"
│   │   • rt_config.policy = SCHED_FIFO
│   │   • rt_config.priority = 80
│   │
│   ├─> 2. Crea oggetto Orchestrator
│   │   ```cpp
│   │   Orchestrator orchestrator(listen_address);
│   │   ```
│   │   │
│   │   └─> Costruttore Orchestrator::Orchestrator()
│   │       • Inizializza variabili membro
│   │       • Crea OrchestratorServiceImpl (servizio gRPC)
│   │       • running_ = false
│   │       • next_task_index_ = 0
│   │
│   ├─> 3. Configura Real-Time
│   │   ```cpp
│   │   orchestrator.set_rt_config(rt_config);
│   │   ```
│   │   • Salva configurazione RT
│   │   • Stampa: "Policy: FIFO, Priority: 80, CPU: 0"
│   │
│   ├─> 4. Carica Schedule YAML
│   │   ```cpp
│   │   TaskSchedule schedule = ScheduleParser::parse_yaml(schedule_file);
│   │   orchestrator.load_schedule(schedule);
│   │   ```
│   │   │
│   │   └─> ScheduleParser::parse_yaml()
│   │       │
│   │       ├─> Apre file YAML
│   │       │   ```yaml
│   │       │   schedule:
│   │       │     name: "Example"
│   │       │     tasks:
│   │       │       - id: task_1
│   │       │         address: "task1:50051"
│   │       │         mode: sequential
│   │       │   ```
│   │       │
│   │       ├─> Usa libreria yaml-cpp per parsing
│   │       │   ```cpp
│   │       │   YAML::Node config = YAML::LoadFile(file);
│   │       │   ```
│   │       │
│   │       ├─> Itera su tasks
│   │       │   ```cpp
│   │       │   for (const auto& task_node : config["tasks"]) {
│   │       │       TaskInfo task;
│   │       │       task.id = task_node["id"].as<std::string>();
│   │       │       task.address = task_node["address"].as<std::string>();
│   │       │       // ...
│   │       │       schedule.tasks.push_back(task);
│   │       │   }
│   │       │   ```
│   │       │
│   │       └─> Ritorna TaskSchedule con lista di task
│   │
│   ├─> 5. Avvia Orchestrator
│   │   ```cpp
│   │   orchestrator.start();
│   │   ```
│   │   │
│   │   └─> Orchestrator::start()
│   │       │
│   │       ├─> Controlla se già in esecuzione
│   │       │   ```cpp
│   │       │   if (running_.exchange(true)) return;
│   │       │   ```
│   │       │
│   │       ├─> Crea thread per server gRPC
│   │       │   ```cpp
│   │       │   server_thread_ = std::thread([this]() {
│   │       │       run_server();
│   │       │   });
│   │       │   ```
│   │       │
│   │       └─> run_server() (in thread separato)
│   │           │
│   │           ├─> Crea server gRPC
│   │           │   ```cpp
│   │           │   grpc::ServerBuilder builder;
│   │           │   builder.AddListeningPort(
│   │           │       listen_address_,
│   │           │       grpc::InsecureServerCredentials());
│   │           │   builder.RegisterService(service_.get());
│   │           │   ```
│   │           │
│   │           ├─> Avvia server
│   │           │   ```cpp
│   │           │   std::unique_ptr<grpc::Server> server = builder.BuildAndStart();
│   │           │   ```
│   │           │   • Server ora in ascolto su 0.0.0.0:50050
│   │           │   • Accetta chiamate RPC
│   │           │
│   │           ├─> Configura Real-Time per thread corrente
│   │           │   ```cpp
│   │           │   RTUtils::configure_realtime(rt_config_);
│   │           │   ```
│   │           │   │
│   │           │   └─> RTUtils::configure_realtime()
│   │           │       │
│   │           │       ├─> Imposta scheduling policy
│   │           │       │   ```cpp
│   │           │       │   struct sched_param param;
│   │           │       │   param.sched_priority = 80;
│   │           │       │   sched_setscheduler(0, SCHED_FIFO, &param);
│   │           │       │   ```
│   │           │       │
│   │           │       ├─> Imposta CPU affinity
│   │           │       │   ```cpp
│   │           │       │   cpu_set_t cpuset;
│   │           │       │   CPU_ZERO(&cpuset);
│   │           │       │   CPU_SET(0, &cpuset);  // Core 0
│   │           │       │   pthread_setaffinity_np(pthread_self(), 
│   │           │       │                          sizeof(cpuset), &cpuset);
│   │           │       │   ```
│   │           │       │
│   │           │       └─> Blocca memoria
│   │           │           ```cpp
│   │           │           mlockall(MCL_CURRENT | MCL_FUTURE);
│   │           │           ```
│   │           │
│   │           ├─> Avvia esecuzione schedule
│   │           │   ```cpp
│   │           │   execute_schedule();
│   │           │   ```
│   │           │   │
│   │           │   └─> (Vedi Fase 4)
│   │           │
│   │           └─> Attende shutdown
│   │               ```cpp
│   │               server->Wait();
│   │               ```
│   │
│   └─> 6. Attende completamento
│       ```cpp
│       orchestrator.wait_for_completion();
│       ```
│       • Blocca finché tutti i task non sono completati
```

---

### Fase 3: Inizializzazione Task Wrapper

```
CONTAINER TASK1
│
├─> Esegue: ./task_main --name task_1 \
│                        --address 0.0.0.0:50051 \
│                        --orchestrator orchestrator:50050
│
├─> main() in task_main.cpp
│   │
│   ├─> 1. Parse argomenti
│   │   • task_id = "task_1"
│   │   • listen_address = "0.0.0.0:50051"
│   │   • orchestrator_address = "orchestrator:50050"
│   │
│   ├─> 2. Crea TaskWrapper
│   │   ```cpp
│   │   TaskWrapper task_wrapper(
│   │       task_id,
│   │       listen_address,
│   │       orchestrator_address,
│   │       example_task_function  // Callback per esecuzione
│   │   );
│   │   ```
│   │   │
│   │   └─> Costruttore TaskWrapper::TaskWrapper()
│   │       │
│   │       ├─> Inizializza variabili
│   │       │   • task_id_ = "task_1"
│   │       │   • listen_address_ = "0.0.0.0:50051"
│   │       │   • state_ = TASK_STATE_IDLE
│   │       │   • running_ = false
│   │       │
│   │       ├─> Salva callback
│   │       │   • execution_callback_ = example_task_function
│   │       │
│   │       ├─> Crea servizio gRPC
│   │       │   ```cpp
│   │       │   service_ = std::make_unique<TaskServiceImpl>(this);
│   │       │   ```
│   │       │
│   │       └─> Crea stub per orchestrator
│   │           ```cpp
│   │           auto channel = grpc::CreateChannel(
│   │               orchestrator_address,
│   │               grpc::InsecureChannelCredentials());
│   │           orchestrator_stub_ = OrchestratorService::NewStub(channel);
│   │           ```
│   │
│   ├─> 3. Avvia TaskWrapper
│   │   ```cpp
│   │   task_wrapper.start();
│   │   ```
│   │   │
│   │   └─> TaskWrapper::start()
│   │       │
│   │       ├─> Crea server gRPC
│   │       │   ```cpp
│   │       │   grpc::ServerBuilder builder;
│   │       │   builder.AddListeningPort(listen_address_, ...);
│   │       │   builder.RegisterService(service_.get());
│   │       │   server_ = builder.BuildAndStart();
│   │       │   ```
│   │       │   • Server in ascolto su 0.0.0.0:50051
│   │       │
│   │       └─> Imposta running_ = true
│   │
│   └─> 4. Loop infinito
│       ```cpp
│       while (true) {
│           std::this_thread::sleep_for(std::chrono::seconds(1));
│           if (task_wrapper.get_state() == TASK_STATE_STOPPED) break;
│       }
│       ```
│       • Attende comandi dall'orchestrator
```

**NOTA:** task2 e task3 seguono lo stesso processo, ma con:
- task2: porta 50052
- task3: porta 50053

---

### Fase 4: Esecuzione Schedule

```
ORCHESTRATOR (thread execute_schedule)
│
├─> execute_schedule()
│   │
│   ├─> Ordina task per tempo di esecuzione
│   │   ```cpp
│   │   schedule_.sort_by_time();
│   │   ```
│   │   • Sequential tasks: scheduled_time = 0
│   │   • Timed tasks: scheduled_time = valore specifico
│   │
│   ├─> Salva tempo di inizio
│   │   ```cpp
│   │   start_time_us_ = get_current_time_us();
│   │   ```
│   │
│   └─> Loop sui task
│       ```cpp
│       for (size_t i = 0; i < schedule_.tasks.size(); i++) {
│           const TaskInfo& task = schedule_.tasks[i];
│           
│           // Controlla dipendenze
│           if (!task.depends_on.empty()) {
│               wait_for_task(task.depends_on);
│           }
│           
│           // Controlla timing
│           if (task.mode == TASK_MODE_TIMED) {
│               wait_until(task.scheduled_time_us);
│           }
│           
│           // Avvia task
│           start_task(task);
│       }
│       ```
│       │
│       └─> Per ogni task: start_task(task)
│           │
│           └─> (Vedi Fase 5)
```

---

### Fase 5: Avvio Task (Chiamata RPC StartTask)

```
ORCHESTRATOR                                    TASK WRAPPER (task_1)
│                                               │
├─> start_task(task_info)                       │
│   │                                           │
│   ├─> Prepara messaggio StartTaskRequest     │
│   │   ```cpp                                  │
│   │   StartTaskRequest request;               │
│   │   request.set_task_id("task_1");         │
│   │   request.set_scheduled_time_us(0);      │
│   │   request.set_deadline_us(3000000);      │
│   │   request.set_rt_policy("fifo");         │
│   │   request.set_rt_priority(30);           │
│   │   request.set_cpu_affinity(0);           │
│   │                                           │
│   │   // Copia parametri                      │
│   │   auto* params = request.mutable_parameters();
│   │   (*params)["input"] = "10";             │
│   │   ```                                     │
│   │                                           │
│   ├─> Crea stub per task                     │
│   │   ```cpp                                  │
│   │   auto channel = grpc::CreateChannel(    │
│   │       "task1:50051",                      │
│   │       grpc::InsecureChannelCredentials());
│   │   auto stub = TaskService::NewStub(channel);
│   │   ```                                     │
│   │                                           │
│   ├─> Chiama RPC StartTask                   │
│   │   ```cpp                                  │
│   │   grpc::ClientContext context;           │
│   │   StartTaskResponse response;            │
│   │   grpc::Status status = stub->StartTask( │
│   │       &context, request, &response);     │
│   │   ```                                     │
│   │                                           │
│   │   ════════════ gRPC ════════════>         │
│   │                                           │
│   │                                           ├─> TaskServiceImpl::StartTask()
│   │                                           │   │
│   │                                           │   ├─> Verifica stato
│   │                                           │   │   ```cpp
│   │                                           │   │   if (state_ != TASK_STATE_IDLE) {
│   │                                           │   │       response->set_success(false);
│   │                                           │   │       return grpc::Status::OK;
│   │                                           │   │   }
│   │                                           │   │   ```
│   │                                           │   │
│   │                                           │   ├─> Stampa log
│   │                                           │   │   ```cpp
│   │                                           │   │   std::cout << "[Task task_1] "
│   │                                           │   │             << "Received start command"
│   │                                           │   │             << std::endl;
│   │                                           │   │   ```
│   │                                           │   │
│   │                                           │   ├─> Chiama execute_task()
│   │                                           │   │   ```cpp
│   │                                           │   │   wrapper_->execute_task(*request);
│   │                                           │   │   ```
│   │                                           │   │   │
│   │                                           │   │   └─> (Vedi Fase 6)
│   │                                           │   │
│   │                                           │   └─> Prepara risposta
│   │                                           │       ```cpp
│   │                                           │       response->set_success(true);
│   │                                           │       response->set_task_id("task_1");
│   │                                           │       response->set_actual_start_time_us(
│   │                                           │           get_current_time_us());
│   │                                           │       return grpc::Status::OK;
│   │                                           │       ```
│   │                                           │
│   │   <════════════ gRPC ════════════         │
│   │                                           │
│   ├─> Riceve risposta                         │
│   │   ```cpp                                  │
│   │   if (status.ok() && response.success()) {
│   │       std::cout << "[Orchestrator] "     │
│   │                 << "Task task_1 started"  │
│   │                 << std::endl;             │
│   │   }                                       │
│   │   ```                                     │
│   │                                           │
│   └─> Salva in pending_tasks_                │
│       ```cpp                                  │
│       pending_tasks_++;                       │
│       ```                                     │
```

---

### Fase 6: Esecuzione Task

```
TASK WRAPPER (task_1)
│
├─> execute_task(request)
│   │
│   ├─> 1. Cambia stato
│   │   ```cpp
│   │   state_ = TASK_STATE_STARTING;
│   │   ```
│   │
│   ├─> 2. Configura Real-Time
│   │   ```cpp
│   │   if (rt_config_.policy != RT_POLICY_NONE) {
│   │       RTUtils::configure_realtime(rt_config_);
│   │   }
│   │   ```
│   │   │
│   │   └─> RTUtils::configure_realtime()
│   │       • sched_setscheduler(SCHED_FIFO, priority=30)
│   │       • pthread_setaffinity_np(cpu=0)
│   │       • mlockall(MCL_CURRENT | MCL_FUTURE)
│   │
│   ├─> 3. Prepara parametri
│   │   ```cpp
│   │   std::map<std::string, std::string> params;
│   │   for (const auto& pair : request.parameters()) {
│   │       params[pair.first] = pair.second;
│   │   }
│   │   params["task_id"] = task_id_;
│   │   ```
│   │   • params["input"] = "10"
│   │   • params["task_id"] = "task_1"
│   │
│   ├─> 4. Cambia stato e salva tempo
│   │   ```cpp
│   │   state_ = TASK_STATE_RUNNING;
│   │   start_time_us_ = get_current_time_us();
│   │   ```
│   │
│   ├─> 5. Esegue callback utente
│   │   ```cpp
│   │   std::map<std::string, std::string> output;
│   │   TaskResult result = execution_callback_(params, output);
│   │   ```
│   │   │
│   │   └─> example_task_function(params, output)
│   │       │
│   │       ├─> Legge parametri
│   │       │   ```cpp
│   │       │   auto input_it = params.find("input");
│   │       │   if (input_it == params.end()) {
│   │       │       return TASK_RESULT_FAILURE;
│   │       │   }
│   │       │   int input_value = std::stoi(input_it->second);
│   │       │   ```
│   │       │   • input_value = 10
│   │       │
│   │       ├─> Esegue logica task
│   │       │   ```cpp
│   │       │   // Task 1: moltiplica per 5
│   │       │   int output_value = input_value * 5;
│   │       │   ```
│   │       │   • output_value = 50
│   │       │
│   │       │   • Simula carico computazionale
│   │       │   ```cpp
│   │       │   for (int i = 0; i < 10000000; i++) {
│   │       │       // Calcoli...
│   │       │       if (i % 100000 == 0) {
│   │       │           std::this_thread::sleep_for(
│   │       │               std::chrono::seconds(1));
│   │       │       }
│   │       │   }
│   │       │   ```
│   │       │
│   │       ├─> Salva output
│   │       │   ```cpp
│   │       │   output["result"] = std::to_string(output_value);
│   │       │   ```
│   │       │   • output["result"] = "50"
│   │       │
│   │       ├─> Stampa log
│   │       │   ```cpp
│   │       │   std::cout << "[Task 1] Input: " << input_value
│   │       │             << std::endl;
│   │       │   std::cout << "[Task 1] Output: " << output_value
│   │       │             << std::endl;
│   │       │   ```
│   │       │
│   │       └─> Ritorna successo
│   │           ```cpp
│   │           return TASK_RESULT_SUCCESS;
│   │           ```
│   │
│   ├─> 6. Salva tempo di fine
│   │   ```cpp
│   │   end_time_us_ = get_current_time_us();
│   │   ```
│   │
│   ├─> 7. Cambia stato
│   │   ```cpp
│   │   if (result == TASK_RESULT_SUCCESS) {
│   │       state_ = TASK_STATE_COMPLETED;
│   │   } else {
│   │       state_ = TASK_STATE_FAILED;
│   │   }
│   │   ```
│   │
│   └─> 8. Notifica orchestrator
│       ```cpp
│       notify_orchestrator(result, output);
│       ```
│       │
│       └─> (Vedi Fase 7)
```

---

### Fase 7: Notifica Completamento (RPC NotifyTaskEnd)

```
TASK WRAPPER (task_1)                          ORCHESTRATOR
│                                               │
├─> notify_orchestrator(result, output)        │
│   │                                           │
│   ├─> Prepara messaggio TaskEndNotification  │
│   │   ```cpp                                  │
│   │   TaskEndNotification notification;      │
│   │   notification.set_task_id("task_1");    │
│   │   notification.set_result(TASK_RESULT_SUCCESS);
│   │   notification.set_start_time_us(start_time_us_);
│   │   notification.set_end_time_us(end_time_us_);
│   │   notification.set_execution_duration_us(
│   │       end_time_us_ - start_time_us_);    │
│   │                                           │
│   │   // Copia output                         │
│   │   auto* output_data = notification.mutable_output_data();
│   │   for (const auto& pair : output) {      │
│   │       (*output_data)[pair.first] = pair.second;
│   │   }                                       │
│   │   ```                                     │
│   │   • output_data["result"] = "50"         │
│   │                                           │
│   ├─> Chiama RPC NotifyTaskEnd               │
│   │   ```cpp                                  │
│   │   TaskEndResponse response;              │
│   │   grpc::ClientContext context;           │
│   │   grpc::Status status =                  │
│   │       orchestrator_stub_->NotifyTaskEnd( │
│   │           &context, notification, &response);
│   │   ```                                     │
│   │                                           │
│   │   ════════════ gRPC ════════════>         │
│   │                                           │
│   │                                           ├─> OrchestratorServiceImpl::NotifyTaskEnd()
│   │                                           │   │
│   │                                           │   ├─> Stampa log
│   │                                           │   │   ```cpp
│   │                                           │   │   std::cout << "[Orchestrator] "
│   │                                           │   │             << "Task task_1 completed"
│   │                                           │   │             << " (duration: "
│   │                                           │   │             << notification->execution_duration_us()
│   │                                           │   │             << " us)" << std::endl;
│   │                                           │   │   ```
│   │                                           │   │
│   │                                           │   ├─> Chiama on_task_end()
│   │                                           │   │   ```cpp
│   │                                           │   │   orchestrator_->on_task_end(*notification);
│   │                                           │   │   ```
│   │                                           │   │   │
│   │                                           │   │   └─> Orchestrator::on_task_end()
│   │                                           │   │       │
│   │                                           │   │       ├─> Lock mutex
│   │                                           │   │       │   ```cpp
│   │                                           │   │       │   std::lock_guard<std::mutex> lock(mutex_);
│   │                                           │   │       │   ```
│   │                                           │   │       │
│   │                                           │   │       ├─> Salva risultato
│   │                                           │   │       │   ```cpp
│   │                                           │   │       │   completed_tasks_[notification.task_id()] = notification;
│   │                                           │   │       │   ```
│   │                                           │   │       │
│   │                                           │   │       ├─> Decrementa pending
│   │                                           │   │       │   ```cpp
│   │                                           │   │       │   pending_tasks_--;
│   │                                           │   │       │   ```
│   │                                           │   │       │
│   │                                           │   │       ├─> Salva output per task dipendenti
│   │                                           │   │       │   ```cpp
│   │                                           │   │       │   task_outputs_["task_1"] = notification.output_data();
│   │                                           │   │       │   ```
│   │                                           │   │       │   • task_outputs_["task_1"]["result"] = "50"
│   │                                           │   │       │
│   │                                           │   │       ├─> Notifica condition variable
│   │                                           │   │       │   ```cpp
│   │                                           │   │       │   cv_.notify_all();
│   │                                           │   │       │   ```
│   │                                           │   │       │
│   │                                           │   │       └─> Avvia task dipendenti
│   │                                           │   │           ```cpp
│   │                                           │   │           schedule_dependent_tasks("task_1");
│   │                                           │   │           ```
│   │                                           │   │           │
│   │                                           │   │           └─> Cerca task con depends_on="task_1"
│   │                                           │   │               • Trova task_3
│   │                                           │   │               • Avvia task_3 (Fase 5)
│   │                                           │   │
│   │                                           │   └─> Prepara risposta
│   │                                           │       ```cpp
│   │                                           │       response->set_acknowledged(true);
│   │                                           │       response->set_message("Received");
│   │                                           │       return grpc::Status::OK;
│   │                                           │       ```
│   │                                           │
│   │   <════════════ gRPC ════════════         │
│   │                                           │
│   └─> Riceve risposta                         │
│       ```cpp                                  │
│       if (status.ok()) {                      │
│           std::cout << "[Task task_1] "       │
│                     << "Notified orchestrator"│
│                     << std::endl;             │
│       }                                       │
│       ```                                     │
```

---

### Fase 8: Task con Dipendenze

```
ORCHESTRATOR
│
├─> schedule_dependent_tasks("task_1")
│   │
│   ├─> Cerca task dipendenti
│   │   ```cpp
│   │   for (const auto& task : schedule_.tasks) {
│   │       if (task.depends_on == "task_1") {
│   │           // Trovato task_3
│   │       }
│   │   }
│   │   ```
│   │
│   └─> Avvia task_3
│       │
│       ├─> Prepara StartTaskRequest per task_3
│       │   ```cpp
│       │   StartTaskRequest request;
│       │   request.set_task_id("task_3");
│       │   
│       │   // Copia parametri da YAML
│       │   auto* params = request.mutable_parameters();
│       │   (*params)["input2"] = "22";  // Da YAML
│       │   
│       │   // Aggiungi output da task_1
│       │   const auto& task1_output = task_outputs_["task_1"];
│       │   for (const auto& pair : task1_output) {
│       │       (*params)["dep_" + pair.first] = pair.second;
│       │   }
│       │   ```
│       │   • params["input2"] = "22"
│       │   • params["dep_result"] = "50"  (da task_1)
│       │
│       └─> Chiama start_task(task_3)
│           │
│           └─> (Torna a Fase 5, ma per task_3)
│
│
TASK WRAPPER (task_3)
│
├─> execute_task(request)
│   │
│   └─> example_task_function(params, output)
│       │
│       ├─> Legge parametri
│       │   ```cpp
│       │   auto input1_it = params.find("dep_result");
│       │   auto input2_it = params.find("input2");
│       │   
│       │   int input1_value = std::stoi(input1_it->second);  // 50
│       │   int input2_value = std::stoi(input2_it->second);  // 22
│       │   ```
│       │
│       ├─> Esegue logica
│       │   ```cpp
│       │   // Task 3: moltiplica input1 * input2
│       │   int output_value = input1_value * input2_value;
│       │   ```
│       │   • output_value = 50 * 22 = 1100
│       │
│       ├─> Salva output
│       │   ```cpp
│       │   output["result"] = std::to_string(output_value);
│       │   ```
│       │   • output["result"] = "1100"
│       │
│       └─> Ritorna TASK_RESULT_SUCCESS
│
└─> Notifica orchestrator (Fase 7)
```

---

### Fase 9: Task Temporizzati (Timed)

```
ORCHESTRATOR
│
├─> execute_schedule()
│   │
│   └─> Loop sui task
│       │
│       ├─> Task 2 (mode: timed, scheduled_time: 2000000 us)
│       │   │
│       │   ├─> Controlla tempo corrente
│       │   │   ```cpp
│       │   │   int64_t current_time = get_current_time_us() - start_time_us_;
│       │   │   int64_t wait_time = task.scheduled_time_us - current_time;
│       │   │   ```
│       │   │   • current_time = 500000 us (500 ms)
│       │   │   • scheduled_time = 2000000 us (2000 ms)
│       │   │   • wait_time = 1500000 us (1500 ms)
│       │   │
│       │   ├─> Attende fino al tempo schedulato
│       │   │   ```cpp
│       │   │   if (wait_time > 0) {
│       │   │       std::this_thread::sleep_for(
│       │   │           std::chrono::microseconds(wait_time));
│       │   │   }
│       │   │   ```
│       │   │   • Dorme per 1500 ms
│       │   │
│       │   └─> Avvia task_2
│       │       ```cpp
│       │       start_task(task);
│       │       ```
│       │       │
│       │       └─> (Fase 5 per task_2)
```

---

### Fase 10: Completamento e Shutdown

```
ORCHESTRATOR
│
├─> wait_for_completion()
│   │
│   ├─> Attende tutti i task
│   │   ```cpp
│   │   std::unique_lock<std::mutex> lock(mutex_);
│   │   cv_.wait(lock, [this]() {
│   │       return pending_tasks_ == 0;
│   │   });
│   │   ```
│   │   • Blocca finché pending_tasks_ != 0
│   │   • Viene risvegliato da cv_.notify_all() in on_task_end()
│   │
│   └─> Tutti i task completati
│       • pending_tasks_ = 0
│       • Ritorna al main()
│
├─> main() - Stampa summary
│   ```cpp
│   auto history = orchestrator.get_execution_history();
│   
│   for (const auto& exec : history) {
│       std::cout << "Task: " << exec.task_id << std::endl;
│       std::cout << "  Duration: " << exec.execution_duration_us << " us" << std::endl;
│       std::cout << "  Result: " << exec.result << std::endl;
│   }
│   ```
│   
│   Output:
│   ```
│   === Execution Summary ===
│   Task: task_1
│     Duration: 100500000 us
│     Result: SUCCESS
│   
│   Task: task_2
│     Duration: 100500000 us
│     Result: SUCCESS
│   
│   Task: task_3
│     Duration: 10000000 us
│     Result: SUCCESS
│   
│   Total tasks: 3
│   Successful: 3
│   Failed: 0
│   ```
│
├─> orchestrator.stop()
│   │
│   ├─> Ferma server gRPC
│   │   ```cpp
│   │   server_->Shutdown();
│   │   ```
│   │
│   ├─> Attende thread
│   │   ```cpp
│   │   if (server_thread_.joinable()) {
│   │       server_thread_.join();
│   │   }
│   │   ```
│   │
│   └─> Imposta running_ = false
│
└─> return 0;
    • Programma termina con successo
```

---

## 🔍 Dettagli Tecnici Importanti

### 1. Thread Safety

Il sistema usa diversi meccanismi per la sicurezza dei thread:

```cpp
// Mutex per proteggere dati condivisi
std::mutex mutex_;
std::lock_guard<std::mutex> lock(mutex_);  // RAII lock

// Atomic per flag booleani
std::atomic<bool> running_;
if (running_.exchange(true)) { ... }  // Atomico

// Condition variable per sincronizzazione
std::condition_variable cv_;
cv_.wait(lock, [this]() { return condition; });
cv_.notify_all();
```

### 2. Gestione Memoria

```cpp
// Smart pointers (gestione automatica)
std::unique_ptr<OrchestratorServiceImpl> service_;
service_ = std::make_unique<OrchestratorServiceImpl>(this);
// Memoria liberata automaticamente quando esce dallo scope

// Shared pointers (riferimenti condivisi)
std::shared_ptr<grpc::Channel> channel;
channel = grpc::CreateChannel(...);
// Memoria liberata quando ultimo riferimento viene distrutto
```

### 3. Serializzazione gRPC

```cpp
// Messaggio protobuf
StartTaskRequest request;
request.set_task_id("task_1");

// Serializzazione (automatica in gRPC)
// request → bytes binari → rete → bytes binari → request

// gRPC gestisce:
// - Serializzazione/deserializzazione
// - Compressione
// - Gestione errori di rete
// - Timeout
// - Retry
```

### 4. Real-Time Guarantees

```cpp
// Policy SCHED_FIFO
// - Task con priorità più alta esegue sempre
// - Nessun time slice (esegue fino a completamento o blocco)
// - Priorità 1-99 (99 = massima)

// CPU Affinity
// - Lega processo a core specifico
// - Riduce context switch
// - Migliora cache locality

// Memory Locking
// - mlockall() blocca tutta la memoria in RAM
// - Previene page faults (swap)
// - Garantisce accesso deterministico
```

---

## 📊 Timeline Completa Esempio

```
T=0ms
│
├─ Docker Compose avvia container
│  ├─ orchestrator (0-100ms)
│  ├─ task1 (0-100ms)
│  ├─ task2 (0-100ms)
│  └─ task3 (0-100ms)
│
T=100ms
│
├─ Orchestrator legge schedule YAML
│  • 3 task da eseguire
│  • task_1: sequential (subito)
│  • task_2: timed (2000ms)
│  • task_3: sequential (dipende da task_1)
│
T=150ms
│
├─ Orchestrator → StartTask(task_1)
│  • gRPC call a task1:50051
│  • Parametri: input="11"
│
T=200ms
│
├─ Task 1 inizia esecuzione
│  • Configura RT: FIFO, priority 30
│  • Input: 11
│  • Calcola: 11 * 5 = 55
│  • Simula carico (100 secondi)
│
T=100200ms (100 secondi dopo)
│
├─ Task 1 completa
│  • Output: result="55"
│  • NotifyTaskEnd → Orchestrator
│
T=100250ms
│
├─ Orchestrator riceve notifica task_1
│  • Salva output: task_outputs_["task_1"]["result"] = "55"
│  • Trova task dipendente: task_3
│  • StartTask(task_3) con dep_result="55", input2="22"
│
T=100300ms
│
├─ Task 3 inizia esecuzione
│  • Input1: 55 (da task_1)
│  • Input2: 22
│  • Calcola: 55 * 22 = 1210
│  • Sleep 10 secondi
│
T=2000ms (in parallelo)
│
├─ Orchestrator attende fino a 2000ms
│  • scheduled_time per task_2
│
T=2000ms
│
├─ Orchestrator → StartTask(task_2)
│  • Parametri: input="9"
│
T=2050ms
│
├─ Task 2 inizia esecuzione
│  • Input: 9
│  • Calcola: 9 + 1 = 10
│  • Simula carico (100 secondi)
│
T=102050ms
│
├─ Task 2 completa
│  • Output: result="10"
│  • NotifyTaskEnd → Orchestrator
│
T=110300ms
│
├─ Task 3 completa
│  • Output: result="1210"
│  • NotifyTaskEnd → Orchestrator
│
T=110350ms
│
├─ Orchestrator: tutti i task completati
│  • pending_tasks_ = 0
│  • cv_.notify_all()
│  • wait_for_completion() ritorna
│
T=110400ms
│
├─ Main stampa summary
│  • Task 1: SUCCESS, 100s
│  • Task 2: SUCCESS, 100s
│  • Task 3: SUCCESS, 10s
│
T=110500ms
│
├─ orchestrator.stop()
│  • Shutdown server gRPC
│  • Join thread
│
T=110600ms
│
└─ Programma termina
   • return 0
```

---

**Fine del diagramma di flusso dettagliato** 🎉
