# Esempio Pratico: Come Aggiungere un Nuovo Task

## Scenario
Vogliamo creare un task che calcola numeri primi fino a N.

## Step 1: Definisci la funzione in `my_tasks.h`

Apri `tasks/my_tasks.h` e aggiungi:

```cpp
// ============================================================
// TASK: PRIME CALCULATOR
// Calcola numeri primi fino a N
// Input: max_number (numero massimo)
// Output: primes (array di primi), count (quanti primi)
// ============================================================
inline TaskResult prime_calculator(const std::string& params_json, std::string& output_json) {
    std::cout << "[" << std::setw(13) << get_absolute_time_ms() << " ms] "
              << "[Prime Calculator] Starting..." << std::endl;
    
    // Parse input
    json params = json::parse(params_json);
    int max_number = params.value("max_number", 1000);
    
    std::cout << "[Prime Calculator] Finding primes up to " << max_number << std::endl;
    
    // Algoritmo: Crivello di Eratostene
    std::vector<bool> is_prime(max_number + 1, true);
    is_prime[0] = is_prime[1] = false;
    
    for (int i = 2; i * i <= max_number; i++) {
        if (is_prime[i]) {
            for (int j = i * i; j <= max_number; j += i) {
                is_prime[j] = false;
            }
        }
    }
    
    // Raccogli i primi
    std::vector<int> primes;
    for (int i = 2; i <= max_number; i++) {
        if (is_prime[i]) {
            primes.push_back(i);
        }
    }
    
    // Prepara output
    json output;
    output["primes"] = primes;
    output["count"] = primes.size();
    output["max_checked"] = max_number;
    output_json = output.dump();
    
    std::cout << "[" << std::setw(13) << get_absolute_time_ms() << " ms] "
              << "[Prime Calculator] Found " << primes.size() << " primes" << std::endl;
    
    return TASK_RESULT_SUCCESS;
}
```

## Step 2: Registra il task in `task_runner.cpp`

Apri `tasks/task_runner.cpp` e trova la sezione del registry (circa linea 90):

```cpp
// Map task IDs to their execution functions
std::map<std::string, std::function<TaskResult(const std::string&, std::string&)>> task_registry;
task_registry["task_1"] = task1_data_analyzer;
task_registry["task_2"] = task2_image_processor;
task_registry["task_3"] = task3_report_generator;
task_registry["custom"] = my_custom_task;

// AGGIUNGI QUESTA RIGA:
task_registry["prime_calc"] = prime_calculator;
```

## Step 3: Compila

```bash
cd build
cmake ..
make task_runner
```

Output atteso:
```
[ 50%] Building CXX object CMakeFiles/task_runner.dir/tasks/task_runner.cpp.o
[100%] Linking CXX executable bin/task_runner
[100%] Built target task_runner
```

## Step 4: Crea lo schedule YAML

Crea `schedules/prime_schedule.yaml`:

```yaml
schedule:
  name: "Prime Number Calculator"
  description: "Calculate prime numbers"
  
  tasks:
    - id: prime_task
      address: "task1:50051"
      mode: timed
      scheduled_time_us: 1000000  # Start at 1 second
      deadline_us: 5000000        # 5 seconds deadline
      estimated_duration_us: 2000000  # ~2 seconds
      parameters:
        max_number: 10000
```

## Step 5: Testa in locale

```bash
# Terminal 1: Avvia orchestrator
./build/bin/orchestrator_main \
    --address 0.0.0.0:50050 \
    --schedule schedules/prime_schedule.yaml

# Terminal 2: Avvia task
./build/bin/task_runner \
    --name prime_calc \
    --address 0.0.0.0:50051 \
    --orchestrator localhost:50050
```

## Step 6: Usa con Docker

Modifica `docker-compose.yml`:

```yaml
services:
  orchestrator:
    build:
      context: .
      dockerfile: Dockerfile.orchestrator
    command: >
      ./orchestrator_main
      --address 0.0.0.0:50050
      --schedule /app/schedules/prime_schedule.yaml
    volumes:
      - ./schedules:/app/schedules
    networks:
      - orchestrator-net

  task1:
    build:
      context: .
      dockerfile: Dockerfile.task
    command: >
      ./task_runner
      --name prime_calc
      --address 0.0.0.0:50051
      --orchestrator orchestrator:50050
    depends_on:
      - orchestrator
    networks:
      - orchestrator-net

networks:
  orchestrator-net:
    driver: bridge
```

Poi esegui:

```bash
docker-compose up --build
```

## Output Atteso

```
[Orchestrator] Loading schedule from: schedules/prime_schedule.yaml
[Orchestrator] Loaded task: prime_task (timed)
[Orchestrator] Starting task: prime_task at 1000000 us
[Prime Calculator] Starting...
[Prime Calculator] Finding primes up to 10000
[Prime Calculator] Found 1229 primes
[Task] Task completed successfully
```

## 🎯 Vantaggi di Questo Approccio

1. **Modifichi solo `my_tasks.h`** - non tocchi i wrapper
2. **Un solo eseguibile** - `task_runner` gestisce tutti i task
3. **Facile debug** - ogni task è una funzione isolata
4. **Riutilizzabile** - stesso runner per task diversi
5. **Parametrizzabile** - tutto configurato via YAML

## 🔄 Workflow Completo

```
1. Scrivi funzione in my_tasks.h
2. Registra in task_runner.cpp
3. Compila: make task_runner
4. Crea schedule YAML
5. Testa: ./task_runner --name <task_id>
6. Deploy: docker-compose up
```

## 💡 Tips Avanzati

### Task con Dipendenze

```cpp
inline TaskResult aggregator_task(const std::string& params_json, std::string& output_json) {
    json params = json::parse(params_json);
    
    // Accedi ai dati del task precedente
    if (params.contains("dep_output")) {
        json dep_data = params["dep_output"];
        
        // Usa i dati, ad esempio:
        int prime_count = dep_data.value("count", 0);
        std::cout << "Received " << prime_count << " primes from previous task" << std::endl;
    }
    
    // ... resto della logica
}
```

### Task con Carico Pesante

```cpp
inline TaskResult heavy_computation(const std::string& params_json, std::string& output_json) {
    json params = json::parse(params_json);
    int iterations = params.value("iterations", 1000000);
    
    // Mostra progresso
    for (int i = 0; i < iterations; i++) {
        // ... computazione ...
        
        if (i % 100000 == 0) {
            std::cout << "Progress: " << (i * 100 / iterations) << "%" << std::endl;
        }
    }
    
    return TASK_RESULT_SUCCESS;
}
```

### Task con Gestione Errori

```cpp
inline TaskResult safe_task(const std::string& params_json, std::string& output_json) {
    try {
        json params = json::parse(params_json);
        
        // Valida parametri
        if (!params.contains("required_param")) {
            std::cerr << "ERROR: Missing required_param" << std::endl;
            return TASK_RESULT_FAILURE;
        }
        
        // ... logica ...
        
        return TASK_RESULT_SUCCESS;
        
    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        return TASK_RESULT_FAILURE;
    }
}
```
