# 📊 Schema del Sistema - gRPC Orchestrator

## 🎯 Panoramica Generale

Il sistema è un **orchestratore di task distribuiti** che utilizza gRPC per la comunicazione tra componenti. Permette di eseguire task in modo coordinato con supporto per scheduling real-time, dipendenze tra task e parametrizzazione.

### Cosa fa questo sistema?

Immagina di avere diversi programmi (chiamati "task") che devono eseguire operazioni in un ordine specifico o a orari precisi. L'orchestrator è come un direttore d'orchestra che:

1. **Legge uno spartito** (file YAML) che dice quali task eseguire e quando
2. **Coordina i musicisti** (task) dicendo loro quando iniziare
3. **Ascolta quando finiscono** e passa i risultati al task successivo
4. **Garantisce precisione** usando tecniche real-time per evitare ritardi

### Perché è scritto in C++?

C++ è stato scelto per:
- **Performance** - Esecuzione velocissima
- **Controllo preciso** - Accesso diretto alle funzionalità del sistema operativo Linux
- **Real-time** - Possibilità di usare scheduling deterministico
- **gRPC nativo** - Librerie gRPC ottimizzate per C++

---

## 🏗️ Architettura del Sistema

```
┌─────────────────────────────────────────────────────────────────┐
│                        DOCKER NETWORK                            │
│                      (grpc_network)                              │
│                                                                   │
│  ┌──────────────────────────────────────────────────────────┐   │
│  │                    ORCHESTRATOR                           │   │
│  │                  (orchestrator:50050)                     │   │
│  │                                                            │   │
│  │  • Legge schedule YAML                                    │   │
│  │  • Gestisce esecuzione task                               │   │
│  │  • Scheduling real-time (FIFO/RR/DEADLINE)               │   │
│  │  • CPU affinity (core 0)                                  │   │
│  │  • Priorità RT: 80                                        │   │
│  └──────────────────────────────────────────────────────────┘   │
│           │                    │                    │             │
│           │ gRPC              │ gRPC              │ gRPC         │
│           ▼                    ▼                    ▼             │
│  ┌─────────────┐      ┌─────────────┐      ┌─────────────┐     │
│  │   TASK 1    │      │   TASK 2    │      │   TASK 3    │     │
│  │ task1:50051 │      │ task2:50052 │      │ task3:50053 │     │
│  │             │      │             │      │             │     │
│  │ • Wrapper   │      │ • Wrapper   │      │ • Wrapper   │     │
│  │ • RT: FIFO  │      │ • RT: FIFO  │      │ • RT: FIFO  │     │
│  │ • Priority  │      │ • Priority  │      │ • Priority  │     │
│  │ • CPU any   │      │ • CPU any   │      │ • CPU any   │     │
│  └─────────────┘      └─────────────┘      └─────────────┘     │
│                                                                   │
└─────────────────────────────────────────────────────────────────┘
```

---

## 🔄 Flusso di Esecuzione

### 1. **Inizializzazione**

```
┌─────────────┐
│   START     │
└──────┬──────┘
       │
       ▼
┌─────────────────────────────────────┐
│ 1. Docker Compose avvia containers  │
│    - orchestrator                   │
│    - task1, task2, task3            │
└──────┬──────────────────────────────┘
       │
       ▼
┌─────────────────────────────────────┐
│ 2. Orchestrator si avvia            │
│    - Configura RT scheduling        │
│    - Legge file YAML schedule       │
│    - Avvia gRPC server :50050       │
└──────┬──────────────────────────────┘
       │
       ▼
┌─────────────────────────────────────┐
│ 3. Task Wrappers si avviano         │
│    - Configurano RT scheduling      │
│    - Avviano gRPC server            │
│    - Si registrano (opzionale)      │
└──────┬──────────────────────────────┘
       │
       ▼
┌─────────────────────────────────────┐
│ 4. Sistema pronto                   │
│    - Orchestrator attende schedule  │
│    - Task attendono comandi         │
└─────────────────────────────────────┘
```

### 2. **Esecuzione Task**

```
ORCHESTRATOR                          TASK WRAPPER
     │                                      │
     │  1. Legge schedule YAML              │
     │     (task_1, task_2, task_3)         │
     │                                      │
     │  2. Determina ordine esecuzione      │
     │     - Sequential: subito             │
     │     - Timed: attende timestamp       │
     │     - Dipendenze: attende task       │
     │                                      │
     │  3. StartTask(request) ───────────>  │
     │     • task_id                        │
     │     • scheduled_time_us              │
     │     • deadline_us                    │
     │     • parameters                     │
     │     • rt_policy, rt_priority         │
     │     • cpu_affinity                   │
     │                                      │
     │                                      │  4. Configura RT
     │                                      │     - sched_setscheduler()
     │                                      │     - sched_setaffinity()
     │                                      │     - mlockall()
     │                                      │
     │                                      │  5. Esegue task
     │                                      │     - Elabora parametri
     │                                      │     - Esegue logica
     │                                      │     - Genera output
     │                                      │
     │  <───────────── StartTaskResponse    │
     │     • success                        │
     │     • actual_start_time_us           │
     │                                      │
     │                                      │  6. Task completo
     │                                      │
     │  <───────────── NotifyTaskEnd        │
     │     • task_id                        │
     │     • result (SUCCESS/FAILURE)       │
     │     • start_time_us                  │
     │     • end_time_us                    │
     │     • execution_duration_us          │
     │     • output_data (parametri)        │
     │                                      │
     │  7. TaskEndResponse ──────────────>  │
     │     • acknowledged                   │
     │                                      │
     │  8. Passa output_data a task         │
     │     dipendenti (se presenti)         │
     │                                      │
     │  9. Avvia prossimo task              │
     │                                      │
```

---

## 📋 Componenti Principali

### 1. **Orchestrator** (`orchestrator_main`)

