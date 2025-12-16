#!/bin/bash

# Verify Real-Time Configuration in Docker Containers

set -e

# Colors
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
BLUE='\033[0;34m'
NC='\033[0m'

echo -e "${BLUE}=== Real-Time Configuration Verification ===${NC}"
echo ""

# Function to check container RT config
check_container_rt() {
    local container_name=$1
    local expected_policy=$2
    local expected_priority=$3
    local expected_cpu=$4
    
    echo -e "${YELLOW}Checking ${container_name}...${NC}"
    
    # Check if container is running
    if ! docker ps | grep -q "$container_name"; then
        echo -e "${RED}  ✗ Container not running${NC}"
        return 1
    fi
    
    # Get container PID
    local pid=$(docker inspect -f '{{.State.Pid}}' "$container_name" 2>/dev/null)
    if [ -z "$pid" ] || [ "$pid" == "0" ]; then
        echo -e "${RED}  ✗ Cannot get container PID${NC}"
        return 1
    fi
    
    echo "  Container PID: $pid"
    
    # Check scheduling policy and priority
    local sched_info=$(chrt -p $pid 2>/dev/null || echo "N/A")
    echo "  Scheduling: $sched_info"
    
    # Check CPU affinity
    local cpu_affinity=$(taskset -cp $pid 2>/dev/null | awk '{print $NF}' || echo "N/A")
    echo "  CPU Affinity: $cpu_affinity"
    
    # Check ulimits
    echo "  Ulimits:"
    docker exec "$container_name" sh -c "ulimit -r" 2>/dev/null | xargs echo "    RT Priority:" || echo "    RT Priority: N/A"
    docker exec "$container_name" sh -c "ulimit -l" 2>/dev/null | xargs echo "    Memlock:" || echo "    Memlock: N/A"
    
    # Check capabilities
    local caps=$(docker exec "$container_name" sh -c "grep Cap /proc/self/status 2>/dev/null" || echo "N/A")
    if echo "$caps" | grep -q "Cap"; then
        echo -e "  ${GREEN}✓ Capabilities present${NC}"
    else
        echo -e "  ${RED}✗ Cannot read capabilities${NC}"
    fi
    
    # Check memory lock
    local vmlck=$(cat /proc/$pid/status 2>/dev/null | grep VmLck | awk '{print $2}')
    if [ -n "$vmlck" ] && [ "$vmlck" != "0" ]; then
        echo -e "  ${GREEN}✓ Memory locked: ${vmlck} kB${NC}"
    else
        echo "  Memory locked: ${vmlck:-0} kB (will lock on task execution)"
    fi
    
    echo ""
}

# Check orchestrator
check_container_rt "grpc_orchestrator" "FIFO" "80" "0"

# Check tasks
check_container_rt "grpc_task1" "FIFO" "75" "1"
check_container_rt "grpc_task2" "FIFO" "75" "2"
check_container_rt "grpc_task3" "FIFO" "75" "3"

echo -e "${BLUE}=== Verification Complete ===${NC}"
echo ""
echo -e "${YELLOW}Note:${NC} Real-time scheduling is applied to worker threads,"
echo "not the main container process. Check logs for RT configuration messages."
echo ""
echo "To view logs:"
echo "  docker logs grpc_orchestrator"
echo "  docker logs grpc_task1"
