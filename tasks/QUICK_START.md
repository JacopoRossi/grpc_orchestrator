# 🚀 Quick Start - Task Runner Modulare

## In 3 Minuti

### 1️⃣ Apri `tasks/my_tasks.h`

Trova la sezione "ESEMPIO: TASK PERSONALIZZATO" e modifica:

```cpp
inline TaskResult my_custom_task(const std::string& params_json, std::string& output_json) {
    std::cout << "[My Task] Starting..." << std::endl;
    
    // Parse parametri
    json params = json::parse(params_json);
    int value = params.value("my_value", 42);
    
    // La tua logica
    int result = value * 2;
    std::cout << "[My Task] Calculated: " << value << " * 2 = " << result << std::endl;
    
    // Output
    json output;
    output["result"] = result;
    output_json = output.dump();
    
    return TASK_RESULT_SUCCESS;
}
```

### 2️⃣ Compila

```bash
cd build
cmake ..
make task_runner
```

### 3️⃣ Testa

```bash
# Terminal 1: Orchestrator
./build/bin/orchestrator_main --address 0.0.0.0:50050

# Terminal 2: Task
./build/bin/task_runner \
    --name custom \
    --address 0.0.0.0:50051 \
    --orchestrator localhost:50050
```

Poi dall'orchestrator vedrai il task eseguirsi!

## 📋 Checklist

- [ ] Modificato `my_tasks.h`
- [ ] Compilato con `make task_runner`
- [ ] Testato in locale
- [ ] Funziona? ✅

## 🎯 Prossimi Passi

1. Leggi `ESEMPIO_USO.md` per esempi completi
2. Leggi `README.md` per la guida completa
3. Crea il tuo schedule YAML personalizzato

## ⚡ Comandi Utili

```bash
# Compila solo task_runner
make task_runner

# Compila tutto
make -j$(nproc)

# Pulisci e ricompila
rm -rf build && mkdir build && cd build && cmake .. && make task_runner

# Testa con Docker
docker-compose up --build task1
```

## 🐛 Problemi Comuni

**Task non trovato?**
- Verifica che il nome nel registry corrisponda al `--name`
- Controlla di aver ricompilato

**Errore di compilazione?**
- Verifica la sintassi in `my_tasks.h`
- Controlla che tutti gli include siano presenti

**Task non si connette?**
- Verifica che l'orchestrator sia avviato
- Controlla gli indirizzi (localhost vs hostname Docker)

## 💡 Pro Tip

Usa questo template per nuovi task:

```cpp
inline TaskResult my_new_task(const std::string& params_json, std::string& output_json) {
    // 1. Parse
    json params = json::parse(params_json);
    
    // 2. Logica
    // ... il tuo codice ...
    
    // 3. Output
    json output;
    output["status"] = "done";
    output_json = output.dump();
    
    return TASK_RESULT_SUCCESS;
}
```

Poi registra in `task_runner.cpp`:
```cpp
task_registry["my_new_task"] = my_new_task;
```

Fine! 🎉