**Responsabilità:**
- Legge e interpreta file YAML di schedule
- Gestisce la sequenza di esecuzione dei task
- Invia comandi StartTask ai task wrapper
- Riceve notifiche di completamento (NotifyTaskEnd)
- Gestisce dipendenze tra task
- Passa parametri output tra task dipendenti

**File principali:**
- `src/orchestrator.cpp` - Logica principale
- `src/schedule.cpp` - Parsing YAML
- `include/orchestrator.h` - Interfaccia

**Configurazione RT:**
- Policy: FIFO (configurabile)
- Priorità: 80 (alta)
- CPU Affinity: core 0
- Memory locking: abilitato

---

### 2. **Task Wrapper** (`task_main`)

**Responsabilità:**
- Espone servizio gRPC TaskService
- Riceve comandi StartTask dall'orchestrator
- Configura scheduling real-time per il task
- Esegue la logica del task
- Notifica l'orchestrator al completamento
- Gestisce parametri input/output

**File principali:**
- `src/task_wrapper.cpp` - Implementazione wrapper
- `include/task_wrapper.h` - Interfaccia

**Configurazione RT:**
- Policy: FIFO (configurabile per task)
- Priorità: configurabile (default 75)
- CPU Affinity: configurabile
- Memory locking: abilitato

---

### 3. **Real-Time Utils** (`rt_utils`)

**Responsabilità:**
- Configurazione scheduling real-time Linux
- Gestione CPU affinity
- Memory locking (mlockall)
- Utility per timing preciso

**File principali:**
- `src/rt_utils.cpp` - Implementazione
- `include/rt_utils.h` - Interfaccia

**Funzioni chiave:**
- `configure_realtime()` - Setup RT scheduling
- `set_cpu_affinity()` - Binding a CPU core
- `lock_memory()` - Previene page faults

---

### 4. **Schedule Parser**

**Responsabilità:**
- Parsing file YAML di configurazione
- Validazione schedule
- Gestione dipendenze tra task
- Configurazione parametri RT per task

**Formato YAML:**
```yaml
schedule:
  name: "Nome Schedule"
  description: "Descrizione"
  
  defaults:
    deadline_us: 5000000
    
  tasks:
    - id: task_1
      address: "task1:50051"
      mode: sequential          # sequential | timed | hybrid
      scheduled_time_us: 0
      deadline_us: 3000000
      rt_policy: "fifo"         # none | fifo | rr | deadline
      rt_priority: 30           # 1-99
      cpu_affinity: 0           # -1 = no affinity
      parameters:
        key: "value"
      depends_on: task_id       # Dipendenza (opzionale)
```

---

## 🔌 Protocollo gRPC

### Servizi Definiti

#### **OrchestratorService** (esposto dall'Orchestrator)

```protobuf
service OrchestratorService {
  // Notifica completamento task
  rpc NotifyTaskEnd(TaskEndNotification) returns (TaskEndResponse);
  
  // Health check
  rpc HealthCheck(HealthCheckRequest) returns (HealthCheckResponse);
}
```

#### **TaskService** (esposto dai Task Wrapper)

```protobuf
service TaskService {
  // Avvia esecuzione task
  rpc StartTask(StartTaskRequest) returns (StartTaskResponse);
  
  // Ferma task (graceful)
  rpc StopTask(StopTaskRequest) returns (StopTaskResponse);
  
  // Stato task
  rpc GetTaskStatus(TaskStatusRequest) returns (TaskStatusResponse);
}
```

### Messaggi Principali

**StartTaskRequest:**
- `task_id` - Identificatore univoco
- `scheduled_time_us` - Timestamp esecuzione
- `deadline_us` - Deadline task
- `parameters` - Mappa parametri input
- `rt_policy` - Policy RT ("fifo", "rr", "deadline", "none")
- `rt_priority` - Priorità RT (1-99)
- `cpu_affinity` - Core CPU (-1 = nessuna)

**TaskEndNotification:**
- `task_id` - Identificatore task
- `result` - Risultato (SUCCESS, FAILURE, TIMEOUT, CANCELLED)
- `start_time_us` - Timestamp inizio
- `end_time_us` - Timestamp fine
- `execution_duration_us` - Durata esecuzione
- `error_message` - Messaggio errore (se presente)
- `output_data` - Mappa parametri output

---

## ⚙️ Modalità di Esecuzione Task

### 1. **Sequential Mode**
```
Task eseguito immediatamente quando le dipendenze sono soddisfatte

Timeline:
  0ms ────> task_1 (sequential) ────> completa
                                         │
                                         ▼
                                      task_3 (sequential, depends_on: task_1)
```

### 2. **Timed Mode**
```
Task eseguito a un timestamp specifico

Timeline:
  0ms ──────────────> 2000ms ────> task_2 (timed) ────> completa
```

### 3. **Hybrid Mode**
```
Combina dipendenze e timing

Timeline:
  0ms ────> task_1 ────> completa (500ms)
                            │
                            ▼
                         attende fino a 2000ms
                            │
                            ▼
                         task_2 (hybrid, depends_on: task_1, scheduled_time: 2000ms)
```

---

## 🔗 Gestione Dipendenze

### Passaggio Parametri tra Task

```
TASK 1                    ORCHESTRATOR                    TASK 3
  │                             │                             │
  │  Esegue con input: "10"     │                             │
  │  Output: "50"               │                             │
  │                             │                             │
  │  NotifyTaskEnd ──────────>  │                             │
  │  output_data:               │                             │
  │    result: "50"             │                             │
  │                             │                             │
  │                             │  Legge depends_on: task_1   │
  │                             │  Combina parametri:         │
  │                             │    input1: "50" (da task_1) │
  │                             │    input2: "22" (da YAML)   │
  │                             │                             │
  │                             │  StartTask ──────────────>  │
  │                             │  parameters:                │
  │                             │    input1: "50"             │
  │                             │    input2: "22"             │
  │                             │                             │
  │                             │                             │  Esegue
  │                             │                             │  Output: "1100"
```

