# Modalità Ibrida - Esecuzione Sequenziale e Temporizzata

## 🎯 Cosa è stato implementato

Il sistema ora supporta **due modalità di esecuzione** per i task:

### 1. **Modalità SEQUENZIALE** (`TASK_MODE_SEQUENTIAL`)
- Il task aspetta che un altro task specifico finisca prima di partire
- Utile per task con dipendenze (es: task_3 deve aspettare task_1)

### 2. **Modalità TEMPORIZZATA** (`TASK_MODE_TIMED`)
- Il task parte a un tempo preciso dall'inizio dell'esecuzione
- Non aspetta altri task, parte esattamente al tempo schedulato
- Utile per task indipendenti con timing preciso

## 📋 Configurazione Attuale

Nel file `src/schedule.cpp`, lo schedule è configurato così:

```cpp
// Task 1: Parte subito (sequenziale)
task1.execution_mode = TASK_MODE_SEQUENTIAL;
task1.wait_for_task_id = "";  // Nessuna dipendenza
task1.scheduled_time_us = 0;  // Parte immediatamente

// Task 2: Parte a 2 secondi (temporizzato)
task2.execution_mode = TASK_MODE_TIMED;
task2.scheduled_time_us = 2000000;  // 2 secondi dall'inizio

// Task 3: Parte quando task_1 finisce (sequenziale)
task3.execution_mode = TASK_MODE_SEQUENTIAL;
task3.wait_for_task_id = "task_1";  // Aspetta task_1
```

## 🔄 Come Funziona

### Timeline di esecuzione:

```
Tempo 0ms:    Task 1 PARTE (sequenziale, nessuna dipendenza)
              ↓
Tempo ~500ms: Task 1 FINISCE
              ↓
              Task 3 PARTE (aspettava task_1)
              ↓
Tempo 2000ms: Task 2 PARTE (temporizzato, parte esattamente a 2 secondi)
              ↓
Tempo ~2500ms: Task 3 FINISCE
              ↓
Tempo ~2800ms: Task 2 FINISCE
              ↓
              TUTTI I TASK COMPLETATI
```

### Output previsto:

```
[Orchestrator] Scheduler loop started (HYBRID MODE)
[Orchestrator] Supporting both sequential and timed execution

========================================
[Orchestrator] Processing task 1/3: task_1
[Orchestrator] Mode: SEQUENTIAL
========================================
/ciao                          <- Task 1 stampa

[Orchestrator] Task task_1 completed

========================================
[Orchestrator] Processing task 2/3: task_2
[Orchestrator] Mode: TIMED (scheduled at 2000 ms)
[Orchestrator] Waiting 1500 ms until scheduled time...
========================================

========================================
[Orchestrator] Processing task 3/3: task_3
[Orchestrator] Mode: SEQUENTIAL (waiting for task_1)
[Orchestrator] Dependency satisfied, starting task...
========================================
 Jacopo/                       <- Task 3 stampa (dopo task_1)

[Orchestrator] Task task_3 completed

 sono                          <- Task 2 stampa (a 2 secondi)
```

## 🔧 Come Modificare lo Schedule

### Esempio 1: Tutti i task in sequenza

```cpp
task1.execution_mode = TASK_MODE_SEQUENTIAL;
task1.wait_for_task_id = "";

task2.execution_mode = TASK_MODE_SEQUENTIAL;
task2.wait_for_task_id = "task_1";  // Aspetta task_1

task3.execution_mode = TASK_MODE_SEQUENTIAL;
task3.wait_for_task_id = "task_2";  // Aspetta task_2
```

### Esempio 2: Tutti i task temporizzati

```cpp
task1.execution_mode = TASK_MODE_TIMED;
task1.scheduled_time_us = 0;        // Parte subito

task2.execution_mode = TASK_MODE_TIMED;
task2.scheduled_time_us = 2000000;  // Parte a 2 secondi

task3.execution_mode = TASK_MODE_TIMED;
task3.scheduled_time_us = 5000000;  // Parte a 5 secondi
```

### Esempio 3: Catena di dipendenze

```cpp
task1.execution_mode = TASK_MODE_SEQUENTIAL;
task1.wait_for_task_id = "";

task2.execution_mode = TASK_MODE_SEQUENTIAL;
task2.wait_for_task_id = "task_1";

task3.execution_mode = TASK_MODE_SEQUENTIAL;
task3.wait_for_task_id = "task_1";  // Anche task_3 aspetta task_1
```

## 📝 Modifiche ai File

### File modificati:

1. **`include/schedule.h`**
   - Aggiunto enum `TaskExecutionMode`
   - Aggiunto campo `execution_mode` a `ScheduledTask`
   - Aggiunto campo `wait_for_task_id` per le dipendenze

