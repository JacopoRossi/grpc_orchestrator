# Verifica Esecuzione Real-Time

## 🔍 Come Verificare che i Task Siano Real-Time

Ci sono diversi modi per verificare che i task stiano effettivamente eseguendo con scheduling real-time.

---

## 1. 🖥️ Verifica con `chrt` (Scheduling Policy e Priorità)

Il comando `chrt` mostra la policy di scheduling e la priorità di un processo.

### Durante l'esecuzione:

```bash
# Trova i PID dei processi
ps aux | grep orchestrator_main
ps aux | grep task_main

# Verifica la policy e priorità
chrt -p <PID>
```

### Output atteso (real-time):
```
pid 12345's current scheduling policy: SCHED_FIFO
pid 12345's current scheduling priority: 80
```

### Output NON real-time:
```
pid 12345's current scheduling policy: SCHED_OTHER
pid 12345's current scheduling priority: 0
```

### Con Docker:

```bash
# Entra nel container
sudo docker exec -it grpc_orchestrator /bin/bash

# Trova il PID
ps aux | grep orchestrator_main

# Verifica
chrt -p <PID>
```

---

## 2. 📊 Verifica con `/proc/<PID>/sched`

Questo file mostra informazioni dettagliate sullo scheduling.

```bash
# Trova il PID
PID=$(pgrep orchestrator_main)

# Guarda le info di scheduling
cat /proc/$PID/sched
```

### Cosa cercare:

```
policy                       :                    1    ← 1 = SCHED_FIFO (real-time)
prio                         :                   20    ← Priorità kernel (99-priority)
```

- **policy = 0**: SCHED_OTHER (normale, NON real-time)
- **policy = 1**: SCHED_FIFO (real-time)
- **policy = 2**: SCHED_RR (real-time round-robin)

---

## 3. 🎯 Verifica CPU Affinity

Verifica che ogni processo sia legato al core CPU corretto.

```bash
# Trova il PID
PID=$(pgrep orchestrator_main)

# Verifica affinity
taskset -p $PID
```

### Output atteso:

```
pid 12345's current affinity mask: 1    ← Core 0 (orchestrator)
```

Decodifica della mask:
- `1` = 0001 binario = Core 0
- `2` = 0010 binario = Core 1
- `4` = 0100 binario = Core 2
- `8` = 1000 binario = Core 3

### Con Docker:

```bash
sudo docker exec -it grpc_orchestrator taskset -p $(pgrep orchestrator_main)
```

---

## 4. 📈 Verifica con `top` / `htop`

Usa `htop` per vedere la priorità real-time visivamente.

```bash
htop
```

### Cosa cercare:

- Premi `F5` (Tree view) per vedere la gerarchia
- Cerca la colonna **PRI** (Priority)
- Processi real-time hanno priorità **negativa** (es: -21, -26)
- Processi normali hanno priorità 20

### Calcolo priorità in htop:
```
htop_priority = 20 - rt_priority
```

Quindi:
- RT priority 80 → htop mostra -60
- RT priority 75 → htop mostra -55

---

## 5. 🔬 Test di Latenza e Jitter

Crea un test che misura la precisione del timing.

### Script di test:

```bash
#!/bin/bash
# test_realtime_precision.sh

echo "Testing real-time precision..."
echo "Starting orchestrator and tasks..."

# Avvia orchestrator e task
# ... (avvia i processi)

# Monitora i log e calcola jitter
docker-compose logs -f orchestrator | grep "Task.*started" | while read line; do
    timestamp=$(echo $line | grep -oP '\d{10,}')
    echo "Task started at: $timestamp us"
done
```

### Cosa verificare:

1. **Jitter basso**: La variazione tra esecuzioni dovrebbe essere minima (< 100 us)
2. **Timing preciso**: Task temporizzati devono partire esattamente al tempo schedulato
3. **Nessuna preemption**: Task real-time non devono essere interrotti

---

## 6. 📝 Verifica nei Log

I log dell'orchestrator mostrano se il real-time è attivo.

### Output con real-time ATTIVO:

```
[Main] Configuring real-time scheduling
[Orchestrator] Real-time configuration set:
  Policy: fifo
  Priority: 80
  CPU Affinity: 0
```

### Output SENZA real-time:

```
[Main] Running in non-real-time mode
```

---

## 7. 🐳 Verifica Configurazione Docker

Controlla che Docker abbia i permessi necessari.

```bash
# Verifica capabilities del container
sudo docker inspect grpc_orchestrator | grep -A 10 CapAdd
```

### Output atteso:

```json
"CapAdd": [
    "SYS_NICE"
],
```

### Verifica limiti:

```bash
sudo docker inspect grpc_orchestrator | grep -A 5 Ulimits
```

### Output atteso:

```json
"Ulimits": [
    {
        "Name": "rtprio",
        "Hard": 99,
        "Soft": 99
    },
    {
        "Name": "rttime",
        "Hard": -1,
        "Soft": -1
    }
]
```

---

## 8. 🧪 Test Pratico Completo

### Script di verifica automatica:

```bash
#!/bin/bash
# verify_realtime.sh

echo "=== Real-Time Verification ==="
echo ""

# 1. Verifica processi
echo "1. Checking processes..."
ORCH_PID=$(pgrep orchestrator_main)
if [ -z "$ORCH_PID" ]; then
    echo "❌ Orchestrator not running"
    exit 1
fi
echo "✅ Orchestrator PID: $ORCH_PID"

# 2. Verifica scheduling policy
echo ""
echo "2. Checking scheduling policy..."
POLICY=$(chrt -p $ORCH_PID | grep "scheduling policy" | awk '{print $NF}')
if [ "$POLICY" == "SCHED_FIFO" ] || [ "$POLICY" == "SCHED_RR" ]; then
    echo "✅ Real-time policy: $POLICY"
else
    echo "❌ NOT real-time: $POLICY"
fi

# 3. Verifica priorità
echo ""
echo "3. Checking priority..."
PRIO=$(chrt -p $ORCH_PID | grep "priority" | awk '{print $NF}')
if [ "$PRIO" -gt 0 ]; then
    echo "✅ Real-time priority: $PRIO"
else
    echo "❌ NOT real-time priority: $PRIO"
fi

# 4. Verifica CPU affinity
echo ""
echo "4. Checking CPU affinity..."
AFFINITY=$(taskset -p $ORCH_PID | awk '{print $NF}')
echo "✅ CPU affinity mask: $AFFINITY"

# 5. Verifica memory lock
echo ""
echo "5. Checking memory lock..."
LOCKED=$(grep VmLck /proc/$ORCH_PID/status | awk '{print $2}')
if [ "$LOCKED" -gt 0 ]; then
    echo "✅ Memory locked: ${LOCKED} kB"
else
    echo "⚠️  Memory not locked"
fi

echo ""
echo "=== Verification Complete ==="
```

### Esegui:

```bash
chmod +x verify_realtime.sh
sudo ./verify_realtime.sh
```

---

## 9. 📊 Monitoring Continuo

### Con `watch`:

```bash
# Monitora continuamente la priorità
watch -n 1 'ps -eLo pid,tid,class,rtprio,pri,nice,cmd | grep -E "orchestrator|task_main"'
```

### Output atteso:

```
  PID   TID CLS RTPRIO PRI  NI CMD
12345 12345  FF     80 120   - ./orchestrator_main
12346 12346  FF     75 115   - ./task_main
12347 12347  FF     75 115   - ./task_main
12348 12348  FF     75 115   - ./task_main
```

Legenda:
- **CLS**: Scheduling class
  - `FF` = SCHED_FIFO (real-time)
  - `RR` = SCHED_RR (real-time)
  - `TS` = SCHED_OTHER (normale)
- **RTPRIO**: Real-time priority (1-99)
- **PRI**: Kernel priority

---

## 10. 🎯 Test di Performance

### Misura il jitter:

```bash
# Esegui più volte e misura la variazione
for i in {1..10}; do
    sudo docker-compose up > log_$i.txt 2>&1
    sudo docker-compose down
    
    # Estrai i tempi
    grep "Started:" log_$i.txt | awk '{print $3}'
done
```

### Analizza i risultati:

- **Real-time**: Jitter < 100 us
- **Non real-time**: Jitter > 1000 us (1 ms)

---

## 🚀 Quick Check (Metodo Veloce)

```bash
# Avvia il sistema
sudo docker-compose up -d

# Verifica tutto in un comando
sudo docker exec grpc_orchestrator bash -c '
    PID=$(pgrep orchestrator_main)
    echo "PID: $PID"
    echo "Policy: $(chrt -p $PID | grep policy)"
    echo "Priority: $(chrt -p $PID | grep priority)"
    echo "Affinity: $(taskset -p $PID)"
'
```

---

## ✅ Checklist Completa

- [ ] Scheduling policy è SCHED_FIFO o SCHED_RR
- [ ] Priority è > 0 (es: 75-80)
- [ ] CPU affinity è impostata correttamente
- [ ] Memory è locked (VmLck > 0)
- [ ] Docker ha capability SYS_NICE
- [ ] Ulimits rtprio = 99
- [ ] Log mostra "Configuring real-time scheduling"
- [ ] Jitter è basso (< 100 us)
- [ ] Task temporizzati partono al tempo esatto

---

## 🐛 Troubleshooting

### Se NON è real-time:

1. **Verifica permessi**:
   ```bash
   sudo cat /etc/security/limits.conf | grep rtprio
   ```

2. **Verifica che usi sudo**:
   ```bash
   # SBAGLIATO:
   ./orchestrator_main
   
   # CORRETTO:
   sudo ./orchestrator_main --policy fifo --priority 80
   ```

3. **Verifica Docker capabilities**:
   ```bash
   sudo docker-compose down
   sudo docker-compose up
   ```

4. **Verifica kernel**:
   ```bash
   uname -a | grep PREEMPT
   ```

---

## 📈 Risultati Attesi

### Con Real-Time:
- ✅ Latenza: < 100 us
- ✅ Jitter: < 50 us
- ✅ Timing preciso: ±10 us
- ✅ Nessuna preemption

### Senza Real-Time:
- ❌ Latenza: > 1 ms
- ❌ Jitter: > 1 ms
- ❌ Timing impreciso: ±1 ms
- ❌ Preemption frequente

---

## 🎯 Conclusione

Usa questi metodi per verificare che il tuo sistema sia effettivamente real-time:

1. **Quick check**: `chrt -p <PID>` e verifica log
2. **Monitoring**: `htop` o `watch ps`
3. **Test completo**: Script `verify_realtime.sh`
4. **Performance**: Misura jitter e latenza

Buona verifica! 🚀