---

## 🚀 Real-Time Scheduling

### Configurazione RT

**Orchestrator:**
```bash
Policy: SCHED_FIFO
Priority: 80 (alta)
CPU Affinity: core 0
Memory: locked (mlockall)
```

**Task:**
```bash
Policy: SCHED_FIFO (configurabile per task)
Priority: 30-99 (configurabile)
CPU Affinity: configurabile
Memory: locked (mlockall)
```

### Perché Real-Time?

1. **Determinismo** - Esecuzione prevedibile
2. **Bassa latenza** - Riduce jitter
3. **Priorità garantita** - Task critici non vengono interrotti
4. **CPU isolation** - Affinity previene migrazione tra core
5. **No page faults** - Memory locking elimina swap

### Limiti di Sistema

Configurati in `/etc/security/limits.conf`:
```
rtprio: 99      # Max RT priority
rttime: -1      # Unlimited RT time
memlock: 2GB    # Max locked memory
```

---

## 📊 Stati del Task

```
┌─────────┐
│ UNKNOWN │ ──> Stato iniziale
└────┬────┘
     │
     ▼
┌─────────┐
│  IDLE   │ ──> Task wrapper attivo, nessun task in esecuzione
└────┬────┘
     │ StartTask ricevuto
     ▼
┌──────────┐
│ STARTING │ ──> Configurazione RT in corso
└────┬─────┘
     │
     ▼
┌─────────┐
│ RUNNING │ ──> Task in esecuzione
└────┬────┘
     │
     ├──> Successo ──> ┌───────────┐
     │                 │ COMPLETED │
     │                 └───────────┘
     │
     ├──> Errore ───> ┌────────┐
     │                │ FAILED │
     │                └────────┘
     │
     └──> StopTask ─> ┌─────────┐
                      │ STOPPED │
                      └─────────┘
```

---

## 🐳 Deployment Docker

### Struttura Container

```
docker-compose.yml
├── orchestrator (1 container)
│   ├── Image: Dockerfile.orchestrator
│   ├── Port: 50050
│   ├── Network: grpc_network
│   ├── Capabilities: SYS_NICE
│   └── RT limits: rtprio=99, memlock=2GB
│
├── task1 (1 container)
│   ├── Image: Dockerfile.task
│   ├── Port: 50051
│   ├── Network: grpc_network
│   └── RT limits: rtprio=99, memlock=2GB
│
├── task2 (1 container)
│   └── ... (simile a task1, porta 50052)
│
└── task3 (1 container)
    └── ... (simile a task1, porta 50053)
```

### Build Process

```
Dockerfile.orchestrator / Dockerfile.task
│
├── Base: Ubuntu 22.04
├── Install: build-essential, cmake, git
├── Install: gRPC, protobuf
├── Copy: source code
├── Generate: protobuf/gRPC code
├── Build: CMake + make
└── Entrypoint: orchestrator_main / task_main
```

---

## 📁 Struttura File System

```
grpc_orchestrator/
│
├── proto/
│   └── orchestrator.proto          # Definizioni gRPC
│
├── include/
│   ├── orchestrator.h              # Header orchestrator
│   ├── task_wrapper.h              # Header task wrapper
│   ├── schedule.h                  # Header schedule parser
│   └── rt_utils.h                  # Header RT utilities
│
├── src/
│   ├── orchestrator.cpp            # Implementazione orchestrator
│   ├── task_wrapper.cpp            # Implementazione task wrapper
│   ├── schedule.cpp                # Parser YAML
│   └── rt_utils.cpp                # Utilities RT
│
├── schedules/
│   ├── example_parametrized.yaml   # Schedule con parametri
│   ├── example_sequential.yaml     # Schedule sequenziale
│   ├── example_timed.yaml          # Schedule temporizzato
│   └── example_hybrid.yaml         # Schedule ibrido
│
├── CMakeLists.txt                  # Build configuration
├── docker-compose.yml              # Docker orchestration
├── Dockerfile.orchestrator         # Docker image orchestrator
├── Dockerfile.task                 # Docker image task
│
└── build/                          # Directory build (generata)
    └── bin/
        ├── orchestrator_main       # Eseguibile orchestrator
        └── task_main               # Eseguibile task wrapper
```

---

## 🔍 Esempio Completo di Esecuzione

### Schedule YAML
```yaml
schedule:
  name: "Example"
  tasks:
    - id: task_1
      address: "task1:50051"
      mode: sequential
      parameters:
        input: "10"
    
    - id: task_2
      address: "task2:50052"
      mode: timed
      scheduled_time_us: 2000000
      parameters:
        input: "5"
    
    - id: task_3
      address: "task3:50053"
      mode: sequential
      depends_on: task_1
      parameters:
        input2: "7"
```

### Timeline Esecuzione

```
T=0ms
  │
  ├─> Orchestrator legge schedule
  │   - 3 task da eseguire
  │   - task_1: sequential (subito)
  │   - task_2: timed (2000ms)
  │   - task_3: sequential (dipende da task_1)
  │
  ├─> StartTask(task_1)
  │   - Task 1 riceve comando
  │   - Configura RT: FIFO, priority 30
  │   - Esegue con input="10"
  │
T=500ms
  │
  ├─> Task 1 completa
  │   - NotifyTaskEnd(output="50")
  │   - Orchestrator riceve notifica
  │
  ├─> StartTask(task_3)
  │   - Task 3 riceve comando
  │   - Parametri: input1="50" (da task_1), input2="7"
  │   - Configura RT: FIFO, priority 30
  │   - Esegue
  │
T=1000ms
  │
  ├─> Task 3 completa
  │   - NotifyTaskEnd(output="350")
  │
T=2000ms
  │
  ├─> StartTask(task_2)
  │   - Task 2 riceve comando (timed)
  │   - Configura RT: FIFO, priority 30
  │   - Esegue con input="5"
  │
T=2500ms
  │
  ├─> Task 2 completa
  │   - NotifyTaskEnd(output="6")
  │
  └─> Schedule completato
      - Tutti i task eseguiti con successo
```

