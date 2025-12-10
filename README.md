# gRPC Orchestrator

Un sistema di orchestrazione task real-time basato su gRPC per C++.

## Descrizione

Questo progetto implementa un orchestrator che comunica con task tramite gRPC. L'orchestrator legge uno schedule temporale dei task e sa quando devono essere mandati in esecuzione. Per triggerare l'esecuzione di un task si usa un wrapper del task che fornisce il comando `.start()`, mentre quando un task termina l'orchestrator riceve un comando `.end()`.

## Architettura

### Componenti Principali

1. **Orchestrator**: 
   - Legge lo schedule temporale dei task
   - Invia comandi `.start()` ai task al momento schedulato
   - Riceve notifiche `.end()` quando i task completano
   - Traccia lo stato di esecuzione di tutti i task

2. **Task Wrapper**:
   - Espone un servizio gRPC per ricevere comandi dall'orchestrator
   - Esegue il task quando riceve il comando `.start()`
   - Notifica l'orchestrator con `.end()` al completamento

3. **Schedule**:
   - Definisce quando ogni task deve essere eseguito
   - Include parametri, deadline e priorità per ogni task

### Protocollo gRPC

Il protocollo è definito in `proto/orchestrator.proto` e include:

- **OrchestratorService**: Servizio esposto dall'orchestrator
  - `NotifyTaskEnd`: Riceve notifiche di completamento dai task
  - `HealthCheck`: Verifica lo stato dell'orchestrator

- **TaskService**: Servizio esposto da ogni task
  - `StartTask`: Avvia l'esecuzione del task
  - `StopTask`: Ferma il task (graceful shutdown)
  - `GetTaskStatus`: Ottiene lo stato corrente del task

## Deployment Options

Questo progetto supporta due modalità di deployment:

1. **🐳 Docker (Consigliato)**: Ogni componente gira in un container separato
2. **💻 Native**: Compilazione ed esecuzione diretta sul sistema host

## 🐳 Quick Start con Docker

```bash
# Build delle immagini Docker
./docker-build.sh

# Avvia tutti i container
./docker-run.sh
```

Vedi [DOCKER.md](DOCKER.md) per la guida completa Docker.

## 💻 Requisiti per Compilazione Native

- CMake >= 3.15
- C++17 compiler (GCC 7+, Clang 5+, MSVC 2017+)
- gRPC
- Protocol Buffers
- Threads (pthread)

### Installazione Dipendenze (Ubuntu/Debian)

```bash
# Installa build tools
sudo apt-get update
sudo apt-get install -y build-essential cmake

# Installa gRPC e protobuf
sudo apt-get install -y libgrpc++-dev libprotobuf-dev protobuf-compiler-grpc

# Oppure compila da sorgente (versione più recente)
git clone --recurse-submodules -b v1.58.0 https://github.com/grpc/grpc
cd grpc
mkdir -p cmake/build
cd cmake/build
cmake -DgRPC_INSTALL=ON -DgRPC_BUILD_TESTS=OFF ../..
make -j$(nproc)
sudo make install
```

## Compilazione

```bash
# Crea directory di build
mkdir build
cd build

# Configura con CMake
cmake ..

# Compila
make -j$(nproc)

# I binari saranno in build/bin/
```

## Utilizzo

### 1. Avviare l'Orchestrator

```bash
# Usa lo schedule di test predefinito
./build/bin/orchestrator_main

# Oppure specifica un indirizzo e un file di schedule
./build/bin/orchestrator_main 0.0.0.0:50050 /path/to/schedule.yaml
```

### 2. Avviare i Task

In terminali separati, avvia ogni task:

```bash
# Task 1
./build/bin/task_main task_1 localhost:50051 localhost:50050

# Task 2
./build/bin/task_main task_2 localhost:50052 localhost:50050

# Task 3
./build/bin/task_main task_3 localhost:50053 localhost:50050
```

### 3. Osservare l'Esecuzione

L'orchestrator invierà automaticamente i comandi `.start()` ai task secondo lo schedule. Quando ogni task completa, invierà una notifica `.end()` all'orchestrator.

## Esempio di Output

