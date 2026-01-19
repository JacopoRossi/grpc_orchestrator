# 🔄 Migrazione al Sistema Modulare

## ✅ Modifiche Completate

### File Rimossi
- ❌ `examples/task1_data_analyzer.cpp` - sostituito da `tasks/my_tasks.h`
- ❌ `examples/task2_image_processor.cpp` - sostituito da `tasks/my_tasks.h`
- ❌ `examples/task3_report_generator.cpp` - sostituito da `tasks/my_tasks.h`

### File Creati
- ✅ `tasks/my_tasks.h` - Definizioni task modulari
- ✅ `tasks/task_runner.cpp` - Runner generico
- ✅ `tasks/README.md` - Guida completa
- ✅ `tasks/ESEMPIO_USO.md` - Esempi pratici
- ✅ `tasks/QUICK_START.md` - Quick start

### File Aggiornati
- ✅ `CMakeLists.txt` - Rimossi target obsoleti, aggiunto `task_runner`
- ✅ `docker-compose.yml` - Tutti i task usano ora `task_runner`

## 📊 Prima vs Dopo

### Prima (Sistema Vecchio)
```
examples/
├── task1_data_analyzer.cpp      (251 righe)
├── task2_image_processor.cpp    (300+ righe)
└── task3_report_generator.cpp   (350+ righe)
```
**Problema**: Per ogni nuovo task serviva un file completo con main, parsing args, wrapper, ecc.

### Dopo (Sistema Modulare)
```
tasks/
├── my_tasks.h          (Tutte le funzioni task)
└── task_runner.cpp     (Main generico riutilizzabile)
```
**Vantaggio**: Per aggiungere un task basta una funzione in `my_tasks.h`!

## 🎯 Struttura Attuale

```
grpc_orchestrator/
├── examples/
│   ├── orchestrator_main.cpp    ← Orchestrator (invariato)
│   └── task_main.cpp            ← Legacy (commentato)
├── tasks/                        ← NUOVO!
│   ├── my_tasks.h               ← Definisci qui i tuoi task
│   ├── task_runner.cpp          ← Runner generico
│   ├── README.md
│   ├── QUICK_START.md
│   └── ESEMPIO_USO.md
├── CMakeLists.txt               ← Aggiornato
└── docker-compose.yml           ← Aggiornato
```

## 🚀 Come Compilare

```bash
cd build
cmake ..
make task_runner
```

Oppure tutto:
```bash
make -j$(nproc)
```

## 🐳 Docker Compose

Ora tutti i task usano lo stesso eseguibile `task_runner`:

```yaml
task1:
  command: ./task_runner --name task_1 --address 0.0.0.0:50051 ...

task2:
  command: ./task_runner --name task_2 --address 0.0.0.0:50052 ...

task3:
  command: ./task_runner --name task_3 --address 0.0.0.0:50053 ...
```

Il task ID (`--name`) determina quale funzione eseguire dal registry.

## 📝 Task Disponibili

Nel registry di `task_runner.cpp`:

| Task ID   | Funzione              | Descrizione              |
|-----------|-----------------------|--------------------------|
| `task_1`  | `task1_data_analyzer` | Analizza dati            |
| `task_2`  | `task2_image_processor` | Elabora immagini       |
| `task_3`  | `task3_report_generator` | Genera report         |
| `custom`  | `my_custom_task`      | Template personalizzato  |

## ➕ Aggiungere un Nuovo Task

1. **Definisci in `my_tasks.h`**:
   ```cpp
   inline TaskResult my_new_task(const std::string& params_json, std::string& output_json) {
       // ... logica ...
       return TASK_RESULT_SUCCESS;
   }
   ```

2. **Registra in `task_runner.cpp`**:
   ```cpp
   task_registry["my_new_task"] = my_new_task;
   ```

3. **Compila**:
   ```bash
   make task_runner
   ```

4. **Usa nello YAML**:
   ```yaml
   - id: my_task
     address: "task1:50051"
     parameters:
       my_param: 123
   ```

5. **Esegui**:
   ```bash
   ./task_runner --name my_new_task --address 0.0.0.0:50051 ...
   ```

## ✨ Vantaggi del Nuovo Sistema

1. ✅ **Meno codice duplicato** - un solo main per tutti i task
2. ✅ **Più facile da mantenere** - modifichi solo `my_tasks.h`
3. ✅ **Più veloce da sviluppare** - aggiungi task in minuti
4. ✅ **Più pulito** - separazione logica vs infrastruttura
5. ✅ **Stesso Dockerfile** - tutti i task usano `Dockerfile.task`

## 🔧 Compatibilità

- ✅ Tutti gli schedule YAML esistenti funzionano senza modifiche
- ✅ I parametri JSON sono gestiti allo stesso modo
- ✅ Le dipendenze tra task funzionano come prima
- ✅ La configurazione RT è identica

## 📚 Prossimi Passi

1. Leggi `tasks/QUICK_START.md` per iniziare
2. Vedi `tasks/ESEMPIO_USO.md` per esempi completi
3. Modifica `tasks/my_tasks.h` per i tuoi task
4. Compila e testa!

---

**Nota**: Il vecchio `task_main.cpp` è commentato in `CMakeLists.txt` per retrocompatibilità, ma non è più necessario.