---

## 🛠️ Comandi Utili

### Docker

```bash
# Build
docker-compose build

# Avvia sistema
docker-compose up

# Avvia in background
docker-compose up -d

# Vedi log
docker-compose logs -f

# Vedi log orchestrator
docker-compose logs -f orchestrator

# Ferma sistema
docker-compose down

# Stato container
docker ps

# Entra in container
docker exec -it grpc_orchestrator /bin/bash
```

### Native (senza Docker)

```bash
# Compila
mkdir build && cd build
cmake ..
make -j$(nproc)

# Esegui orchestrator
sudo ./build/bin/orchestrator_main \
  --address 0.0.0.0:50050 \
  --schedule schedules/example_parametrized.yaml \
  --policy fifo \
  --priority 80 \
  --cpu-affinity 0 \
  --lock-memory

# Esegui task
sudo ./build/bin/task_main \
  --name task_1 \
  --address localhost:50051 \
  --orchestrator localhost:50050 \
  --policy fifo \
  --priority 75 \
  --cpu-affinity 1 \
  --lock-memory
```

---

## 📈 Metriche e Monitoring

### Log Output

**Orchestrator:**
```
[Orchestrator] Starting orchestrator on 0.0.0.0:50050
[Orchestrator] Schedule loaded: 3 tasks
[Orchestrator] Starting task task_1
[Orchestrator] Task task_1 started successfully
[Orchestrator] Task task_1 completed in 500ms
```

**Task:**
```
[Task task_1] Starting task wrapper on 0.0.0.0:50051
[Task task_1] Received StartTask request
[Task task_1] Configured RT: FIFO, priority 30, CPU 0
[Task task_1] Executing task with parameters: input=10
[Task task_1] Task completed, output: 50
[Task task_1] Notified orchestrator
```

### Health Check

```bash
# Verifica orchestrator
grpcurl -plaintext localhost:50050 orchestrator.OrchestratorService/HealthCheck

# Verifica task
grpcurl -plaintext localhost:50051 orchestrator.TaskService/GetTaskStatus
```

---

## 🎓 Concetti Chiave

### 1. **Orchestrazione**
L'orchestrator coordina l'esecuzione di task multipli secondo uno schedule definito, gestendo dipendenze e timing.

### 2. **gRPC**
Framework RPC (Remote Procedure Call) che permette comunicazione efficiente tra servizi distribuiti.

### 3. **Real-Time Scheduling**
Scheduling deterministico che garantisce esecuzione prevedibile e bassa latenza per task critici.

### 4. **CPU Affinity**
Binding di processi a core CPU specifici per ridurre context switch e migliorare performance.

### 5. **Memory Locking**
Blocco della memoria in RAM per prevenire page faults e garantire accesso deterministico.

### 6. **Parametrizzazione**
Passaggio di dati tra task tramite parametri input/output, permettendo pipeline di elaborazione.

---

## 🔐 Sicurezza e Permessi

### Capabilities Richieste

- **SYS_NICE** - Permette modifica scheduling policy e priorità
- **IPC_LOCK** - Permette memory locking (mlockall)

### Limiti RT

Configurati tramite `ulimits` in Docker:
```yaml
ulimits:
  rtprio: 99        # Max RT priority
  rttime: -1        # Unlimited RT CPU time
  memlock: 2048000000  # 2GB locked memory
```

### Security Options

```yaml
security_opt:
  - seccomp:unconfined  # Disabilita seccomp per RT
```

---

## 🚨 Troubleshooting

### Problemi Comuni

1. **Task non si connette all'orchestrator**
   - Verifica che orchestrator sia avviato
   - Controlla indirizzi e porte
   - Verifica network Docker

2. **Errore RT scheduling**
   - Verifica capabilities (SYS_NICE)
   - Controlla ulimits (rtprio, rttime)
   - Esegui con sudo (native)

3. **Memory locking fallisce**
   - Aumenta ulimit memlock
   - Verifica IPC_LOCK capability

4. **Task non parte**
   - Controlla log orchestrator
   - Verifica schedule YAML
   - Controlla dipendenze task

---

## 📚 Riferimenti

- **gRPC**: https://grpc.io/
- **Protocol Buffers**: https://protobuf.dev/
- **Linux RT Scheduling**: `man sched(7)`
- **Docker Compose**: https://docs.docker.com/compose/
- **CMake**: https://cmake.org/

---

## ✅ Checklist Deployment

- [ ] Docker e Docker Compose installati
- [ ] Repository clonato
- [ ] `docker-compose build` completato
- [ ] Schedule YAML configurato
- [ ] Porte 50050-50053 disponibili
- [ ] `docker-compose up` avviato
- [ ] Log mostrano task in esecuzione
- [ ] Task completano con successo

---

---

## 💻 Spiegazione del Codice C++ (per chi non conosce C++)

Questa sezione spiega il codice in modo semplice, anche se non hai mai programmato in C++.

### Cos'è C++?

C++ è un linguaggio di programmazione:
- **Compilato** - Il codice viene tradotto in linguaggio macchina prima dell'esecuzione
- **Tipizzato** - Ogni variabile ha un tipo specifico (numero, testo, ecc.)
- **Orientato agli oggetti** - Organizza il codice in "classi" (come modelli) e "oggetti" (istanze dei modelli)

### Struttura Base di un Programma C++

