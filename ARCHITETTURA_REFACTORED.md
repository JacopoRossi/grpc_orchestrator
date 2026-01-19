# 🏗️ Architettura Refactored - Deploy Manager + Orchestrator

## 📊 Panoramica

Il sistema è stato refactorato in **due componenti principali**:

### 1. **Deploy Manager** 🚀
Responsabile dell'**infrastruttura** e del **deployment**

### 2. **Orchestrator** ⚡
Responsabile dello **scheduling** e dell'**esecuzione**

---

## 🎯 Separazione delle Responsabilità

### Deploy Manager

**Cosa fa:**
- ✅ Legge `deployment_config.yaml`
- ✅ Builda le immagini Docker
- ✅ Crea la rete Docker
- ✅ Deploya i container (orchestrator + tasks)
- ✅ Configura risorse, capabilities, health checks
- ✅ Gestisce il ciclo di vita dei container

**Quando si usa:**
- All'avvio del sistema
- Quando aggiungi/rimuovi task
- Per aggiornare configurazioni infrastrutturali
- Per cleanup e manutenzione

**File chiave:**
- `deploy/deployment_config.yaml` - Configurazione deployment
- `include/deploy_manager.h` - Header
- `src/deploy_manager.cpp` - Implementazione
- `examples/deploy_manager_main.cpp` - Eseguibile

### Orchestrator

**Cosa fa:**
- ✅ Legge `schedule.yaml`
- ✅ Esegue i task secondo le sequenze definite
- ✅ Gestisce timing e deadline
- ✅ Gestisce dipendenze tra task
- ✅ Passa parametri e output tra task
- ✅ Monitora l'esecuzione

**Quando si usa:**
- Dopo che i container sono deployati
- Per eseguire workflow di task
- Per testare sequenze diverse
- Per produzione

**File chiave:**
- `schedules/*.yaml` - Configurazioni schedule
- `include/orchestrator.h` - Header
- `src/orchestrator.cpp` - Implementazione
- `examples/orchestrator_main.cpp` - Eseguibile

---

## 🔄 Flusso Completo

```
┌─────────────────────────────────────────────────────────────┐
│ 1. DEPLOY MANAGER                                           │
│    ./deploy_manager_main deploy                             │
│                                                              │
│    ┌─────────────────────────────────────────────┐         │
│    │ Legge deployment_config.yaml                │         │
│    │ - Definizioni task                          │         │
│    │ - Configurazioni Docker                     │         │
│    │ - Network settings                          │         │
│    └─────────────────────────────────────────────┘         │
│                        │                                     │
│                        ▼                                     │
│    ┌─────────────────────────────────────────────┐         │
│    │ Build Immagini Docker                       │         │
│    │ - grpc_orchestrator:latest                  │         │
│    │ - grpc_task:latest                          │         │
│    └─────────────────────────────────────────────┘         │
│                        │                                     │
│                        ▼                                     │
│    ┌─────────────────────────────────────────────┐         │
│    │ Deploy Container                            │         │
│    │ - Crea network: grpc_network                │         │
│    │ - Deploy task1, task2, task3                │         │
│    │ - Deploy orchestrator                       │         │
│    │ - Verifica health checks                    │         │
│    └─────────────────────────────────────────────┘         │
└─────────────────────────────────────────────────────────────┘
                        │
                        ▼
┌─────────────────────────────────────────────────────────────┐
│ 2. ORCHESTRATOR (avviato automaticamente nel container)    │
│                                                              │
│    ┌─────────────────────────────────────────────┐         │
│    │ Legge schedule.yaml                         │         │
│    │ - Task da eseguire                          │         │
│    │ - Timing e sequenze                         │         │
│    │ - Parametri                                 │         │
│    │ - Dipendenze                                │         │
│    └─────────────────────────────────────────────┘         │
│                        │                                     │
│                        ▼                                     │
│    ┌─────────────────────────────────────────────┐         │
│    │ Esecuzione Task                             │         │
│    │                                              │         │
│    │  t=0s    → task_1 (sequential)              │         │
│    │  t=2s    → task_2 (timed)                   │         │
│    │  after 1 → task_3 (depends on task_1)       │         │
│    │                                              │         │
│    └─────────────────────────────────────────────┘         │
│                        │                                     │
│                        ▼                                     │
│    ┌─────────────────────────────────────────────┐         │
│    │ Comunicazione gRPC con Task Container       │         │
│    │                                              │         │
│    │  Orchestrator → Task: StartTask(params)     │         │
│    │  Task → Orchestrator: TaskEnd(result)       │         │
│    │                                              │         │
│    └─────────────────────────────────────────────┘         │
└─────────────────────────────────────────────────────────────┘
                        │
                        ▼
┌─────────────────────────────────────────────────────────────┐
│ 3. TASK EXECUTION (nei container task)                     │
│                                                              │
│    ┌─────────────────────────────────────────────┐         │
│    │ Task Runner (task_runner)                   │         │
│    │ - Riceve comando StartTask                  │         │
│    │ - Esegue funzione da my_tasks.h             │         │
│    │ - Ritorna risultato                         │         │
│    └─────────────────────────────────────────────┘         │
└─────────────────────────────────────────────────────────────┘
```

---

## 📁 Struttura File

