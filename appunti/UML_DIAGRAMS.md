# UML Diagrams - gRPC Orchestrator System

## 1. Sequence Diagram - Task Execution Flow

```
┌──────────┐         ┌──────────────┐         ┌──────────────┐         ┌──────────────┐
│  Docker  │         │ Orchestrator │         │ Task Wrapper │         │   Callback   │
│ Compose  │         │              │         │              │         │   Function   │
└────┬─────┘         └──────┬───────┘         └──────┬───────┘         └──────┬───────┘
     │                      │                        │                        │
     │ Start containers     │                        │                        │
     ├─────────────────────>│                        │                        │
     │                      │                        │                        │
     │                      │ Initialize             │                        │
     │                      ├───────────┐            │                        │
     │                      │           │            │                        │
     │                      │<──────────┘            │                        │
     │                      │                        │                        │
     ├────────────────────────────────────────────────>                       │
     │                      │                        │                        │
     │                      │                        │ Initialize             │
     │                      │                        ├───────────┐            │
     │                      │                        │           │            │
     │                      │                        │<──────────┘            │
     │                      │                        │                        │
     │                      │                        │ State = IDLE           │
     │                      │                        ├───────────┐            │
     │                      │                        │           │            │
     │                      │                        │<──────────┘            │
     │                      │                        │                        │
     │ Load schedule        │                        │                        │
     │                      │<──────────┐            │                        │
     │                      │           │            │                        │
     │                      │───────────┘            │                        │
     │                      │                        │                        │
     │                      │ StartTask(params)      │                        │
     │                      ├───────────────────────>│                        │
     │                      │                        │                        │
     │                      │                        │ State = STARTING       │
     │                      │                        ├───────────┐            │
     │                      │                        │           │            │
     │                      │                        │<──────────┘            │
     │                      │                        │                        │
     │                      │                        │ Apply RT config        │
     │                      │                        ├───────────┐            │
     │                      │                        │           │            │
     │                      │                        │<──────────┘            │
     │                      │                        │                        │
     │                      │  Response(success)     │                        │
     │                      │<───────────────────────┤                        │
     │                      │                        │                        │
     │                      │                        │ State = RUNNING        │
     │                      │                        ├───────────┐            │
     │                      │                        │           │            │
     │                      │                        │<──────────┘            │
     │                      │                        │                        │
     │                      │                        │ Execute()              │
     │                      │                        ├───────────────────────>│
     │                      │                        │                        │
     │                      │                        │                        │ User logic
     │                      │                        │                        ├──────────┐
     │                      │                        │                        │          │
     │                      │                        │                        │<─────────┘
     │                      │                        │                        │
     │                      │                        │  Result                │
     │                      │                        │<───────────────────────┤
     │                      │                        │                        │
     │                      │                        │ State = COMPLETED      │
     │                      │                        ├───────────┐            │
     │                      │                        │           │            │
     │                      │                        │<──────────┘            │
     │                      │                        │                        │
     │                      │ NotifyTaskEnd(result)  │                        │
     │                      │<───────────────────────┤                        │
     │                      │                        │                        │
     │                      │ Update status          │                        │
     │                      ├───────────┐            │                        │
     │                      │           │            │                        │
     │                      │<──────────┘            │                        │
     │                      │                        │                        │
     │                      │  Acknowledged          │                        │
     │                      ├───────────────────────>│                        │
     │                      │                        │                        │
     │                      │                        │ State = IDLE           │
     │                      │                        ├───────────┐            │
     │                      │                        │           │            │
     │                      │                        │<──────────┘            │
     │                      │                        │                        │
```

---

## 2. Architecture Diagram - System Components

