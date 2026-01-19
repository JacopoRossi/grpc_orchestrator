#!/bin/bash
# Quick build script for deploy_manager_main only

set -e  # Exit on error

echo "🔨 Building deploy_manager_main (standalone)..."

# Colors
GREEN='\033[0;32m'
RED='\033[0;31m'
NC='\033[0m' # No Color

# Check dependencies
if ! command -v protoc &> /dev/null; then
    echo -e "${RED}Error: protoc not found${NC}"
    exit 1
fi

if ! command -v g++ &> /dev/null; then
    echo -e "${RED}Error: g++ not found${NC}"
    exit 1
fi

# Create build directory
mkdir -p build_deploy
cd build_deploy

echo "📦 Generating protobuf files..."
protoc --cpp_out=. --grpc_out=. \
    --plugin=protoc-gen-grpc=$(which grpc_cpp_plugin) \
    -I ../proto \
    ../proto/orchestrator.proto

echo "🔧 Compiling source files..."
g++ -std=c++17 -O2 -c orchestrator.pb.cc -I . -I ../include
g++ -std=c++17 -O2 -c orchestrator.grpc.pb.cc -I . -I ../include
g++ -std=c++17 -O2 -c ../src/deploy_manager.cpp -I . -I ../include
g++ -std=c++17 -O2 -c ../examples/deploy_manager_main.cpp -I . -I ../include

echo "🔗 Linking..."
g++ deploy_manager_main.o deploy_manager.o \
    orchestrator.pb.o orchestrator.grpc.pb.o \
    -lyaml-cpp -lgrpc++ -lprotobuf -lpthread \
    -o deploy_manager_main

# Move to parent directory
mv deploy_manager_main ../

cd ..

echo -e "${GREEN}✅ Build successful!${NC}"
echo ""
echo "Run with:"
echo "  ./deploy_manager_main build"
echo "  ./deploy_manager_main deploy"
echo ""