```
grpc_orchestrator/
│
├── deploy/                          # Deploy Manager
│   ├── deployment_config.yaml       # Configurazione deployment
│   └── README.md                    # Guida Deploy Manager
│
├── schedules/                       # Orchestrator
│   ├── example_parametrized.yaml    # Schedule di esempio
│   └── ...
│
├── tasks/                           # Task Definitions
│   ├── my_tasks.h                   # Funzioni task
│   ├── task_runner.cpp              # Runner generico
│   └── README.md
│
├── include/
│   ├── deploy_manager.h             # Header Deploy Manager
│   ├── orchestrator.h               # Header Orchestrator
│   ├── task_wrapper.h
│   └── schedule.h
│
├── src/
│   ├── deploy_manager.cpp           # Implementazione Deploy Manager
│   ├── orchestrator.cpp             # Implementazione Orchestrator
│   ├── task_wrapper.cpp
│   └── schedule.cpp
│
└── examples/
    ├── deploy_manager_main.cpp      # Main Deploy Manager
    ├── orchestrator_main.cpp        # Main Orchestrator
    └── ...
```

---

## 🎮 Comandi Principali

### Deploy Manager

```bash
# Build immagini
./build/bin/deploy_manager_main build

# Deploy tutto
./build/bin/deploy_manager_main deploy

# Status
./build/bin/deploy_manager_main status

# Stop
./build/bin/deploy_manager_main stop

# Cleanup
./build/bin/deploy_manager_main cleanup
```

### Orchestrator (eseguito automaticamente nel container)

L'orchestrator viene avviato automaticamente dal Deploy Manager.
Se vuoi eseguirlo manualmente:

```bash
./build/bin/orchestrator_main \
    --address 0.0.0.0:50050 \
    --schedule schedules/example_parametrized.yaml
```

---

## 🔧 Workflow di Sviluppo

### Scenario 1: Aggiungere un Nuovo Task

1. **Definisci la funzione** in `tasks/my_tasks.h`
2. **Registra** in `tasks/task_runner.cpp`
3. **Compila**: `make task_runner`
4. **Aggiungi al deployment** in `deploy/deployment_config.yaml`
5. **Aggiungi allo schedule** in `schedules/my_schedule.yaml`
6. **Rebuild e redeploy**:
   ```bash
   ./deploy_manager_main cleanup
   ./deploy_manager_main deploy
   ```

### Scenario 2: Modificare uno Schedule

1. **Modifica** `schedules/my_schedule.yaml`
2. **Aggiorna** la configurazione dell'orchestrator in `deployment_config.yaml`
3. **Redeploy** solo l'orchestrator:
   ```bash
   docker stop grpc_orchestrator
   docker rm grpc_orchestrator
   ./deploy_manager_main deploy
   ```

### Scenario 3: Testare in Locale

1. **Avvia task manualmente**:
   ```bash
   ./task_runner --name task_1 --address 0.0.0.0:50051 --orchestrator localhost:50050
   ```

2. **Avvia orchestrator**:
   ```bash
   ./orchestrator_main --address 0.0.0.0:50050 --schedule schedules/test.yaml
   ```

---

## 📊 Confronto: Prima vs Dopo

### Prima (Monolitico)

```
docker-compose.yml
├── Definisce container
├── Definisce network
├── Definisce build
├── Definisce comandi
└── Configurazione mista

Orchestrator
├── Legge schedule
├── Esegue task
└── (Nessuna gestione deployment)
```

**Problemi:**
- ❌ Configurazione sparsa
- ❌ Difficile gestire programmaticamente
- ❌ Orchestrator non sa dei container
- ❌ Deployment manuale

### Dopo (Refactored)

```
Deploy Manager
├── deployment_config.yaml
├── Build immagini
├── Deploy container
├── Gestione network
└── Health checks

Orchestrator
├── schedule.yaml
├── Esegue task
├── Gestisce sequenze
└── Monitora esecuzione
```

**Vantaggi:**
- ✅ Separazione chiara
- ✅ Gestione programmatica
- ✅ Configurazioni strutturate
- ✅ Deployment automatico
- ✅ Facile testing

---

## 🎯 Casi d'Uso

### Production Deployment

```bash
# 1. Deploy infrastruttura
./deploy_manager_main deploy --config deploy/production.yaml

# 2. Orchestrator si avvia automaticamente e legge lo schedule
# 3. Task vengono eseguiti secondo lo schedule
# 4. Monitora con:
./deploy_manager_main status
docker logs -f grpc_orchestrator
```

### Development

```bash
# 1. Deploy con configurazione dev
./deploy_manager_main deploy --config deploy/dev.yaml

# 2. Modifica task in my_tasks.h
# 3. Rebuild solo task
make task_runner

# 4. Redeploy solo task
docker stop grpc_task1
docker rm grpc_task1
# ... redeploy task1 ...
```

### Testing

```bash
# 1. Cleanup
./deploy_manager_main cleanup

# 2. Deploy con schedule di test
./deploy_manager_main deploy --config deploy/test.yaml

# 3. Verifica risultati
docker logs grpc_orchestrator
```

---

## 🚀 Prossimi Passi

1. ✅ Compila il progetto: `cd build && cmake .. && make`
2. ✅ Testa il Deploy Manager: `./bin/deploy_manager_main build`
3. ✅ Configura il tuo deployment: modifica `deploy/deployment_config.yaml`
4. ✅ Configura il tuo schedule: modifica `schedules/my_schedule.yaml`
5. ✅ Deploy: `./bin/deploy_manager_main deploy`
6. ✅ Monitora: `./bin/deploy_manager_main status`

---

## 📚 Documentazione

- `deploy/README.md` - Guida Deploy Manager
- `tasks/README.md` - Guida Task Runner
- `TASK_DOCUMENTATION.md` - Documentazione task
- Questo file - Architettura completa
