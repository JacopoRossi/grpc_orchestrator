# Modalità di Esecuzione Sequenziale

## Descrizione

L'orchestrator è stato configurato per eseguire i task in **modalità sequenziale**:

1. **Task 1** viene lanciato su un thread separato
2. L'orchestrator attende il segnale di `end` dal Task 1
3. Solo dopo aver ricevuto il segnale, viene lanciato il **Task 2** su un nuovo thread
4. L'orchestrator attende il segnale di `end` dal Task 2
5. Solo dopo aver ricevuto il segnale, viene lanciato il **Task 3** su un nuovo thread
6. L'orchestrator attende il segnale di `end` dal Task 3
7. Tutti i task completati, il sistema si ferma

## Caratteristiche

- ✅ Ogni task viene eseguito su un **thread separato**
- ✅ L'esecuzione è **strettamente sequenziale** (un task alla volta)
- ✅ Il prossimo task parte **solo dopo** aver ricevuto il segnale di end dal precedente
- ✅ Sincronizzazione tramite **condition variable** (`task_end_cv_`)
- ✅ Funziona con **Docker** e richiede `sudo`

## Come Usare

### Avvio con Docker Compose

```bash
# Usando lo script helper
./run_docker.sh up

# Oppure manualmente
sudo docker-compose build
sudo docker-compose up
```

### Comandi Disponibili

```bash
# Avvio del sistema
./run_docker.sh up

# Stop del sistema
./run_docker.sh down

# Restart
./run_docker.sh restart

# Visualizza i log dell'orchestrator
./run_docker.sh logs orchestrator

# Visualizza i log di un task specifico
./run_docker.sh logs task1

# Pulizia completa
./run_docker.sh clean
```

### Visualizzare i Log in Tempo Reale

```bash
# Tutti i container
sudo docker-compose logs -f

# Solo orchestrator
sudo docker-compose logs -f orchestrator

# Solo un task specifico
sudo docker-compose logs -f task1
```

## Output Atteso

L'output mostrerà chiaramente l'esecuzione sequenziale:

```
[Orchestrator] ========================================
[Orchestrator] Starting task 1/3: task_1
[Orchestrator] ========================================

[Task task_1] Received start command
[Task task_1] Starting task execution
...
[Task task_1] Task execution completed successfully
[Orchestrator] Task task_1 completed and acknowledged
[Orchestrator] Proceeding to next task...

[Orchestrator] ========================================
[Orchestrator] Starting task 2/3: task_2
[Orchestrator] ========================================

[Task task_2] Received start command
...
```

## Implementazione Tecnica

### Modifiche Principali

1. **orchestrator.h**: Aggiunta `task_end_cv_` per sincronizzazione
2. **orchestrator.cpp**: 
   - `scheduler_loop()` esegue un loop sequenziale sui task
   - Ogni task viene lanciato su un thread separato con `std::thread().detach()`
   - Dopo il lancio, lo scheduler attende sulla condition variable
   - `on_task_end()` notifica la condition variable quando un task termina

### Sincronizzazione

```cpp
// Lancia task su thread separato
std::thread([this, task]() {
    execute_task(task);
}).detach();

// Attende che il task completi
std::unique_lock<std::mutex> lock(mutex_);
task_end_cv_.wait(lock, [this, &task]() {
    return active_tasks_.find(task.task_id) == active_tasks_.end();
});
```

## Note

- Le configurazioni real-time sono state **disabilitate** per evitare problemi di segmentation fault
- Per riabilitarle, modificare `docker-compose.yml` aggiungendo i parametri `--policy`, `--priority`, ecc.
- Il sistema si ferma automaticamente dopo aver completato tutti i task
