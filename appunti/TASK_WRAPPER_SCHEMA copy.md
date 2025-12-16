# Task Wrapper - Detailed Architecture Schema

## Task Wrapper Component Diagram

```
┌─────────────────────────────────────────────────────────────────────────────────────┐
│                              TASK WRAPPER                                           │
│                         (Infrastructure Layer)                                      │
│                                                                                     │
│  ┌───────────────────────────────────────────────────────────────────────────────┐ │
│  │                         IDENTIFICATION                                        │ │
│  │                                                                               │ │
│  │  • task_id_: string                                                           │ │
│  │    └─> Unique identifier for this task instance                              │ │
│  │                                                                               │ │
│  │  • listen_address_: string                                                    │ │
│  │    └─> gRPC server address (e.g., "0.0.0.0:50051")                          │ │
│  │                                                                               │ │
│  │  • orchestrator_address_: string                                              │ │
│  │    └─> Orchestrator gRPC address for notifications                           │ │
│  └───────────────────────────────────────────────────────────────────────────────┘ │
│                                                                                     │
│  ┌───────────────────────────────────────────────────────────────────────────────┐ │
│  │                      gRPC SERVER (Incoming Commands)                          │ │
│  │                                                                               │ │
│  │  • server_: unique_ptr<grpc::Server>                                          │ │
│  │    └─> Listens for commands from orchestrator                                │ │
│  │                                                                               │ │
│  │  • service_: unique_ptr<TaskServiceImpl>                                      │ │
│  │    └─> Implements 3 RPC methods:                                             │ │
│  │        ┌──────────────────────────────────────────────────────────┐          │ │
│  │        │  1. StartTask(request) → response                        │          │ │
│  │        │     • Validates state (must be IDLE)                     │          │ │
│  │        │     • Calls execute_task()                               │          │ │
│  │        │     • Returns success + start_time                       │          │ │
│  │        │                                                           │          │ │
│  │        │  2. StopTask(request) → response                         │          │ │
│  │        │     • Sets stop_requested_ flag                          │          │ │
│  │        │     • Triggers graceful shutdown                         │          │ │
│  │        │     • Returns stop_time                                  │          │ │
│  │        │                                                           │          │ │
│  │        │  3. GetTaskStatus(request) → response                    │          │ │
│  │        │     • Returns current state                              │          │ │
│  │        │     • Returns timing information                         │          │ │
│  │        │     • Returns resource usage (TODO)                      │          │ │
│  │        └──────────────────────────────────────────────────────────┘          │ │
│  └───────────────────────────────────────────────────────────────────────────────┘ │
│                                                                                     │
│  ┌───────────────────────────────────────────────────────────────────────────────┐ │
│  │                    gRPC CLIENT (Outgoing Notifications)                       │ │
│  │                                                                               │ │
│  │  • orchestrator_stub_: unique_ptr<OrchestratorService::Stub>                  │ │
│  │    └─> Sends notifications back to orchestrator                              │ │
│  │                                                                               │ │
│  │  • notify_orchestrator_end(result, error_msg)                                 │ │
│  │    └─> Sends TaskEndNotification with:                                       │ │
│  │        • task_id                                                              │ │
│  │        • result (SUCCESS/FAILURE/CANCELLED)                                   │ │
│  │        • start_time_us                                                        │ │
│  │        • end_time_us                                                          │ │
│  │        • execution_duration_us                                                │ │
│  │        • error_message (if any)                                               │ │
│  └───────────────────────────────────────────────────────────────────────────────┘ │
│                                                                                     │
│  ┌───────────────────────────────────────────────────────────────────────────────┐ │
│  │                         STATE MANAGEMENT                                      │ │
│  │                                                                               │ │
│  │  • state_: atomic<TaskState>                                                  │ │
│  │    └─> Current task state (thread-safe)                                      │ │
│  │        ┌────────────────────────────────────────────────────────┐            │ │
│  │        │  IDLE       → Waiting for StartTask command            │            │ │
│  │        │  STARTING   → Initializing execution thread            │            │ │
│  │        │  RUNNING    → Executing user callback                  │            │ │
│  │        │  COMPLETED  → Finished, sending notification           │            │ │
│  │        │  STOPPED    → Shutdown requested                       │            │ │
│  │        │  CANCELLED  → Execution cancelled                      │            │ │
│  │        └────────────────────────────────────────────────────────┘            │ │
│  │                                                                               │ │
│  │  • running_: atomic<bool>                                                     │ │
│  │    └─> Wrapper is active and listening                                       │ │
│  │                                                                               │ │
│  │  • stop_requested_: atomic<bool>                                              │ │
│  │    └─> Graceful shutdown flag                                                │ │
│  └───────────────────────────────────────────────────────────────────────────────┘ │
│                                                                                     │
│  ┌───────────────────────────────────────────────────────────────────────────────┐ │
│  │                        EXECUTION MANAGEMENT                                   │ │
│  │                                                                               │ │
│  │  • execution_callback_: TaskExecutionCallback                                 │ │
│  │    └─> User-defined function to execute                                      │ │
│  │        Type: function<TaskResult(const map<string, string>&)>                │ │
│  │        • Input: Parameters map (from schedule)                               │ │
│  │        • Output: TaskResult (SUCCESS/FAILURE/UNKNOWN)                        │ │
│  │                                                                               │ │
│  │  • execution_thread_: std::thread                                             │ │
│  │    └─> Separate thread for task execution                                    │ │
│  │        • Created on StartTask                                                │ │
│  │        • Runs task_execution_thread()                                        │ │
│  │        • Joined on stop() or completion                                      │ │
│  │                                                                               │ │
│  │  • execute_task(request)                                                      │ │
│  │    └─> Entry point for task execution                                        │ │
│  │        1. Join previous thread if exists                                     │ │
│  │        2. Create new thread with request                                     │ │
│  │        3. Thread runs task_execution_thread()                                │ │
│  │                                                                               │ │
│  │  • task_execution_thread(request)                                             │ │
│  │    └─> Main execution logic:                                                 │ │
│  │        ┌──────────────────────────────────────────────────────────┐          │ │
│  │        │  1. State = STARTING                                     │          │ │
│  │        │  2. Apply RT configuration (if set)                      │          │ │
│  │        │  3. Record start_time_us                                 │          │ │
│  │        │  4. Convert request params to map                        │          │ │
│  │        │  5. Add task_id to params                                │          │ │
│  │        │  6. State = RUNNING                                      │          │ │
│  │        │  7. Call execution_callback_(params)                     │          │ │
│  │        │  8. Catch exceptions                                     │          │ │
│  │        │  9. Record end_time_us                                   │          │ │
│  │        │ 10. Check stop_requested flag                            │          │ │
│  │        │ 11. State = COMPLETED                                    │          │ │
│  │        │ 12. notify_orchestrator_end()                            │          │ │
│  │        │ 13. State = IDLE                                         │          │ │
│  │        └──────────────────────────────────────────────────────────┘          │ │
│  └───────────────────────────────────────────────────────────────────────────────┘ │
│                                                                                     │
│  ┌───────────────────────────────────────────────────────────────────────────────┐ │
│  │                      REAL-TIME CONFIGURATION                                  │ │
│  │                                                                               │ │
│  │  • rt_config_: RTConfig                                                       │ │
│  │    └─> Real-time parameters for execution thread                             │ │
│  │        ┌──────────────────────────────────────────────────────────┐          │ │
│  │        │  • policy: RT_POLICY_NONE / FIFO / RR                    │          │ │
│  │        │    └─> Scheduling policy                                 │          │ │
│  │        │                                                           │          │ │
│  │        │  • priority: int (1-99)                                  │          │ │
│  │        │    └─> Real-time priority level                          │          │ │
│  │        │                                                           │          │ │
│  │        │  • cpu_affinity: int (-1 or core number)                 │          │ │
│  │        │    └─> Pin thread to specific CPU core                   │          │ │
│  │        │                                                           │          │ │
│  │        │  • lock_memory: bool                                     │          │ │
│  │        │    └─> Call mlockall() to prevent paging                 │          │ │
│  │        │                                                           │          │ │
│  │        │  • prefault_stack: bool                                  │          │ │
│  │        │    └─> Pre-fault stack pages                             │          │ │
│  │        └──────────────────────────────────────────────────────────┘          │ │
│  │                                                                               │ │
│  │  • set_rt_config(config)                                                      │ │
│  │    └─> Set RT config (called before start)                                   │ │
│  │        Applied to execution thread via RTUtils::apply_rt_config()            │ │
│  └───────────────────────────────────────────────────────────────────────────────┘ │
│                                                                                     │
│  ┌───────────────────────────────────────────────────────────────────────────────┐ │
│  │                          TIMING TRACKING                                      │ │
│  │                                                                               │ │
│  │  • start_time_us_: int64_t                                                    │ │
│  │    └─> Task start timestamp (microseconds)                                   │ │
│  │        Set when state → RUNNING                                              │ │
│  │                                                                               │ │
│  │  • end_time_us_: int64_t                                                      │ │
│  │    └─> Task end timestamp (microseconds)                                     │ │
│  │        Set when callback returns                                             │ │
│  │                                                                               │ │
│  │  • get_start_time_us()                                                        │ │
│  │    └─> Returns start timestamp                                               │ │
│  │                                                                               │ │
│  │  • get_elapsed_time_us()                                                      │ │
│  │    └─> Returns elapsed time:                                                 │ │
│  │        • If RUNNING: current_time - start_time                               │ │
│  │        • If COMPLETED: end_time - start_time                                 │ │
│  │                                                                               │ │
│  │  • get_current_time_us()                                                      │ │
│  │    └─> Helper: std::chrono::steady_clock in microseconds                     │ │
│  └───────────────────────────────────────────────────────────────────────────────┘ │
│                                                                                     │
│  ┌───────────────────────────────────────────────────────────────────────────────┐ │
│  │                         THREAD SAFETY                                         │ │
│  │                                                                               │ │
│  │  • mutex_: mutable std::mutex                                                 │ │
│  │    └─> Protects shared state during configuration                            │ │
│  │        Used in:                                                               │ │
│  │        • set_rt_config()                                                      │ │
│  │        • Any operation requiring exclusive access                            │ │
│  │                                                                               │ │
│  │  • Atomic variables (lock-free):                                              │ │
│  │    • state_ (atomic<TaskState>)                                               │ │
│  │    • running_ (atomic<bool>)                                                  │ │
│  │    • stop_requested_ (atomic<bool>)                                           │ │
│  └───────────────────────────────────────────────────────────────────────────────┘ │
│                                                                                     │
│  ┌───────────────────────────────────────────────────────────────────────────────┐ │
│  │                         LIFECYCLE METHODS                                     │ │
│  │                                                                               │ │
│  │  • Constructor(task_id, listen_addr, orch_addr, callback)                    │ │
│  │    └─> Initialize all components                                             │ │
│  │        1. Store identification info                                          │ │
│  │        2. Create TaskServiceImpl                                             │ │
│  │        3. Create gRPC channel to orchestrator                                │ │
│  │        4. Create orchestrator stub                                           │ │
│  │        5. Set initial state = IDLE                                           │ │
│  │                                                                               │ │
│  │  • start()                                                                    │ │
│  │    └─> Start listening for commands                                          │ │
│  │        1. Set running_ = true                                                │ │
│  │        2. Build gRPC server                                                  │ │
│  │        3. Register TaskServiceImpl                                           │ │
│  │        4. Start server on listen_address                                     │ │
│  │        5. State = IDLE (ready for commands)                                  │ │
│  │                                                                               │ │
│  │  • stop()                                                                     │ │
│  │    └─> Graceful shutdown                                                     │ │
│  │        1. Set running_ = false                                               │ │
│  │        2. Set stop_requested_ = true                                         │ │
│  │        3. Join execution_thread_ if running                                  │ │
│  │        4. Shutdown gRPC server                                               │ │
│  │        5. State = STOPPED                                                    │ │
│  │                                                                               │ │
│  │  • Destructor                                                                 │ │
│  │    └─> Calls stop() to ensure cleanup                                        │ │
│  └───────────────────────────────────────────────────────────────────────────────┘ │
│                                                                                     │
└─────────────────────────────────────────────────────────────────────────────────────┘
```