```
                                    ┌─────────────────────────────────────┐
                                    │         User / Developer            │
                                    │                                     │
                                    │  • Define Schedule (YAML)           │
                                    │  • Configure Docker Compose         │
                                    │  • Implement Task Callbacks         │
                                    └──────────────┬──────────────────────┘
                                                   │
                                                   │ docker-compose up
                                                   │
                    ┌──────────────────────────────▼──────────────────────────────┐
                    │                    Docker Environment                        │
                    │                                                              │
                    │  ┌────────────────────────────────────────────────────────┐ │
                    │  │              Docker Network Bridge                     │ │
                    │  │              (grpc_orchestrator_network)               │ │
                    │  └────────────────────────────────────────────────────────┘ │
                    │                                                              │
                    │  ┌──────────────────────────────────────────────────────┐   │
                    │  │         Orchestrator Container (CPU 0)               │   │
                    │  │                                                      │   │
                    │  │  ┌────────────────────────────────────────────┐     │   │
                    │  │  │      Orchestrator Process                  │     │   │
                    │  │  │                                            │     │   │
                    │  │  │  • Schedule Parser (YAML)                 │     │   │
                    │  │  │  • Scheduler Engine                       │     │   │
                    │  │  │  • Task Queue Manager                     │     │   │
                    │  │  │  • gRPC Server (:50050)                   │     │   │
                    │  │  │  • gRPC Client Pool (to tasks)            │     │   │
                    │  │  │  • RT Configuration (FIFO, Priority 80)   │     │   │
                    │  │  │  • Memory Locking (mlockall)              │     │   │
                    │  │  └────────────────────────────────────────────┘     │   │
                    │  └──────────────────┬───────────────────────────────────┘   │
                    │                     │                                       │
                    │                     │ gRPC Communication                    │
                    │                     │ (StartTask / NotifyTaskEnd)           │
                    │         ┌───────────┼───────────┬───────────────┐           │
                    │         │           │           │               │           │
                    │  ┌──────▼──────┐ ┌──▼──────┐ ┌──▼──────┐  ┌────▼──────┐   │
                    │  │   Task 1    │ │  Task 2 │ │  Task 3 │  │  Task N   │   │
                    │  │ Container   │ │Container│ │Container│  │ Container │   │
                    │  │  (CPU 1)    │ │ (CPU 2) │ │ (CPU 3) │  │  (CPU N)  │   │
                    │  │             │ │         │ │         │  │           │   │
                    │  │ ┌─────────┐ │ │┌───────┐│ │┌───────┐│  │┌─────────┐│   │
                    │  │ │  Task   │ │ ││ Task  ││ ││ Task  ││  ││  Task   ││   │
                    │  │ │ Wrapper │ │ ││Wrapper││ ││Wrapper││  ││ Wrapper ││   │
                    │  │ │         │ │ ││       ││ ││       ││  ││         ││   │
                    │  │ │ • gRPC  │ │ ││• gRPC ││ ││• gRPC ││  ││ • gRPC  ││   │
                    │  │ │  Server │ │ ││ Server││ ││ Server││  ││  Server ││   │
                    │  │ │ :50051  │ │ ││:50052 ││ ││:50053 ││  ││ :5005N  ││   │
                    │  │ │         │ │ ││       ││ ││       ││  ││         ││   │
                    │  │ │ • State │ │ ││• State││ ││• State││  ││ • State ││   │
                    │  │ │ Machine │ │ ││Machine││ ││Machine││  ││ Machine ││   │
                    │  │ │         │ │ ││       ││ ││       ││  ││         ││   │
                    │  │ │ • RT    │ │ ││• RT   ││ ││• RT   ││  ││ • RT    ││   │
                    │  │ │  Thread │ │ ││ Thread││ ││ Thread││  ││  Thread ││   │
                    │  │ │         │ │ ││       ││ ││       ││  ││         ││   │
                    │  │ │ • Memory│ │ ││• Mem  ││ ││• Mem  ││  ││ • Memory││   │
                    │  │ │  Lock   │ │ ││ Lock  ││ ││ Lock  ││  ││  Lock   ││   │
                    │  │ └────┬────┘ │ │└───┬───┘│ │└───┬───┘│  │└────┬────┘│   │
                    │  │      │      │ │    │    │ │    │    │  │     │     │   │
                    │  │ ┌────▼────┐ │ │┌───▼───┐│ │┌───▼───┐│  │┌────▼────┐│   │
                    │  │ │Callback │ │ ││Callback││││Callback││  ││Callback ││   │
                    │  │ │Function │ │ ││Function│││Function││  ││Function ││   │
                    │  │ │(Custom) │ │ ││(Custom)│││(Custom)││  ││(Custom) ││   │
                    │  │ └─────────┘ │ │└───────┘│ │└───────┘│  │└─────────┘│   │
                    │  └─────────────┘ └─────────┘ └─────────┘  └───────────┘   │
                    │                                                              │
                    └──────────────────────────────────────────────────────────────┘
                                                   │
                    ┌──────────────────────────────▼──────────────────────────────┐
                    │                    Linux Kernel Layer                        │
                    │                                                              │
                    │  • Real-Time Scheduler (SCHED_FIFO / SCHED_RR)              │
                    │  • Memory Management (mlockall, mlock)                      │
                    │  • CPU Affinity (sched_setaffinity)                         │
                    │  • Control Groups (cgroups)                                 │
                    │  • Namespaces (isolation)                                   │
                    │  • Network Stack (TCP/IP, gRPC over HTTP/2)                 │
                    │                                                              │
                    └──────────────────────────────────────────────────────────────┘
```

