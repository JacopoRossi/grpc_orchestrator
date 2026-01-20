# gRPC Orchestrator - Sistema di Orchestrazione Real-Time per Task Distribuiti

Sistema di orchestrazione distribuito basato su gRPC per l'esecuzione coordinata di task con supporto real-time. Ideale per pipeline di elaborazione dati, sistemi di controllo, e applicazioni che richiedono scheduling deterministico.

## 📋 Indice

- [Caratteristiche Principali](#-caratteristiche-principali)
- [Architettura](#-architettura)
- [Componenti del Sistema](#-componenti-del-sistema)
- [Installazione](#-installazione)
- [Guida Rapida](#-guida-rapida)
- [Utilizzo](#-utilizzo)
- [Documentazione Dettagliata](#-documentazione-dettagliata)
- [Esempi](#-esempi)

## ✨ Caratteristiche Principali

- **Orchestrazione Distribuita**: Coordina l'esecuzione di task su container/processi separati
- **Scheduling Real-Time**: Supporto per policy real-time (SCHED_FIFO, SCHED_RR, SCHED_DEADLINE)
- **Modalità di Esecuzione**: 
  - **Timed**: Esecuzione a tempi schedulati precisi
  - **Sequential**: Esecuzione con dipendenze tra task
- **Comunicazione gRPC**: Protocollo ad alte prestazioni per comunicazione tra orchestrator e task
- **Parametri JSON**: Passaggio flessibile di parametri e output tra task
- **Gestione Dipendenze**: I task possono dipendere dall'output di altri task
- **Deployment Automatizzato**: Deploy manager per distribuzione automatica su Docker
- **Build Manager**: Sistema integrato per build e push su GitLab Container Registry
- **Configurazione CPU Affinity**: Binding dei task su core CPU specifici
- **Memory Locking**: Prevenzione di page fault per prestazioni deterministiche

## 🏗 Architettura

```
┌─────────────────────────────────────────────────────────────┐
│                      Orchestrator                           │
│  - Carica lo schedule (YAML)                               │
│  - Coordina l'esecuzione dei task                          │
│  - Applica configurazione real-time                        │
│  - Raccoglie risultati e metriche                          │
└────────────┬────────────────────────────┬──────────────────┘
             │ gRPC (StartTask)           │ gRPC (StartTask)
             │                            │
      ┌──────▼──────┐              ┌─────▼───────┐
      │   Task 1    │              │   Task 2    │
      │  Wrapper    │              │  Wrapper    │
      │  + Logic    │              │  + Logic    │
      └──────┬──────┘              └─────┬───────┘
             │ gRPC (NotifyTaskEnd)      │ gRPC (NotifyTaskEnd)
             └────────────┬──────────────┘
                          │
                          ▼
                   [Orchestrator]
                   Raccoglie risultati
```

## 🔧 Componenti del Sistema

### 1. **Orchestrator** (`orchestrator_main`)
Centro di controllo che:
- Legge lo schedule da file YAML
- Invia comandi di start ai task via gRPC
- Riceve notifiche di completamento
- Gestisce dipendenze tra task
- Raccoglie statistiche e metriche
- Applica configurazione real-time

### 2. **Task Runner** (`task_runner`)
Wrapper generico per eseguire task personalizzati:
- Riceve comandi dall'orchestrator
- Esegue la logica del task
- Notifica il completamento
- Supporta parametri JSON
- Gestisce configurazione real-time

### 3. **Deploy Manager** (`deploy_manager_main`)
Sistema per deployment automatizzato:
- Legge configurazione da YAML
- Crea network Docker
- Effettua login su GitLab Registry
- Effettua pull delle immagini
- Avvia container orchestrator e task
- Monitora health status

### 4. **Builder Manager** (`builder_manager_main`)
Sistema per build e distribuzione immagini:
- Build di immagini Docker da Dockerfile
- Tag per GitLab Container Registry
- Push automatico su registry
- Gestione credenziali GitLab
- Cleanup immagini locali

## 🚀 Installazione

### Requisiti
- **C++17** o superiore
- **CMake 3.15+**
- **gRPC** e **Protobuf**
- **yaml-cpp**
- **nlohmann_json**
- **Docker** (per deployment containerizzato)
- **Linux** con supporto real-time (opzionale)

### Build dal Codice Sorgente

```bash
# Clona il repository
git clone <repository-url>
cd grpc_orchestrator

# Crea directory di build
mkdir -p build
cd build

# Configura e compila (tutti i componenti)
cmake ..
make -j$(nproc)

# Oppure compila componenti specifici
cmake -DBUILD_ORCHESTRATOR=ON -DBUILD_TASKS=OFF ..
make orchestrator_main
```

### Build con Docker

```bash
# Build immagine orchestrator
docker build -f Dockerfile.orchestrator -t grpc_orchestrator:latest .

# Build immagine task
docker build -f Dockerfile.task -t grpc_task:latest .
```

## 🎯 Guida Rapida

### 1. Definire uno Schedule

Crea un file `my_schedule.yaml`:

```yaml
schedule:
  name: "Simple Pipeline"
  description: "Esempio base di pipeline"
  
  tasks:
    - id: task_1
      address: "task1:50051"
      mode: timed
      scheduled_time_us: 1000000  # 1 secondo
      deadline_us: 5000000        # 5 secondi
      parameters:
        data_size: 1000
    
    - id: task_2
      address: "task2:50052"
      mode: sequential
      depends_on: task_1
      deadline_us: 5000000
      parameters:
        process_type: "filter"
```

### 2. Avviare con Docker Compose

```bash
# Avvia tutti i servizi
docker-compose --profile all up

# Oppure avvia solo orchestrator e task
docker-compose --profile orchestrator up -d
docker-compose --profile tasks up -d
```

### 3. Esecuzione Manuale

Terminale 1 - Orchestrator:
```bash
./build/bin/orchestrator_main \
    --address 0.0.0.0:50050 \
    --schedule schedules/my_schedule.yaml \
    --policy fifo \
    --priority 80
```

Terminale 2 - Task 1:
```bash
./build/bin/task_runner \
    --name task_1 \
    --address 0.0.0.0:50051 \
    --orchestrator localhost:50050
```

Terminale 3 - Task 2:
```bash
./build/bin/task_runner \
    --name task_2 \
    --address 0.0.0.0:50052 \
    --orchestrator localhost:50050
```

## 📚 Utilizzo

### Creare Task Personalizzati

Vedi [tasks/README.md](tasks/README.md) per la guida completa alla creazione di task.

Esempio veloce in `tasks/my_tasks.h`:

```cpp
inline TaskResult my_custom_task(const std::string& params_json, std::string& output_json) {
    // Parse input
    json params = json::parse(params_json);
    int value = params.value("input_value", 0);
    
    // La tua logica
    int result = value * 2;
    
    // Output
    json output;
    output["result"] = result;
    output_json = output.dump();
    
    return TASK_RESULT_SUCCESS;
}
```

Registra in `task_runner.cpp`:
```cpp
task_registry["my_custom_task"] = my_custom_task;
```

### Deployment Automatizzato

Configura `deploy/deployment_config.yaml` e poi:

```bash
./build/bin/deploy_manager_main \
    --config deploy/deployment_config.yaml \
    --action deploy
```

Azioni disponibili:
- `deploy`: Deploy completo (orchestrator + tasks)
- `tasks-only`: Deploy solo task
- `cleanup`: Pulizia container e network
- `status`: Mostra stato deployment
- `logs`: Segui i log

### Build e Push Immagini

Configura `deploy/builder_config.yaml` e poi:

```bash
./build/bin/builder_manager_main \
    --config deploy/builder_config.yaml \
    --action build-push
```

Azioni disponibili:
- `build`: Build di tutte le immagini
- `push`: Push su GitLab Registry
- `build-push`: Build e push combinati
- `status`: Mostra immagini disponibili
- `cleanup`: Rimuovi immagini locali

## 📖 Documentazione Dettagliata

- **[USER_GUIDE.md](docs/USER_GUIDE.md)** - Guida utente completa con esempi pratici
- **[DEVELOPER_GUIDE.md](docs/DEVELOPER_GUIDE.md)** - Guida per sviluppatori e architettura interna
- **[tasks/README.md](tasks/README.md)** - Guida alla creazione di task personalizzati
- **[TIME_MEASUREMENT.md](docs/TIME_MEASUREMENT.md)** - Misurazione tempi e latenze
- **[CONTEXT_SWITCH_MEASUREMENT.md](docs/CONTEXT_SWITCH_MEASUREMENT.md)** - Misurazione context switch

## 💡 Esempi

### Esempio 1: Pipeline Sequenziale
```bash
# Usa schedule pre-configurato
docker-compose --profile all up
# Vedi schedules/example_sequential.yaml
```

### Esempio 2: Task con Real-Time
```bash
./orchestrator_main \
    --schedule schedules/example_parametrized_rt.yaml \
    --policy fifo \
    --priority 90 \
    --cpu-affinity 0 \
    --lock-memory
```

### Esempio 3: Pipeline Ibrida (Timed + Sequential)
Vedi `schedules/example_hybrid.yaml` per un esempio di:
- Task timed eseguiti a intervalli precisi
- Task sequenziali con dipendenze
- Passaggio dati tra task

## 🔬 Configurazione Real-Time

Per prestazioni real-time ottimali:

1. **Kernel Configuration**:
```bash
# Abilita real-time scheduling
sudo sysctl -w kernel.sched_rt_runtime_us=-1
```

2. **Container Privileges**:
```yaml
cap_add:
  - SYS_NICE
ulimits:
  rtprio: 99
  rttime: -1
  memlock: 2048000000
```

3. **CPU Isolation** (opzionale):
```bash
# In /etc/default/grub
GRUB_CMDLINE_LINUX="isolcpus=1,2,3"
```

## 🐛 Troubleshooting

### Task non si connette all'orchestrator
- Verifica che l'orchestrator sia avviato e in ascolto
- Controlla hostname/indirizzo nel comando del task
- Verifica porte e firewall

### Real-time policy fallisce
- Esegui come root o con `CAP_SYS_NICE`
- Verifica limiti ulimit: `ulimit -r`
- Controlla `/etc/security/limits.conf`

### Build fallisce
- Installa dipendenze: `sudo apt install libgrpc++-dev libprotobuf-dev libyaml-cpp-dev nlohmann-json3-dev`
- Verifica versione CMake: `cmake --version` (richiesta 3.15+)

## 📄 Licenza

[Specificare licenza]

## 🤝 Contribuire

Contributi benvenuti! Apri issue o pull request.

## 📧 Contatti

[Inserire informazioni di contatto]

---

**Nota**: Questo sistema è progettato per scenari che richiedono coordinazione precisa e deterministica di task distribuiti. Per casi d'uso più semplici, considera alternative più leggere.