```cpp
#include <iostream>  // Importa librerie (come import in Python)

int main() {         // Funzione principale (punto di ingresso)
    std::cout << "Hello World" << std::endl;  // Stampa a schermo
    return 0;        // Ritorna 0 = successo
}
```

**Spiegazione:**
- `#include` - Importa codice da altre librerie
- `int main()` - Funzione che viene eseguita all'avvio
- `std::cout` - Oggetto per stampare su console
- `<<` - Operatore per inviare dati allo stream di output
- `return 0` - Termina il programma con codice di successo

---

### 📁 File Principali del Progetto

#### 1. **orchestrator_main.cpp** - Programma Principale Orchestrator

```cpp
int main(int argc, char** argv) {
    // argc = numero di argomenti da linea di comando
    // argv = array di stringhe con gli argomenti
    
    std::string listen_address = "0.0.0.0:50050";  // Stringa (testo)
    std::string schedule_file;                      // Stringa vuota
    RTConfig rt_config;                             // Oggetto di configurazione
    
    // Loop per leggere gli argomenti
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];  // Converte C-string a C++ string
        
        if (arg == "--address" && i + 1 < argc) {
            listen_address = argv[++i];  // ++i incrementa e poi usa
        }
    }
    
    // Crea oggetto Orchestrator
    Orchestrator orchestrator(listen_address);
    
    // Carica schedule da file YAML
    TaskSchedule schedule = ScheduleParser::parse_yaml(schedule_file);
    
    // Avvia orchestrator
    orchestrator.start();
    
    // Attende completamento
    orchestrator.wait_for_completion();
    
    return 0;  // Successo
}
```

**Concetti C++:**

- **`std::string`** - Tipo per testo (come `str` in Python)
- **`int argc, char** argv`** - Argomenti da linea di comando
- **`for (int i = 1; i < argc; i++)`** - Loop che itera sugli argomenti
- **`Orchestrator orchestrator(...)`** - Crea un oggetto chiamando il costruttore
- **`orchestrator.start()`** - Chiama un metodo dell'oggetto

---

#### 2. **task_main.cpp** - Programma Principale Task

```cpp
// Funzione che esegue la logica del task
TaskResult example_task_function(
    const std::map<std::string, std::string>& params,  // Input (mappa chiave-valore)
    std::map<std::string, std::string>& output)        // Output (passato per riferimento)
{
    // Legge parametro "input" dalla mappa
    auto input_it = params.find("input");
    if (input_it == params.end()) {
        return TASK_RESULT_FAILURE;  // Parametro non trovato
    }
    
    // Converte stringa a intero
    int input_value = std::stoi(input_it->second);
    
    // Esegue operazione (esempio: moltiplica per 5)
    int output_value = input_value * 5;
    
    // Salva risultato nella mappa output
    output["result"] = std::to_string(output_value);
    
    return TASK_RESULT_SUCCESS;  // Successo
}

int main(int argc, char** argv) {
    std::string task_id = "task_1";
    std::string listen_address = "0.0.0.0:50051";
    std::string orchestrator_address = "orchestrator:50050";
    
    // Crea task wrapper passando la funzione di esecuzione
    TaskWrapper task_wrapper(
        task_id,
        listen_address,
        orchestrator_address,
        example_task_function  // Passa la funzione come parametro
    );
    
    // Avvia il wrapper (inizia ad ascoltare comandi)
    task_wrapper.start();
    
    // Loop infinito finché non viene fermato
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        
        if (task_wrapper.get_state() == TASK_STATE_STOPPED) {
            break;  // Esce dal loop
        }
    }
    
    return 0;
}
```

**Concetti C++:**

- **`std::map<K, V>`** - Dizionario/mappa (come `dict` in Python)
- **`const`** - Indica che il parametro non può essere modificato
- **`&`** (riferimento) - Passa la variabile per riferimento (modifiche visibili fuori dalla funzione)
- **`auto`** - Tipo dedotto automaticamente dal compilatore
- **`->second`** - Accede al valore in una coppia chiave-valore
- **`std::stoi()`** - Converte string a int
- **`std::to_string()`** - Converte int a string
- **Lambda/Callback** - `example_task_function` è passata come parametro

---

#### 3. **orchestrator.cpp** - Implementazione Orchestrator

```cpp
class Orchestrator {
private:
    // Variabili membro (attributi della classe)
    std::string listen_address_;           // Indirizzo di ascolto
    TaskSchedule schedule_;                // Schedule caricato
    std::mutex mutex_;                     // Mutex per thread-safety
    std::atomic<bool> running_;            // Flag atomico (thread-safe)
    std::thread server_thread_;            // Thread per server gRPC
    
public:
    // Costruttore (viene chiamato quando crei un oggetto)
    Orchestrator(const std::string& listen_address)
        : listen_address_(listen_address)  // Lista di inizializzazione
        , running_(false)
        , next_task_index_(0)
    {
        // Corpo del costruttore
        service_ = std::make_unique<OrchestratorServiceImpl>(this);
    }
    
    // Distruttore (viene chiamato quando l'oggetto viene distrutto)
    ~Orchestrator() {
        stop();  // Ferma tutto prima di distruggere
    }
    
    // Metodo pubblico
    void start() {
        if (running_.exchange(true)) {  // Atomic exchange
            return;  // Già in esecuzione
        }
        
        // Crea un nuovo thread per il server gRPC
        server_thread_ = std::thread([this]() {
            // Lambda che cattura 'this' (puntatore all'oggetto corrente)
            run_server();
        });
    }
    
    void on_task_end(const TaskEndNotification& notification) {
        // Lock il mutex per accesso thread-safe
        std::lock_guard<std::mutex> lock(mutex_);
        
        // Salva risultato
        completed_tasks_[notification.task_id()] = notification;
        
        // Avvia prossimo task
        schedule_next_task();
    }
    
private:
    // Metodo privato (solo accessibile dentro la classe)
    void run_server() {
        // Implementazione server gRPC
    }
};
```