---

## Task Wrapper Data Flow

```
                    ┌──────────────────────────────────┐
                    │   Orchestrator sends command     │
                    │   StartTask(params)              │
                    └────────────┬─────────────────────┘
                                 │
                                 ▼
                    ┌──────────────────────────────────┐
                    │   gRPC Server receives request   │
                    │   (TaskServiceImpl::StartTask)   │
                    └────────────┬─────────────────────┘
                                 │
                                 ▼
                    ┌──────────────────────────────────┐
                    │   Validate state == IDLE         │
                    └────────────┬─────────────────────┘
                                 │
                                 ▼
                    ┌──────────────────────────────────┐
                    │   Call execute_task(request)     │
                    └────────────┬─────────────────────┘
                                 │
                                 ▼
                    ┌──────────────────────────────────┐
                    │   Create execution_thread_       │
                    │   Run task_execution_thread()    │
                    └────────────┬─────────────────────┘
                                 │
                                 ▼
                    ┌──────────────────────────────────┐
                    │   State = STARTING               │
                    └────────────┬─────────────────────┘
                                 │
                                 ▼
                    ┌──────────────────────────────────┐
                    │   Apply RT Configuration         │
                    │   • Set scheduling policy        │
                    │   • Set priority                 │
                    │   • Set CPU affinity             │
                    │   • Lock memory (mlockall)       │
                    │   • Prefault stack               │
                    └────────────┬─────────────────────┘
                                 │
                                 ▼
                    ┌──────────────────────────────────┐
                    │   Record start_time_us           │
                    └────────────┬─────────────────────┘
                                 │
                                 ▼
                    ┌──────────────────────────────────┐
                    │   Convert params to map          │
                    │   Add task_id to params          │
                    └────────────┬─────────────────────┘
                                 │
                                 ▼
                    ┌──────────────────────────────────┐
                    │   State = RUNNING                │
                    └────────────┬─────────────────────┘
                                 │
                                 ▼
                    ┌──────────────────────────────────┐
                    │   Call execution_callback_()     │
                    │   (User-defined logic)           │
                    └────────────┬─────────────────────┘
                                 │
                                 ▼
                    ┌──────────────────────────────────┐
                    │   Receive result                 │
                    │   (SUCCESS/FAILURE/UNKNOWN)      │
                    └────────────┬─────────────────────┘
                                 │
                                 ▼
                    ┌──────────────────────────────────┐
                    │   Record end_time_us             │
                    │   Calculate duration             │
                    └────────────┬─────────────────────┘
                                 │
                                 ▼
                    ┌──────────────────────────────────┐
                    │   State = COMPLETED              │
                    └────────────┬─────────────────────┘
                                 │
                                 ▼
                    ┌──────────────────────────────────┐
                    │   notify_orchestrator_end()      │
                    │   Send TaskEndNotification       │
                    └────────────┬─────────────────────┘
                                 │
                                 ▼
                    ┌──────────────────────────────────┐
                    │   Wait for acknowledgment        │
                    └────────────┬─────────────────────┘
                                 │
                                 ▼
                    ┌──────────────────────────────────┐
                    │   State = IDLE                   │
                    │   Ready for next command         │
                    └──────────────────────────────────┘
```

