#!/bin/bash

# Build Docker images for gRPC Orchestrator

set -e

echo "=== Building Docker Images for gRPC Orchestrator ==="

# Colors
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

# Build orchestrator image
echo -e "${GREEN}Building Orchestrator image...${NC}"
docker build -f Dockerfile.orchestrator -t grpc-orchestrator:latest .

# Build task image
echo -e "${GREEN}Building Task image...${NC}"
docker build -f Dockerfile.task -t grpc-task:latest .

echo -e "${GREEN}=== Build Complete ===${NC}"
echo ""
echo "Images created:"
echo "  - grpc-orchestrator:latest"
echo "  - grpc-task:latest"
echo ""
echo "To run the system:"
echo "  docker-compose up"
echo ""
echo "To run in background:"
echo "  docker-compose up -d"