### Orchestrator
```
=== gRPC Orchestrator ===
Starting orchestrator service...
[Main] Using test schedule
[Orchestrator] Loaded schedule with 3 tasks
[Orchestrator] Starting orchestrator on 0.0.0.0:50050
[Orchestrator] gRPC server listening on 0.0.0.0:50050
[Orchestrator] Scheduler started
[Orchestrator] Scheduling task: task_1 at time 1000000 us
[Orchestrator] Task task_1 started successfully
[Orchestrator] Received task end notification: task_1
[Orchestrator] Task task_1 completed with result: 1 (duration: 500123 us)
...
```

### Task
```
=== gRPC Task Wrapper ===
Task ID: task_1
Listen Address: localhost:50051
Orchestrator Address: localhost:50050
[Task task_1] Task wrapper created
[Task task_1] Starting task wrapper on localhost:50051
[Task task_1] gRPC server listening on localhost:50051
[Main] Task wrapper started, waiting for commands...
[Task task_1] Received start command
[Task task_1] Starting task execution
[Task Function] Starting execution with parameters:
  mode = fast
  iterations = 100
[Task Function] Simulating work for 500 ms...
[Task Function] Work completed successfully
[Task task_1] Task execution completed successfully
[Task task_1] Notifying orchestrator of task end
[Task task_1] Orchestrator acknowledged task end
```

## Struttura del Progetto

```
grpc_orchestrator/
├── CMakeLists.txt           # Configurazione build
├── README.md                # Questo file
├── proto/
│   └── orchestrator.proto   # Definizione protocollo gRPC
├── include/
│   ├── orchestrator.h       # Header orchestrator
│   ├── task_wrapper.h       # Header task wrapper
│   └── schedule.h           # Header schedule
├── src/
│   ├── orchestrator.cpp     # Implementazione orchestrator
│   ├── task_wrapper.cpp     # Implementazione task wrapper
│   └── schedule.cpp         # Implementazione schedule parser
└── examples/
    ├── orchestrator_main.cpp # Esempio orchestrator
    └── task_main.cpp         # Esempio task
```

## Personalizzazione

### Creare un Task Personalizzato

```cpp
#include "task_wrapper.h"

// Definisci la funzione di esecuzione del task
TaskResult my_custom_task(const std::map<std::string, std::string>& params) {
    // Implementa la logica del task
    std::cout << "Executing custom task..." << std::endl;
    
    // Fai qualcosa con i parametri
    auto it = params.find("my_param");
    if (it != params.end()) {
        std::cout << "Parameter value: " << it->second << std::endl;
    }
    
    // Ritorna il risultato
    return TASK_RESULT_SUCCESS;
}

int main() {
    TaskWrapper wrapper(
        "my_task",
        "localhost:50051",
        "localhost:50050",
        my_custom_task
    );
    
    wrapper.start();
    // ... mantieni in esecuzione
}
```

### Creare uno Schedule Personalizzato

```cpp
#include "schedule.h"

TaskSchedule create_custom_schedule() {
    TaskSchedule schedule;
    
    schedule.time_horizon_start_us = 0;
    schedule.time_horizon_end_us = 10000000;  // 10 secondi
    schedule.tick_duration_us = 1000;
    
    ScheduledTask task;
    task.task_id = "my_task";
    task.task_address = "localhost:50051";
    task.scheduled_time_us = 1000000;  // 1 secondo
    task.deadline_us = 2000000;
    task.priority = 10;
    task.parameters["my_param"] = "my_value";
    
    schedule.tasks.push_back(task);
    
    return schedule;
}
```

## Caratteristiche

- ✅ Comunicazione asincrona tramite gRPC
- ✅ Scheduling temporale preciso (microsecondo)
- ✅ Tracciamento dello stato dei task
- ✅ Gestione errori e timeout
- ✅ Supporto parametri task
- ✅ Notifiche di completamento
- ✅ Health check
- ✅ Graceful shutdown
- ✅ Thread-safe

## Sviluppi Futuri

- [ ] Parser YAML completo per schedule
- [ ] Supporto retry automatico
- [ ] Monitoring risorse (CPU, memoria)
- [ ] Dashboard web per visualizzazione
- [ ] Supporto task periodici
- [ ] Load balancing tra task
- [ ] Persistenza stato
- [ ] Autenticazione gRPC

## Licenza

Questo progetto è parte del sistema SpaceRTM.

## Autore

Creato per il progetto SpaceRTM - Real-Time Mission Control