---

## 3. Detailed Sequence Diagram - Complete System Flow

```
User          Docker       Orchestrator    Schedule    Task          Callback
 │            Compose          │            YAML      Wrapper        Function
 │               │             │             │          │               │
 │ Start system  │             │             │          │               │
 ├──────────────>│             │             │          │               │
 │               │             │             │          │               │
 │               │ Start tasks │             │          │               │
 │               ├─────────────────────────────────────>│               │
 │               │             │             │          │               │
 │               │             │             │          │ Init wrapper  │
 │               │             │             │          ├──────────┐    │
 │               │             │             │          │          │    │
 │               │             │             │          │<─────────┘    │
 │               │             │             │          │               │
 │               │             │             │          │ Apply RT      │
 │               │             │             │          ├──────────┐    │
 │               │             │             │          │          │    │
 │               │             │             │          │<─────────┘    │
 │               │             │             │          │               │
 │               │             │             │          │ Lock memory   │
 │               │             │             │          ├──────────┐    │
 │               │             │             │          │          │    │
 │               │             │             │          │<─────────┘    │
 │               │             │             │          │               │
 │               │             │             │          │ Start gRPC    │
 │               │             │             │          ├──────────┐    │
 │               │             │             │          │          │    │
 │               │             │             │          │<─────────┘    │
 │               │             │             │          │               │
 │               │             │             │          │ IDLE state    │
 │               │             │             │          ├──────────┐    │
 │               │             │             │          │          │    │
 │               │             │             │          │<─────────┘    │
 │               │             │             │          │               │
 │               │ Start orch  │             │          │               │
 │               ├────────────>│             │          │               │
 │               │             │             │          │               │
 │               │             │ Init        │          │               │
 │               │             ├────────┐    │          │               │
 │               │             │        │    │          │               │
 │               │             │<───────┘    │          │               │
 │               │             │             │          │               │
 │               │             │ Load YAML   │          │               │
 │               │             ├────────────>│          │               │
 │               │             │             │          │               │
 │               │             │ Task defs   │          │               │
 │               │             │<────────────┤          │               │
 │               │             │             │          │               │
 │               │             │ Parse       │          │               │
 │               │             ├────────┐    │          │               │
 │               │             │        │    │          │               │
 │               │             │<───────┘    │          │               │
 │               │             │             │          │               │
 │               │             │ Build queue │          │               │
 │               │             ├────────┐    │          │               │
 │               │             │        │    │          │               │
 │               │             │<───────┘    │          │               │
 │               │             │             │          │               │
 │               │             │ Start sched │          │               │
 │               │             ├────────┐    │          │               │
 │               │             │        │    │          │               │
 │               │             │<───────┘    │          │               │
 │               │             │             │          │               │
 │               │             │ Select task │          │               │
 │               │             ├────────┐    │          │               │
 │               │             │        │    │          │               │
 │               │             │<───────┘    │          │               │
 │               │             │             │          │               │
 │               │             │ StartTask(params)      │               │
 │               │             ├────────────────────────>│               │
 │               │             │             │          │               │
 │               │             │             │          │ STARTING      │
 │               │             │             │          ├──────────┐    │
 │               │             │             │          │          │    │
 │               │             │             │          │<─────────┘    │
 │               │             │             │          │               │
 │               │             │             │          │ Create thread │
 │               │             │             │          ├──────────┐    │
 │               │             │             │          │          │    │
 │               │             │             │          │<─────────┘    │
 │               │             │             │          │               │
 │               │             │             │          │ Apply RT      │
 │               │             │             │          ├──────────┐    │
 │               │             │             │          │          │    │
 │               │             │             │          │<─────────┘    │
 │               │             │             │          │               │
 │               │             │  Response(success)     │               │
 │               │             │<────────────────────────┤               │
 │               │             │             │          │               │
 │               │             │             │          │ RUNNING       │
 │               │             │             │          ├──────────┐    │
 │               │             │             │          │          │    │
 │               │             │             │          │<─────────┘    │
 │               │             │             │          │               │
 │               │             │             │          │ Execute       │
 │               │             │             │          ├──────────────>│
 │               │             │             │          │               │
 │               │             │             │          │               │ Run logic
 │               │             │             │          │               ├─────────┐
 │               │             │             │          │               │         │
 │               │             │             │          │               │<────────┘
 │               │             │             │          │               │
 │               │             │             │          │  Result       │
 │               │             │             │          │<──────────────┤
 │               │             │             │          │               │
 │               │             │             │          │ COMPLETED     │
 │               │             │             │          ├──────────┐    │
 │               │             │             │          │          │    │
 │               │             │             │          │<─────────┘    │
 │               │             │             │          │               │
 │               │             │ NotifyTaskEnd(result)  │               │
 │               │             │<────────────────────────┤               │
 │               │             │             │          │               │
 │               │             │ Update      │          │               │
 │               │             ├────────┐    │          │               │
 │               │             │        │    │          │               │
 │               │             │<───────┘    │          │               │
 │               │             │             │          │               │
 │               │             │  Ack        │          │               │
 │               │             ├────────────────────────>│               │
 │               │             │             │          │               │
 │               │             │             │          │ IDLE          │
 │               │             │             │          ├──────────┐    │
 │               │             │             │          │          │    │
 │               │             │             │          │<─────────┘    │
 │               │             │             │          │               │
 │               │             │ More tasks? │          │               │
 │               │             ├────────┐    │          │               │
 │               │             │        │    │          │               │
 │               │             │<───────┘    │          │               │
 │               │             │             │          │               │
 │               │             │ [Loop continues for remaining tasks]   │
 │               │             │             │          │               │
 │               │             │ All done    │          │               │
 │               │             ├────────┐    │          │               │
 │               │             │        │    │          │               │
 │               │             │<───────┘    │          │               │
 │               │             │             │          │               │
 │               │             │ Summary     │          │               │
 │               │             ├────────┐    │          │               │
 │               │             │        │    │          │               │
 │               │             │<───────┘    │          │               │
 │               │             │             │          │               │
 │               │             │ Shutdown    │          │               │
 │               │             ├────────┐    │          │               │
 │               │             │        │    │          │               │
 │               │             │<───────┘    │          │               │
 │               │             │             │          │               │
 │               │ SIGTERM     │             │          │               │
 │               ├─────────────────────────────────────>│               │
 │               │             │             │          │               │
 │               │             │             │          │ Shutdown      │
 │               │             │             │          ├──────────┐    │
 │               │             │             │          │          │    │
 │               │             │             │          │<─────────┘    │
 │               │             │             │          │               │
 │               │             │             │          │ STOPPED       │
 │               │             │             │          ├──────────┐    │
 │               │             │             │          │          │    │
 │               │             │             │          │<─────────┘    │
 │               │             │             │          │               │
```

