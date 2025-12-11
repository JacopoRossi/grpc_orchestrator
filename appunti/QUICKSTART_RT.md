# Quick Start - Real-Time Mode

Guida rapida per eseguire il sistema con configurazione real-time.

## 🚀 Esecuzione Rapida

### 1. Build delle immagini Docker
```bash
./docker-build.sh
```

### 2. Avvia il sistema
```bash
./docker-run.sh
```

Fatto! Il sistema è ora in esecuzione con configurazione real-time.

## 📊 Configurazione Real-Time Applicata

### Orchestrator
- **Scheduling Policy**: SCHED_FIFO
- **Priority**: 80
- **CPU Affinity**: CPU 0
- **Memory Locking**: Abilitato
- **RT Priority Limit**: 99

### Tasks (task_1, task_2, task_3)
- **Scheduling Policy**: SCHED_FIFO
- **Priority**: 75
- **CPU Affinity**: CPU 1, 2, 3 (uno per task)
- **Memory Locking**: Abilitato
- **RT Priority Limit**: 99

### Parametri Docker
Ogni container è configurato con:
```yaml
cpuset: "N"                    # CPU dedicata
cap_add: [SYS_NICE]           # Capability per RT scheduling
ulimits:
  rtprio: 99                   # Max RT priority
  rttime: -1                   # Unlimited RT time
  memlock: 2048000000          # 2GB memory lock limit
security_opt:
  - seccomp:unconfined         # Riduce overhead syscall
```

## 🔍 Verifica Configurazione

Verifica che la configurazione RT sia applicata:
```bash
./docker-verify-rt.sh
```

Visualizza i log per confermare l'applicazione RT:
```bash
# Orchestrator
docker logs grpc_orchestrator | grep -i "real-time\|policy\|priority"

# Task 1
docker logs grpc_task1 | grep -i "real-time\|policy\|priority"
```

## 🛠️ Comandi Utili

### Avvia in background
```bash
docker-compose up -d
```

### Visualizza log in tempo reale
```bash
# Tutti i servizi
docker-compose logs -f

# Solo orchestrator
docker logs -f grpc_orchestrator

# Solo task 1
docker logs -f grpc_task1
```

### Ferma il sistema
```bash
docker-compose down
```

### Riavvia un singolo servizio
```bash
docker-compose restart orchestrator
docker-compose restart task1
```

### Verifica stato servizi
```bash
docker-compose ps
```

## 📈 Monitoring Real-Time

### Verifica scheduling policy di un container
```bash
# Get container PID
PID=$(docker inspect -f '{{.State.Pid}}' grpc_orchestrator)

# Check scheduling
chrt -p $PID

# Check CPU affinity
taskset -cp $PID

# Check memory lock
cat /proc/$PID/status | grep VmLck
```

### Verifica priorità dei thread
```bash
# Lista tutti i thread con priorità
ps -eLo pid,tid,class,rtprio,comm | grep -E 'orchestrator|task_main'
```

### Monitora latenza (richiede cyclictest)
```bash
# Installa rt-tests se necessario
sudo apt-get install rt-tests

# Test latenza
sudo cyclictest -p 80 -t1 -n -i 1000 -l 100000
```

## ⚙️ Personalizzazione

### Modifica parametri RT

Edita `docker-compose.yml` per cambiare:

**Policy** (fifo, rr, none):
```yaml
--policy fifo
```

**Priority** (1-99):
```yaml
--priority 80
```

**CPU Affinity**:
```yaml
cpuset: "2"           # Docker level
--cpu-affinity 2      # Application level
```

**Memory Locking**:
```yaml
--lock-memory         # Abilita
# Rimuovi flag per disabilitare
```

### Esempio: Orchestrator con priorità 90 su CPU 4
```yaml
orchestrator:
  command: >
    ./orchestrator_main
    --address 0.0.0.0:50050
    --policy fifo
    --priority 90
    --cpu-affinity 0
    --lock-memory
  cpuset: "4"
```

## 🐛 Troubleshooting

### Errore: "Failed to lock memory"
```bash
# Aumenta memlock limit in docker-compose.yml
ulimits:
  memlock: 4096000000  # 4GB invece di 2GB
```

### Errore: "Operation not permitted" per scheduling
```bash
# Verifica che SYS_NICE capability sia presente
docker exec grpc_orchestrator capsh --print | grep sys_nice
```

### Container non si avvia
```bash
# Verifica log
docker logs grpc_orchestrator

# Verifica che i core CPU esistano
nproc  # Mostra numero di CPU disponibili
```

### Latenza elevata
1. Verifica che il sistema host non abbia altri processi pesanti
2. Controlla che i core CPU siano effettivamente isolati
3. Verifica che memory locking sia attivo
4. Considera di isolare i core CPU nel kernel boot (vedi REALTIME.md)

## 📚 Documentazione Completa

Per informazioni dettagliate su:
- Configurazione avanzata real-time
- Parametri Docker in dettaglio
- Best practices per sistemi RT
- Troubleshooting approfondito

Consulta: **[REALTIME.md](REALTIME.md)**

## 🔄 Workflow Tipico

```bash
# 1. Build (solo la prima volta o dopo modifiche)
./docker-build.sh

# 2. Avvia sistema
./docker-run.sh

# 3. In un altro terminale, verifica RT config
./docker-verify-rt.sh

# 4. Monitora log
docker-compose logs -f

# 5. Quando hai finito, ferma tutto
# Premi Ctrl+C nel terminale di docker-run.sh
# Oppure:
docker-compose down
```

## 💡 Note Importanti

⚠️ **CPU Affinity**: I container usano CPU 0-3. Se il tuo sistema ha meno di 4 core, modifica `cpuset` in `docker-compose.yml`

⚠️ **Memoria**: Ogni container può bloccare fino a 2GB di RAM. Assicurati di avere RAM sufficiente.

⚠️ **Sicurezza**: `seccomp:unconfined` riduce la sicurezza. Rimuovilo in ambienti di produzione non controllati.

⚠️ **RT Time Limit**: `rttime: -1` (illimitato) può causare system hang se il codice RT ha bug. Usa con cautela.
