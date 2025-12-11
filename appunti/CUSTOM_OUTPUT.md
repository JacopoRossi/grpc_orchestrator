# Output Personalizzato dei Task

## Descrizione

Ogni task stampa una stringa personalizzata durante l'esecuzione:

- **Task 1**: `/ciao`
- **Task 2**: ` sono `
- **Task 3**: ` Jacopo/`

## Esecuzione

L'orchestrator esegue i task **una sola volta** in **sequenza rigorosa** e poi si ferma automaticamente:

1. Task 1 viene lanciato su thread separato → stampa `/ciao` → invia segnale di end → **orchestrator attende**
2. **Solo dopo** aver ricevuto il segnale di end dal Task 1, viene lanciato Task 2 → stampa ` sono ` → invia segnale di end → **orchestrator attende**
3. **Solo dopo** aver ricevuto il segnale di end dal Task 2, viene lanciato Task 3 → stampa ` Jacopo/` → invia segnale di end
4. L'orchestrator si ferma (exit code 0)
5. **Non c'è restart automatico**

### Caratteristiche Chiave

- ✅ **Esecuzione strettamente sequenziale**: il Task 2 parte SOLO dopo che il Task 1 ha completato
- ✅ **Thread separati**: ogni task viene eseguito su un thread dedicato
- ✅ **Sincronizzazione tramite segnali**: l'orchestrator attende il segnale di end prima di procedere
- ✅ **Una sola esecuzione**: nessun loop, nessun restart automatico

## Output Atteso

```
[Task task_1] Starting task execution
[Task Function] Starting execution with parameters:
  ...
  task_id = task_1

========================================
/ciao
========================================

[Task Function] Simulating work for 500 ms...
[Task Function] Progress: 20%
...
[Task Function] Work completed successfully
[Task task_1] Task execution completed successfully

[Orchestrator] Task task_1 completed and acknowledged
[Orchestrator] Proceeding to next task...

[Task task_2] Starting task execution
...
========================================
 sono 
========================================
...

[Task task_3] Starting task execution
...
========================================
 Jacopo/
========================================
...

[Orchestrator] All tasks completed successfully!
[Orchestrator] Orchestrator stopped
grpc_orchestrator exited with code 0
```

## Modifiche Implementate

### 1. Task Function (`examples/task_main.cpp`)

Modificata la funzione `example_task_function` per:
- Leggere il `task_id` dai parametri
- Stampare la stringa corrispondente in base al task_id
- Mantenere la simulazione del lavoro

### 2. Task Wrapper (`src/task_wrapper.cpp`)

Modificato `task_execution_thread` per:
- Aggiungere automaticamente `task_id` ai parametri passati alla callback
- Permettere alla funzione di esecuzione di identificare quale task sta eseguendo

### 3. Docker Compose (`docker-compose.yml`)

Rimosso `restart: unless-stopped` da tutti i servizi:
- orchestrator
- task1
- task2
- task3

Questo garantisce che dopo l'esecuzione completa, i container non vengano riavviati automaticamente.

## Come Testare

```bash
# Build
sudo docker-compose build

# Avvio (esegue una volta e si ferma)
sudo docker-compose up

# Visualizza solo le stringhe stampate
sudo docker-compose logs | grep -E "(/ciao| sono | Jacopo/)"

# Output atteso:
# grpc_task1      | /ciao
# grpc_task2      |  sono 
# grpc_task3      |  Jacopo/

# Pulizia
sudo docker-compose down
```

## Note

- L'esecuzione è **strettamente sequenziale**: un task alla volta
- Ogni task viene eseguito su un **thread separato**
- L'orchestrator **termina automaticamente** dopo aver completato tutti i task
- I task rimangono in attesa ma l'orchestrator esce con **exit code 0**
- Per riavviare l'esecuzione, eseguire nuovamente `sudo docker-compose up`