---

## 4. Component Interaction Diagram

```
┌──────────────────────────────────────────────────────────────────────────┐
│                              System Boundary                             │
│                                                                          │
│  ┌────────────────────────────────────────────────────────────────────┐ │
│  │                         Orchestrator                               │ │
│  │                                                                    │ │
│  │  ┌──────────────┐      ┌──────────────┐      ┌──────────────┐    │ │
│  │  │   Schedule   │      │  Scheduler   │      │     Task     │    │ │
│  │  │    Parser    │─────>│    Engine    │─────>│    Queue     │    │ │
│  │  └──────────────┘      └──────┬───────┘      └──────────────┘    │ │
│  │         ▲                      │                                  │ │
│  │         │                      │                                  │ │
│  │         │                      ▼                                  │ │
│  │  ┌──────┴──────┐      ┌──────────────┐      ┌──────────────┐    │ │
│  │  │    YAML     │      │     gRPC     │      │     gRPC     │    │ │
│  │  │    File     │      │    Server    │      │    Client    │    │ │
│  │  └─────────────┘      └──────────────┘      └──────┬───────┘    │ │
│  │                                                     │            │ │
│  └─────────────────────────────────────────────────────┼────────────┘ │
│                                                        │              │
│                                                        │ gRPC         │
│                                                        │ Protocol     │
│                                                        │              │
│  ┌─────────────────────────────────────────────────────┼────────────┐ │
│  │                          Tasks                      │            │ │
│  │                                                     │            │ │
│  │  ┌──────────────┐      ┌──────────────┐      ┌────▼────────┐   │ │
│  │  │     Task     │      │     Task     │      │     gRPC    │   │ │
│  │  │   Wrapper    │◄─────│    State     │◄─────│    Server   │   │ │
│  │  │              │      │   Machine    │      │             │   │ │
│  │  └──────┬───────┘      └──────────────┘      └─────────────┘   │ │
│  │         │                                                       │ │
│  │         │                                                       │ │
│  │         ▼                                                       │ │
│  │  ┌──────────────┐      ┌──────────────┐      ┌─────────────┐  │ │
│  │  │      RT      │      │   Execution  │      │    gRPC     │  │ │
│  │  │    Thread    │─────>│   Callback   │      │   Client    │  │ │
│  │  │              │      │   (Custom)   │      │             │  │ │
│  │  └──────────────┘      └──────────────┘      └──────┬──────┘  │ │
│  │                                                      │         │ │
│  └──────────────────────────────────────────────────────┼─────────┘ │
│                                                         │           │
│                                                         │ Notify    │
│                                                         │ TaskEnd   │
│                                                         │           │
└─────────────────────────────────────────────────────────┼───────────┘
                                                          │
                                                          ▼
                                                 ┌─────────────────┐
                                                 │  Orchestrator   │
                                                 │   gRPC Server   │
                                                 └─────────────────┘
```

