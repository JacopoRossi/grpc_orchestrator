# Guida Utente - gRPC Orchestrator

Guida completa all'utilizzo del sistema di orchestrazione gRPC per utenti finali.

## 📋 Indice

- [Introduzione](#introduzione)
- [Concetti Fondamentali](#concetti-fondamentali)
- [Installazione e Setup](#installazione-e-setup)
- [Configurazione dello Schedule](#configurazione-dello-schedule)
- [Modalità di Esecuzione](#modalità-di-esecuzione)
- [Creazione di Task Personalizzati](#creazione-di-task-personalizzati)
- [Deployment con Docker](#deployment-con-docker)
- [Build e Distribuzione Immagini](#build-e-distribuzione-immagini)
- [Configurazione Real-Time](#configurazione-real-time)
- [Monitoring e Debugging](#monitoring-e-debugging)
- [Best Practices](#best-practices)
- [Esempi Pratici](#esempi-pratici)

## Introduzione

Il gRPC Orchestrator è un sistema per coordinare l'esecuzione di task distribuiti su più processi o container. È particolarmente adatto per:

- **Pipeline di elaborazione dati** con dipendenze complesse
- **Sistemi di controllo** che richiedono timing deterministico
- **Applicazioni real-time** con requisiti di latenza stringenti
- **Workflow distribuiti** con task eterogenei

### Vantaggi Chiave

✅ **Scheduling Preciso**: Esegui task a tempi esatti (precisione microsecondo)  
✅ **Gestione Dipendenze**: I task possono attendere il completamento di altri task  
✅ **Parametrizzazione**: Passa dati tra task in formato JSON  
✅ **Real-Time Support**: Supporto per policy real-time Linux  
✅ **Containerizzazione**: Deploy facile con Docker  

## Concetti Fondamentali

### Architettura del Sistema

```
┌────────────────┐
│  Schedule YAML │  ← Definisce cosa eseguire e quando
└───────┬────────┘
        │
        ▼
┌───────────────┐
│ Orchestrator  │  ← Coordina tutto
└───────┬───────┘
        │ gRPC
    ┌───┴───┐
    ▼       ▼
┌──────┐ ┌──────┐
│Task 1│ │Task 2│  ← Eseguono la logica
└──────┘ └──────┘
```

### Componenti

1. **Schedule**: File YAML che definisce quando e come eseguire i task
2. **Orchestrator**: Processo centrale che legge lo schedule e coordina i task
3. **Task Runner**: Wrapper che esegue la logica dei task
4. **Task Function**: La tua funzione C++ che implementa la logica

### Modalità di Esecuzione

#### 🕐 Timed Mode
Il task viene eseguito a un tempo assoluto definito:
```yaml
mode: timed
scheduled_time_us: 5000000  # Esegui dopo 5 secondi dall'avvio
```

#### 🔗 Sequential Mode  
Il task attende che un altro task sia completato:
```yaml
mode: sequential
depends_on: task_1  # Attendi che task_1 finisca
```

### Flusso di Dati

I task possono passarsi dati:
```
Task 1 → [output_json] → Task 2 (riceve in params["dep_output"])
```

## Installazione e Setup

### Requisiti di Sistema

```bash
# Ubuntu/Debian
sudo apt update
sudo apt install -y \
    build-essential \
    cmake \
    libgrpc++-dev \
    libprotobuf-dev \
    protobuf-compiler-grpc \
    libyaml-cpp-dev \
    nlohmann-json3-dev

# Verifica versioni
cmake --version    # Richiesto: >= 3.15
g++ --version      # Richiesto: >= 8.0 (C++17)
```

### Compilazione

```bash
# Clone e setup
git clone <repository-url>
cd grpc_orchestrator

# Build completo
mkdir -p build && cd build
cmake ..
make -j$(nproc)

# I binari saranno in build/bin/
ls -l bin/
```

### Verifica Installazione

```bash
# Test orchestrator
./bin/orchestrator_main --help

# Test task runner
./bin/task_runner --help

# Test con schedule di esempio
./bin/orchestrator_main --schedule ../schedules/example_parametrized.yaml
```

## Configurazione dello Schedule

Lo schedule è un file YAML che definisce l'intera pipeline.

### Struttura Base

```yaml
schedule:
  name: "Nome Pipeline"
  description: "Descrizione"
  
  defaults:
    deadline_us: 10000000  # Default per tutti i task
    
  tasks:
    - id: task_id
      address: "host:port"
      mode: timed
      # ... altri parametri
```

### Parametri dei Task

#### Parametri Obbligatori

```yaml
- id: task_1                    # ID univoco del task
  address: "task1:50051"        # Indirizzo gRPC del task
  mode: timed                   # o 'sequential'
```

#### Parametri di Timing

```yaml
  scheduled_time_us: 1000000    # Quando iniziare (microsecondi)
  deadline_us: 5000000          # Deadline massima
  estimated_duration_us: 3000000 # Stima durata (per logging)
```

#### Parametri Applicativi

```yaml
  parameters:                   # Parametri custom per il task
    input_size: 1000
    algorithm: "fast"
    threshold: 0.5
```

#### Dipendenze

```yaml
  mode: sequential
  depends_on: task_1           # Attendi task_1
```

#### Configurazione Real-Time

```yaml
  rt_policy: "fifo"            # none, fifo, rr, deadline
  rt_priority: 80              # 1-99 (99 = massima priorità)
  cpu_affinity: 2              # CPU core da usare (-1 = nessun binding)
```

### Esempio Completo

```yaml
schedule:
  name: "Data Processing Pipeline"
  description: "Analizza dati, elabora immagini, genera report"
  
  defaults:
    deadline_us: 30000000
    rt_policy: "fifo"
    rt_priority: 30
    
  tasks:
    # Task 1: Data Analyzer
    - id: data_analyzer
      address: "task1:50051"
      mode: timed
      scheduled_time_us: 1000000      # Inizia dopo 1 secondo
      deadline_us: 10000000
      estimated_duration_us: 4000000
      cpu_affinity: 0
      parameters:
        data_size: 500000
        statistics: ["mean", "stddev", "min", "max"]
    
    # Task 2: Image Processor (parallelo a task 1)
    - id: image_processor
      address: "task2:50052"
      mode: timed
      scheduled_time_us: 2000000      # Inizia dopo 2 secondi
      deadline_us: 8000000
      cpu_affinity: 1
      parameters:
        width: 800
        height: 600
        filters: ["gaussian", "sharpen"]
    
    # Task 3: Report Generator (dipende da task 1)
    - id: report_generator
      address: "task3:50053"
      mode: sequential
      depends_on: data_analyzer
      deadline_us: 20000000
      cpu_affinity: 2
      parameters:
        format: "pdf"
        charts: ["histogram", "scatter"]
```

### Conversione Unità di Tempo

```
1 secondo       = 1.000.000 microsecondi
1 millisecondo  = 1.000 microsecondi
100 millisecondi = 100.000 microsecondi
```

Esempi:
```yaml
scheduled_time_us: 1000000    # 1 secondo
scheduled_time_us: 500000     # 500 ms
scheduled_time_us: 10000      # 10 ms
```

## Modalità di Esecuzione

### Esecuzione Manuale (Sviluppo)

Utile per debug e testing.

**Terminale 1 - Avvia Orchestrator:**
```bash
cd build/bin

./orchestrator_main \
    --address 0.0.0.0:50050 \
    --schedule ../../schedules/my_schedule.yaml \
    --policy fifo \
    --priority 80
```

**Terminale 2 - Avvia Task 1:**
```bash
./task_runner \
    --name task_1 \
    --address 0.0.0.0:50051 \
    --orchestrator localhost:50050
```

**Terminale 3 - Avvia Task 2:**
```bash
./task_runner \
    --name task_2 \
    --address 0.0.0.0:50052 \
    --orchestrator localhost:50050
```

### Esecuzione con Docker Compose (Produzione)

**1. Configura docker-compose.yml**

Il file è già configurato. Modifica se necessario:
```yaml
orchestrator:
  command: >
    ./orchestrator_main
    --address 0.0.0.0:50050
    --schedule schedules/my_schedule.yaml  # ← Cambia qui
    --policy fifo
    --priority 80
```

**2. Avvia tutti i servizi:**
```bash
# Avvia tutto
docker-compose --profile all up

# Oppure in background
docker-compose --profile all up -d

# Vedi log
docker-compose logs -f
```

**3. Controlla stato:**
```bash
docker-compose ps
```

**4. Stop:**
```bash
docker-compose down
```

### Opzioni Orchestrator

```bash
./orchestrator_main [OPZIONI]

Opzioni:
  --address <addr>        Indirizzo di ascolto (default: 0.0.0.0:50050)
  --schedule <file>       File YAML dello schedule
  --policy <policy>       Policy RT: none, fifo, rr (default: none)
  --priority <n>          Priorità RT: 1-99 (default: 50)
  --cpu-affinity <n>      CPU core (-1 = nessun binding)
  --lock-memory           Blocca memoria in RAM (no page faults)
  --help                  Mostra aiuto
```

### Opzioni Task Runner

```bash
./task_runner [OPZIONI]

Opzioni:
  --name <name>           Nome task (deve corrispondere al registro)
  --address <addr>        Indirizzo di ascolto
  --orchestrator <addr>   Indirizzo orchestrator
  --policy <policy>       Policy RT (opzionale)
  --priority <n>          Priorità RT (opzionale)
  --cpu-affinity <n>      CPU core (opzionale)
  --lock-memory           Blocca memoria (opzionale)
  --help                  Mostra aiuto
```

## Creazione di Task Personalizzati

### Guida Rapida

**1. Apri `tasks/my_tasks.h`**

**2. Aggiungi la tua funzione:**

```cpp
#include <nlohmann/json.hpp>
#include "orchestrator.pb.h"

using json = nlohmann::json;
using orchestrator::TaskResult;

// La tua nuova funzione task
inline TaskResult my_analysis_task(
    const std::string& params_json, 
    std::string& output_json) 
{
    try {
        // 1. Parse parametri in input
        json params = json::parse(params_json);
        
        // Leggi parametri (con default)
        int data_size = params.value("data_size", 1000);
        std::string mode = params.value("mode", "fast");
        
        // 2. La tua logica
        std::cout << "[MyTask] Processing " << data_size 
                  << " items in " << mode << " mode" << std::endl;
        
        // Simula elaborazione
        std::vector<double> results;
        for (int i = 0; i < data_size; i++) {
            results.push_back(i * 1.5);
        }
        
        // Calcola statistiche
        double sum = 0;
        for (auto v : results) sum += v;
        double mean = sum / results.size();
        
        // 3. Prepara output (sarà passato a task dipendenti)
        json output;
        output["processed_items"] = data_size;
        output["mean"] = mean;
        output["status"] = "completed";
        output_json = output.dump();
        
        // 4. Ritorna successo
        return orchestrator::TASK_RESULT_SUCCESS;
        
    } catch (const std::exception& e) {
        std::cerr << "[MyTask] Error: " << e.what() << std::endl;
        return orchestrator::TASK_RESULT_FAILURE;
    }
}
```

**3. Registra in `tasks/task_runner.cpp`:**

Trova il blocco di registrazione (circa linea 90) e aggiungi:

```cpp
// Registra i task disponibili
std::unordered_map<std::string, TaskFunction> task_registry;
task_registry["task_1"] = data_analyzer_task;
task_registry["task_2"] = image_processor_task;
task_registry["task_3"] = report_generator_task;
task_registry["my_analysis"] = my_analysis_task;  // ← Aggiungi qui
```

**4. Ricompila:**

```bash
cd build
make task_runner
```

**5. Usa nello schedule:**

```yaml
tasks:
  - id: analysis
    address: "task1:50051"
    mode: timed
    scheduled_time_us: 1000000
    parameters:
      data_size: 50000
      mode: "accurate"
```

**6. Avvia task runner con il nome corretto:**

```bash
./task_runner \
    --name my_analysis \
    --address 0.0.0.0:50051 \
    --orchestrator localhost:50050
```

### Task con Dipendenze

Se il tuo task dipende da altri, l'output del task precedente arriva in `params["dep_output"]`:

```cpp
inline TaskResult dependent_task(
    const std::string& params_json,
    std::string& output_json)
{
    json params = json::parse(params_json);
    
    // Ricevi output dal task precedente
    if (params.contains("dep_output")) {
        json dep_output = params["dep_output"];
        
        // Usa i dati del task precedente
        double mean = dep_output.value("mean", 0.0);
        int items = dep_output.value("processed_items", 0);
        
        std::cout << "Previous task processed " << items 
                  << " items with mean: " << mean << std::endl;
    }
    
    // ... tua logica ...
    
    return orchestrator::TASK_RESULT_SUCCESS;
}
```

Schedule corrispondente:
```yaml
tasks:
  - id: first_task
    # ...
  
  - id: dependent_task
    mode: sequential
    depends_on: first_task  # Riceverà l'output di first_task
```

### Template Task Completo

```cpp
inline TaskResult template_task(
    const std::string& params_json,
    std::string& output_json)
{
    auto start_time = std::chrono::steady_clock::now();
    
    try {
        // Parse input
        json params = json::parse(params_json);
        
        // Log inizio
        std::cout << "[Template] Starting with params: " 
                  << params.dump(2) << std::endl;
        
        // Leggi parametri
        int param1 = params.value("param1", 100);
        std::string param2 = params.value("param2", "default");
        
        // Check dipendenze
        if (params.contains("dep_output")) {
            json dep = params["dep_output"];
            std::cout << "[Template] Received dependency output" << std::endl;
        }
        
        // ==================
        // TUA LOGICA QUI
        // ==================
        
        // Simula lavoro
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        // Calcola tempo esecuzione
        auto end_time = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
            end_time - start_time).count();
        
        // Prepara output
        json output;
        output["execution_time_us"] = duration;
        output["result_value"] = param1 * 2;
        output["status"] = "success";
        output_json = output.dump();
        
        std::cout << "[Template] Completed in " << duration << " us" << std::endl;
        
        return orchestrator::TASK_RESULT_SUCCESS;
        
    } catch (const json::exception& e) {
        std::cerr << "[Template] JSON error: " << e.what() << std::endl;
        return orchestrator::TASK_RESULT_FAILURE;
    } catch (const std::exception& e) {
        std::cerr << "[Template] Error: " << e.what() << std::endl;
        return orchestrator::TASK_RESULT_FAILURE;
    }
}
```

## Deployment con Docker

### Deploy Manager

Il Deploy Manager automatizza il deployment su Docker.

**1. Configura `deploy/deployment_config.yaml`:**

```yaml
deployment:
  name: "Production Deployment"
  description: "Deploy orchestrator and tasks"
  
  network:
    name: grpc_network
    driver: bridge
  
  # Configurazione GitLab (opzionale)
  gitlab:
    enabled: true
    registry_url: "registry.gitlab.com"
    project_path: "username/grpc_orchestrator"
    use_ci_token: false
    # username e token si leggono da variabili d'ambiente
  
  orchestrator:
    container_name: grpc_orchestrator
    hostname: orchestrator
    image: "grpc_orchestrator:latest"
    dockerfile: "Dockerfile.orchestrator"
    address: "0.0.0.0:50050"
    schedule_file: "schedules/example_parametrized.yaml"
    
    # RT config
    rt_policy: "fifo"
    rt_priority: 80
    rt_cpu_affinity: 0
    lock_memory: true
    
    # Resources
    cpuset: "0"
    memlock: 2048000000
    
    capabilities:
      - SYS_NICE
  
  tasks:
    - id: task_1
      container_name: grpc_task1
      hostname: task1
      image: "grpc_task:latest"
      dockerfile: "Dockerfile.task"
      address: "0.0.0.0:50051"
      task_type: "task_1"
      
      rt_policy: "fifo"
      rt_priority: 30
      rt_cpu_affinity: 1
      cpuset: "1"
      memlock: 2048000000
      
      capabilities:
        - SYS_NICE
    
    # ... altri task ...
  
  strategy:
    type: "all"
    wait_for_healthy: true
    startup_timeout: 30
```

**2. Usa Deploy Manager:**

```bash
# Build del deploy manager
cd build
make deploy_manager_main

# Deploy completo
./bin/deploy_manager_main \
    --config ../deploy/deployment_config.yaml \
    --action deploy

# Deploy solo task (orchestrator manuale)
./bin/deploy_manager_main \
    --config ../deploy/deployment_config.yaml \
    --action tasks-only

# Mostra stato
./bin/deploy_manager_main \
    --config ../deploy/deployment_config.yaml \
    --action status

# Segui log
./bin/deploy_manager_main \
    --config ../deploy/deployment_config.yaml \
    --action logs

# Cleanup
./bin/deploy_manager_main \
    --config ../deploy/deployment_config.yaml \
    --action cleanup
```

### Variabili d'Ambiente per GitLab

Se usi GitLab Container Registry:

```bash
# Imposta credenziali
export GITLAB_USERNAME="your_username"
export GITLAB_TOKEN="your_personal_access_token"

# Oppure usa CI token (in GitLab CI/CD)
export CI_JOB_TOKEN="$CI_JOB_TOKEN"
```

## Build e Distribuzione Immagini

### Builder Manager

Il Builder Manager automatizza build e push delle immagini Docker.

**1. Configura `deploy/builder_config.yaml`:**

```yaml
builder:
  name: "Docker Image Builder"
  description: "Build and push images to GitLab"
  
  gitlab:
    registry_url: "registry.gitlab.com"
    project_path: "username/grpc_orchestrator"
    use_ci_token: false
  
  images:
    - dockerfile: "Dockerfile.orchestrator"
      image_name: "grpc_orchestrator"
      image_tag: "latest"
      build_context: "."
      no_cache: false
      build_args:
        BUILD_TYPE: "Release"
    
    - dockerfile: "Dockerfile.task"
      image_name: "grpc_task"
      image_tag: "latest"
      build_context: "."
      no_cache: false
  
  settings:
    build_parallel: false
    push_after_build: true
    cleanup_local_after_push: false
```

**2. Usa Builder Manager:**

```bash
# Build immagini
cd build
./bin/builder_manager_main \
    --config ../deploy/builder_config.yaml \
    --action build

# Push su GitLab
./bin/builder_manager_main \
    --config ../deploy/builder_config.yaml \
    --action push

# Build e push in un solo comando
./bin/builder_manager_main \
    --config ../deploy/builder_config.yaml \
    --action build-push

# Mostra immagini locali
./bin/builder_manager_main \
    --config ../deploy/builder_config.yaml \
    --action status

# Cleanup immagini locali
./bin/builder_manager_main \
    --config ../deploy/builder_config.yaml \
    --action cleanup
```

## Configurazione Real-Time

### Prerequisiti Sistema

**1. Configura kernel Linux:**

```bash
# Permetti scheduling real-time illimitato
sudo sysctl -w kernel.sched_rt_runtime_us=-1

# Rendi permanente
echo "kernel.sched_rt_runtime_us=-1" | sudo tee -a /etc/sysctl.conf
```

**2. Configura limiti utente:**

Modifica `/etc/security/limits.conf`:
```
your_username  hard  rtprio  99
your_username  soft  rtprio  99
your_username  hard  memlock 2048000
your_username  soft  memlock 2048000
```

**3. Riavvia o rilogga per applicare le modifiche.**

### Policy Real-Time

#### SCHED_FIFO (Raccomandato)
Priorità fissa, preemption di task a priorità inferiore:
```bash
./orchestrator_main --policy fifo --priority 80
```

#### SCHED_RR  
Come FIFO ma con time-slicing tra task stessa priorità:
```bash
./orchestrator_main --policy rr --priority 80
```

#### Normal (Default)
Nessuna garanzia real-time:
```bash
./orchestrator_main --policy none
```

### CPU Affinity

Associa processo a CPU core specifico:

```bash
# Task su core 0
./task_runner --cpu-affinity 0 ...

# Task su core 2
./task_runner --cpu-affinity 2 ...

# Nessun binding
./task_runner --cpu-affinity -1 ...
```

### Memory Locking

Previene page fault bloccando memoria in RAM:

```bash
./orchestrator_main --lock-memory ...
```

Richiede:
- Permessi adeguati (root o CAP_IPC_LOCK)
- Limite memlock sufficiente

### Esempio Configurazione Completa RT

```bash
# Orchestrator su core 0, priorità massima
./orchestrator_main \
    --schedule schedules/rt_schedule.yaml \
    --policy fifo \
    --priority 99 \
    --cpu-affinity 0 \
    --lock-memory

# Task critico su core 1
./task_runner \
    --name critical_task \
    --address 0.0.0.0:50051 \
    --orchestrator localhost:50050 \
    --policy fifo \
    --priority 80 \
    --cpu-affinity 1 \
    --lock-memory

# Task normale su core 2
./task_runner \
    --name normal_task \
    --address 0.0.0.0:50052 \
    --orchestrator localhost:50050 \
    --cpu-affinity 2
```

### CPU Isolation (Avanzato)

Per prestazioni ottimali, isola core CPU:

**1. Modifica `/etc/default/grub`:**
```
GRUB_CMDLINE_LINUX="isolcpus=1,2,3 nohz_full=1,2,3 rcu_nocbs=1,2,3"
```

**2. Aggiorna GRUB:**
```bash
sudo update-grub
sudo reboot
```

**3. Usa core isolati per task RT:**
```bash
./task_runner --cpu-affinity 1 ...  # Core 1 è isolato
```

## Monitoring e Debugging

### Log e Output

L'orchestrator stampa informazioni dettagliate:

```
[Orchestrator] Loading schedule from: schedules/my_schedule.yaml
[Orchestrator] Loaded 3 tasks
[Orchestrator] Starting scheduler thread
[Scheduler] Executing task: task_1 at time 1.000s
[Task] task_1 started, executing...
[Orchestrator] Task task_1 completed in 4.123s
```

### Metriche Task

Ogni task può includere metriche nell'output:

```cpp
json output;
output["execution_time_us"] = duration;
output["items_processed"] = count;
output["cpu_usage"] = cpu_percent;
output_json = output.dump();
```

### Strumenti di Monitoring

**1. Docker logs:**
```bash
docker logs grpc_orchestrator
docker logs grpc_task1
docker logs -f grpc_task2  # Segui in real-time
```

**2. Docker stats:**
```bash
docker stats grpc_orchestrator grpc_task1 grpc_task2
```

**3. Monitoring real-time con `htop`:**
```bash
htop -p $(pgrep orchestrator_main),$(pgrep task_runner)
```

### File di Log

Configura logging su file aggiungendo in `orchestrator_main.cpp`:

```cpp
std::ofstream log_file("orchestrator.log");
std::cout.rdbuf(log_file.rdbuf());  // Redirect cout
```

### Debug Task Personalizzati

Aggiungi debug print nei tuoi task:

```cpp
inline TaskResult my_task(const std::string& params_json, std::string& output_json) {
    std::cout << "[DEBUG] Task started" << std::endl;
    std::cout << "[DEBUG] Params: " << params_json << std::endl;
    
    // ... logica ...
    
    std::cout << "[DEBUG] Output: " << output_json << std::endl;
    std::cout << "[DEBUG] Task completed" << std::endl;
    
    return TASK_RESULT_SUCCESS;
}
```

### Profiling Prestazioni

Vedi documenti dedicati:
- [TIME_MEASUREMENT.md](TIME_MEASUREMENT.md) per latenze
- [CONTEXT_SWITCH_MEASUREMENT.md](CONTEXT_SWITCH_MEASUREMENT.md) per context switch

## Best Practices

### 1. Design dello Schedule

✅ **DO:**
- Usa `estimated_duration_us` per documentare la durata attesa
- Imposta deadline realistiche (almeno 2x la durata stimata)
- Usa `sequential` mode per dipendenze chiare
- Documenta parametri nel campo `description`

❌ **DON'T:**
- Non schedulare troppi task contemporaneamente
- Non ignorare le deadline (causa errori)
- Non creare cicli di dipendenze

### 2. Creazione Task

✅ **DO:**
- Gestisci sempre le eccezioni
- Valida parametri in input
- Ritorna output JSON ben formattato
- Usa logging per debug
- Testa task isolatamente prima dell'integrazione

❌ **DON'T:**
- Non assumere parametri sempre presenti (usa `.value()` con default)
- Non fare blocking I/O lunghi
- Non lasciare memory leak

### 3. Real-Time

✅ **DO:**
- Testa configurazione RT su sistema di sviluppo prima
- Usa policy FIFO per task critici
- Assegna priorità in modo gerarchico (orchestrator > task critici > task normali)
- Usa CPU affinity per separare task

❌ **DON'T:**
- Non usare priorità 99 per tutto
- Non mescolare task RT e non-RT sullo stesso core
- Non dimenticare di configurare limiti sistema

### 4. Deployment

✅ **DO:**
- Usa Docker per produzione
- Testa schedule localmente prima del deploy
- Monitora health check dei container
- Fai backup delle configurazioni

❌ **DON'T:**
- Non committare credenziali in YAML
- Non modificare container in esecuzione
- Non ignorare errori di health check

## Esempi Pratici

### Esempio 1: Pipeline Dati Sequenziale

**Scenario**: Carica dati → Elabora → Salva risultati

**Schedule** (`schedules/data_pipeline.yaml`):
```yaml
schedule:
  name: "Data Pipeline"
  
  tasks:
    - id: data_loader
      address: "task1:50051"
      mode: timed
      scheduled_time_us: 0
      parameters:
        source: "database"
        query: "SELECT * FROM data"
    
    - id: data_processor
      address: "task2:50052"
      mode: sequential
      depends_on: data_loader
      parameters:
        algorithm: "ml_model_v2"
    
    - id: data_saver
      address: "task3:50053"
      mode: sequential
      depends_on: data_processor
      parameters:
        destination: "results_table"
```

### Esempio 2: Pipeline Parallela

**Scenario**: Elabora 3 flussi dati indipendenti in parallelo

**Schedule**:
```yaml
tasks:
  - id: stream_a
    address: "task1:50051"
    mode: timed
    scheduled_time_us: 0
    parameters:
      stream: "A"
  
  - id: stream_b
    address: "task2:50052"
    mode: timed
    scheduled_time_us: 0
    parameters:
      stream: "B"
  
  - id: stream_c
    address: "task3:50053"
    mode: timed
    scheduled_time_us: 0
    parameters:
      stream: "C"
```

### Esempio 3: Pipeline Ibrida

**Scenario**: Alcuni task paralleli, poi aggregazione

**Schedule**:
```yaml
tasks:
  # Fase 1: Parallel processing
  - id: preprocess_a
    address: "task1:50051"
    mode: timed
    scheduled_time_us: 0
  
  - id: preprocess_b
    address: "task2:50052"
    mode: timed
    scheduled_time_us: 0
  
  # Fase 2: Wait for both, then aggregate
  - id: aggregate
    address: "task3:50053"
    mode: sequential
    depends_on: preprocess_b  # Wait for last parallel task
    scheduled_time_us: 5000000  # O usa tempo fisso
```

### Esempio 4: Periodic Task

**Scenario**: Esegui task ogni secondo per 10 secondi

**Schedule**:
```yaml
tasks:
  - id: periodic_1
    mode: timed
    scheduled_time_us: 1000000
    # ...
  - id: periodic_2
    mode: timed
    scheduled_time_us: 2000000
  - id: periodic_3
    mode: timed
    scheduled_time_us: 3000000
  # ... fino a 10 secondi
```

---

## Prossimi Passi

1. 📖 Leggi [DEVELOPER_GUIDE.md](DEVELOPER_GUIDE.md) per dettagli architetturali
2. 🔬 Esplora gli schedule di esempio in `schedules/`
3. 💻 Crea il tuo primo task personalizzato
4. 🚀 Fai deploy in produzione

## Supporto

Per problemi o domande:
- Consulta la documentazione tecnica
- Controlla gli esempi nella directory `schedules/`
- Apri una issue su GitHub

---

**Buon orchestration! 🎵**
