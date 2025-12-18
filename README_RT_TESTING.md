# Real-Time Priority Testing

## Quick Start

Testa le primitive real-time con un singolo comando:

```bash
./test_rt_priority.sh
```

Questo script:
1. Builda il progetto
2. Avvia i container Docker
3. Esegue un test con due task sullo stesso core CPU con priorità diverse
4. Mostra i log per verificare il comportamento

## Cosa Testa

Il test dimostra la **preemption basata sulla priorità** con SCHED_FIFO:

- **Task 1**: Priorità RT 30, core 0, esecuzione lunga (3 secondi)
- **Task 3**: Priorità RT 80, core 0, esecuzione breve (2 secondi)

### Comportamento Atteso

1. Task 1 inizia per primo con priorità 30
2. Task 3 inizia dopo (grazie a `depends_on: task_1`)
3. **Task 3 preempta Task 1** perché ha priorità maggiore (80 > 30)
4. Task 3 completa per primo (anche se è partito dopo)
5. Task 1 riprende e completa dopo Task 3

Questo dimostra che lo scheduler real-time di Linux funziona correttamente.

## Configurazione YAML

### Esempio Base

```yaml
tasks:
  - id: my_task
    rt_policy: "fifo"      # SCHED_FIFO
    rt_priority: 80        # Priorità alta (1-99)
    cpu_affinity: 0        # Core 0
```

### Campi Disponibili

- **`rt_policy`**: `"none"`, `"fifo"`, `"rr"`, `"deadline"`
- **`rt_priority`**: 1-99 (99 = massima priorità)
- **`cpu_affinity`**: 0, 1, 2, ... o -1 (nessuna affinità)

## File di Esempio

- **`schedules/example_rt_priority_test.yaml`**: Test completo di preemption
- **`schedules/template_full.yaml`**: Template con tutti i campi RT documentati

## Documentazione Completa

- **`appunti/RT_PRIORITY_TESTING.md`**: Guida dettagliata all'uso delle funzionalità RT
- **`appunti/CHANGELOG_RT_FEATURES.md`**: Dettagli tecnici delle modifiche

## Requisiti

- Docker con capability `SYS_NICE` (già configurato)
- Kernel Linux con supporto real-time

## Verifica Risultati

Dopo il test, controlla i log:

```bash
# Log orchestrator
docker-compose logs orchestrator

# Log task1 (priorità bassa)
docker-compose logs task1

# Log task3 (priorità alta)
docker-compose logs task3
```

Cerca questi messaggi:

```
[Task task_1] Applying RT config: policy=fifo, priority=30, cpu_affinity=0
[Task task_3] Applying RT config: policy=fifo, priority=80, cpu_affinity=0
```

E verifica che task_3 completi prima di task_1.

## Cleanup

```bash
docker-compose down
```

## Troubleshooting

### "Failed to apply RT configuration"

Verifica che il container abbia il capability `SYS_NICE`:

```bash
docker-compose exec task1 capsh --print | grep sys_nice
```

Dovrebbe mostrare `cap_sys_nice` nella lista.

### Task non preempta

Verifica che:
1. Entrambi i task siano sullo stesso core (`cpu_affinity` uguale)
2. Le priorità siano diverse
3. La policy sia `"fifo"` o `"rr"`

## Prossimi Passi

Prova a:
1. Modificare le priorità in `example_rt_priority_test.yaml`
2. Cambiare il core CPU
3. Usare policy diverse (`"rr"` invece di `"fifo"`)
4. Creare il tuo schedule personalizzato

Buon testing! 🚀
