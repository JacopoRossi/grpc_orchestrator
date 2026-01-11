# Task Architecture - Separate Executables

## Struttura dei File

```
grpc_orchestrator/
├── examples/
│   ├── orchestrator_main.cpp          # Orchestrator principale
│   ├── task1_data_analyzer.cpp        # Task 1: Analisi dati
│   ├── task2_image_processor.cpp      # Task 2: Elaborazione immagini
│   ├── task3_report_generator.cpp     # Task 3: Generazione report
│   └── task_main.cpp                  # Legacy (backward compatibility)
├── CMakeLists.txt                     # Build configuration
└── docker-compose.yml                 # Container orchestration
```

## Eseguibili Generati

Dopo la compilazione, vengono generati i seguenti eseguibili:

1. **`orchestrator_main`**: Orchestratore principale
2. **`task1_data_analyzer`**: Task 1 standalone
3. **`task2_image_processor`**: Task 2 standalone
4. **`task3_report_generator`**: Task 3 standalone
5. **`task_main`**: Legacy (per retrocompatibilità)

## Vantaggi dell'Architettura Separata

### 1. Separazione delle Responsabilità
Ogni task è un programma completo e autonomo:
- Codice dedicato e focalizzato
- Nessuna dipendenza tra task a livello di codice
- Più facile da comprendere e debuggare

### 2. Deployment Indipendente
Puoi deployare solo i task necessari:
```bash
# Deploy solo Task 1 e Task 3
docker-compose up orchestrator task1 task3
```

### 3. Scalabilità Orizzontale
Puoi avviare multiple istanze dello stesso task:
```yaml
# docker-compose.yml
task1_replica1:
  image: task1_data_analyzer
  command: ./task1_data_analyzer --name task_1_replica1 ...

task1_replica2:
  image: task1_data_analyzer
  command: ./task1_data_analyzer --name task_1_replica2 ...
```

### 4. Manutenibilità
- Modifiche a Task 1 non richiedono ricompilazione di Task 2 e 3
- Testing isolato di ogni task
- Versioning indipendente possibile

### 5. Sviluppo Parallelo
Team diversi possono lavorare su task diversi senza conflitti.

## Compilazione

### Build Completo
```bash
mkdir build && cd build
cmake ..
make -j$(nproc)
```

Questo genera tutti gli eseguibili in `build/bin/`:
```
build/bin/
├── orchestrator_main
├── task1_data_analyzer
├── task2_image_processor
├── task3_report_generator
└── task_main
```

### Build Selettivo
Puoi compilare solo specifici eseguibili:
```bash
# Solo Task 1
make task1_data_analyzer

# Solo Task 2 e Task 3
make task2_image_processor task3_report_generator
```

## Esecuzione

### Con Docker Compose (Raccomandato)
```bash
# Build e avvio completo
docker-compose build
docker-compose up

# Solo specifici task
docker-compose up orchestrator task1 task3
```

### Manuale (per testing/debug)

**Terminale 1 - Orchestrator:**
```bash
./build/bin/orchestrator_main \
  --address 0.0.0.0:50050 \
  --schedule schedules/example_parametrized.yaml
```

**Terminale 2 - Task 1:**
```bash
./build/bin/task1_data_analyzer \
  --name task_1 \
  --address 0.0.0.0:50051 \
  --orchestrator localhost:50050
```

**Terminale 3 - Task 2:**
```bash
./build/bin/task2_image_processor \
  --name task_2 \
  --address 0.0.0.0:50052 \
  --orchestrator localhost:50050
```

**Terminale 4 - Task 3:**
```bash
./build/bin/task3_report_generator \
  --name task_3 \
  --address 0.0.0.0:50053 \
  --orchestrator localhost:50050
```

## Configurazione Docker

Ogni task ha il proprio container nel `docker-compose.yml`:

```yaml
task1:
  command: ./task1_data_analyzer --name task_1 ...
  
task2:
  command: ./task2_image_processor --name task_2 ...
  
task3:
  command: ./task3_report_generator --name task_3 ...
```

## Aggiungere un Nuovo Task

### 1. Crea il file sorgente
```bash
# examples/task4_new_processor.cpp
#include "task_wrapper.h"
// ... implementazione
```