**Concetti C++:**

- **`class`** - Definisce una classe (template per oggetti)
- **`private:`** - Membri accessibili solo dentro la classe
- **`public:`** - Membri accessibili da fuori
- **`std::mutex`** - Mutex per sincronizzazione tra thread
- **`std::atomic<T>`** - Tipo atomico (operazioni thread-safe)
- **`std::thread`** - Thread (esecuzione parallela)
- **`std::lock_guard`** - RAII lock (blocca mutex automaticamente)
- **`[this]() { ... }`** - Lambda che cattura il puntatore all'oggetto
- **Costruttore/Distruttore** - Metodi speciali per inizializzazione/pulizia

---

#### 4. **task_wrapper.cpp** - Implementazione Task Wrapper

```cpp
void TaskWrapper::execute_task(const StartTaskRequest& request) {
    // Cambia stato a STARTING
    state_ = TASK_STATE_STARTING;
    
    // Configura real-time scheduling
    if (rt_config_.policy != RT_POLICY_NONE) {
        RTUtils::configure_realtime(rt_config_);
    }
    
    // Prepara parametri
    std::map<std::string, std::string> params;
    for (const auto& pair : request.parameters()) {
        params[pair.first] = pair.second;
    }
    params["task_id"] = task_id_;
    
    // Cambia stato a RUNNING
    state_ = TASK_STATE_RUNNING;
    start_time_us_ = get_current_time_us();
    
    // Esegue la callback (funzione passata dall'utente)
    std::map<std::string, std::string> output;
    TaskResult result = execution_callback_(params, output);
    
    // Salva tempo di fine
    end_time_us_ = get_current_time_us();
    
    // Cambia stato in base al risultato
    if (result == TASK_RESULT_SUCCESS) {
        state_ = TASK_STATE_COMPLETED;
    } else {
        state_ = TASK_STATE_FAILED;
    }
    
    // Notifica orchestrator
    notify_orchestrator(result, output);
}

void TaskWrapper::notify_orchestrator(
    TaskResult result,
    const std::map<std::string, std::string>& output)
{
    // Crea messaggio gRPC
    TaskEndNotification notification;
    notification.set_task_id(task_id_);
    notification.set_result(result);
    notification.set_start_time_us(start_time_us_);
    notification.set_end_time_us(end_time_us_);
    notification.set_execution_duration_us(end_time_us_ - start_time_us_);
    
    // Copia output nella mappa del messaggio
    for (const auto& pair : output) {
        (*notification.mutable_output_data())[pair.first] = pair.second;
    }
    
    // Chiama RPC
    TaskEndResponse response;
    grpc::ClientContext context;
    grpc::Status status = orchestrator_stub_->NotifyTaskEnd(
        &context, notification, &response);
    
    if (!status.ok()) {
        std::cerr << "Failed to notify orchestrator: " 
                  << status.error_message() << std::endl;
    }
}
```

**Concetti C++:**

- **`for (const auto& pair : container)`** - Range-based for loop (come `for x in list` in Python)
- **`->` (freccia)** - Accede a membro tramite puntatore
- **`.` (punto)** - Accede a membro tramite oggetto
- **`mutable_*()`** - Ottiene puntatore modificabile a campo protobuf
- **`&context`** - Passa indirizzo (puntatore) alla variabile
- **Callback** - `execution_callback_` è una funzione memorizzata come variabile

---

### 🔧 CMakeLists.txt - Sistema di Build

CMake è un tool che genera i file per compilare il progetto. Ecco cosa fa:

```cmake
cmake_minimum_required(VERSION 3.15)  # Versione minima CMake
project(grpc_orchestrator VERSION 1.0.0 LANGUAGES CXX)  # Nome progetto

# Imposta standard C++17
set(CMAKE_CXX_STANDARD 17)

# Trova librerie necessarie
find_package(Threads REQUIRED)    # Libreria per thread
find_package(Protobuf REQUIRED)   # Protocol Buffers
find_package(gRPC CONFIG REQUIRED) # gRPC

# Genera codice da file .proto
add_custom_command(
    OUTPUT orchestrator.pb.cc orchestrator.pb.h
    COMMAND protoc --cpp_out=. orchestrator.proto
    DEPENDS proto/orchestrator.proto
)

# Crea libreria
add_library(orchestrator_lib
    src/orchestrator.cpp
    src/task_wrapper.cpp
    src/schedule.cpp
    src/rt_utils.cpp
    orchestrator.pb.cc  # File generato
)

# Collega librerie
target_link_libraries(orchestrator_lib
    PUBLIC
        gRPC::grpc++        # Libreria gRPC
        protobuf::libprotobuf  # Libreria protobuf
        Threads::Threads    # Libreria thread
        yaml-cpp            # Libreria YAML
)

# Crea eseguibile orchestrator
add_executable(orchestrator_main
    examples/orchestrator_main.cpp
)

# Collega libreria all'eseguibile
target_link_libraries(orchestrator_main
    PRIVATE orchestrator_lib
)

# Crea eseguibile task
add_executable(task_main
    examples/task_main.cpp
)

target_link_libraries(task_main
    PRIVATE orchestrator_lib
)
```

**Processo di Build:**

1. **CMake** legge `CMakeLists.txt`
2. **Genera** file Makefile (istruzioni per compilare)
3. **Protoc** genera codice C++ da file `.proto`
4. **Compilatore** (g++) compila file `.cpp` in file oggetto `.o`
5. **Linker** collega file oggetto e librerie in eseguibili finali

**Comandi:**

```bash
mkdir build          # Crea directory build
cd build             # Entra nella directory
cmake ..             # Genera Makefile
make -j$(nproc)      # Compila in parallelo
```

---

### 🔌 Protocol Buffers e gRPC

#### Cos'è Protocol Buffers?