---

## State Transition Diagram

```
                    ┌─────────────┐
                    │   STOPPED   │
                    └──────┬──────┘
                           │
                           │ start()
                           ▼
                    ┌─────────────┐
              ┌────>│    IDLE     │<────┐
              │     └──────┬──────┘     │
              │            │            │
              │            │ StartTask  │
              │            │ received   │
              │            ▼            │
              │     ┌─────────────┐    │
              │     │  STARTING   │    │
              │     └──────┬──────┘    │
              │            │            │
              │            │ RT config  │
              │            │ applied    │
              │            ▼            │
              │     ┌─────────────┐    │
              │     │   RUNNING   │    │
              │     └──────┬──────┘    │
              │            │            │
              │            │ Callback   │
              │            │ returns    │
              │            ▼            │
              │     ┌─────────────┐    │
              │     │  COMPLETED  │    │
              │     └──────┬──────┘    │
              │            │            │
              │            │ Notified   │
              │            │ orchestr.  │
              └────────────┴────────────┘
                           
                           
       Alternative path:
       
       RUNNING ──stop()──> CANCELLED ──cleanup──> IDLE
```

---

## Component Responsibilities Summary

| Component | Responsibility | Key Methods/Fields |
|-----------|----------------|-------------------|
| **Identification** | Store task identity and addresses | `task_id_`, `listen_address_`, `orchestrator_address_` |
| **gRPC Server** | Receive commands from orchestrator | `server_`, `service_`, `StartTask()`, `StopTask()`, `GetTaskStatus()` |
| **gRPC Client** | Send notifications to orchestrator | `orchestrator_stub_`, `notify_orchestrator_end()` |
| **State Management** | Track task lifecycle state | `state_`, `running_`, `stop_requested_` |
| **Execution** | Run user-defined task logic | `execution_callback_`, `execution_thread_`, `execute_task()` |
| **RT Configuration** | Apply real-time settings | `rt_config_`, `set_rt_config()` |
| **Timing** | Track execution timestamps | `start_time_us_`, `end_time_us_`, `get_elapsed_time_us()` |
| **Thread Safety** | Protect shared resources | `mutex_`, atomic variables |
| **Lifecycle** | Manage wrapper lifetime | `start()`, `stop()`, constructor, destructor |

