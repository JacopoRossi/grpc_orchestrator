# Guida Completa - gRPC Orchestrator

Questa guida ti spiega **passo dopo passo** come eseguire il progetto gRPC Orchestrator, partendo da zero. Non serve sapere niente in anticipo!

---

## 📋 Indice

1. [Cos'è questo progetto?](#cosè-questo-progetto)
2. [Cosa ti serve prima di iniziare](#cosa-ti-serve-prima-di-iniziare)
3. [Metodo 1: Esecuzione con Docker (CONSIGLIATO)](#metodo-1-esecuzione-con-docker-consigliato)
4. [Metodo 2: Esecuzione senza Docker (Nativa)](#metodo-2-esecuzione-senza-docker-nativa)
5. [Come verificare che funziona](#come-verificare-che-funziona)
6. [Come fermare tutto](#come-fermare-tutto)
7. [Risoluzione problemi](#risoluzione-problemi)

---

## Cos'è questo progetto?

Questo è un **orchestratore di task** che usa gRPC per comunicare. In pratica:

- C'è un **Orchestrator** (il capo) che decide quando far partire i task
- Ci sono 3 **Task** (i lavoratori) che aspettano ordini dall'orchestrator
- Quando l'orchestrator dice "vai", un task parte, fa il suo lavoro, e poi dice "ho finito"

Tutto questo avviene tramite comunicazione di rete usando gRPC.

---

## Cosa ti serve prima di iniziare

### Se usi Docker (più facile):
- **Docker** installato sul tuo computer
- **Docker Compose** installato (di solito viene con Docker)

### Se NON usi Docker (più complicato):
- **Linux** (Ubuntu 20.04 o superiore consigliato)
- Accesso a internet per scaricare le dipendenze
- Permessi sudo (per installare pacchetti)

---

## Metodo 1: Esecuzione con Docker (CONSIGLIATO)

Docker è **più facile** perché non devi installare niente manualmente. Docker crea dei "container" (come piccoli computer virtuali) dove tutto è già configurato.

### Passo 1: Installa Docker

Se non hai Docker installato:

#### Su Ubuntu/Debian:
```bash
# Aggiorna il sistema
sudo apt-get update

# Installa Docker
sudo apt-get install -y docker.io docker-compose

# Avvia Docker
sudo systemctl start docker
sudo systemctl enable docker

# Aggiungi il tuo utente al gruppo docker (così non serve sudo ogni volta)
sudo usermod -aG docker $USER

# IMPORTANTE: Esci e rientra nel terminale per applicare le modifiche
```

#### Verifica che Docker funzioni:
```bash
docker --version
docker-compose --version
```

Dovresti vedere le versioni installate.

### Passo 2: Vai nella cartella del progetto

```bash
cd /home/vboxuser/projects/grpc_orchestrator
```

### Passo 3: Costruisci le immagini Docker

Le "immagini" sono come modelli che Docker usa per creare i container.

```bash
docker-compose build
```

Questo comando:
- Legge i file `Dockerfile.orchestrator` e `Dockerfile.task`
- Scarica Ubuntu
- Installa tutte le dipendenze necessarie
- Compila il codice C++
- Crea le immagini pronte all'uso

**Tempo richiesto**: 5-10 minuti la prima volta (dipende dalla tua connessione internet)

### Passo 4: Avvia tutti i container

```bash
docker-compose up
```

Questo comando:
- Crea 4 container: 1 orchestrator + 3 task
- Li avvia tutti insieme
- Li connette in una rete virtuale
- Mostra i log di tutti nel terminale

**Cosa vedrai**: Un sacco di messaggi che scorrono. È normale! Sono i log di tutti i container.

### Passo 5: Osserva l'esecuzione

Dovresti vedere output come:

```
orchestrator_1  | === gRPC Orchestrator ===
orchestrator_1  | [Orchestrator] Starting orchestrator on 0.0.0.0:50050
task1_1         | === gRPC Task Wrapper ===
task1_1         | Task ID: task_1
task1_1         | [Task task_1] Starting task wrapper on 0.0.0.0:50051
...
orchestrator_1  | [Orchestrator] Task task_1 started successfully
task1_1         | /ciao
task2_1         |  sono 
task3_1         |  Jacopo/
```

### Passo 6: Ferma tutto

Quando hai finito, premi `Ctrl+C` nel terminale.

Per fermare e rimuovere tutti i container:

```bash
docker-compose down
```

---

## Metodo 2: Esecuzione senza Docker (Nativa)

Questo metodo è più complicato ma ti dà più controllo. Dovrai installare tutte le dipendenze manualmente.

### Passo 1: Installa le dipendenze di base

```bash
# Aggiorna il sistema
sudo apt-get update

# Installa gli strumenti di compilazione
sudo apt-get install -y build-essential cmake git pkg-config
```

### Passo 2: Installa gRPC e Protobuf

Hai due opzioni:

#### Opzione A: Installazione dai repository (più veloce ma versione più vecchia)

```bash
sudo apt-get install -y \
    libgrpc++-dev \
    libprotobuf-dev \
    protobuf-compiler-grpc
```

#### Opzione B: Compilazione da sorgente (più lento ma versione più recente)

```bash
# Scarica gRPC
cd ~
git clone --recurse-submodules -b v1.58.0 https://github.com/grpc/grpc
cd grpc

# Crea directory di build
mkdir -p cmake/build
cd cmake/build

# Configura
cmake -DgRPC_INSTALL=ON \
      -DgRPC_BUILD_TESTS=OFF \
      -DCMAKE_BUILD_TYPE=Release \
      ../..

# Compila (ci vorrà un po'...)
make -j$(nproc)

# Installa
sudo make install

# Aggiorna la cache delle librerie
sudo ldconfig
```

**Tempo richiesto**: 30-60 minuti per l'opzione B

### Passo 3: Vai nella cartella del progetto

```bash
cd /home/vboxuser/projects/grpc_orchestrator
```

### Passo 4: Compila il progetto

```bash
# Crea la directory di build
mkdir -p build
cd build

# Configura con CMake
cmake ..

# Compila
make -j$(nproc)
```

Se tutto va bene, vedrai:

```
[100%] Built target orchestrator_main
[100%] Built target task_main
```

I programmi compilati saranno in `build/bin/`:
- `orchestrator_main` - L'orchestrator
- `task_main` - Il programma per i task

### Passo 5: Configura i permessi real-time

**IMPORTANTE**: Per usare lo scheduling real-time, devi configurare i limiti di sistema.

```bash
# Aggiungi limiti real-time per il tuo utente
sudo bash -c 'cat >> /etc/security/limits.conf << EOF
$USER soft rtprio 99
$USER hard rtprio 99
$USER soft memlock unlimited
$USER hard memlock unlimited
EOF'

# Esci e rientra nel terminale per applicare le modifiche
```

### Passo 6: Esegui il programma con real-time

Ora devi aprire **4 terminali separati** (uno per l'orchestrator e tre per i task).

**NOTA**: Tutti i comandi includono i parametri real-time (policy FIFO, priorità, CPU affinity).

#### Terminale 1 - Orchestrator

```bash
cd /home/vboxuser/projects/grpc_orchestrator
sudo ./build/bin/orchestrator_main \
  --address 0.0.0.0:50050 \
  --policy fifo \
  --priority 80 \
  --cpu-affinity 0 \
  --lock-memory
```

Vedrai:
```
=== gRPC Orchestrator ===
[Main] Configuring real-time scheduling
[Orchestrator] Starting orchestrator on 0.0.0.0:50050
[Orchestrator] gRPC server listening on 0.0.0.0:50050
```

#### Terminale 2 - Task 1

```bash
cd /home/vboxuser/projects/grpc_orchestrator
sudo ./build/bin/task_main \
  --name task_1 \
  --address localhost:50051 \
  --orchestrator localhost:50050 \
  --policy fifo \
  --priority 75 \
  --cpu-affinity 1 \
  --lock-memory
```

#### Terminale 3 - Task 2

```bash
cd /home/vboxuser/projects/grpc_orchestrator
sudo ./build/bin/task_main \
  --name task_2 \
  --address localhost:50052 \
  --orchestrator localhost:50050 \
  --policy fifo \
  --priority 75 \
  --cpu-affinity 2 \
  --lock-memory
```

#### Terminale 4 - Task 3

```bash
cd /home/vboxuser/projects/grpc_orchestrator
sudo ./build/bin/task_main \
  --name task_3 \
  --address localhost:50053 \
  --orchestrator localhost:50050 \
  --policy fifo \
  --priority 75 \
  --cpu-affinity 3 \
  --lock-memory
```

### Passo 7: Osserva l'esecuzione

Dopo qualche secondo, vedrai i task partire in sequenza. Ogni task stamperà il suo messaggio:

- Task 1: `/ciao`
- Task 2: ` sono `
- Task 3: ` Jacopo/`

### Passo 8: Ferma tutto

In ogni terminale, premi `Ctrl+C` per fermare il programma.

---

## Come verificare che funziona

### Con Docker:

1. Controlla che i container siano in esecuzione:
```bash
docker ps
```

Dovresti vedere 4 container: `grpc_orchestrator`, `grpc_task1`, `grpc_task2`, `grpc_task3`

2. Guarda i log di un singolo container:
```bash
docker logs grpc_orchestrator
docker logs grpc_task1
```

### Senza Docker:

1. Verifica che i processi siano attivi:
```bash
ps aux | grep orchestrator_main
ps aux | grep task_main
```

2. Verifica che le porte siano in ascolto:
```bash
netstat -tulpn | grep 5005
```

Dovresti vedere le porte 50050, 50051, 50052, 50053 in LISTEN.

---

## Come fermare tutto

### Con Docker:

```bash
# Ferma i container (mantiene i dati)
docker-compose stop

# Ferma e rimuove i container
docker-compose down

# Rimuovi anche le immagini (per risparmiare spazio)
docker-compose down --rmi all
```

### Senza Docker:

Premi `Ctrl+C` in ogni terminale dove hai avviato un programma.

Se i processi non si fermano:

```bash
# Trova i processi
ps aux | grep orchestrator_main
ps aux | grep task_main

# Uccidi i processi (sostituisci <PID> con il numero del processo)
kill <PID>

# Se non funziona, forza la terminazione
kill -9 <PID>
```

---

## Risoluzione problemi

### Problema: "docker: command not found"

**Soluzione**: Docker non è installato. Torna al Passo 1 del Metodo 1.

### Problema: "permission denied" quando usi docker

**Soluzione**: 
```bash
# Aggiungi il tuo utente al gruppo docker
sudo usermod -aG docker $USER

# Esci e rientra nel terminale
exit
# Poi riapri il terminale
```

### Problema: "port already in use" o "address already in use"

**Soluzione**: Una porta è già occupata da un altro programma.

```bash
# Trova cosa sta usando la porta 50050
sudo lsof -i :50050

# Uccidi il processo
sudo kill <PID>
```

### Problema: Compilazione fallisce con "gRPC not found"

**Soluzione**: gRPC non è installato correttamente.

```bash
# Verifica che gRPC sia installato
pkg-config --modversion grpc++

# Se non funziona, reinstalla
sudo apt-get install --reinstall libgrpc++-dev
```

### Problema: I task non si connettono all'orchestrator

**Soluzione**: 

1. Verifica che l'orchestrator sia partito per primo
2. Controlla che gli indirizzi siano corretti
3. Verifica che non ci sia un firewall che blocca le connessioni

```bash
# Verifica connettività
telnet localhost 50050
```

### Problema: Docker build è lentissimo

**Soluzione**: È normale la prima volta. Docker deve scaricare Ubuntu e compilare tutto. Le volte successive sarà molto più veloce perché usa la cache.

### Problema: "No space left on device"

**Soluzione**: Disco pieno.

```bash
# Pulisci immagini Docker inutilizzate
docker system prune -a

# Libera spazio nella build directory
cd /home/vboxuser/projects/grpc_orchestrator
rm -rf build
```

---

## Comandi utili

### Docker:

```bash
# Vedi tutti i container (anche quelli fermati)
docker ps -a

# Vedi i log in tempo reale
docker-compose logs -f

# Vedi i log di un solo container
docker-compose logs -f orchestrator

# Entra in un container in esecuzione
docker exec -it grpc_orchestrator /bin/bash

# Ricostruisci tutto da zero
docker-compose build --no-cache

# Avvia in background (senza vedere i log)
docker-compose up -d

# Ferma tutto
docker-compose down
```

### Compilazione nativa:

```bash
# Ricompila solo se ci sono modifiche
cd build
make

# Ricompila tutto da zero
cd /home/vboxuser/projects/grpc_orchestrator
rm -rf build
mkdir build
cd build
cmake ..
make -j$(nproc)

# Compila in modalità debug (con simboli di debug)
cmake -DCMAKE_BUILD_TYPE=Debug ..
make

# Compila in modalità release (ottimizzato)
cmake -DCMAKE_BUILD_TYPE=Release ..
make
```

---

## Parametri real-time spiegati

### Parametri usati:

- **`--policy fifo`**: Usa lo scheduling FIFO real-time (First In First Out)
- **`--priority <N>`**: Priorità real-time (1-99, più alto = più priorità)
  - Orchestrator: 80 (priorità alta)
  - Task: 75 (priorità leggermente inferiore)
- **`--cpu-affinity <N>`**: Lega il processo a un core CPU specifico
  - Orchestrator: core 0
  - Task 1: core 1
  - Task 2: core 2
  - Task 3: core 3
- **`--lock-memory`**: Blocca la memoria in RAM (evita page fault)

### Altri parametri disponibili:

```bash
# Vedi tutti i parametri
./build/bin/orchestrator_main --help
./build/bin/task_main --help

# Usa policy Round-Robin invece di FIFO
--policy rr

# Disabilita real-time (non consigliato)
--policy none
```

---

## Riepilogo veloce

### Con Docker:
```bash
cd /home/vboxuser/projects/grpc_orchestrator
docker-compose build
docker-compose up
# Premi Ctrl+C per fermare
docker-compose down
```

### Senza Docker (con real-time):
```bash
# Una volta sola: compila
cd /home/vboxuser/projects/grpc_orchestrator
mkdir build && cd build
cmake .. && make -j$(nproc)

# Ogni volta: apri 4 terminali ed esegui con sudo
# Terminale 1 (Orchestrator):
sudo ./build/bin/orchestrator_main --address 0.0.0.0:50050 --policy fifo --priority 80 --cpu-affinity 0 --lock-memory

# Terminale 2 (Task 1):
sudo ./build/bin/task_main --name task_1 --address localhost:50051 --orchestrator localhost:50050 --policy fifo --priority 75 --cpu-affinity 1 --lock-memory

# Terminale 3 (Task 2):
sudo ./build/bin/task_main --name task_2 --address localhost:50052 --orchestrator localhost:50050 --policy fifo --priority 75 --cpu-affinity 2 --lock-memory

# Terminale 4 (Task 3):
sudo ./build/bin/task_main --name task_3 --address localhost:50053 --orchestrator localhost:50050 --policy fifo --priority 75 --cpu-affinity 3 --lock-memory
```

---

## Domande frequenti

**Q: Quale metodo devo usare?**  
A: Se sei alle prime armi, usa Docker. È più semplice e funziona sempre.

**Q: Posso usare Windows?**  
A: Con Docker sì (Docker Desktop). Senza Docker è molto complicato, meglio usare Linux.

**Q: Quanto spazio serve?**  
A: Con Docker circa 2-3 GB. Senza Docker circa 500 MB.

**Q: Posso modificare il codice?**  
A: Sì! Dopo le modifiche:
- Con Docker: `docker-compose build` e poi `docker-compose up`
- Senza Docker: `cd build && make`

**Q: Come faccio a vedere solo i log dell'orchestrator?**  
A: Con Docker: `docker-compose logs -f orchestrator`

**Q: I task devono partire in un ordine specifico?**  
A: Sì, l'orchestrator deve partire per primo. I task possono partire in qualsiasi ordine dopo.

**Q: Posso cambiare le porte?**  
A: Sì, modifica i parametri `--address` quando avvii i programmi. Con Docker, modifica `docker-compose.yml`.

**Q: Perché serve sudo per eseguire i programmi?**  
A: Lo scheduling real-time richiede privilegi elevati. Serve sudo per impostare priorità real-time e bloccare la memoria.

**Q: Posso eseguire senza real-time?**  
A: Sì, rimuovi tutti i parametri `--policy`, `--priority`, `--cpu-affinity`, `--lock-memory` e non serve sudo. Ma le performance saranno inferiori.

**Q: Cosa significa CPU affinity?**  
A: Lega ogni processo a un core CPU specifico, evitando che il sistema operativo sposti i processi tra core (migliora le performance real-time).

**Q: Quanti core CPU servono?**  
A: Minimo 4 core (1 per orchestrator + 3 per i task). Con meno core, riduci i valori di `--cpu-affinity` o rimuovi il parametro.

---

## Conclusione

Ora dovresti essere in grado di eseguire il progetto! Se hai problemi:

1. Rileggi la sezione "Risoluzione problemi"
2. Controlla i log per vedere gli errori
3. Verifica che tutte le dipendenze siano installate
4. Prova a ricompilare da zero

Buon lavoro! 🚀
