#!/bin/bash

# Demo script to run orchestrator and tasks

set -e

echo "=== gRPC Orchestrator Demo ==="

# Colors
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

# Check if binaries exist
if [ ! -f "build/bin/orchestrator_main" ] || [ ! -f "build/bin/task_main" ]; then
    echo -e "${YELLOW}Binaries not found. Building first...${NC}"
    ./build.sh
fi

# Kill any existing processes
echo "Cleaning up any existing processes..."
pkill -f orchestrator_main || true
pkill -f task_main || true
sleep 1

# Create log directory
mkdir -p logs

# Start tasks in background
echo -e "${GREEN}Starting Task 1...${NC}"
./build/bin/task_main task_1 localhost:50051 localhost:50050 > logs/task_1.log 2>&1 &
TASK1_PID=$!

echo -e "${GREEN}Starting Task 2...${NC}"
./build/bin/task_main task_2 localhost:50052 localhost:50050 > logs/task_2.log 2>&1 &
TASK2_PID=$!

echo -e "${GREEN}Starting Task 3...${NC}"
./build/bin/task_main task_3 localhost:50053 localhost:50050 > logs/task_3.log 2>&1 &
TASK3_PID=$!

# Give tasks time to start
echo "Waiting for tasks to initialize..."
sleep 2

# Start orchestrator (runs in foreground)
echo -e "${GREEN}Starting Orchestrator...${NC}"
echo "Press Ctrl+C to stop all processes"
echo ""

# Trap Ctrl+C to cleanup
trap cleanup INT

cleanup() {
    echo ""
    echo "Stopping all processes..."
    kill $TASK1_PID $TASK2_PID $TASK3_PID 2>/dev/null || true
    exit 0
}

# Run orchestrator
./build/bin/orchestrator_main

# Cleanup when orchestrator exits
cleanup