---

## Key Design Patterns

### 1. **Wrapper Pattern**
- TaskWrapper wraps user callback with infrastructure
- User code is isolated from gRPC, RT, and state management
- Clean separation: infrastructure vs. business logic

### 2. **State Machine**
- Clear state transitions (IDLE → STARTING → RUNNING → COMPLETED → IDLE)
- Atomic state variable for thread-safe access
- State validation before operations

### 3. **Thread-per-Task**
- Each task execution runs in separate thread
- Main thread handles gRPC server
- Execution thread applies RT configuration

### 4. **Bidirectional Communication**
- Server role: Receive commands from orchestrator
- Client role: Send notifications to orchestrator
- Decoupled request/response pattern

### 5. **Resource Management (RAII)**
- unique_ptr for automatic cleanup
- Destructor ensures graceful shutdown
- Thread joining on stop

---

## Memory Layout

```
TaskWrapper Object
├── Identification (strings)
│   ├── task_id_              : ~32 bytes
│   ├── listen_address_       : ~32 bytes
│   └── orchestrator_address_ : ~32 bytes
│
├── gRPC Components (smart pointers)
│   ├── server_               : 8 bytes (ptr) + heap allocation
│   ├── service_              : 8 bytes (ptr) + heap allocation
│   └── orchestrator_stub_    : 8 bytes (ptr) + heap allocation
│
├── Execution (callback + thread)
│   ├── execution_callback_   : ~32 bytes (std::function)
│   └── execution_thread_     : ~8 bytes (thread handle)
│
├── State (atomic variables)
│   ├── state_                : 4 bytes (atomic int)
│   ├── running_              : 1 byte (atomic bool)
│   └── stop_requested_       : 1 byte (atomic bool)
│
├── Timing (integers)
│   ├── start_time_us_        : 8 bytes
│   └── end_time_us_          : 8 bytes
│
├── Thread Safety
│   └── mutex_                : ~40 bytes (platform dependent)
│
└── RT Configuration
    └── rt_config_            : ~20 bytes (struct)

Total: ~300-400 bytes (excluding heap allocations)
```