2. **`src/schedule.cpp`**
   - Aggiornato `create_test_schedule()` con la configurazione ibrida
   - Aggiunto `task_id` ai parametri di ogni task

3. **`src/orchestrator.cpp`**
   - Riscritto `scheduler_loop()` per gestire entrambe le modalità
   - Aggiunto tracking delle dipendenze
   - Aggiunto timing preciso per task temporizzati

## 🐳 Compatibilità Docker

Le modifiche funzionano **sia con Docker che senza**:

- Con Docker: usa gli hostname dei container (`task1:50051`, `task2:50051`, ecc.)
- Senza Docker: usa localhost con porte diverse (`localhost:50051`, `localhost:50052`, ecc.)

La rilevazione è automatica tramite la variabile d'ambiente `DOCKER_CONTAINER`.

## 🚀 Come Compilare e Testare

### Senza Docker:

```bash
# Compila
cd /home/vboxuser/projects/grpc_orchestrator
mkdir -p build && cd build
cmake ..
make -j$(nproc)

# Esegui (4 terminali separati)
# Terminale 1:
sudo ./build/bin/orchestrator_main --address 0.0.0.0:50050 --policy fifo --priority 80 --cpu-affinity 0 --lock-memory

# Terminale 2:
sudo ./build/bin/task_main --name task_1 --address localhost:50051 --orchestrator localhost:50050 --policy fifo --priority 75 --cpu-affinity 1 --lock-memory

# Terminale 3:
sudo ./build/bin/task_main --name task_2 --address localhost:50052 --orchestrator localhost:50050 --policy fifo --priority 75 --cpu-affinity 2 --lock-memory

# Terminale 4:
sudo ./build/bin/task_main --name task_3 --address localhost:50053 --orchestrator localhost:50050 --policy fifo --priority 75 --cpu-affinity 3 --lock-memory
```

### Con Docker:

```bash
# Ricompila le immagini
docker-compose build

# Avvia
docker-compose up

# Osserva i log
docker-compose logs -f
```

## 🎨 Personalizzazione

Per cambiare il comportamento, modifica `src/schedule.cpp` nella funzione `create_test_schedule()`:

```cpp
// Cambia execution_mode:
task.execution_mode = TASK_MODE_SEQUENTIAL;  // o TASK_MODE_TIMED

// Per task sequenziali, specifica la dipendenza:
task.wait_for_task_id = "task_1";  // Aspetta task_1

// Per task temporizzati, specifica il tempo:
task.scheduled_time_us = 3000000;  // 3 secondi
```

Poi ricompila:
```bash
cd build && make -j$(nproc)
```

O con Docker:
```bash
docker-compose build
```

## ✅ Vantaggi della Modalità Ibrida

1. **Flessibilità**: Puoi mescolare task sequenziali e temporizzati
2. **Dipendenze**: Task possono aspettare altri task specifici
3. **Timing preciso**: Task temporizzati partono esattamente al tempo schedulato
4. **Parallelismo**: Task temporizzati possono eseguire in parallelo con task sequenziali
5. **Compatibilità**: Funziona sia con Docker che senza

## 🔍 Debug

Per vedere cosa sta succedendo, osserva i log:

```
[Orchestrator] Mode: SEQUENTIAL (waiting for task_1)
```
→ Task in modalità sequenziale, aspetta task_1

```
[Orchestrator] Mode: TIMED (scheduled at 2000 ms)
[Orchestrator] Waiting 1500 ms until scheduled time...
```
→ Task in modalità temporizzata, aspetta il tempo schedulato

```
[Orchestrator] Dependency satisfied, starting task...
```
→ La dipendenza è soddisfatta, il task parte

```
[Orchestrator] Task launched (not waiting)
```
→ Task temporizzato lanciato, l'orchestrator non aspetta

## 📊 Esempio Completo

Configurazione attuale (task_1 → task_3, task_2 a 2 secondi):

```
T=0ms:     Task 1 PARTE
T=500ms:   Task 1 FINISCE → Task 3 PARTE
T=2000ms:  Task 2 PARTE (timing preciso)
T=2500ms:  Task 3 FINISCE
T=2800ms:  Task 2 FINISCE
```

Output:
```
/ciao        (task_1 a ~500ms)
 Jacopo/     (task_3 a ~2500ms, dopo task_1)
 sono        (task_2 a ~2800ms, partito a 2000ms)
```

## 🎯 Conclusione

Ora hai un sistema che supporta:
- ✅ Task che aspettano altri task (sequenziale con dipendenze)
- ✅ Task che partono a tempi precisi (temporizzato)
- ✅ Mix di entrambe le modalità
- ✅ Funziona con Docker e senza Docker
- ✅ Mantiene il supporto real-time

Buon lavoro! 🚀
