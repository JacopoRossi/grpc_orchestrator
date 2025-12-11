# Configurazione Real-Time

## Descrizione

Orchestrator e tutti i task sono configurati per eseguire in **modalità real-time** con scheduling FIFO.

## Configurazioni Real-Time Attive

### Orchestrator
- **Policy**: FIFO (First-In-First-Out)
- **Priority**: 80 (alta priorità)
- **CPU Affinity**: CPU 0 (dedicato)
- **cpuset**: "0" (isolamento CPU)

### Task 1
- **Policy**: FIFO
- **Priority**: 75
- **CPU Affinity**: CPU 1 (dedicato)
- **cpuset**: "1"

### Task 2
- **Policy**: FIFO
- **Priority**: 75
- **CPU Affinity**: CPU 2 (dedicato)
- **cpuset**: "2"

### Task 3
- **Policy**: FIFO
- **Priority**: 75
- **CPU Affinity**: CPU 3 (dedicato)
- **cpuset**: "3"

## Caratteristiche

### ✅ Scheduling Real-Time FIFO
- L'orchestrator ha priorità 80 (più alta dei task)
- I task hanno priorità 75
- Scheduling deterministico e prevedibile

### ✅ CPU Affinity
- Ogni componente è vincolato a una CPU specifica
- Riduce context switching e cache misses
- Migliora le prestazioni real-time

### ✅ Isolamento CPU
- Ogni container usa un cpuset dedicato
- Previene interferenze tra i componenti

### ✅ Capabilities
- `CAP_SYS_NICE`: permette di modificare le priorità dei processi
- Necessario per applicare scheduling real-time

### ✅ Ulimits
- `rtprio: 99`: priorità real-time massima consentita
- `rttime: -1`: tempo CPU illimitato per processi RT
- `memlock: 2048000000`: limite memoria lockabile (2GB)

## Note sul Memory Locking

Il **memory locking** (`--lock-memory`) è stato **disabilitato** per evitare segmentation fault. 

Le configurazioni RT attive sono:
- ✅ FIFO scheduling policy
- ✅ Priority assignment
- ✅ CPU affinity
- ❌ Memory locking (disabilitato per stabilità)

## Verifica Configurazioni

### Verifica Orchestrator
```bash
sudo docker logs grpc_orchestrator | grep -E "(Real-time|FIFO|Priority|CPU Affinity)"
```

Output atteso:
```
[Orchestrator] Real-time configuration set:
  Policy: FIFO
  Priority: 80
  CPU Affinity: 0
[RTUtils] Set CPU affinity to CPU 0
[RTUtils] Set thread to FIFO with priority 80
[RTUtils] Real-time configuration applied successfully
```

### Verifica Task
```bash
sudo docker logs grpc_task1 | grep -E "(Real-time|FIFO|Priority|CPU Affinity)"
```

Output atteso:
```
[Task task_1] Real-time configuration set:
  Policy: FIFO
  Priority: 75
  CPU Affinity: 1
[RTUtils] Set CPU affinity to CPU 1
[RTUtils] Set thread to FIFO with priority 75
[RTUtils] Real-time configuration applied successfully
```

## Esecuzione Sequenziale con RT

L'esecuzione rimane **strettamente sequenziale** anche in modalità real-time:

1. **Task 1** (CPU 1, Priority 75) → esegue → completa
2. **Task 2** (CPU 2, Priority 75) → esegue → completa
3. **Task 3** (CPU 3, Priority 75) → esegue → completa

L'**orchestrator** (CPU 0, Priority 80) coordina l'esecuzione sequenziale con priorità più alta.

## Requisiti Sistema

Per utilizzare le configurazioni real-time:

1. **Kernel Linux** con supporto real-time
2. **Docker** con privilegi sufficienti (sudo)
3. **CPU multi-core** (almeno 4 core per questa configurazione)
4. **Capabilities** appropriate nel sistema host

## Avvio

```bash
# Avvio con configurazioni RT
sudo docker-compose up

# Verifica che tutto funzioni
sudo docker ps -a

# Visualizza log
sudo docker logs grpc_orchestrator
sudo docker logs grpc_task1

# Stop
sudo docker-compose down
```

## Troubleshooting

### Segmentation Fault (Exit 139)
Se l'orchestrator crasha con exit code 139:
- Verificare che `--lock-memory` sia disabilitato
- Controllare i limiti di memoria del sistema
- Verificare le capabilities del container

### Priorità Non Applicate
Se le priorità RT non vengono applicate:
- Verificare `CAP_SYS_NICE` nel docker-compose.yml
- Controllare i limiti ulimit (rtprio, rttime)
- Eseguire con sudo

### CPU Affinity Non Funziona
Se l'affinity non viene applicata:
- Verificare che il sistema abbia abbastanza CPU
- Controllare il parametro `cpuset` nel docker-compose.yml
- Verificare con `docker stats` l'utilizzo CPU