Protocol Buffers (protobuf) è un formato per serializzare dati strutturati:

```protobuf
// File .proto
message StartTaskRequest {
  string task_id = 1;
  int64 scheduled_time_us = 2;
  map<string, string> parameters = 4;
}
```

**Viene compilato in codice C++:**

```cpp
class StartTaskRequest {
public:
    std::string task_id() const;
    void set_task_id(const std::string& value);
    
    int64_t scheduled_time_us() const;
    void set_scheduled_time_us(int64_t value);
    
    const std::map<std::string, std::string>& parameters() const;
    std::map<std::string, std::string>* mutable_parameters();
};
```

#### Cos'è gRPC?

gRPC è un framework per chiamate RPC (Remote Procedure Call):

```protobuf
service TaskService {
  rpc StartTask(StartTaskRequest) returns (StartTaskResponse);
}
```

**Viene compilato in:**

```cpp
// Stub (client)
class TaskService::Stub {
public:
    grpc::Status StartTask(
        grpc::ClientContext* context,
        const StartTaskRequest& request,
        StartTaskResponse* response);
};

// Service (server)
class TaskService::Service {
public:
    virtual grpc::Status StartTask(
        grpc::ServerContext* context,
        const StartTaskRequest* request,
        StartTaskResponse* response) = 0;
};
```

**Uso:**

```cpp
// CLIENT
auto stub = TaskService::NewStub(channel);
StartTaskRequest request;
request.set_task_id("task_1");
StartTaskResponse response;
grpc::ClientContext context;
grpc::Status status = stub->StartTask(&context, request, &response);

// SERVER
class TaskServiceImpl : public TaskService::Service {
    grpc::Status StartTask(
        grpc::ServerContext* context,
        const StartTaskRequest* request,
        StartTaskResponse* response) override
    {
        // Implementazione
        response->set_success(true);
        return grpc::Status::OK;
    }
};
```

---

### 🧵 Thread e Sincronizzazione

#### Thread

Un thread è un'esecuzione parallela:

```cpp
#include <thread>

void worker_function() {
    std::cout << "Running in thread" << std::endl;
}

int main() {
    // Crea e avvia thread
    std::thread worker(worker_function);
    
    // Attende che il thread finisca
    worker.join();
    
    return 0;
}
```

#### Mutex (Mutual Exclusion)

Previene accesso concorrente a dati condivisi:

```cpp
#include <mutex>

std::mutex mtx;
int shared_counter = 0;

void increment() {
    std::lock_guard<std::mutex> lock(mtx);  // Blocca mutex
    shared_counter++;
    // Mutex viene sbloccato automaticamente quando lock esce dallo scope
}
```

#### Atomic

Operazioni atomiche (thread-safe senza mutex):

```cpp
#include <atomic>

std::atomic<bool> running(false);

void start() {
    if (running.exchange(true)) {  // Atomico: leggi vecchio valore e imposta true
        return;  // Già in esecuzione
    }
    // Avvia...
}
```

---

### 🕐 Real-Time Scheduling

#### Cos'è lo Scheduling Real-Time?

Linux normalmente usa scheduling "best-effort" (CFS - Completely Fair Scheduler):
- I processi condividono la CPU in modo equo
- Non ci sono garanzie sui tempi di esecuzione

Lo scheduling **real-time** garantisce:
- Esecuzione prevedibile
- Priorità fisse
- Bassa latenza

#### Policy Real-Time

```cpp
#include <sched.h>
#include <pthread.h>

void configure_realtime() {
    struct sched_param param;
    param.sched_priority = 80;  // Priorità (1-99, 99 = massima)
    
    // Imposta policy FIFO (First In First Out)
    if (sched_setscheduler(0, SCHED_FIFO, &param) == -1) {
        perror("sched_setscheduler failed");
    }
}
```

**Policy disponibili:**

- **SCHED_FIFO** - First In First Out, priorità fissa
- **SCHED_RR** - Round Robin, priorità fissa con time slice
- **SCHED_DEADLINE** - Deadline-based scheduling

#### CPU Affinity

Lega un processo a un core CPU specifico:

```cpp
#include <sched.h>

void set_cpu_affinity(int cpu_core) {
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);           // Azzera set
    CPU_SET(cpu_core, &cpuset);  // Imposta core
    
    // Applica affinity al thread corrente
    pthread_setaffinity_np(pthread_self(), sizeof(cpuset), &cpuset);
}
```

#### Memory Locking

Blocca la memoria in RAM (previene swap):

```cpp
#include <sys/mman.h>

void lock_memory() {
    // Blocca tutta la memoria (presente e futura)
    if (mlockall(MCL_CURRENT | MCL_FUTURE) == -1) {
        perror("mlockall failed");
    }
}
```

---

### 📦 Docker e Containerizzazione

#### Dockerfile

Un Dockerfile descrive come costruire un'immagine Docker:

```dockerfile
# Immagine base
FROM ubuntu:22.04

# Installa dipendenze
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    libgrpc++-dev \
    protobuf-compiler-grpc

# Copia codice sorgente
COPY . /app
WORKDIR /app

# Compila
RUN mkdir build && cd build && \
    cmake .. && \
    make -j$(nproc)

# Comando di avvio
CMD ["./build/bin/orchestrator_main"]
```

**Processo:**

1. **FROM** - Scarica immagine base Ubuntu
2. **RUN** - Esegue comandi (installa pacchetti, compila)
3. **COPY** - Copia file dall'host al container
4. **CMD** - Comando eseguito all'avvio del container

#### docker-compose.yml

Definisce e orchestra più container:

```yaml
services:
  orchestrator:
    build:
      context: .
      dockerfile: Dockerfile.orchestrator
    ports:
      - "50050:50050"  # Mappa porta host:container
    networks:
      - grpc_network
    cap_add:
      - SYS_NICE  # Capability per RT scheduling
    ulimits:
      rtprio: 99  # Limite priorità RT
      
  task1:
    build:
      context: .
      dockerfile: Dockerfile.task
    ports:
      - "50051:50051"
    networks:
      - grpc_network

networks:
  grpc_network:
    driver: bridge  # Network virtuale tra container
```

