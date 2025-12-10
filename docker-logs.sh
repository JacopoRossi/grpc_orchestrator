#!/bin/bash

# View logs from Docker containers

echo "=== Docker Container Logs ==="
echo ""
echo "Usage:"
echo "  ./docker-logs.sh              # All containers"
echo "  ./docker-logs.sh orchestrator # Orchestrator only"
echo "  ./docker-logs.sh task1        # Task 1 only"
echo "  ./docker-logs.sh task2        # Task 2 only"
echo "  ./docker-logs.sh task3        # Task 3 only"
echo ""

if [ -z "$1" ]; then
    # Show all logs
    docker-compose logs -f
else
    # Show specific container logs
    docker-compose logs -f "$1"
fi
