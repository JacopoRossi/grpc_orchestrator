# Guida all'Installazione

## Passi per Ubuntu 24.04

### 1. Installa le dipendenze di sistema

```bash
sudo apt-get update
sudo apt-get install -y \
    build-essential \
    cmake \
    libgrpc++-dev \
    libprotobuf-dev \
    protobuf-compiler-grpc \
    pkg-config
```

### 2. Compila il progetto

```bash
cd /home/ice/Desktop/Projects/SpaceRTM/OFFICIAL_DSL/grpc_orchestrator
./build.sh
```

### 3. Esegui il demo

```bash
./run_demo.sh
```

## Opzione Alternativa: Compilare gRPC da Sorgente

Se l'installazione da pacchetto non funziona, puoi compilare gRPC da sorgente:

```bash
# Clona il repository gRPC
cd /tmp
git clone --recurse-submodules -b v1.58.0 https://github.com/grpc/grpc
cd grpc

# Crea directory di build
mkdir -p cmake/build
cd cmake/build

# Configura
cmake -DgRPC_INSTALL=ON \
      -DgRPC_BUILD_TESTS=OFF \
      -DCMAKE_BUILD_TYPE=Release \
      ../..

# Compila (può richiedere 10-30 minuti)
make -j$(nproc)

# Installa
sudo make install
sudo ldconfig
```

Dopo l'installazione, torna alla directory del progetto e compila:

```bash
cd /home/ice/Desktop/Projects/SpaceRTM/OFFICIAL_DSL/grpc_orchestrator
./build.sh
```