**Comandi:**

```bash
docker-compose build  # Costruisce immagini
docker-compose up     # Avvia tutti i container
docker-compose down   # Ferma e rimuove container
```

---

### 🔍 Debugging e Logging

#### Stampa su Console

```cpp
#include <iostream>

std::cout << "Messaggio normale" << std::endl;
std::cerr << "Messaggio di errore" << std::endl;

// Con formattazione
#include <iomanip>
std::cout << std::setw(10) << 123 << std::endl;  // "       123"
```

#### Logging con Timestamp

```cpp
#include <chrono>

int64_t get_timestamp_ms() {
    auto now = std::chrono::system_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch());
    return ms.count();
}

void log(const std::string& message) {
    std::cout << "[" << get_timestamp_ms() << " ms] " 
              << message << std::endl;
}
```

#### Gestione Errori

```cpp
// Try-catch per eccezioni
try {
    int value = std::stoi("not_a_number");
} catch (const std::exception& e) {
    std::cerr << "Error: " << e.what() << std::endl;
}

// Controllo errori syscall
if (sched_setscheduler(0, SCHED_FIFO, &param) == -1) {
    perror("sched_setscheduler");  // Stampa errore errno
    return -1;
}
```

---

### 📚 Librerie Utilizzate

#### 1. **gRPC** (`grpc++`)

Framework per RPC (Remote Procedure Call):

```cpp
#include <grpcpp/grpcpp.h>

// Crea channel (connessione)
auto channel = grpc::CreateChannel(
    "localhost:50050",
    grpc::InsecureChannelCredentials());

// Crea stub (client)
auto stub = TaskService::NewStub(channel);

// Chiama RPC
grpc::ClientContext context;
StartTaskRequest request;
StartTaskResponse response;
grpc::Status status = stub->StartTask(&context, request, &response);
```

#### 2. **Protocol Buffers** (`protobuf`)

Serializzazione dati:

```cpp
#include <google/protobuf/message.h>

StartTaskRequest request;
request.set_task_id("task_1");
request.set_scheduled_time_us(1000000);

// Serializza a stringa binaria
std::string serialized;
request.SerializeToString(&serialized);

// Deserializza
StartTaskRequest request2;
request2.ParseFromString(serialized);
```

#### 3. **yaml-cpp**

Parsing file YAML:

```cpp
#include <yaml-cpp/yaml.h>

YAML::Node config = YAML::LoadFile("config.yaml");

std::string name = config["name"].as<std::string>();
int priority = config["priority"].as<int>();

// Itera su lista
for (const auto& task : config["tasks"]) {
    std::string id = task["id"].as<std::string>();
}
```

#### 4. **Standard Library** (`std`)

```cpp
#include <string>      // std::string
#include <vector>      // std::vector<T>
#include <map>         // std::map<K, V>
#include <thread>      // std::thread
#include <mutex>       // std::mutex
#include <atomic>      // std::atomic<T>
#include <chrono>      // std::chrono (tempo)
#include <algorithm>   // std::sort, std::find, etc.
#include <memory>      // std::unique_ptr, std::shared_ptr
```

---

### 🎓 Glossario C++

| Termine | Significato | Esempio |
|---------|-------------|---------|
| **Classe** | Template per creare oggetti | `class Orchestrator { ... };` |
| **Oggetto** | Istanza di una classe | `Orchestrator orch("0.0.0.0:50050");` |
| **Metodo** | Funzione di una classe | `orch.start();` |
| **Costruttore** | Inizializza oggetto | `Orchestrator(...) { }` |
| **Distruttore** | Pulisce oggetto | `~Orchestrator() { }` |
| **Puntatore** | Indirizzo di memoria | `int* ptr = &value;` |
| **Riferimento** | Alias a variabile | `void func(int& ref)` |
| **Namespace** | Contenitore per nomi | `std::string`, `orchestrator::Task` |
| **Template** | Codice generico | `std::vector<int>`, `std::map<K,V>` |
| **Lambda** | Funzione anonima | `[](int x) { return x * 2; }` |
| **RAII** | Resource Acquisition Is Initialization | `std::lock_guard<std::mutex>` |
| **Smart Pointer** | Puntatore con gestione automatica | `std::unique_ptr<T>` |
| **Const** | Costante (non modificabile) | `const int x = 10;` |
| **Static** | Variabile/metodo di classe | `static int counter;` |
| **Virtual** | Metodo sovrascrivibile | `virtual void func() = 0;` |
| **Override** | Sovrascrive metodo virtuale | `void func() override { }` |

---

### 🚀 Compilazione Passo-Passo

#### 1. Preprocessing

Il preprocessore elabora direttive `#include`, `#define`:

```cpp
// Prima
#include <iostream>
#define MAX 100

// Dopo preprocessing
// ... tutto il contenuto di iostream ...
// MAX viene sostituito con 100
```

#### 2. Compilazione

Il compilatore traduce C++ in assembly:

```cpp
int add(int a, int b) {
    return a + b;
}

// Diventa assembly (x86-64)
add:
    mov eax, edi
    add eax, esi
    ret
```

#### 3. Assemblaggio

L'assembler traduce assembly in codice macchina (binario):

```
48 89 f8    mov rax, rdi
48 01 f0    add rax, rsi
c3          ret
```

#### 4. Linking

Il linker collega file oggetto e librerie:

```
orchestrator.o + task_wrapper.o + libgrpc++.a → orchestrator_main
```

**Risultato:** Eseguibile binario pronto per l'esecuzione.

---

**Fine dello schema dettagliato** 🎉
