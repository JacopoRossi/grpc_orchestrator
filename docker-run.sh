#!/bin/bash

# Run gRPC Orchestrator with Docker Compose

set -e

echo "=== Starting gRPC Orchestrator with Docker Compose ==="

# Colors
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m'

# Check if docker-compose is installed
if ! command -v docker-compose &> /dev/null; then
    echo -e "${RED}Error: docker-compose is not installed${NC}"
    echo "Install it with: sudo apt-get install docker-compose"
    exit 1
fi

# Check if images exist, if not build them
if ! docker images | grep -q "grpc-orchestrator"; then
    echo -e "${YELLOW}Images not found. Building first...${NC}"
    ./docker-build.sh
fi

# Start services
echo -e "${GREEN}Starting services...${NC}"
docker-compose up

# Cleanup function
cleanup() {
    echo ""
    echo -e "${YELLOW}Stopping services...${NC}"
    docker-compose down
}

trap cleanup INT TERM
