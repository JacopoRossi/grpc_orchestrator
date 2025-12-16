# Generic Architecture Diagrams

## Sistema gRPC Orchestrator - Architettura Generale

---

## 1. High-Level Architecture Diagram

```mermaid
graph TB
    subgraph "Docker Environment"
        subgraph "Orchestrator Container"
            ORCH[Orchestrator Process]
            SCHED[Scheduler Engine]
            PARSER[Schedule Parser]
            GRPC_S[gRPC Server]
            GRPC_C[gRPC Client Pool]
            RT_O[RT Configuration]
            
            ORCH --> SCHED
            ORCH --> PARSER
            ORCH --> GRPC_S
            ORCH --> GRPC_C
            ORCH --> RT_O
        end
        
        subgraph "Task Container N"
            TASK_N[Task Process]
            WRAPPER_N[Task Wrapper]
            GRPC_TN[gRPC Server]
            CALLBACK_N[Execution Callback]
            RT_N[RT Thread]
            
            TASK_N --> WRAPPER_N
            WRAPPER_N --> GRPC_TN
            WRAPPER_N --> CALLBACK_N
            WRAPPER_N --> RT_N
        end
        
        subgraph "Task Container 2"
            TASK_2[Task Process]
            WRAPPER_2[Task Wrapper]
            GRPC_T2[gRPC Server]
            CALLBACK_2[Execution Callback]
            RT_2[RT Thread]
            
            TASK_2 --> WRAPPER_2
            WRAPPER_2 --> GRPC_T2
            WRAPPER_2 --> CALLBACK_2
            WRAPPER_2 --> RT_2
        end
        
        subgraph "Task Container 1"
            TASK_1[Task Process]
            WRAPPER_1[Task Wrapper]
            GRPC_T1[gRPC Server]
            CALLBACK_1[Execution Callback]
            RT_1[RT Thread]
            
            TASK_1 --> WRAPPER_1
            WRAPPER_1 --> GRPC_T1
            WRAPPER_1 --> CALLBACK_1
            WRAPPER_1 --> RT_1
        end
    end
    
    SCHEDULE[Schedule YAML File]
    
    SCHEDULE -.->|Load| PARSER
    GRPC_C -->|StartTask| GRPC_T1
    GRPC_C -->|StartTask| GRPC_T2
    GRPC_C -->|StartTask| GRPC_TN
    WRAPPER_1 -.->|NotifyTaskEnd| GRPC_S
    WRAPPER_2 -.->|NotifyTaskEnd| GRPC_S
    WRAPPER_N -.->|NotifyTaskEnd| GRPC_S
    
    style ORCH fill:#e1f5ff
    style TASK_1 fill:#ffe1e1
    style TASK_2 fill:#e1ffe1
    style TASK_N fill:#fff5e1
    style SCHEDULE fill:#f0f0f0
```

---

## 2. Generic Sequence Diagram

```mermaid
sequenceDiagram
    participant DC as Docker Compose
    participant O as Orchestrator
    participant T as Task Container(s)
    participant YAML as Schedule File

    Note over DC: System Startup
    
    %% Startup Phase
    rect rgb(240, 240, 240)
        Note over DC,T: Phase 1: Container Initialization
        DC->>T: Start all task containers
        activate T
        T->>T: Initialize task wrapper
        T->>T: Apply RT configuration
        T->>T: Lock memory (mlockall)
        T->>T: Start gRPC server
        T->>T: Enter IDLE state
        Note over T: Ready to receive commands
        
        DC->>O: Start orchestrator container
        activate O
        O->>O: Initialize orchestrator
        O->>O: Apply RT configuration
        O->>O: Lock memory (mlockall)
        O->>YAML: Load schedule
        YAML-->>O: Task definitions
        O->>O: Parse and validate schedule
        O->>O: Start gRPC server
        O->>O: Start scheduler thread
    end
    
    %% Execution Phase
    rect rgb(200, 230, 255)
        Note over O,T: Phase 2: Task Execution
        
        loop For each task in schedule
            O->>O: Determine next task to execute
            Note over O: Check dependencies<br/>Check timing constraints
            
            O->>T: gRPC: StartTask(task_id, params)
            T->>T: State: IDLE → STARTING
            T->>T: Create execution thread
            T->>T: Apply RT config to thread
            T-->>O: Response: success + start_time
            
            T->>T: State: STARTING → RUNNING
            T->>T: Execute callback function
            Note over T: User-defined logic<br/>with RT guarantees
            T->>T: State: RUNNING → COMPLETED
            
            T->>O: gRPC: NotifyTaskEnd(result, duration)
            O->>O: Update task status
            O->>O: Check schedule completion
            O-->>T: Response: acknowledged
            
            T->>T: State: COMPLETED → IDLE
            Note over T: Ready for next command
        end
    end
    
    %% Completion Phase
    rect rgb(240, 240, 240)
        Note over O,T: Phase 3: Shutdown
        O->>O: All tasks completed
        O->>O: Generate execution summary
        O->>O: Stop scheduler
        O->>O: Shutdown gRPC server
        deactivate O
        
        DC->>T: Send SIGTERM
        T->>T: Graceful shutdown
        T->>T: Stop gRPC server
        deactivate T
    end
```