---

## 5. Data Flow Diagram

```
                    ┌─────────────────────┐
                    │   Schedule YAML     │
                    │                     │
                    │  • Task definitions │
                    │  • Dependencies     │
                    │  • Timing           │
                    │  • Parameters       │
                    └──────────┬──────────┘
                               │
                               │ Load
                               ▼
                    ┌─────────────────────┐
                    │  Schedule Parser    │
                    │                     │
                    │  • Parse YAML       │
                    │  • Validate schema  │
                    │  • Build task list  │
                    └──────────┬──────────┘
                               │
                               │ Task definitions
                               ▼
                    ┌─────────────────────┐
                    │  Scheduler Engine   │
                    │                     │
                    │  • Resolve deps     │
                    │  • Check timing     │
                    │  • Select next task │
                    └──────────┬──────────┘
                               │
                               │ Task to execute
                               ▼
                    ┌─────────────────────┐
                    │   gRPC Client       │
                    │                     │
                    │  • Create stub      │
                    │  • Build request    │
                    │  • Send StartTask   │
                    └──────────┬──────────┘
                               │
                               │ StartTask request
                               ▼
                    ┌─────────────────────┐
                    │   Task Wrapper      │
                    │   (gRPC Server)     │
                    │                     │
                    │  • Receive request  │
                    │  • Validate state   │
                    │  • Create thread    │
                    └──────────┬──────────┘
                               │
                               │ Execute
                               ▼
                    ┌─────────────────────┐
                    │   RT Thread         │
                    │                     │
                    │  • Apply RT config  │
                    │  • Lock memory      │
                    │  • Set affinity     │
                    └──────────┬──────────┘
                               │
                               │ Call
                               ▼
                    ┌─────────────────────┐
                    │  Callback Function  │
                    │  (User-defined)     │
                    │                     │
                    │  • Business logic   │
                    │  • Computations     │
                    │  • I/O operations   │
                    └──────────┬──────────┘
                               │
                               │ Result
                               ▼
                    ┌─────────────────────┐
                    │   Task Wrapper      │
                    │                     │
                    │  • Collect result   │
                    │  • Update state     │
                    │  • Build notif      │
                    └──────────┬──────────┘
                               │
                               │ NotifyTaskEnd
                               ▼
                    ┌─────────────────────┐
                    │  Orchestrator       │
                    │  (gRPC Server)      │
                    │                     │
                    │  • Receive notif    │
                    │  • Update status    │
                    │  • Check completion │
                    └──────────┬──────────┘
                               │
                               │ Continue or finish
                               ▼
                    ┌─────────────────────┐
                    │  Execution Summary  │
                    │                     │
                    │  • Task results     │
                    │  • Timing data      │
                    │  • Success/failure  │
                    └─────────────────────┘
```

