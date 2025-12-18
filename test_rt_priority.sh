#!/bin/bash
# Test script for RT priority scheduling

set -e

echo "=========================================="
echo "RT Priority Test Script"
echo "=========================================="
echo ""

# Build the project
echo "1. Building project..."
docker-compose build

echo ""
echo "2. Starting containers..."
docker-compose up -d

echo ""
echo "3. Waiting for services to be ready..."
sleep 3

echo ""
echo "4. Running RT priority test..."
echo "   This test will run two tasks on the same CPU core (core 0)"
echo "   - Task 1: RT priority 30 (lower)"
echo "   - Task 3: RT priority 80 (higher)"
echo ""
echo "   Expected behavior:"
echo "   - Task 1 starts first"
echo "   - Task 3 preempts Task 1 when it starts"
echo "   - Task 3 completes first (even though it started second)"
echo "   - Task 1 resumes and completes after Task 3"
echo ""

docker-compose exec orchestrator /app/bin/orchestrator_main \
    /app/schedules/example_rt_priority_test.yaml

echo ""
echo "=========================================="
echo "Test completed!"
echo "=========================================="
echo ""
echo "To view logs:"
echo "  docker-compose logs orchestrator"
echo "  docker-compose logs task1"
echo "  docker-compose logs task3"
echo ""
echo "To stop containers:"
echo "  docker-compose down"
echo ""
