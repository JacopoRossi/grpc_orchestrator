# Docker Deployment Guide

Questa guida spiega come eseguire l'orchestrator e i task in container Docker separati.

## Architettura Docker

Il sistema è composto da 4 container:
- **orchestrator**: Container che esegue l'orchestrator
- **task1**: Container per il task 1
- **task2**: Container per il task 2
- **task3**: Container per il task 3

Tutti i container comunicano tramite una rete Docker bridge chiamata `grpc_orchestrator_network`.

## Prerequisiti

```bash
# Installa Docker
sudo apt-get update
sudo apt-get install -y docker.io docker-compose

# Aggiungi il tuo utente al gruppo docker (per evitare sudo)
sudo usermod -aG docker $USER
# Logout e login per applicare le modifiche

# Verifica installazione
docker --version
docker-compose --version
```

## Build delle Immagini

### Opzione 1: Build Automatico

```bash
./docker-build.sh
```

### Opzione 2: Build Manuale

```bash
# Build orchestrator
docker build -f Dockerfile.orchestrator -t grpc-orchestrator:latest .

# Build task
docker build -f Dockerfile.task -t grpc-task:latest .
```

## Esecuzione

### Opzione 1: Script Automatico (Consigliato)

```bash
./docker-run.sh
```

Questo script:
1. Verifica se le immagini esistono (altrimenti le costruisce)
2. Avvia tutti i container con docker-compose
3. Mostra i log in tempo reale
4. Gestisce il cleanup quando premi Ctrl+C

### Opzione 2: Docker Compose Manuale

```bash
# Avvia in foreground (con log visibili)
docker-compose up

# Avvia in background
docker-compose up -d

# Ferma i container
docker-compose down
```

## Visualizzare i Log

### Tutti i container

```bash
./docker-logs.sh
# oppure
docker-compose logs -f
```

### Container specifico

```bash
./docker-logs.sh orchestrator
./docker-logs.sh task1
./docker-logs.sh task2
./docker-logs.sh task3
```

## Gestione dei Container

### Verificare lo stato

```bash
docker-compose ps
```

### Riavviare un container specifico

```bash
docker-compose restart orchestrator
docker-compose restart task1
```

### Fermare tutto

```bash
./docker-stop.sh
# oppure
docker-compose down
```

### Rimuovere anche le immagini

```bash
docker-compose down --rmi all
```

## Networking

I container comunicano usando i seguenti hostname:
- Orchestrator: `orchestrator:50050`
- Task 1: `task1:50051`
- Task 2: `task2:50051`
- Task 3: `task3:50051`

Le porte sono anche esposte sull'host:
- Orchestrator: `localhost:50050`
- Task 1: `localhost:50051`
- Task 2: `localhost:50052`
- Task 3: `localhost:50053`

## Debugging

### Accedere a un container

```bash
# Orchestrator
docker exec -it grpc_orchestrator /bin/bash

# Task 1
docker exec -it grpc_task1 /bin/bash
```

### Verificare la rete

```bash
# Lista reti
docker network ls

# Ispeziona la rete
docker network inspect grpc_orchestrator_network
```

### Verificare connettività tra container

```bash
# Da orchestrator a task1
docker exec grpc_orchestrator ping -c 3 task1

# Test gRPC (se grpcurl è installato)
docker exec grpc_orchestrator grpcurl -plaintext task1:50051 list
```

## Personalizzazione

### Modificare il numero di task

Edita `docker-compose.yml` e aggiungi/rimuovi servizi task:

```yaml
  task4:
    build:
      context: .
      dockerfile: Dockerfile.task
    container_name: grpc_task4
    hostname: task4
    networks:
      - grpc_network
    ports:
      - "50054:50051"
    command: ["./task_main", "task_4", "0.0.0.0:50051", "orchestrator:50050"]
    environment:
      - TASK_ID=task_4
```

Poi aggiorna `src/schedule.cpp` per includere il nuovo task nello schedule.

### Modificare risorse dei container

Aggiungi limiti di risorse in `docker-compose.yml`:

```yaml
  orchestrator:
    # ... altre configurazioni ...
    deploy:
      resources:
        limits:
          cpus: '2.0'
          memory: 1G
        reservations:
          cpus: '1.0'
          memory: 512M
```

## Troubleshooting

### Errore: "port is already allocated"

```bash
# Trova il processo che usa la porta
sudo lsof -i :50050

# Oppure ferma tutti i container
docker-compose down
```

### Errore: "Cannot connect to Docker daemon"

```bash
# Avvia Docker
sudo systemctl start docker

# Verifica stato
sudo systemctl status docker
```

### Container si riavvia continuamente

```bash
# Verifica i log
docker-compose logs orchestrator

# Verifica health check
docker inspect grpc_orchestrator | grep -A 10 Health
```

### Rebuild completo

```bash
# Ferma e rimuovi tutto
docker-compose down --rmi all --volumes

# Rebuild da zero
./docker-build.sh
./docker-run.sh
```

## Performance

### Build più veloce

Usa BuildKit per build parallele:

```bash
DOCKER_BUILDKIT=1 docker build -f Dockerfile.orchestrator -t grpc-orchestrator:latest .
```

### Cache delle dipendenze

Le immagini Docker usano multi-stage build per:
- Separare build e runtime
- Ridurre dimensione finale delle immagini
- Velocizzare rebuild successivi

## Produzione

Per deployment in produzione, considera:

1. **Usare immagini ottimizzate**: Alpine Linux invece di Ubuntu
2. **Secrets management**: Non hardcodare credenziali
3. **Health checks**: Già configurati in docker-compose.yml
4. **Logging**: Usa un driver di logging centralizzato
5. **Monitoring**: Integra Prometheus/Grafana
6. **Orchestrazione**: Usa Kubernetes per scalabilità

## Esempio Output

```
=== Starting gRPC Orchestrator with Docker Compose ===
Starting services...
Creating network "grpc_orchestrator_network" with driver "bridge"
Creating grpc_task1 ... done
Creating grpc_task2 ... done
Creating grpc_task3 ... done
Creating grpc_orchestrator ... done
Attaching to grpc_task1, grpc_task2, grpc_task3, grpc_orchestrator

orchestrator_1  | === gRPC Orchestrator ===
orchestrator_1  | Starting orchestrator service...
orchestrator_1  | [Main] Using test schedule
orchestrator_1  | [ScheduleParser] Created test schedule with 3 tasks (Docker mode)
orchestrator_1  | [Orchestrator] Loaded schedule with 3 tasks
...
```