---

## Thread Model

```
Main Thread                    Execution Thread
    │                               │
    │ start()                       │
    ├─────────────┐                 │
    │ gRPC Server │                 │
    │ listening   │                 │
    └─────────────┘                 │
    │                               │
    │ StartTask received            │
    │                               │
    │ execute_task()                │
    ├──────────────────────────────>│
    │                               │ task_execution_thread()
    │                               ├─────────────┐
    │                               │ Apply RT    │
    │                               │ config      │
    │                               └─────────────┘
    │                               │
    │                               │ State = RUNNING
    │                               │
    │                               │ Call callback
    │                               ├─────────────┐
    │                               │ User logic  │
    │                               └─────────────┘
    │                               │
    │                               │ Notify orch
    │                               │
    │                               │ State = IDLE
    │<──────────────────────────────┤
    │ Thread joined                 │
    │                               ▼
    │                          (Thread exits)
    │
    ▼
```

---

## Error Handling

### Exception Safety
```cpp
try {
    result = execution_callback_(params);
} catch (const std::exception& e) {
    result = TASK_RESULT_FAILURE;
    error_message = "Exception: " + e.what();
} catch (...) {
    result = TASK_RESULT_FAILURE;
    error_message = "Unknown exception";
}
```

### State Validation
```cpp
if (wrapper_->get_state() != TASK_STATE_IDLE) {
    response->set_success(false);
    response->set_message("Task is not in IDLE state");
    return grpc::Status::OK;
}
```

### Graceful Shutdown
```cpp
if (stop_requested_) {
    result = TASK_RESULT_CANCELLED;
    error_message = "Task cancelled by stop request";
}
```

---

## Performance Characteristics

| Operation | Typical Time | Notes |
|-----------|-------------|-------|
| **Constructor** | 10-50 ms | gRPC channel creation |
| **start()** | 50-100 ms | gRPC server startup |
| **StartTask RPC** | 5-20 ms | Network + validation |
| **RT config apply** | 200-300 ms | Memory locking overhead |
| **State transition** | <1 μs | Atomic operation |
| **NotifyTaskEnd RPC** | 5-20 ms | Network + serialization |
| **stop()** | 10-100 ms | Thread join + server shutdown |

---

## Usage Example

```cpp
// 1. Define callback
TaskResult my_callback(const map<string, string>& params) {
    // Your custom logic here
    return TASK_RESULT_SUCCESS;
}

// 2. Create wrapper
TaskWrapper wrapper(
    "task_1",                    // task_id
    "0.0.0.0:50051",            // listen_address
    "orchestrator:50050",        // orchestrator_address
    my_callback                  // execution_callback
);

// 3. Configure RT (optional)
RTConfig rt_config;
rt_config.policy = RT_POLICY_FIFO;
rt_config.priority = 75;
rt_config.cpu_affinity = 1;
rt_config.lock_memory = true;
wrapper.set_rt_config(rt_config);

// 4. Start listening
wrapper.start();

// 5. Wait for commands (wrapper handles everything)
// ...

// 6. Graceful shutdown
wrapper.stop();
```
