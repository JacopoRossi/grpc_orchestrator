#!/bin/bash

# Stop and cleanup Docker containers

echo "=== Stopping gRPC Orchestrator Docker Containers ==="

# Stop all containers
docker-compose down

# Optional: Remove images (uncomment if needed)
# docker rmi grpc-orchestrator:latest grpc-task:latest 2>/dev/null || true

echo "All containers stopped"
