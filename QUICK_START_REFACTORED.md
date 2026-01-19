# 🚀 Quick Start - Architettura Refactored

## In 5 Minuti

### 1️⃣ Compila il Progetto

```bash
cd build
cmake ..
make -j$(nproc)
```

Output atteso:
```
[100%] Built target orchestrator_lib
[100%] Built target orchestrator_main
[100%] Built target deploy_manager_main
[100%] Built target task_runner
```

### 2️⃣ Build delle Immagini Docker

```bash
./bin/deploy_manager_main build
```

Questo comando:
- Builda `grpc_orchestrator:latest`
- Builda `grpc_task:latest`

### 3️⃣ Deploy del Sistema

```bash
./bin/deploy_manager_main deploy
```

Questo comando:
- Crea la rete Docker `grpc_network`
- Deploya 3 task container (task1, task2, task3)
- Deploya l'orchestrator container
- Verifica che tutti siano healthy

### 4️⃣ Verifica lo Status

```bash
./bin/deploy_manager_main status
```

Output atteso:
```
=== Deployment Status ===
Network: grpc_network
Running containers: 4
  - grpc_task1
  - grpc_task2
  - grpc_task3
  - grpc_orchestrator
```

### 5️⃣ Monitora l'Esecuzione

```bash
# Logs orchestrator
docker logs -f grpc_orchestrator

# Logs task
docker logs -f grpc_task1
```

---

## 🎯 Cosa Succede

1. **Deploy Manager** legge `deploy/deployment_config.yaml`
2. Crea i container per orchestrator e task
3. **Orchestrator** (nel container) legge `schedules/example_parametrized.yaml`
4. Esegue i task secondo lo schedule:
   - `task_1` (Data Analyzer) - parte subito
   - `task_2` (Image Processor) - parte a t=2s
   - `task_3` (Report Generator) - parte dopo task_1

---

## 🔧 Modifica un Task

### 1. Apri `tasks/my_tasks.h`

Trova la funzione `task1_data_analyzer` e modifica:

```cpp
inline TaskResult task1_data_analyzer(const std::string& params_json, std::string& output_json) {
    json params = json::parse(params_json);
    int data_size = params.value("data_size", 1000);
    
    std::cout << "[Task 1] Analyzing " << data_size << " data points" << std::endl;
    
    // La tua logica modificata qui
    // ...
    
    json output;
    output["mean"] = 123.45;
    output["samples"] = data_size;
    output_json = output.dump();
    
    return TASK_RESULT_SUCCESS;
}
```

### 2. Ricompila

```bash
cd build
make task_runner
```

### 3. Redeploy

```bash
./bin/deploy_manager_main cleanup
./bin/deploy_manager_main deploy
```

---

## 📝 Modifica lo Schedule

### 1. Apri `schedules/example_parametrized.yaml`

```yaml
tasks:
  - id: task_1
    address: "task1:50051"
    mode: timed
    scheduled_time_us: 1000000  # Cambia timing
    parameters:
      data_size: 1000000  # Cambia parametri
```

### 2. Redeploy solo Orchestrator

```bash
docker stop grpc_orchestrator
docker rm grpc_orchestrator

# Redeploy
./bin/deploy_manager_main deploy
```

---

## 🧹 Cleanup

```bash
# Stop e rimuovi tutto
./bin/deploy_manager_main cleanup
```

---

## 🎓 Prossimi Passi

1. **Leggi l'architettura**: `ARCHITETTURA_REFACTORED.md`
2. **Guida Deploy Manager**: `deploy/README.md`
3. **Guida Task Runner**: `tasks/README.md`
4. **Aggiungi i tuoi task**: `tasks/QUICK_START.md`

---

## 🐛 Problemi Comuni

### "Container già esistente"

```bash
./bin/deploy_manager_main cleanup
./bin/deploy_manager_main deploy
```

### "Immagine non trovata"

```bash
./bin/deploy_manager_main build
```

### "Network già esiste"

```bash
docker network rm grpc_network
./bin/deploy_manager_main deploy
```

### "Health check fallisce"

```bash
# Verifica logs
docker logs grpc_task1

# Entra nel container
docker exec -it grpc_task1 /bin/bash
ps aux | grep task_runner
```

---

## ✅ Checklist

- [ ] Compilato il progetto
- [ ] Build immagini Docker
- [ ] Deploy completato
- [ ] Status mostra 4 container running
- [ ] Logs orchestrator mostrano esecuzione task
- [ ] Tutto funziona? 🎉

---

## 💡 Tips

**Sviluppo rapido:**
```bash
# Modifica my_tasks.h
# Poi:
make task_runner && \
./bin/deploy_manager_main cleanup && \
./bin/deploy_manager_main deploy
```

**Solo logs:**
```bash
docker logs -f grpc_orchestrator
```

**Verifica rete:**
```bash
docker network inspect grpc_network
```

**Lista container:**
```bash
docker ps --filter network=grpc_network
```

---

Fatto! Il tuo sistema è pronto! 🚀
