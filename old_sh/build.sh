#!/bin/bash

# Build script for gRPC Orchestrator

set -e  # Exit on error

echo "=== Building gRPC Orchestrator ==="

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Check if build directory exists
if [ -d "build" ]; then
    echo -e "${YELLOW}Build directory exists. Cleaning...${NC}"
    rm -rf build
fi

# Create build directory
echo "Creating build directory..."
mkdir -p build
cd build

# Run CMake
echo -e "${GREEN}Running CMake...${NC}"
cmake .. -DCMAKE_BUILD_TYPE=Release

# Build
echo -e "${GREEN}Building project...${NC}"
make -j$(nproc)

# Check if build was successful
if [ $? -eq 0 ]; then
    echo -e "${GREEN}=== Build Successful ===${NC}"
    echo ""
    echo "Executables are in: build/bin/"
    echo "  - orchestrator_main"
    echo "  - task_main"
    echo ""
    echo "To run the orchestrator:"
    echo "  ./build/bin/orchestrator_main"
    echo ""
    echo "To run a task:"
    echo "  ./build/bin/task_main <task_id> <listen_address> <orchestrator_address>"
    echo ""
    echo "Or use the run_demo.sh script to run a complete demo"
else
    echo -e "${RED}=== Build Failed ===${NC}"
    exit 1
fi
