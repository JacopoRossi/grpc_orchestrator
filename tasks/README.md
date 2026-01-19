# Task Runner - Sistema Modulare per Task Personalizzati

Questo sistema ti permette di definire i tuoi task in modo semplice senza modificare i wrapper.

## 📁 Struttura

```
tasks/
├── my_tasks.h          # QUI definisci i tuoi task
├── task_runner.cpp     # Main generico (non modificare)
└── README.md           # Questa guida
```

## 🚀 Come Usare

### 1. Definisci i tuoi task in `my_tasks.h`

Apri `my_tasks.h` e aggiungi la tua funzione task:

```cpp
inline TaskResult my_new_task(const std::string& params_json, std::string& output_json) {
    // Parse parametri
    json params = json::parse(params_json);
    int my_param = params.value("my_param", 100);
    
    // La tua logica qui
    std::cout << "Executing my task with param: " << my_param << std::endl;
    
    // Prepara output
    json output;
    output["result"] = "success";
    output["value"] = my_param * 2;
    output_json = output.dump();
    
    return TASK_RESULT_SUCCESS;
}
```

### 2. Registra il task in `task_runner.cpp`

Nel main, aggiungi la tua funzione al registry (linea ~90):

```cpp
task_registry["my_new_task"] = my_new_task;
```

### 3. Compila

```bash
cd build
cmake ..
make task_runner
```

### 4. Esegui

```bash
./task_runner \
    --name my_new_task \
    --address 0.0.0.0:50051 \
    --orchestrator orchestrator:50050
```

## 📝 Esempio Completo

### File: `my_tasks.h`

```cpp
inline TaskResult fibonacci_calculator(const std::string& params_json, std::string& output_json) {
    json params = json::parse(params_json);
    int n = params.value("n", 10);
    
    // Calcola Fibonacci
    std::vector<int> fib(n);
    fib[0] = 0;
    fib[1] = 1;
    for (int i = 2; i < n; i++) {
        fib[i] = fib[i-1] + fib[i-2];
    }
    
    // Output
    json output;
    output["sequence"] = fib;
    output["last_value"] = fib[n-1];
    output_json = output.dump();
    
    return TASK_RESULT_SUCCESS;
}
```

### File: `task_runner.cpp` (aggiungi al registry)

```cpp
task_registry["fibonacci"] = fibonacci_calculator;
```

### File YAML dello schedule

```yaml
tasks:
  - id: fib_task
    address: "task1:50051"
    mode: timed
    scheduled_time_us: 1000000
    parameters:
      n: 20
```

## 🎯 Task Già Disponibili

- **task_1**: Data Analyzer - analizza dati e calcola statistiche
- **task_2**: Image Processor - elabora immagini con filtri
- **task_3**: Report Generator - genera report da altri task
- **custom**: Template per task personalizzato

## 💡 Tips

1. **Parametri**: Usa `params.value("key", default_value)` per parametri opzionali
2. **Dipendenze**: Se il task dipende da altri, i dati arrivano in `params["dep_output"]`
3. **Logging**: Usa `get_absolute_time_ms()` per timestamp consistenti
4. **Errori**: Ritorna `TASK_RESULT_FAILURE` in caso di errore

## 🔧 Workflow Tipico

1. Modifica solo `my_tasks.h` per aggiungere/modificare task
2. Ricompila con `make task_runner`
3. Aggiorna lo YAML dello schedule con i nuovi parametri
4. Esegui con Docker Compose o manualmente

## 📦 Integrazione con Docker

Nel `docker-compose.yml`, usa il task_runner:

```yaml
task1:
  build:
    context: .
    dockerfile: Dockerfile.task
  command: >
    ./task_runner
    --name task_1
    --address 0.0.0.0:50051
    --orchestrator orchestrator:50050
```

## ⚡ Real-Time Configuration

Aggiungi opzioni RT per task critici:

```bash
./task_runner \
    --name critical_task \
    --address 0.0.0.0:50051 \
    --orchestrator orchestrator:50050 \
    --policy fifo \
    --priority 80 \
    --cpu-affinity 2 \
    --lock-memory
```

## 🐛 Debug

Se il task non viene trovato:
1. Verifica che il nome nel registry corrisponda al `--name`
2. Controlla che la funzione sia definita in `my_tasks.h`
3. Assicurati di aver ricompilato dopo le modifiche