---

## 3. Detailed Component Architecture

```mermaid
graph LR
    subgraph "External Layer"
        USER[User/Developer]
        YAML_FILE[Schedule YAML]
        DOCKER[Docker Compose]
    end
    
    subgraph "Orchestrator Layer"
        subgraph "Core Components"
            MAIN_O[orchestrator_main]
            ORCH_CLASS[Orchestrator Class]
            SCHED_ENGINE[Scheduler Engine]
        end
        
        subgraph "Schedule Management"
            PARSER[YAML Parser]
            VALIDATOR[Schedule Validator]
            TASK_QUEUE[Task Queue]
        end
        
        subgraph "Communication"
            GRPC_SERVER_O[gRPC Server<br/>:50050]
            GRPC_CLIENTS[gRPC Client Pool]
            STUB_POOL[Task Stubs]
        end
        
        subgraph "Real-Time"
            RT_CONFIG_O[RT Configuration]
            RT_THREAD_O[RT Scheduler Thread]
        end
    end
    
    subgraph "Task Layer"
        subgraph "Task Infrastructure"
            MAIN_T[task_main]
            WRAPPER[TaskWrapper Class]
            SERVICE[TaskService Impl]
        end
        
        subgraph "Execution"
            CALLBACK[Execution Callback<br/>User-defined]
            EXEC_THREAD[Execution Thread]
        end
        
        subgraph "Communication"
            GRPC_SERVER_T[gRPC Server<br/>:5005X]
            GRPC_CLIENT_T[gRPC Client]
        end
        
        subgraph "Real-Time"
            RT_CONFIG_T[RT Configuration]
            RT_THREAD_T[RT Execution Thread]
        end
        
        subgraph "State Management"
            STATE_MACHINE[State Machine<br/>IDLE/RUNNING/COMPLETED]
        end
    end
    
    USER -->|Define| YAML_FILE
    USER -->|Execute| DOCKER
    DOCKER -->|Start| MAIN_O
    DOCKER -->|Start| MAIN_T
    
    YAML_FILE -->|Load| PARSER
    MAIN_O --> ORCH_CLASS
    ORCH_CLASS --> SCHED_ENGINE
    ORCH_CLASS --> GRPC_SERVER_O
    ORCH_CLASS --> GRPC_CLIENTS
    PARSER --> VALIDATOR
    VALIDATOR --> TASK_QUEUE
    SCHED_ENGINE --> TASK_QUEUE
    SCHED_ENGINE --> RT_THREAD_O
    RT_CONFIG_O --> RT_THREAD_O
    GRPC_CLIENTS --> STUB_POOL
    
    MAIN_T --> WRAPPER
    WRAPPER --> SERVICE
    WRAPPER --> GRPC_SERVER_T
    WRAPPER --> GRPC_CLIENT_T
    WRAPPER --> STATE_MACHINE
    SERVICE --> EXEC_THREAD
    EXEC_THREAD --> CALLBACK
    EXEC_THREAD --> RT_THREAD_T
    RT_CONFIG_T --> RT_THREAD_T
    
    STUB_POOL -.->|StartTask| GRPC_SERVER_T
    GRPC_CLIENT_T -.->|NotifyTaskEnd| GRPC_SERVER_O
    
    style USER fill:#e1e1ff
    style YAML_FILE fill:#f0f0f0
    style ORCH_CLASS fill:#e1f5ff
    style WRAPPER fill:#ffe1e1
    style CALLBACK fill:#ffffe1
```

---

## 4. Data Flow Diagram

