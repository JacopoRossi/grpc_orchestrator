# 🚀 Deploy Manager - Gestione Deployment

Il **Deploy Manager** è il componente responsabile per:
- ✅ Definire i task e le loro configurazioni
- ✅ Fare il build delle immagini Docker
- ✅ Deployare e gestire i container
- ✅ Creare la rete Docker
- ✅ Monitorare lo stato dei container

L'**Orchestrator** invece si occupa solo di:
- ✅ Leggere lo schedule YAML
- ✅ Eseguire i task secondo le sequenze definite
- ✅ Gestire le dipendenze tra task
- ✅ Monitorare l'esecuzione

## 📊 Architettura

```
┌─────────────────────────────────────────────────────────┐
│                    Deploy Manager                        │
│  - Legge deployment_config.yaml                         │
│  - Build immagini Docker                                │
│  - Deploy container (orchestrator + tasks)              │
│  - Gestione network                                     │
└─────────────────────────────────────────────────────────┘
                            │
                            ▼
┌─────────────────────────────────────────────────────────┐
│                     Orchestrator                         │
│  - Legge schedule YAML                                  │
│  - Esegue task secondo sequenze                         │
│  - Gestisce dipendenze                                  │
└─────────────────────────────────────────────────────────┘
                            │
                ┌───────────┴───────────┐
                ▼                       ▼
        ┌──────────────┐        ┌──────────────┐
        │   Task 1     │        │   Task 2     │
        │ (Container)  │        │ (Container)  │
        └──────────────┘        └──────────────┘
```

## 📝 File di Configurazione

### `deployment_config.yaml`

Definisce **COSA** deployare:
- Immagini Docker da buildare
- Container da creare
- Configurazioni di rete
- Risorse e capabilities
- Health checks

### `schedule.yaml` (usato dall'Orchestrator)

Definisce **QUANDO** eseguire:
- Sequenza di esecuzione dei task
- Timing e deadline
- Parametri per ogni task
- Dipendenze tra task

## 🚀 Comandi

### Build delle Immagini

```bash
./deploy_manager_main build
```

Questo comando:
1. Legge `deployment_config.yaml`
2. Builda l'immagine orchestrator (`Dockerfile.orchestrator`)
3. Builda l'immagine task (`Dockerfile.task`)

### Deploy Completo

```bash
./deploy_manager_main deploy
```

Questo comando:
1. Builda le immagini (se necessario)
2. Crea la rete Docker
3. Deploya tutti i task container
4. Deploya l'orchestrator container
5. Verifica che tutti siano healthy

### Verifica Status

```bash
./deploy_manager_main status
```

Mostra:
- Container in esecuzione
- Stato della rete
- Health status

### Stop

```bash
./deploy_manager_main stop
```

Ferma tutti i container senza rimuoverli.

### Cleanup

```bash
./deploy_manager_main cleanup
```

Rimuove:
- Tutti i container
- La rete Docker
- (Le immagini rimangono)

## 📋 Workflow Tipico

### 1. Definisci i Task

Modifica `tasks/my_tasks.h` per aggiungere le tue funzioni task.

### 2. Configura il Deployment

Modifica `deploy/deployment_config.yaml`:

```yaml
tasks:
  - id: "my_new_task"
    container_name: "grpc_task4"
    hostname: "task4"
    port: 50054
    task_type: "my_new_task"  # Nome nel registry
```

### 3. Configura lo Schedule

Modifica `schedules/my_schedule.yaml`:

```yaml
tasks:
  - id: my_task
    address: "task4:50054"
    mode: timed
    scheduled_time_us: 1000000
    parameters:
      my_param: 123
```

### 4. Build e Deploy

```bash
# Build
./deploy_manager_main build

# Deploy
./deploy_manager_main deploy
```

### 5. Monitora

```bash
# Status
./deploy_manager_main status

# Logs orchestrator
docker logs -f grpc_orchestrator

# Logs task
docker logs -f grpc_task1
```

## 🔧 Configurazione Avanzata

### Real-Time Configuration

Nel `deployment_config.yaml`:

```yaml
tasks:
  - id: "critical_task"
    rt_config:
      policy: "fifo"
      priority: 90
      cpu_affinity: 2
    resources:
      cpuset: "2"
      memlock: 4096000000
```

### Health Checks Personalizzati

```yaml
tasks:
  - id: "my_task"
    healthcheck:
      command: "pgrep -f task_runner"
      interval: 10
      timeout: 5
      retries: 5
```

### Deployment Strategy

```yaml
strategy:
  type: "sequential"  # o "parallel"
  wait_for_healthy: true
  startup_timeout: 60  # secondi
```

## 🎯 Vantaggi della Separazione

### Prima (Monolitico)
- Orchestrator gestiva tutto
- Configurazione mista in docker-compose.yml
- Difficile testare componenti separatamente

### Dopo (Separato)
- **Deploy Manager**: infrastruttura e deployment
- **Orchestrator**: solo scheduling ed esecuzione
- Configurazioni separate e chiare
- Facile testare e debuggare

## 📚 Esempi

### Esempio 1: Deploy Locale

```bash
# Build
./build/bin/deploy_manager_main build

# Deploy
./build/bin/deploy_manager_main deploy --config deploy/deployment_config.yaml

# Verifica
./build/bin/deploy_manager_main status
```

### Esempio 2: Aggiungere un Task

1. **Definisci in `my_tasks.h`**:
```cpp
inline TaskResult my_ml_task(const std::string& params_json, std::string& output_json) {
    // ... logica ML ...
    return TASK_RESULT_SUCCESS;
}
```

2. **Registra in `task_runner.cpp`**:
```cpp
task_registry["ml_task"] = my_ml_task;
```

3. **Aggiungi in `deployment_config.yaml`**:
```yaml
tasks:
  - id: "task_ml"
    container_name: "grpc_task_ml"
    hostname: "task_ml"
    port: 50054
    task_type: "ml_task"
```

4. **Aggiungi in `schedule.yaml`**:
```yaml
tasks:
  - id: ml_processing
    address: "task_ml:50054"
    mode: timed
    scheduled_time_us: 2000000
    parameters:
      model: "bert"
      batch_size: 32
```

5. **Rebuild e Redeploy**:
```bash
./deploy_manager_main cleanup
./deploy_manager_main deploy
```

## 🐛 Troubleshooting

### Container non si avvia

```bash
# Verifica logs
docker logs grpc_task1

# Verifica network
docker network inspect grpc_network

# Verifica immagine
docker images | grep grpc
```

### Health check fallisce

```bash
# Entra nel container
docker exec -it grpc_task1 /bin/bash

# Verifica processo
ps aux | grep task_runner

# Verifica rete
ping orchestrator
```

### Build fallisce

```bash
# Build manuale
docker build -f Dockerfile.task -t grpc_task:latest .

# Verifica Dockerfile
cat Dockerfile.task
```

## 📖 File di Riferimento

- `deploy/deployment_config.yaml` - Configurazione deployment
- `schedules/*.yaml` - Configurazioni schedule
- `include/deploy_manager.h` - Header Deploy Manager
- `src/deploy_manager.cpp` - Implementazione Deploy Manager
- `examples/deploy_manager_main.cpp` - Main Deploy Manager

## 🔄 Migrazione da Docker Compose

Il Deploy Manager sostituisce `docker-compose.yml` con vantaggi:
- ✅ Configurazione più strutturata
- ✅ Validazione automatica
- ✅ Gestione programmatica
- ✅ Integrazione con orchestrator
- ✅ Health checks avanzati

Puoi comunque usare docker-compose per sviluppo locale, ma per production usa il Deploy Manager.
