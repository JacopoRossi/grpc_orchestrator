# Guida Docker Compose - gRPC Orchestrator

## 📋 Indice
- [Comandi Rapidi](#comandi-rapidi)
- [Workflow Consigliato](#workflow-consigliato)
- [Comandi Docker Compose Nativi](#comandi-docker-compose-nativi)
- [Troubleshooting](#troubleshooting)

---

## 🚀 Comandi Rapidi

### Usando lo Script Helper

```bash
# Compila tutte le immagini
./docker-helper.sh build-all

# Avvia solo i task
./docker-helper.sh up-tasks

# Avvia l'orchestrator quando serve
./docker-helper.sh up-orchestrator

# Ferma l'orchestrator (i task continuano)
./docker-helper.sh down-orchestrator

# Mostra i log
./docker-helper.sh logs-orchestrator -f

# Stato dei container
./docker-helper.sh status
```

---

## 🔄 Workflow Consigliato

### 1. Prima Configurazione

```bash
# Compila le immagini Docker
./docker-helper.sh build-all
```

### 2. Avvia i Task (sempre attivi)

```bash
# Avvia solo i task containers
./docker-helper.sh up-tasks

# Verifica che siano attivi
docker ps
```

### 3. Avvia l'Orchestrator (quando serve)

```bash
# Avvia l'orchestrator
./docker-helper.sh up-orchestrator

# Segui i log in tempo reale
./docker-helper.sh logs-orchestrator -f
```

### 4. Ferma l'Orchestrator (lascia i task attivi)

```bash
# Ferma solo l'orchestrator
./docker-helper.sh down-orchestrator

# I task rimangono attivi
docker ps  # vedrai solo i task
```

### 5. Riavvia l'Orchestrator

```bash
# Riavvia l'orchestrator quando vuoi
./docker-helper.sh up-orchestrator
```

---

## 🐳 Comandi Docker Compose Nativi

Se preferisci usare docker-compose direttamente:

### Build

```bash
# Build di tutte le immagini
docker-compose build

# Build solo task
docker-compose build task1 task2 task3

# Build solo orchestrator
docker-compose build orchestrator
```

### Avvio con Profili

```bash
# Avvia solo i task
docker-compose --profile tasks up -d

# Avvia solo l'orchestrator
docker-compose --profile orchestrator up -d

# Avvia tutto insieme
docker-compose --profile all up -d
```

### Stop e Rimozione

```bash
# Ferma l'orchestrator
docker-compose stop orchestrator
docker-compose rm -f orchestrator

# Ferma i task
docker-compose stop task1 task2 task3

# Ferma tutto
docker-compose --profile all down
```

### Logs

```bash
# Log dell'orchestrator
docker-compose logs -f orchestrator

# Log di un task specifico
docker-compose logs -f task1

# Log di tutti i container
docker-compose --profile all logs -f
```

### Status

```bash
# Mostra tutti i container del compose
docker-compose ps

# Mostra solo container attivi
docker ps
```

---

## 🔧 Modifica Schedule

Lo schedule è montato come volume, quindi puoi modificarlo senza ricostruire:

```bash
# Modifica lo schedule
nano schedules/example_parametrized.yaml

# Riavvia l'orchestrator per applicare le modifiche
docker-compose restart orchestrator

# Oppure fermalo e riavvialo
./docker-helper.sh down-orchestrator
./docker-helper.sh up-orchestrator
```

---

## 🛠 Troubleshooting

### Container non parte

```bash
# Verifica i log
docker-compose logs orchestrator

# Verifica lo stato
docker-compose ps

# Ricostruisci l'immagine
docker-compose build orchestrator --no-cache
```

### Network non funziona

```bash
# Verifica la rete
docker network inspect grpc_orchestrator_grpc_network

# Ricrea la rete
docker-compose down
docker-compose --profile tasks up -d
```

### Reset Completo

```bash
# Ferma tutto e rimuovi volumi e immagini
./docker-helper.sh clean

# Oppure manualmente
docker-compose --profile all down -v --rmi all
docker network prune -f
```

### Permessi Docker

Se ottieni errori di permessi:

```bash
# Aggiungi il tuo utente al gruppo docker
sudo usermod -aG docker $USER

# Logout e login di nuovo
```

---

## 📊 Monitoraggio

### Real-time Stats

```bash
# Statistiche dei container
docker stats

# Solo per i task
docker stats grpc_task1 grpc_task2 grpc_task3
```

### Health Check

```bash
# Verifica health status
docker inspect grpc_orchestrator | grep -A 5 Health

# Watch continuo
watch -n 2 'docker ps --format "table {{.Names}}\t{{.Status}}"'
```

---

## 🔐 Sicurezza e Performance

Il docker-compose è configurato con:
- **Real-time capabilities**: SYS_NICE per priorità RT
- **CPU pinning**: Orchestrator su CPU 0
- **Memory locking**: 2GB memlock per evitare swap
- **Seccomp**: Disabled per syscall RT
- **Health checks**: Monitoring automatico

---

## 📝 Note Importanti

1. **I task devono essere avviati PRIMA dell'orchestrator** (gestito automaticamente con `depends_on`)
2. **Lo schedule può essere modificato senza rebuild** (montato come volume)
3. **I task possono rimanere sempre attivi**, l'orchestrator viene avviato solo quando serve
4. **Sincronizzazione oraria** con l'host tramite volumi `/etc/localtime` e `/etc/timezone`

---

## 🎯 Quick Reference

| Azione | Comando Script | Comando Docker Compose |
|--------|---------------|------------------------|
| Build tutto | `./docker-helper.sh build-all` | `docker-compose build` |
| Avvia task | `./docker-helper.sh up-tasks` | `docker-compose --profile tasks up -d` |
| Avvia orchestrator | `./docker-helper.sh up-orchestrator` | `docker-compose --profile orchestrator up -d` |
| Stop orchestrator | `./docker-helper.sh down-orchestrator` | `docker-compose stop orchestrator && docker-compose rm -f orchestrator` |
| Log orchestrator | `./docker-helper.sh logs-orchestrator -f` | `docker-compose logs -f orchestrator` |
| Status | `./docker-helper.sh status` | `docker-compose ps` |

---

Creato per: gRPC Orchestrator Project
Ultimo aggiornamento: 2026-01-19
