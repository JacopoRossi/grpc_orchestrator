# Real-Time Configuration Guide

Questa guida spiega come configurare e utilizzare il sistema gRPC Orchestrator in modalità real-time.

## Indice

1. [Panoramica](#panoramica)
2. [Parametri Docker per Real-Time](#parametri-docker-per-real-time)
3. [Configurazione Applicazione](#configurazione-applicazione)
4. [Esempi di Utilizzo](#esempi-di-utilizzo)
5. [Troubleshooting](#troubleshooting)
6. [Best Practices](#best-practices)

## Panoramica

Il sistema supporta scheduling real-time tramite:
- **SCHED_FIFO**: First-In-First-Out scheduling (deterministico)
- **SCHED_RR**: Round-Robin scheduling (time-sliced)
- **Memory Locking**: Previene page faults bloccando la memoria in RAM
- **CPU Affinity**: Isola i task su core CPU specifici

## Parametri Docker per Real-Time

### Parametri Essenziali

```bash
docker run --rm \
  --cpuset-cpus=2 \
  --cap-add=SYS_NICE \
  --ulimit rtprio=99 \
  --ulimit rttime=-1 \
  --ulimit memlock=2048000000 \
  --network=host \
  <image_name> <command>
```

### Descrizione Parametri

#### `--cpuset-cpus=2`
- **Scopo**: Isola il container su specifici core CPU
- **Valore**: Lista di core CPU (es. `2`, `2-3`, `0,2,4`)
- **Perché**: Riduce interferenze da altri processi e migliora determinismo
- **Raccomandazione**: Usa core dedicati, evita core 0 (spesso usato dal sistema)

#### `--cap-add=SYS_NICE`
- **Scopo**: Permette al container di modificare priorità dei processi
- **Perché**: Necessario per impostare scheduling policy real-time (SCHED_FIFO/SCHED_RR)
- **Sicurezza**: Richiesto per chiamate `sched_setscheduler()` e `pthread_setschedparam()`

#### `--ulimit rtprio=99`
- **Scopo**: Imposta il limite massimo di priorità real-time
- **Valore**: 1-99 (99 = massima priorità)
- **Perché**: Permette di usare priorità real-time elevate
- **Default**: 0 (nessuna priorità RT consentita)

#### `--ulimit rttime=-1`
- **Scopo**: Rimuove il limite di tempo CPU per processi real-time
- **Valore**: `-1` (illimitato) o tempo in microsecondi
- **Perché**: Evita che il kernel termini processi RT che usano troppa CPU
- **Attenzione**: Può causare system hang se il processo RT ha bug

#### `--ulimit memlock=2048000000`
- **Scopo**: Aumenta il limite di memoria che può essere bloccata in RAM
- **Valore**: Bytes (es. 2GB = 2048000000)
- **Perché**: `mlockall()` richiede memoria sufficiente per bloccare tutte le pagine
- **Calcolo**: Deve essere >= memoria totale usata dall'applicazione

#### `--network=host`
- **Scopo**: Usa lo stack di rete dell'host invece di bridge virtuale
- **Perché**: Riduce latenza di rete eliminando overhead di virtualizzazione
- **Alternativa**: Usa bridge solo se necessario isolamento di rete

### Parametri Opzionali Raccomandati

#### `--cpu-rt-runtime=950000` e `--cpu-rt-period=1000000`
```bash
--cpu-rt-runtime=950000 \
--cpu-rt-period=1000000
```
- **Scopo**: Controlla quanto tempo CPU è disponibile per processi RT
- **Valore**: `rt_runtime` / `rt_period` = frazione di CPU (es. 95%)
- **Perché**: Previene che processi RT monopolizzino completamente la CPU
- **Default**: 95% del tempo CPU disponibile per RT

#### `--memory-swappiness=0`
```bash
--memory-swappiness=0
```
- **Scopo**: Disabilita lo swap di memoria
- **Perché**: Lo swap causa page faults imprevedibili (killer per RT)
- **Critico**: Essenziale per sistemi real-time

#### `--security-opt seccomp=unconfined`
```bash
--security-opt seccomp=unconfined
```
- **Scopo**: Disabilita il filtro seccomp
- **Perché**: Riduce overhead delle system call
- **Attenzione**: Riduce sicurezza, usa solo in ambienti controllati

### Comando Docker Completo Raccomandato

```bash
docker run --rm \
  --cpuset-cpus=2-3 \
  --cap-add=SYS_NICE \
  --ulimit rtprio=99 \
  --ulimit rttime=-1 \
  --ulimit memlock=4096000000 \
  --cpu-rt-runtime=950000 \
  --cpu-rt-period=1000000 \
  --memory-swappiness=0 \
  --network=host \
  --security-opt seccomp=unconfined \
  <image_name> <command>
```

## Configurazione Applicazione

### Orchestrator

Avvia l'orchestrator con parametri real-time:

```bash
# Modalità FIFO con priorità 80 su CPU 2
./orchestrator_main \
  --address 0.0.0.0:50050 \
  --policy fifo \
  --priority 80 \
  --cpu-affinity 2 \
  --lock-memory

# Modalità Round-Robin
./orchestrator_main \
  --address 0.0.0.0:50050 \
  --policy rr \
  --priority 70 \
  --cpu-affinity 2 \
  --lock-memory
```

### Task Wrapper

Avvia i task con parametri real-time:

```bash
# Task con priorità 75 su CPU 3
./task_main \
  --name task_1 \
  --address localhost:50051 \
  --orchestrator localhost:50050 \
  --policy fifo \
  --priority 75 \
  --cpu-affinity 3 \
  --lock-memory

# Backward compatibility (senza RT)
./task_main task_1 localhost:50051 localhost:50050
```

### Parametri Applicazione

| Parametro | Descrizione | Valori | Default |
|-----------|-------------|--------|---------|
| `--policy` | Scheduling policy | `none`, `fifo`, `rr` | `none` |
| `--priority` | Priorità RT | 1-99 | 50 |
| `--cpu-affinity` | Core CPU | 0-N, -1=nessuno | -1 |
| `--lock-memory` | Blocca memoria | flag | false |

## Esempi di Utilizzo

### Esempio 1: Container Orchestrator Real-Time

```bash
docker run --rm \
  --name orchestrator \
  --cpuset-cpus=2 \
  --cap-add=SYS_NICE \
  --ulimit rtprio=99 \
  --ulimit rttime=-1 \
  --ulimit memlock=2048000000 \
  --network=host \
  grpc-orchestrator:latest \
  /app/bin/orchestrator_main \
    --address 0.0.0.0:50050 \
    --policy fifo \
    --priority 80 \
    --cpu-affinity 2 \
    --lock-memory
```

### Esempio 2: Container Task Real-Time

```bash
docker run --rm \
  --name task_1 \
  --cpuset-cpus=3 \
  --cap-add=SYS_NICE \
  --ulimit rtprio=99 \
  --ulimit rttime=-1 \
  --ulimit memlock=2048000000 \
  --network=host \
  grpc-task:latest \
  /app/bin/task_main \
    --name task_1 \
    --address 0.0.0.0:50051 \
    --orchestrator host.docker.internal:50050 \
    --policy fifo \
    --priority 75 \
    --cpu-affinity 3 \
    --lock-memory
```

### Esempio 3: Docker Compose con Real-Time

```yaml
version: '3.8'

services:
  orchestrator:
    image: grpc-orchestrator:latest
    command: >
      /app/bin/orchestrator_main
      --address 0.0.0.0:50050
      --policy fifo
      --priority 80
      --cpu-affinity 2
      --lock-memory
    cpuset: "2"
    cap_add:
      - SYS_NICE
    ulimits:
      rtprio: 99
      rttime: -1
      memlock: 2048000000
    network_mode: host
    security_opt:
      - seccomp:unconfined

  task_1:
    image: grpc-task:latest
    command: >
      /app/bin/task_main
      --name task_1
      --address 0.0.0.0:50051
      --orchestrator localhost:50050
      --policy fifo
      --priority 75
      --cpu-affinity 3
      --lock-memory
    cpuset: "3"
    cap_add:
      - SYS_NICE
    ulimits:
      rtprio: 99
      rttime: -1
      memlock: 2048000000
    network_mode: host
    security_opt:
      - seccomp:unconfined
    depends_on:
      - orchestrator
```

## Troubleshooting

### Errore: "Failed to lock memory"

**Causa**: Limite `memlock` insufficiente o mancanza di privilegi

**Soluzione**:
```bash
# Aumenta ulimit memlock
--ulimit memlock=4096000000

# Oppure esegui come privileged (non raccomandato)
--privileged

# Verifica limite attuale nel container
docker exec <container> ulimit -l
```

### Errore: "Failed to set scheduling policy: Operation not permitted"

**Causa**: Mancanza capability `SYS_NICE`

**Soluzione**:
```bash
# Aggiungi capability
--cap-add=SYS_NICE

# Verifica capabilities nel container
docker exec <container> capsh --print
```

### Errore: "Priority out of range"

**Causa**: Priorità non valida o `rtprio` ulimit troppo basso

**Soluzione**:
```bash
# Imposta rtprio corretto
--ulimit rtprio=99

# Usa priorità valida (1-99)
--priority 50
```

### Warning: "SCHED_DEADLINE not supported"

**Causa**: Kernel Linux < 3.14

**Soluzione**: Usa `fifo` o `rr` invece di `deadline`

### Latenza Elevata

**Possibili cause**:
1. CPU non isolate: usa `isolcpus` nel kernel boot
2. Interrupts su core RT: usa `irqaffinity` per spostare IRQ
3. Memory non bloccata: aggiungi `--lock-memory`
4. Swap attivo: aggiungi `--memory-swappiness=0`

## Best Practices

### 1. Isolamento CPU

```bash
# Nel kernel boot parameters (GRUB)
isolcpus=2,3 nohz_full=2,3 rcu_nocbs=2,3

# Verifica isolamento
cat /sys/devices/system/cpu/isolated
```

### 2. Priorità Gerarchiche

- **Orchestrator**: Priorità più alta (es. 80)
- **Task critici**: Priorità media-alta (es. 70-75)
- **Task non-critici**: Priorità bassa (es. 50-60)

### 3. Memory Locking

Sempre usare `--lock-memory` per applicazioni RT:
- Previene page faults
- Garantisce latenza prevedibile
- Richiede `memlock` ulimit adeguato

### 4. CPU Affinity

- Usa core dedicati per RT
- Evita core 0 (usato dal sistema)
- Separa orchestrator e task su core diversi

### 5. Monitoring

```bash
# Verifica scheduling policy
chrt -p <pid>

# Monitora latenza
cyclictest -p 80 -t1 -n -i 1000 -l 100000

# Verifica CPU affinity
taskset -p <pid>

# Controlla memory lock
cat /proc/<pid>/status | grep VmLck
```

### 6. Testing

Prima di deployment in produzione:
1. Testa con `cyclictest` per misurare latenza
2. Verifica che memory locking funzioni
3. Controlla che CPU affinity sia applicata
4. Monitora jitter e worst-case latency

### 7. Sicurezza

- `--ulimit rttime=-1` può causare system hang
- Usa `--cpu-rt-runtime` per limitare tempo CPU RT
- Evita `--privileged` se possibile
- Considera `seccomp=unconfined` solo in ambienti sicuri

## Verifica Configurazione

Script per verificare che tutto sia configurato correttamente:

```bash
#!/bin/bash

echo "=== Real-Time Configuration Check ==="

# Check capabilities
echo -n "SYS_NICE capability: "
docker exec <container> capsh --print | grep -q sys_nice && echo "OK" || echo "MISSING"

# Check ulimits
echo "RT Priority limit: $(docker exec <container> ulimit -r)"
echo "RT Time limit: $(docker exec <container> ulimit -R)"
echo "Memlock limit: $(docker exec <container> ulimit -l)"

# Check scheduling
PID=$(docker inspect -f '{{.State.Pid}}' <container>)
echo "Scheduling policy: $(chrt -p $PID)"

# Check CPU affinity
echo "CPU affinity: $(taskset -p $PID)"

# Check memory lock
echo "Locked memory: $(cat /proc/$PID/status | grep VmLck)"
```

## Riferimenti

- [Linux Real-Time Documentation](https://www.kernel.org/doc/html/latest/scheduler/sched-rt-group.html)
- [Docker Runtime Options](https://docs.docker.com/engine/reference/run/)
- [POSIX Real-Time](https://pubs.opengroup.org/onlinepubs/9699919799/basedefs/sched.h.html)
