#!/bin/bash
# Docker Compose Helper Script per gRPC Orchestrator

set -e

PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$PROJECT_DIR"

# Use sudo for docker-compose if not in docker group
DOCKER_CMD="sudo docker-compose"

print_usage() {
    cat << EOF
Docker Compose Helper per gRPC Orchestrator

Uso: $0 [COMANDO]

Comandi disponibili:
  build-all           Compila tutte le immagini Docker
  build-tasks         Compila solo l'immagine dei task
  build-orchestrator  Compila solo l'immagine dell'orchestrator
  
  up-tasks            Avvia solo i container dei task
  up-orchestrator     Avvia solo l'orchestrator
  up-all              Avvia tutti i container (task + orchestrator)
  
  down-orchestrator   Ferma solo l'orchestrator
  down-tasks          Ferma solo i task
  down-all            Ferma tutti i container
  
  logs-orchestrator   Mostra i log dell'orchestrator
  logs-tasks          Mostra i log dei task
  logs-all            Mostra i log di tutti i container
  
  status              Mostra lo stato dei container
  clean               Rimuove tutti i container e le immagini
  
  help                Mostra questo messaggio

Esempi:
  $0 build-all        # Compila tutte le immagini
  $0 up-tasks         # Avvia solo i task
  $0 up-orchestrator  # Avvia l'orchestrator quando serve
  $0 logs-orchestrator -f  # Segui i log in tempo reale

EOF
}

case "$1" in
    build-all)
        echo "=== Building all images ==="
        $DOCKER_CMD build
        ;;
    
    build-tasks)
        echo "=== Building task image ==="
        $DOCKER_CMD build task1 task2 task3
        ;;
    
    build-orchestrator)
        echo "=== Building orchestrator image ==="
        $DOCKER_CMD build orchestrator
        ;;
    
    up-tasks)
        echo "=== Starting task containers ==="
        $DOCKER_CMD --profile tasks up -d
        echo "Tasks started successfully!"
        $DOCKER_CMD ps
        ;;
    
    up-orchestrator)
        echo "=== Starting orchestrator ==="
        $DOCKER_CMD --profile orchestrator up -d
        echo "Orchestrator started successfully!"
        $DOCKER_CMD ps
        ;;
    
    up-all)
        echo "=== Starting all containers ==="
        $DOCKER_CMD --profile all up -d
        echo "All containers started successfully!"
        $DOCKER_CMD ps
        ;;
    
    down-orchestrator)
        echo "=== Stopping orchestrator ==="
        $DOCKER_CMD stop orchestrator
        $DOCKER_CMD rm -f orchestrator
        ;;
    
    down-tasks)
        echo "=== Stopping tasks ==="
        $DOCKER_CMD stop task1 task2 task3
        $DOCKER_CMD rm -f task1 task2 task3
        ;;
    
    down-all)
        echo "=== Stopping all containers ==="
        $DOCKER_CMD --profile all down
        ;;
    
    logs-orchestrator)
        shift
        $DOCKER_CMD logs orchestrator "$@"
        ;;
    
    logs-tasks)
        shift
        $DOCKER_CMD logs task1 task2 task3 "$@"
        ;;
    
    logs-all)
        shift
        $DOCKER_CMD --profile all logs "$@"
        ;;
    
    status)
        echo "=== Container Status ==="
        $DOCKER_CMD ps
        echo ""
        echo "=== Network Info ==="
        sudo docker network ls | grep grpc || echo "Network not found"
        ;;
    
    clean)
        echo "=== Cleaning up ==="
        $DOCKER_CMD --profile all down -v --rmi all
        echo "Cleanup completed!"
        ;;
    
    help|--help|-h)
        print_usage
        ;;
    
    *)
        echo "Errore: Comando sconosciuto '$1'"
        echo ""
        print_usage
        exit 1
        ;;
esac
