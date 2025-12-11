# Quick Start Guide

Guida rapida per iniziare con il gRPC Orchestrator.

## 🐳 Metodo 1: Docker (Più Semplice)

### Prerequisiti
```bash
sudo apt-get install docker.io docker-compose
sudo usermod -aG docker $USER
# Logout e login
```

### Esecuzione
```bash
# 1. Build delle immagini
./docker-build.sh

# 2. Avvia il sistema
./docker-run.sh

# 3. Visualizza i log (in un altro terminale)
./docker-logs.sh

# 4. Ferma tutto
# Premi Ctrl+C nel terminale dove gira docker-run.sh
# oppure
./docker-stop.sh
```

**Vantaggi:**
- ✅ Nessuna dipendenza da installare manualmente
- ✅ Isolamento completo tra componenti
- ✅ Facile da scalare e gestire
- ✅ Funziona su qualsiasi sistema con Docker

---

## 💻 Metodo 2: Native (Compilazione Locale)

### Prerequisiti
```bash
# Disattiva Anaconda se attivo
conda deactivate

# Installa dipendenze
sudo apt-get update
sudo apt-get install -y build-essential cmake libgrpc++-dev libprotobuf-dev protobuf-compiler-grpc pkg-config
```

### Compilazione
```bash
./build.sh
```

### Esecuzione
```bash
# Metodo automatico
./run_demo.sh

# Metodo manuale (4 terminali separati)
# Terminale 1 - Orchestrator
./build/bin/orchestrator_main

# Terminale 2 - Task 1
./build/bin/task_main task_1 localhost:50051 localhost:50050

# Terminale 3 - Task 2
./build/bin/task_main task_2 localhost:50052 localhost:50050

# Terminale 4 - Task 3
./build/bin/task_main task_3 localhost:50053 localhost:50050
```

**Vantaggi:**
- ✅ Performance native
- ✅ Debugging più semplice
- ✅ Nessun overhead di Docker

---

## 📊 Output Atteso

Quando il sistema è in esecuzione, vedrai:

```
[Orchestrator] Loaded schedule with 3 tasks
[Orchestrator] Scheduler started
[Orchestrator] Scheduling task: task_1 at time 1000000 us
[Orchestrator] Task task_1 started successfully
[Orchestrator] Received task end notification: task_1
[Orchestrator] Task task_1 completed with result: 1 (duration: 500537 us)
[Orchestrator] Scheduling task: task_2 at time 2000000 us
...
```

## 🔧 Personalizzazione

### Modificare lo Schedule

Edita `src/schedule.cpp` nella funzione `create_test_schedule()`:

```cpp
ScheduledTask task1;
task1.task_id = "my_task";
task1.task_address = "localhost:50051";  // o "task1:50051" per Docker
task1.scheduled_time_us = 1000000;  // 1 secondo
task1.parameters["my_param"] = "my_value";
```

### Creare un Task Personalizzato

```cpp
#include "task_wrapper.h"

TaskResult my_task(const std::map<std::string, std::string>& params) {
    // La tua logica qui
    std::cout << "Executing my custom task" << std::endl;
    return TASK_RESULT_SUCCESS;
}

int main() {
    TaskWrapper wrapper("my_task", "localhost:50051", "localhost:50050", my_task);
    wrapper.start();
    // Mantieni in esecuzione...
}
```

## 📚 Documentazione Completa

- [README.md](README.md) - Documentazione completa del progetto
- [DOCKER.md](DOCKER.md) - Guida dettagliata Docker
- [INSTALL.md](INSTALL.md) - Istruzioni di installazione

## 🐛 Troubleshooting

### Errore: "port already in use"
```bash
# Trova e termina il processo
sudo lsof -i :50050
kill -9 <PID>
```

### Errore CMake con Anaconda
```bash
# Disattiva Anaconda
conda deactivate
./build.sh
```

### Docker: "Cannot connect to Docker daemon"
```bash
sudo systemctl start docker
sudo usermod -aG docker $USER
# Logout e login
```

## 🎯 Prossimi Passi

1. ✅ Esegui il sistema con lo schedule di test
2. 📝 Modifica lo schedule per i tuoi task
3. 🔧 Implementa la logica dei tuoi task personalizzati
4. 🐳 Deploy in produzione con Docker
5. 📊 Aggiungi monitoring e logging

## 💡 Tips

- Usa Docker per deployment in produzione
- Usa native per sviluppo e debugging
- I log sono in `logs/` quando usi `run_demo.sh`
- Premi Ctrl+C per fermare gracefully
