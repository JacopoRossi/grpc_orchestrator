#!/bin/bash

# Script per lanciare il sistema con Docker Compose usando sudo

set -e

echo "=== gRPC Orchestrator - Docker Runner ==="
echo ""

# Check if docker-compose is installed
if ! command -v docker-compose &> /dev/null; then
    echo "Error: docker-compose not found"
    exit 1
fi

# Parse command line arguments
ACTION=${1:-up}

case $ACTION in
    up)
        echo "Building and starting containers..."
        sudo docker-compose build
        sudo docker-compose up
        ;;
    down)
        echo "Stopping and removing containers..."
        sudo docker-compose down
        ;;
    restart)
        echo "Restarting containers..."
        sudo docker-compose restart
        ;;
    logs)
        SERVICE=${2:-orchestrator}
        echo "Showing logs for $SERVICE..."
        sudo docker-compose logs -f $SERVICE
        ;;
    build)
        echo "Building containers..."
        sudo docker-compose build
        ;;
    clean)
        echo "Cleaning up containers, networks, and volumes..."
        sudo docker-compose down -v
        sudo docker system prune -f
        ;;
    *)
        echo "Usage: $0 {up|down|restart|logs [service]|build|clean}"
        echo ""
        echo "Commands:"
        echo "  up       - Build and start all containers"
        echo "  down     - Stop and remove containers"
        echo "  restart  - Restart all containers"
        echo "  logs     - Show logs (default: orchestrator, or specify service)"
        echo "  build    - Build containers without starting"
        echo "  clean    - Remove containers, networks, and volumes"
        exit 1
        ;;
esac