```mermaid
flowchart TD
    START([System Start])
    
    START --> LOAD_YAML[Load Schedule YAML]
    LOAD_YAML --> PARSE[Parse Task Definitions]
    PARSE --> VALIDATE[Validate Schedule]
    VALIDATE --> BUILD_QUEUE[Build Task Queue]
    
    BUILD_QUEUE --> SCHED_LOOP{Scheduler Loop}
    
    SCHED_LOOP -->|Sequential Task| CHECK_DEP[Check Dependencies]
    SCHED_LOOP -->|Timed Task| CHECK_TIME[Check Scheduled Time]
    
    CHECK_DEP -->|Ready| SELECT_TASK[Select Task]
    CHECK_DEP -->|Not Ready| WAIT_DEP[Wait for Dependency]
    WAIT_DEP --> SCHED_LOOP
    
    CHECK_TIME -->|Time Reached| SELECT_TASK
    CHECK_TIME -->|Not Yet| WAIT_TIME[Wait Until Time]
    WAIT_TIME --> SCHED_LOOP
    
    SELECT_TASK --> CREATE_STUB[Create gRPC Stub]
    CREATE_STUB --> SEND_START[Send StartTask Request]
    SEND_START --> TASK_EXEC[Task Executes]
    
    TASK_EXEC --> APPLY_RT[Apply RT Config]
    APPLY_RT --> RUN_CALLBACK[Run User Callback]
    RUN_CALLBACK --> SEND_END[Send NotifyTaskEnd]
    
    SEND_END --> UPDATE_STATUS[Update Task Status]
    UPDATE_STATUS --> CHECK_COMPLETE{All Tasks Done?}
    
    CHECK_COMPLETE -->|No| SCHED_LOOP
    CHECK_COMPLETE -->|Yes| SUMMARY[Generate Summary]
    SUMMARY --> STOP([System Stop])
    
    style START fill:#90EE90
    style STOP fill:#FFB6C1
    style SCHED_LOOP fill:#FFE4B5
    style TASK_EXEC fill:#E0FFFF
    style RUN_CALLBACK fill:#FFFFE0
```

---

## 5. Task State Machine

```mermaid
stateDiagram-v2
    [*] --> IDLE: Container Starts
    
    IDLE --> STARTING: Receive StartTask
    STARTING --> RUNNING: RT Config Applied
    RUNNING --> COMPLETED: Execution Done
    COMPLETED --> IDLE: Notification Sent
    
    RUNNING --> CANCELLED: Stop Requested
    CANCELLED --> IDLE: Cleanup Done
    
    IDLE --> STOPPED: Shutdown Signal
    STOPPED --> [*]
    
    state IDLE {
        [*] --> Listening
        Listening --> WaitingCommand
        WaitingCommand --> Listening
    }
    
    state RUNNING {
        [*] --> ApplyingRT
        ApplyingRT --> ExecutingCallback
        ExecutingCallback --> [*]
    }
    
    state COMPLETED {
        [*] --> SendingNotification
        SendingNotification --> WaitingAck
        WaitingAck --> [*]
    }
```

---

## 6. System Layers

```mermaid
graph TB
    subgraph "Layer 1: User Interface"
        CLI[Command Line Interface]
        YAML[YAML Schedule Definition]
        DOCKER_COMPOSE[Docker Compose Configuration]
    end
    
    subgraph "Layer 2: Container Orchestration"
        DOCKER_ENGINE[Docker Engine]
        NETWORK[Docker Network Bridge]
        VOLUMES[Shared Volumes]
    end
    
    subgraph "Layer 3: Application Layer"
        ORCHESTRATOR[Orchestrator Service]
        TASKS[Task Services]
    end
    
    subgraph "Layer 4: Communication Layer"
        GRPC[gRPC Protocol]
        PROTOBUF[Protocol Buffers]
    end
    
    subgraph "Layer 5: Real-Time Layer"
        RT_SCHEDULER[RT Scheduler FIFO/RR]
        MEMORY_LOCK[Memory Locking mlockall]
        CPU_AFFINITY[CPU Affinity]
    end
    
    subgraph "Layer 6: Operating System"
        LINUX_KERNEL[Linux Kernel]
        CGROUPS[Control Groups]
        NAMESPACES[Namespaces]
    end
    
    CLI --> DOCKER_ENGINE
    YAML --> ORCHESTRATOR
    DOCKER_COMPOSE --> DOCKER_ENGINE
    
    DOCKER_ENGINE --> ORCHESTRATOR
    DOCKER_ENGINE --> TASKS
    DOCKER_ENGINE --> NETWORK
    DOCKER_ENGINE --> VOLUMES
    
    ORCHESTRATOR --> GRPC
    TASKS --> GRPC
    GRPC --> PROTOBUF
    
    ORCHESTRATOR --> RT_SCHEDULER
    TASKS --> RT_SCHEDULER
    RT_SCHEDULER --> MEMORY_LOCK
    RT_SCHEDULER --> CPU_AFFINITY
    
    DOCKER_ENGINE --> LINUX_KERNEL
    RT_SCHEDULER --> LINUX_KERNEL
    MEMORY_LOCK --> LINUX_KERNEL
    CPU_AFFINITY --> LINUX_KERNEL
    
    LINUX_KERNEL --> CGROUPS
    LINUX_KERNEL --> NAMESPACES
    
    style CLI fill:#e1e1ff
    style ORCHESTRATOR fill:#e1f5ff
    style TASKS fill:#ffe1e1
    style GRPC fill:#e1ffe1
    style RT_SCHEDULER fill:#ffffe1
    style LINUX_KERNEL fill:#f0f0f0
```