### 2. Aggiungi al CMakeLists.txt
```cmake
add_executable(task4_new_processor
    examples/task4_new_processor.cpp
)

target_link_libraries(task4_new_processor
    PRIVATE orchestrator_lib
)
```

### 3. Aggiungi al docker-compose.yml
```yaml
task4:
  build:
    context: .
    dockerfile: Dockerfile.task
  command: ./task4_new_processor --name task_4 ...
```

### 4. Aggiungi allo schedule YAML
```yaml
tasks:
  - id: task_4
    address: "task4:50054"
    mode: sequential
    parameters:
      # ... parametri specifici
```

## Best Practices

### 1. Naming Convention
- File: `task{N}_{descriptive_name}.cpp`
- Eseguibile: `task{N}_{descriptive_name}`
- Container: `grpc_task{N}`
- Hostname: `task{N}`

### 2. Error Handling
Ogni task deve gestire:
- Parsing parametri JSON
- Validazione input
- Errori di elaborazione
- Graceful shutdown (SIGINT/SIGTERM)

### 3. Logging
Usa timestamp consistenti:
```cpp
std::cout << "[" << std::setw(13) << get_absolute_time_ms() << " ms] "
          << "[Task N - Name] Message" << std::endl;
```

### 4. Output JSON
Sempre ritornare JSON valido:
```cpp
json output = json::object();
output["result"] = value;
output["status"] = "success";
output_json = output.dump();
```

## Confronto con Approccio Monolitico

### Prima (task_main.cpp monolitico)
```cpp
if (task_id == "task_1") {
    // logica task 1
} else if (task_id == "task_2") {
    // logica task 2
} else if (task_id == "task_3") {
    // logica task 3
}
```

**Problemi:**
- ❌ Tutti i task nello stesso eseguibile
- ❌ Ricompilazione completa per ogni modifica
- ❌ Difficile testare task singoli
- ❌ Deployment all-or-nothing

### Ora (eseguibili separati)
```
task1_data_analyzer.cpp     → task1_data_analyzer
task2_image_processor.cpp   → task2_image_processor
task3_report_generator.cpp  → task3_report_generator
```

**Vantaggi:**
- ✅ Ogni task è indipendente
- ✅ Ricompilazione selettiva
- ✅ Testing isolato
- ✅ Deployment flessibile
- ✅ Scalabilità orizzontale

## Migrazione da task_main.cpp

Il file `task_main.cpp` è mantenuto per retrocompatibilità ma è **deprecated**.

Per migrare:
1. Identifica quale task stai usando (task_1, task_2, task_3)
2. Usa l'eseguibile corrispondente:
   - `task_1` → `task1_data_analyzer`
   - `task_2` → `task2_image_processor`
   - `task_3` → `task3_report_generator`
3. Aggiorna docker-compose.yml o script di avvio

## Testing

### Test Singolo Task
```bash
# Avvia solo orchestrator e task1
docker-compose up orchestrator task1

# Verifica logs
docker logs grpc_task1 -f
```

### Test Pipeline Completa
```bash
# Avvia tutto
docker-compose up

# Verifica esecuzione
docker-compose logs -f
```

### Debug
```bash
# Entra nel container
docker exec -it grpc_task1 /bin/bash

# Verifica eseguibile
ls -la /app/task1_data_analyzer

# Test manuale
./task1_data_analyzer --help
```

## Troubleshooting

### Eseguibile non trovato
```
Error: ./task1_data_analyzer: No such file or directory
```

**Soluzione:** Ricompila con `docker-compose build`

### Task non si connette
```
Error: Failed to connect to orchestrator
```

**Soluzione:** Verifica che orchestrator sia avviato e raggiungibile:
```bash
docker-compose ps
docker-compose logs orchestrator
```

### Parametri mancanti
```
Error: Missing 'data_size' parameter
```

**Soluzione:** Verifica `schedules/example_parametrized.yaml` contenga i parametri corretti per il task.

## Conclusione

L'architettura con eseguibili separati è la scelta corretta per un sistema di orchestrazione professionale. Offre massima flessibilità, manutenibilità e scalabilità, seguendo i principi di:
- **Single Responsibility Principle**
- **Separation of Concerns**
- **Microservices Architecture**