---

## Legend

```
┌─────────┐
│Component│  = System component or process
└─────────┘

    │
    │       = Data flow or control flow
    ▼

    ├────>  = Message or request

◄───┤       = Response

─────       = Bidirectional communication

┌────┐
│    │      = Internal operation
└────┘
```

---

## Key Interactions

### 1. Orchestrator → Task (Command)
- **Protocol**: gRPC (StartTask RPC)
- **Direction**: Orchestrator → Task
- **Data**: Task ID, parameters, timing, priority
- **Purpose**: Initiate task execution

### 2. Task → Orchestrator (Notification)
- **Protocol**: gRPC (NotifyTaskEnd RPC)
- **Direction**: Task → Orchestrator
- **Data**: Result, duration, error message
- **Purpose**: Report task completion

### 3. Task Wrapper → Callback (Execution)
- **Protocol**: Function call
- **Direction**: Wrapper → User code
- **Data**: Parameters map
- **Purpose**: Execute user-defined logic

### 4. YAML → Parser (Configuration)
- **Protocol**: File I/O
- **Direction**: File → Parser
- **Data**: Schedule definition
- **Purpose**: Define task execution plan

---

## Timing Characteristics

```
Event                          Time (typical)
─────────────────────────────────────────────
Container startup              1-2 seconds
RT configuration               200-300 ms
gRPC connection setup          10-50 ms
StartTask request/response     5-20 ms
Task execution (user logic)    Variable (100ms - seconds)
NotifyTaskEnd request/response 5-20 ms
State transition               <1 ms
Memory locking                 100-200 ms
```