---

## 7. Communication Protocol

```mermaid
sequenceDiagram
    participant O as Orchestrator
    participant T as Task
    
    Note over O,T: Command Flow
    
    O->>T: StartTask Request
    Note right of O: task_id<br/>scheduled_time<br/>deadline<br/>priority<br/>parameters
    
    T->>T: Process Request
    T-->>O: StartTask Response
    Note left of T: success<br/>actual_start_time<br/>message
    
    Note over O,T: Execution Phase
    T->>T: Execute Task
    
    Note over O,T: Notification Flow
    T->>O: NotifyTaskEnd Request
    Note right of T: task_id<br/>result<br/>start_time<br/>end_time<br/>duration<br/>error_message
    
    O->>O: Update Status
    O-->>T: NotifyTaskEnd Response
    Note left of O: acknowledged
    
    Note over O,T: Optional: Status Query
    O->>T: GetTaskStatus Request
    T-->>O: GetTaskStatus Response
    Note left of T: state<br/>elapsed_time<br/>cpu_usage<br/>memory_usage
```

---

## Key Architectural Principles

### 1. **Separation of Concerns**
- **Orchestrator**: Scheduling, coordination, monitoring
- **Task Wrapper**: Infrastructure, communication, lifecycle
- **Task Callback**: Business logic, user-defined behavior

### 2. **Scalability**
- Add new tasks by deploying new containers
- No code changes in orchestrator required
- Dynamic task discovery via schedule YAML

### 3. **Reusability**
- Task containers are stateless between executions
- Same task can be called multiple times
- Wrapper infrastructure is generic and stable

### 4. **Real-Time Guarantees**
- Dedicated CPU cores per container
- Memory locking prevents page faults
- FIFO/RR scheduling for deterministic execution
- Stack pre-faulting for predictable latency

### 5. **Fault Tolerance**
- Task failures don't crash orchestrator
- Retry mechanisms for transient failures
- Critical task flag for schedule abortion
- Graceful shutdown on errors

### 6. **Observability**
- Detailed logging at each layer
- Execution summaries with timing data
- State tracking for debugging
- gRPC status monitoring

---

## Technology Stack

| Layer | Technology |
|-------|-----------|
| **Containerization** | Docker, Docker Compose |
| **Communication** | gRPC, Protocol Buffers |
| **Language** | C++17 |
| **Build System** | CMake |
| **Real-Time** | Linux RT (PREEMPT_RT optional) |
| **Scheduling** | SCHED_FIFO, SCHED_RR |
| **Memory** | mlockall(), stack pre-faulting |
| **Configuration** | YAML |

---

## Deployment Model

```
Host Machine (Linux)
│
├── Docker Engine
│   │
│   ├── grpc_orchestrator_network (Bridge)
│   │   │
│   │   ├── Orchestrator Container
│   │   │   ├── CPU: Core 0
│   │   │   ├── Priority: 80 (FIFO)
│   │   │   ├── Memory: Locked
│   │   │   └── Port: 50050
│   │   │
│   │   ├── Task Container 1
│   │   │   ├── CPU: Core 1
│   │   │   ├── Priority: 75 (FIFO)
│   │   │   ├── Memory: Locked
│   │   │   └── Port: 50051
│   │   │
│   │   ├── Task Container 2
│   │   │   ├── CPU: Core 2
│   │   │   ├── Priority: 75 (FIFO)
│   │   │   ├── Memory: Locked
│   │   │   └── Port: 50052
│   │   │
│   │   └── Task Container N
│   │       ├── CPU: Core N
│   │       ├── Priority: 75 (FIFO)
│   │       ├── Memory: Locked
│   │       └── Port: 5005N
│   │
│   └── Shared Volumes
│       ├── /schedules (YAML files)
│       └── /logs (execution logs)
```

---

## Extension Points

### Adding New Tasks
1. Implement callback function with signature: `TaskResult callback(const map<string, string>&)`
2. Create new container with `task_main` + your callback
3. Add task definition to schedule YAML
4. Deploy with Docker Compose

### Custom Scheduling Policies
1. Extend `ScheduleParser` for new task modes
2. Implement scheduling logic in `Orchestrator::scheduler_loop()`
3. Update YAML schema and validation

### Monitoring Integration
1. Implement metrics collection in `TaskWrapper`
2. Expose metrics via gRPC service
3. Connect to monitoring system (Prometheus, Grafana, etc.)

### Multi-Host Deployment
1. Replace Docker network with overlay network
2. Update task addresses in schedule YAML
3. Deploy containers across multiple hosts
4. Ensure network latency meets RT requirements
