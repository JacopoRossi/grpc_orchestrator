# Guida per Sviluppatori - gRPC Orchestrator

Documentazione tecnica completa dell'architettura e implementazione del sistema.

## 📋 Indice

- [Architettura del Sistema](#architettura-del-sistema)
- [Componenti Principali](#componenti-principali)
- [Protocollo gRPC](#protocollo-grpc)
- [Strutture Dati](#strutture-dati)
- [Flusso di Esecuzione](#flusso-di-esecuzione)
- [Sistema Real-Time](#sistema-real-time)
- [Build System](#build-system)
- [Estensione del Sistema](#estensione-del-sistema)
- [Testing e Debugging](#testing-e-debugging)
- [Performance e Ottimizzazione](#performance-e-ottimizzazione)
- [Riferimenti API](#riferimenti-api)

## Architettura del Sistema

### Panoramica

Il sistema è basato su un'architettura distribuita con coordinazione centralizzata:

```
┌─────────────────────────────────────────────────────────────┐
│                         Orchestrator                        │
│  ┌────────────┐  ┌──────────────┐  ┌──────────────────┐   │
│  │ Scheduler  │  │ gRPC Server  │  │ Execution Tracker│   │
│  │  Thread    │  │   (recv)     │  │  & Dependencies  │   │
│  └────┬───────┘  └──────┬───────┘  └────────┬─────────┘   │
│       │                 │                    │              │
│       │ read schedule   │ NotifyTaskEnd      │ track state │
│       ▼                 ▼                    ▼              │
│  ┌─────────────────────────────────────────────────────┐   │
│  │            TaskSchedule + TaskExecution             │   │
│  └─────────────────────────────────────────────────────┘   │
└───────────────────┬─────────────────┬─────────────────────┘
                    │ StartTask (gRPC) │
           ┌────────┴────────┐        │
           │                 │        │
    ┌──────▼──────┐   ┌──────▼──────┐│
    │  Task 1     │   │  Task 2     ││
    │ ┌─────────┐ │   │ ┌─────────┐ ││
    │ │ Wrapper │ │   │ │ Wrapper │ ││
    │ │  gRPC   │ │   │ │  gRPC   │ ││
    │ │ Server  │ │   │ │ Server  │ ││
    │ └────┬────┘ │   │ └────┬────┘ ││
    │      │      │   │      │      ││
    │ ┌────▼────┐ │   │ ┌────▼────┐ ││
    │ │ Task    │ │   │ │ Task    │ ││
    │ │ Logic   │ │   │ │ Logic   │ ││
    │ └─────────┘ │   │ └─────────┘ ││
    └─────────────┘   └─────────────┘│
           │                          │
           └──────────┬───────────────┘
                      │ NotifyTaskEnd (gRPC)
                      ▼
              [Back to Orchestrator]
```

### Principi Architetturali

1. **Separazione delle Responsabilità**
   - Orchestrator: coordinamento e scheduling
   - TaskWrapper: gestione ciclo vita task e comunicazione
   - TaskLogic: logica applicativa (user-defined)

2. **Comunicazione Asincrona**
   - StartTask: orchestrator → task (comando di avvio)
   - NotifyTaskEnd: task → orchestrator (notifica completamento)

3. **Stato Distribuito**
   - Ogni task mantiene il proprio stato
   - Orchestrator traccia esecuzione e dipendenze

4. **Real-Time Determinism**
   - Scheduling basato su tempi assoluti
   - Configurazione RT per prevedibilità

## Componenti Principali

### 1. Orchestrator

**File**: `src/orchestrator.cpp`, `include/orchestrator.h`

#### Classi

**`Orchestrator`**: Classe principale che gestisce il ciclo vita dell'orchestrazione.

```cpp
class Orchestrator {
public:
    // Constructor: inizializza gRPC server
    Orchestrator(const std::string& listen_address);
    
    // Load schedule da file YAML
    void load_schedule(const TaskSchedule& schedule);
    
    // Configura parametri real-time
    void set_rt_config(const RTConfig& config);
    
    // Avvia orchestrazione
    void start();
    
    // Stop orchestrazione
    void stop();
    
    // Attendi completamento tutti i task
    void wait_for_completion();
    
    // Callback da TaskService
    void on_task_end(const TaskEndNotification& notification);
    
private:
    // Loop principale dello scheduler
    void scheduler_loop();
    
    // Esegue un task (invia StartTask via gRPC)
    void execute_task(const ScheduledTask& task);
};
```

#### Thread Model

L'orchestrator usa 2 thread principali:

1. **Scheduler Thread** (`scheduler_thread_`)
   - Cicla attraverso lo schedule
   - Invia comandi StartTask ai task
   - Gestisce dipendenze sequenziali
   - Monitora timing

2. **gRPC Server Thread** (`server_thread_`)
   - Riceve NotifyTaskEnd dai task completati
   - Aggiorna stato esecuzione
   - Notifica scheduler (via `task_end_cv_`)

#### Gestione Dipendenze

```cpp
// Task sequenziali: attende completamento task precedente
if (task.execution_mode == TASK_MODE_SEQUENTIAL && 
    !task.wait_for_task_id.empty()) 
{
    std::unique_lock<std::mutex> lock(mutex_);
    task_end_cv_.wait(lock, [&] {
        return task_completed_[task.wait_for_task_id];
    });
}
```

#### Tracking Esecuzione

```cpp
struct TaskExecution {
    std::string task_id;
    int64_t scheduled_time_us;
    int64_t actual_start_time_us;
    int64_t end_time_us;
    int64_t context_switch_time_us;
    TaskState state;
    TaskResult result;
    std::string output_data_json;
};

// Memorizzati in:
std::unordered_map<std::string, TaskExecution> active_tasks_;
std::vector<TaskExecution> completed_tasks_;
```

### 2. Task Wrapper

**File**: `src/task_wrapper.cpp`, `include/task_wrapper.h`

#### Responsabilità

- Riceve comandi dall'orchestrator (StartTask, StopTask)
- Esegue la logica del task in thread separato
- Applica configurazione real-time al thread di esecuzione
- Notifica orchestrator al completamento

#### Architettura

```cpp
class TaskWrapper {
public:
    TaskWrapper(
        const std::string& task_id,
        const std::string& listen_address,
        const std::string& orchestrator_address,
        TaskExecutionCallback execution_callback
    );
    
    void start();  // Avvia gRPC server
    void execute_task(const StartTaskRequest& request);
    
private:
    // Esecuzione in thread separato
    void task_execution_thread(StartTaskRequest request);
    
    // Notifica orchestrator
    void notify_orchestrator_end(TaskResult result, ...);
    
    TaskExecutionCallback execution_callback_;  // User logic
    std::thread execution_thread_;
    RTConfig rt_config_;
};
```

#### Callback Utente

```cpp
using TaskExecutionCallback = std::function<
    TaskResult(const std::string& params_json, std::string& output_json)
>;

// Esempio:
TaskResult my_task(const std::string& params_json, std::string& output_json) {
    // Parse input
    json params = json::parse(params_json);
    
    // Execute logic
    // ...
    
    // Prepare output
    json output;
    output["result"] = value;
    output_json = output.dump();
    
    return TASK_RESULT_SUCCESS;
}
```

### 3. Schedule Parser

**File**: `src/schedule.cpp`, `include/schedule.h`

#### Parsing YAML

```cpp
class ScheduleParser {
public:
    static TaskSchedule parse_yaml(const std::string& yaml_path);
};

// Internamente usa yaml-cpp
YAML::Node config = YAML::LoadFile(yaml_path);
YAML::Node tasks_node = config["schedule"]["tasks"];

for (const auto& task_node : tasks_node) {
    ScheduledTask task;
    task.task_id = task_node["id"].as<std::string>();
    task.task_address = task_node["address"].as<std::string>();
    task.scheduled_time_us = task_node["scheduled_time_us"].as<int64_t>();
    // ...
}
```

#### Struttura Schedule

```cpp
struct TaskSchedule {
    int64_t time_horizon_start_us;
    int64_t time_horizon_end_us;
    int64_t tick_duration_us;
    std::vector<ScheduledTask> tasks;
    
    void sort_by_time();  // Ordina task per tempo schedulato
};

struct ScheduledTask {
    std::string task_id;
    std::string task_address;
    int64_t scheduled_time_us;
    int64_t deadline_us;
    std::string parameters_json;
    TaskExecutionMode execution_mode;
    std::string wait_for_task_id;
    
    // RT config
    std::string rt_policy;
    int32_t rt_priority;
    int32_t cpu_affinity;
};
```

### 4. Real-Time Utilities

**File**: `src/rt_utils.cpp`, `include/rt_utils.h`

#### Configurazione RT

```cpp
struct RTConfig {
    RTSchedulingPolicy policy;  // FIFO, RR, DEADLINE
    int priority;               // 1-99
    int cpu_affinity;           // CPU core ID
    bool lock_memory;           // mlockall()
    bool prefault_stack;        // Touch stack pages
    size_t stack_size;
};

class RTUtils {
public:
    static bool lock_memory();
    static bool set_thread_realtime(RTSchedulingPolicy policy, int priority);
    static bool set_cpu_affinity(int cpu_id);
    static bool apply_rt_config(const RTConfig& config);
};
```

#### Implementazione Policy

```cpp
bool RTUtils::set_thread_realtime(RTSchedulingPolicy policy, int priority) {
    struct sched_param param;
    param.sched_priority = priority;
    
    int sched_policy = policy_to_sched_policy(policy);
    
    if (pthread_setschedparam(pthread_self(), sched_policy, &param) != 0) {
        return false;
    }
    return true;
}

int RTUtils::policy_to_sched_policy(RTSchedulingPolicy policy) {
    switch (policy) {
        case RT_POLICY_FIFO: return SCHED_FIFO;
        case RT_POLICY_RR: return SCHED_RR;
        case RT_POLICY_DEADLINE: return SCHED_DEADLINE;
        default: return SCHED_OTHER;
    }
}
```

#### Memory Locking

```cpp
bool RTUtils::lock_memory() {
    // Lock all current and future memory
    if (mlockall(MCL_CURRENT | MCL_FUTURE) != 0) {
        return false;
    }
    return true;
}

void RTUtils::prefault_stack(size_t size) {
    // Touch memory to force allocation
    unsigned char dummy[size];
    memset(dummy, 0, size);
}
```

### 5. Deploy Manager

**File**: `src/deploy_manager.cpp`, `include/deploy_manager.h`

#### Funzionalità

- Parse configurazione deployment da YAML
- Login su GitLab Container Registry
- Pull immagini Docker
- Avvio container con configurazione RT
- Gestione network
- Health check

#### Esempio Deployment

```cpp
DeployManager deploy_manager;
deploy_manager.load_config("deploy/deployment_config.yaml");

// Login su GitLab
deploy_manager.login_to_gitlab();

// Pull immagini
deploy_manager.pull_all_images();

// Deploy
deploy_manager.deploy_all();

// Attendi healthy
deploy_manager.wait_for_healthy("grpc_orchestrator", 30);
```

### 6. Builder Manager

**File**: `src/builder_manager.cpp`, `include/builder_manager.h`

#### Funzionalità

- Build immagini Docker da Dockerfile
- Tag per GitLab Registry
- Push su registry
- Cleanup immagini locali

#### Esempio Build & Push

```cpp
BuilderManager builder;
builder.load_config("deploy/builder_config.yaml");

// Build tutte le immagini
builder.build_all_images();

// Tag per GitLab
builder.tag_for_gitlab("grpc_orchestrator", "latest");

// Push
builder.push_all_to_gitlab();
```

## Protocollo gRPC

### Definizione Protobuf

**File**: `proto/orchestrator.proto`

#### Service Definitions

```protobuf
// Service esposto da orchestrator
service OrchestratorService {
  rpc NotifyTaskEnd(TaskEndNotification) returns (TaskEndResponse);
  rpc HealthCheck(HealthCheckRequest) returns (HealthCheckResponse);
}

// Service esposto da ogni task
service TaskService {
  rpc StartTask(StartTaskRequest) returns (StartTaskResponse);
  rpc StopTask(StopTaskRequest) returns (StopTaskResponse);
  rpc GetTaskStatus(TaskStatusRequest) returns (TaskStatusResponse);
}
```

#### Messages

**StartTaskRequest**:
```protobuf
message StartTaskRequest {
  string task_id = 1;
  int64 scheduled_time_us = 2;
  int64 deadline_us = 3;
  string parameters_json = 4;
  string rt_policy = 6;
  int32 rt_priority = 7;
  int32 cpu_affinity = 8;
}
```

**TaskEndNotification**:
```protobuf
message TaskEndNotification {
  string task_id = 1;
  TaskResult result = 2;
  int64 start_time_us = 3;
  int64 end_time_us = 4;
  int64 execution_duration_us = 5;
  string error_message = 6;
  string metrics_json = 7;
  string output_data_json = 8;  // Per task dipendenti
}
```

#### Enumerazioni

```protobuf
enum TaskState {
  TASK_STATE_UNKNOWN = 0;
  TASK_STATE_IDLE = 1;
  TASK_STATE_STARTING = 2;
  TASK_STATE_RUNNING = 3;
  TASK_STATE_COMPLETED = 4;
  TASK_STATE_FAILED = 5;
  TASK_STATE_STOPPED = 6;
}

enum TaskResult {
  TASK_RESULT_UNKNOWN = 0;
  TASK_RESULT_SUCCESS = 1;
  TASK_RESULT_FAILURE = 2;
  TASK_RESULT_TIMEOUT = 3;
  TASK_RESULT_CANCELLED = 4;
}
```

### Flusso Messaggi

```
Orchestrator                           Task
     |                                  |
     | StartTaskRequest                |
     |--------------------------------->|
     |                                  | [Esegue task]
     |                                  |
     | StartTaskResponse                |
     |<---------------------------------|
     |                                  |
     |                                  | [Task completa]
     |                                  |
     | TaskEndNotification              |
     |<---------------------------------|
     |                                  |
     | TaskEndResponse                  |
     |--------------------------------->|
     |                                  |
```

### Implementazione Client/Server

**Server (Task)**:
```cpp
class TaskServiceImpl : public TaskService::Service {
    grpc::Status StartTask(
        grpc::ServerContext* context,
        const StartTaskRequest* request,
        StartTaskResponse* response) override 
    {
        wrapper_->execute_task(*request);
        response->set_success(true);
        response->set_actual_start_time_us(get_current_time_us());
        return grpc::Status::OK;
    }
};

// Avvio server
grpc::ServerBuilder builder;
builder.AddListeningPort(listen_address_, grpc::InsecureServerCredentials());
builder.RegisterService(&service_);
server_ = builder.BuildAndStart();
```

**Client (Orchestrator)**:
```cpp
// Crea stub per task
auto channel = grpc::CreateChannel(
    task.task_address, 
    grpc::InsecureChannelCredentials()
);
auto stub = TaskService::NewStub(channel);

// Invia StartTask
StartTaskRequest request;
request.set_task_id(task.task_id);
request.set_scheduled_time_us(task.scheduled_time_us);
// ...

StartTaskResponse response;
grpc::ClientContext context;
grpc::Status status = stub->StartTask(&context, request, &response);

if (!status.ok()) {
    std::cerr << "StartTask failed: " << status.error_message() << std::endl;
}
```

## Strutture Dati

### Thread Safety

Il sistema usa pattern thread-safe per gestire concorrenza:

```cpp
class Orchestrator {
private:
    mutable std::mutex mutex_;
    std::condition_variable task_end_cv_;
    std::condition_variable completion_cv_;
    
    std::unordered_map<std::string, TaskExecution> active_tasks_;
    std::vector<TaskExecution> completed_tasks_;
    std::unordered_map<std::string, bool> task_completed_;
    std::unordered_map<std::string, std::string> task_outputs_;
};
```

### Sincronizzazione

**Attesa Completamento Task**:
```cpp
void Orchestrator::wait_for_completion() {
    std::unique_lock<std::mutex> lock(mutex_);
    completion_cv_.wait(lock, [this] {
        return pending_tasks_.load() == 0;
    });
}
```

**Notifica Completamento**:
```cpp
void Orchestrator::on_task_end(const TaskEndNotification& notification) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Update state
    task_completed_[notification.task_id()] = true;
    task_outputs_[notification.task_id()] = notification.output_data_json();
    
    // Notify waiting threads
    task_end_cv_.notify_all();
    
    if (--pending_tasks_ == 0) {
        completion_cv_.notify_all();
    }
}
```

## Flusso di Esecuzione

### 1. Inizializzazione

```
main()
  ├─> Parse args
  ├─> Create Orchestrator
  ├─> Load Schedule (YAML → TaskSchedule)
  ├─> Set RT Config
  └─> Start Orchestrator
       ├─> Start gRPC Server Thread
       └─> Start Scheduler Thread
```

### 2. Scheduler Loop

```cpp
void Orchestrator::scheduler_loop() {
    start_time_us_ = get_current_time_us();
    
    while (running_ && next_task_index_ < schedule_.tasks.size()) {
        const ScheduledTask& task = schedule_.tasks[next_task_index_];
        
        // Check dependencies
        if (task.execution_mode == TASK_MODE_SEQUENTIAL) {
            std::unique_lock<std::mutex> lock(mutex_);
            task_end_cv_.wait(lock, [&] {
                return task_completed_[task.wait_for_task_id];
            });
        }
        
        // Wait for scheduled time
        int64_t now = get_current_time_us() - start_time_us_;
        if (now < task.scheduled_time_us) {
            std::this_thread::sleep_for(
                std::chrono::microseconds(task.scheduled_time_us - now)
            );
        }
        
        // Execute task
        execute_task(task);
        next_task_index_++;
    }
}
```

### 3. Task Execution

```
execute_task()
  ├─> Create gRPC Channel to task
  ├─> Prepare StartTaskRequest
  │    ├─> task_id
  │    ├─> scheduled_time_us
  │    ├─> deadline_us
  │    ├─> parameters_json
  │    └─> RT config
  ├─> Send StartTask RPC
  └─> Track in active_tasks_

Task receives StartTask
  ├─> Spawn execution thread
  ├─> Apply RT config to thread
  ├─> Execute user callback
  ├─> Send NotifyTaskEnd
  └─> Return StartTaskResponse

Orchestrator receives NotifyTaskEnd
  ├─> Update task_completed_
  ├─> Store output in task_outputs_
  ├─> Move to completed_tasks_
  ├─> Notify task_end_cv_
  └─> Check completion
```

### 4. Passaggio Dati tra Task

```yaml
# Schedule
tasks:
  - id: producer
    # ... produce output ...
    
  - id: consumer
    depends_on: producer  # Riceve output di producer
```

```cpp
// Producer task
TaskResult producer(const std::string& params_json, std::string& output_json) {
    json output;
    output["data"] = "important_value";
    output_json = output.dump();
    return TASK_RESULT_SUCCESS;
}

// Consumer task
TaskResult consumer(const std::string& params_json, std::string& output_json) {
    json params = json::parse(params_json);
    
    // L'orchestrator inserisce output di producer qui
    if (params.contains("dep_output")) {
        json dep_output = params["dep_output"];
        std::string data = dep_output["data"];
        // Use data...
    }
    
    return TASK_RESULT_SUCCESS;
}
```

**Implementazione in Orchestrator**:
```cpp
void Orchestrator::execute_task(const ScheduledTask& task) {
    StartTaskRequest request;
    
    // Add task parameters
    json params = json::parse(task.parameters_json);
    
    // If sequential task, add dependency output
    if (!task.wait_for_task_id.empty()) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (task_outputs_.count(task.wait_for_task_id) > 0) {
            params["dep_output"] = json::parse(
                task_outputs_[task.wait_for_task_id]
            );
        }
    }
    
    request.set_parameters_json(params.dump());
    
    // Send RPC...
}
```

## Sistema Real-Time

### Configurazione RT

Il sistema applica configurazione real-time a più livelli:

1. **Orchestrator Thread**: Scheduler ha priorità massima
2. **Task Execution Thread**: Ogni task può avere configurazione diversa
3. **Memory Locking**: Previene page fault

### Applicazione RT al Thread

```cpp
void TaskWrapper::task_execution_thread(StartTaskRequest request) {
    // Apply RT config from request
    RTConfig task_rt_config;
    task_rt_config.policy = RTUtils::string_to_policy(request.rt_policy());
    task_rt_config.priority = request.rt_priority();
    task_rt_config.cpu_affinity = request.cpu_affinity();
    
    if (!RTUtils::apply_rt_config(task_rt_config)) {
        std::cerr << "Failed to apply RT config" << std::endl;
    }
    
    // Execute user callback
    std::string output_json;
    TaskResult result = execution_callback_(
        request.parameters_json(), 
        output_json
    );
    
    // Notify orchestrator
    notify_orchestrator_end(result, "", output_json);
}
```

### Misurazione Context Switch

```cpp
struct TaskExecution {
    int64_t context_switch_time_us;  // Time between tasks
};

// Nel on_task_end:
void Orchestrator::on_task_end(const TaskEndNotification& notification) {
    TaskExecution exec;
    exec.end_time_us = notification.end_time_us();
    
    // Calculate context switch time
    if (last_task_end_time_us_ > 0) {
        exec.context_switch_time_us = 
            notification.start_time_us() - last_task_end_time_us_;
    }
    
    last_task_end_time_us_ = notification.end_time_us();
    completed_tasks_.push_back(exec);
}
```

### Best Practices RT

1. **Priorità**:
   - Orchestrator: 90-99
   - Task critici: 70-80
   - Task normali: 30-50

2. **CPU Affinity**:
   - Orchestrator: core 0
   - Task critici: core isolati (1, 2, 3)
   - Task normali: core shared

3. **Memory**:
   - Sempre lock memory per task RT
   - Prefault stack

4. **Timing**:
   - Usa `clock_gettime(CLOCK_MONOTONIC)` per timestamp
   - Evita system call bloccanti

## Build System

### CMakeLists.txt

Il sistema usa CMake con compilazione modulare:

```cmake
# Opzioni build
option(BUILD_ORCHESTRATOR "Build orchestrator" ON)
option(BUILD_TASKS "Build task runner" ON)
option(BUILD_DEPLOY_MANAGER "Build deploy manager" ON)
option(BUILD_BUILDER_MANAGER "Build builder manager" ON)
option(BUILD_ALL "Build all components" ON)
```

### Librerie

**core_lib**: Componenti comuni
```cmake
add_library(core_lib
    src/rt_utils.cpp
    src/schedule.cpp
    ${PROTO_SRCS}
    ${GRPC_SRCS}
)
target_link_libraries(core_lib
    gRPC::grpc++
    protobuf::libprotobuf
    yaml-cpp
    nlohmann_json::nlohmann_json
)
```

**orchestrator_lib**: Logica orchestrazione
```cmake
add_library(orchestrator_lib
    src/orchestrator.cpp
    src/task_wrapper.cpp
)
target_link_libraries(orchestrator_lib core_lib)
```

### Generazione Protobuf

```cmake
foreach(PROTO_FILE ${PROTO_FILES})
    add_custom_command(
        OUTPUT ${PROTO_SRC} ${PROTO_HDR} ${GRPC_SRC} ${GRPC_HDR}
        COMMAND ${PROTOBUF_PROTOC_EXECUTABLE}
        ARGS --grpc_out=${CMAKE_CURRENT_BINARY_DIR}
             --cpp_out=${CMAKE_CURRENT_BINARY_DIR}
             -I${PROTO_PATH}
             --plugin=protoc-gen-grpc=${grpc_cpp_plugin_location}
             ${PROTO_FILE}
    )
endforeach()
```

### Build Targets

```bash
# Build tutto
cmake .. && make

# Build selettivo
cmake -DBUILD_ALL=OFF -DBUILD_ORCHESTRATOR=ON ..
make orchestrator_main

# Build specifico
make task_runner
make deploy_manager_main
```

## Estensione del Sistema

### Aggiungere un Nuovo Task Type

**1. Definisci funzione in `tasks/my_tasks.h`:**
```cpp
inline TaskResult my_new_feature(
    const std::string& params_json,
    std::string& output_json)
{
    // Implementation
    return TASK_RESULT_SUCCESS;
}
```

**2. Registra in `tasks/task_runner.cpp`:**
```cpp
task_registry["my_new_feature"] = my_new_feature;
```

**3. Usa nello schedule:**
```yaml
- id: new_task
  address: "task1:50051"
  # ...
```

**4. Avvia task runner:**
```bash
./task_runner --name my_new_feature ...
```

### Aggiungere Nuovo Parametro Schedule

**1. Estendi `ScheduledTask` in `include/schedule.h`:**
```cpp
struct ScheduledTask {
    // Existing fields...
    int64_t my_new_param;  // Add new field
};
```

**2. Parse in `src/schedule.cpp`:**
```cpp
TaskSchedule ScheduleParser::parse_yaml(const std::string& yaml_path) {
    // ...
    if (task_node["my_new_param"]) {
        task.my_new_param = task_node["my_new_param"].as<int64_t>();
    }
}
```

**3. Usa in YAML:**
```yaml
tasks:
  - id: task1
    my_new_param: 12345
```

### Aggiungere Nuovo Servizio gRPC

**1. Definisci in `proto/orchestrator.proto`:**
```protobuf
service MyNewService {
  rpc MyMethod(MyRequest) returns (MyResponse);
}

message MyRequest {
  string data = 1;
}

message MyResponse {
  bool success = 1;
}
```

**2. Implementa service:**
```cpp
class MyNewServiceImpl : public MyNewService::Service {
    grpc::Status MyMethod(
        grpc::ServerContext* context,
        const MyRequest* request,
        MyResponse* response) override
    {
        // Implementation
        response->set_success(true);
        return grpc::Status::OK;
    }
};
```

**3. Registra nel server:**
```cpp
grpc::ServerBuilder builder;
MyNewServiceImpl my_service;
builder.RegisterService(&my_service);
server_ = builder.BuildAndStart();
```

## Testing e Debugging

### Unit Testing (Esempio)

```cpp
// test/test_schedule_parser.cpp
#include <gtest/gtest.h>
#include "schedule.h"

TEST(ScheduleParser, ParseYAML) {
    TaskSchedule schedule = ScheduleParser::parse_yaml(
        "test_data/simple_schedule.yaml"
    );
    
    ASSERT_EQ(schedule.tasks.size(), 3);
    EXPECT_EQ(schedule.tasks[0].task_id, "task_1");
    EXPECT_EQ(schedule.tasks[0].scheduled_time_us, 1000000);
}
```

### Debug Logging

Aggiungi macro di debug:
```cpp
#ifdef DEBUG_ORCHESTRATOR
#define DEBUG_LOG(msg) std::cout << "[DEBUG] " << msg << std::endl
#else
#define DEBUG_LOG(msg)
#endif

// Use
DEBUG_LOG("Executing task: " << task.task_id);
```

Compila con:
```bash
cmake -DCMAKE_BUILD_TYPE=Debug -DDEBUG_ORCHESTRATOR=ON ..
```

### Profiling con gprof

```bash
# Compile with profiling
cmake -DCMAKE_CXX_FLAGS="-pg" ..
make

# Run
./orchestrator_main ...

# Analyze
gprof orchestrator_main gmon.out > analysis.txt
```

### Debugging gRPC

```cpp
// Abilita verbose logging gRPC
setenv("GRPC_VERBOSITY", "DEBUG", 1);
setenv("GRPC_TRACE", "all", 1);
```

## Performance e Ottimizzazione

### Latenza

**Misurare Latenza Task**:
```cpp
auto start = std::chrono::high_resolution_clock::now();

// Execute task
execution_callback_(params, output);

auto end = std::chrono::high_resolution_clock::now();
auto latency_us = std::chrono::duration_cast<std::chrono::microseconds>(
    end - start
).count();

std::cout << "Task latency: " << latency_us << " us" << std::endl;
```

**Misurare Overhead gRPC**:
```cpp
int64_t grpc_start = get_current_time_us();
stub->StartTask(&context, request, &response);
int64_t grpc_end = get_current_time_us();

int64_t grpc_overhead = grpc_end - grpc_start;
```

### Ottimizzazioni

**1. Reduce Context Switches**:
- Usa CPU affinity per evitare migrazione
- Priorità RT per preemption controllata

**2. Minimize gRPC Overhead**:
- Usa async API per throughput
- Reuse channels e stubs

**3. Memory Pool**:
```cpp
// Pre-allocate per evitare malloc in RT path
std::vector<char> buffer;
buffer.reserve(1024 * 1024);  // 1MB
```

**4. Lock-Free Data Structures**:
```cpp
std::atomic<TaskState> state_;
std::atomic<int> pending_tasks_;
```

### Benchmarking

Script per benchmark:
```bash
#!/bin/bash
# benchmark.sh

for i in {1..10}; do
    echo "Run $i"
    ./orchestrator_main --schedule schedules/benchmark.yaml \
        --policy fifo --priority 90
    sleep 2
done | tee benchmark_results.txt
```

Analizza risultati con script Python:
```python
import re

with open('benchmark_results.txt') as f:
    data = f.read()
    
latencies = re.findall(r'Task latency: (\d+) us', data)
latencies = [int(x) for x in latencies]

print(f"Mean: {np.mean(latencies)} us")
print(f"Std: {np.std(latencies)} us")
print(f"Max: {np.max(latencies)} us")
```

## Riferimenti API

### Orchestrator API

```cpp
// Creazione
Orchestrator orch("0.0.0.0:50050");

// Configurazione
RTConfig config;
config.policy = RT_POLICY_FIFO;
config.priority = 80;
orch.set_rt_config(config);

// Load schedule
TaskSchedule schedule = ScheduleParser::parse_yaml("schedule.yaml");
orch.load_schedule(schedule);

// Esecuzione
orch.start();
orch.wait_for_completion();

// Risultati
auto history = orch.get_execution_history();
for (const auto& exec : history) {
    std::cout << exec.task_id << ": " 
              << exec.execution_duration_us << " us" << std::endl;
}

// Cleanup
orch.stop();
```

### TaskWrapper API

```cpp
// Definisci callback
TaskResult my_task(const std::string& params, std::string& output) {
    // ...
    return TASK_RESULT_SUCCESS;
}

// Crea wrapper
TaskWrapper wrapper(
    "task_1",
    "0.0.0.0:50051",
    "orchestrator:50050",
    my_task
);

// Configurazione RT
RTConfig config;
wrapper.set_rt_config(config);

// Avvia
wrapper.start();

// Attendi segnale stop (Ctrl+C)
signal(SIGINT, signal_handler);
pause();

// Cleanup
wrapper.stop();
```

### RTUtils API

```cpp
// Memory locking
RTUtils::lock_memory();
RTUtils::prefault_stack(8 * 1024 * 1024);

// Thread RT
RTUtils::set_thread_realtime(RT_POLICY_FIFO, 80);
RTUtils::set_cpu_affinity(2);

// Full config
RTConfig config;
config.policy = RT_POLICY_FIFO;
config.priority = 90;
config.cpu_affinity = 0;
config.lock_memory = true;
RTUtils::apply_rt_config(config);

// Check capabilities
if (RTUtils::has_rt_capabilities()) {
    std::cout << "RT available" << std::endl;
}
```

### Schedule Parser API

```cpp
// Parse from YAML
TaskSchedule schedule = ScheduleParser::parse_yaml("path/to/schedule.yaml");

// Access tasks
for (const auto& task : schedule.tasks) {
    std::cout << task.task_id << std::endl;
}

// Sort by time
schedule.sort_by_time();

// Create programmatically
TaskSchedule schedule;
ScheduledTask task;
task.task_id = "task_1";
task.task_address = "localhost:50051";
task.scheduled_time_us = 1000000;
schedule.tasks.push_back(task);
```

---

## Conclusioni

Questa guida copre l'architettura completa e i dettagli implementativi del sistema gRPC Orchestrator. Per ulteriori informazioni:

- **Esempi pratici**: Directory `examples/`
- **Schedule di riferimento**: Directory `schedules/`
- **Test**: Directory `test/` (se presente)
- **Issue tracker**: GitHub issues

**Happy coding! 🚀**
