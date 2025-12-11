# Fix Timing - Tempi Relativi nell'Execution Summary

## 🐛 Problema Risolto

Prima, nell'Execution Summary, i tempi di inizio (`Started`) erano sempre 0:

```
Task: task_1
  Scheduled: 0 us
  Started: 0 us          ← SEMPRE 0!
  Ended: 20796855084 us
  Duration: 20796855084 us
```

Questo succedeva perché i timestamp erano assoluti (dall'epoch) invece di essere **relativi all'inizio dell'esecuzione**.

## ✅ Soluzione Implementata

Ora tutti i tempi sono **relativi a `start_time_us_`** (il momento in cui l'orchestrator parte).

### Modifiche al file `src/orchestrator.cpp`:

#### 1. **Tempo di inizio task** (riga 322)
```cpp
// PRIMA:
exec.actual_start_time_us = get_current_time_us();

// DOPO:
exec.actual_start_time_us = get_current_time_us() - start_time_us_;  // Relative to start
```

#### 2. **Aggiornamento dopo risposta task** (riga 367)
```cpp
// PRIMA:
it->second.actual_start_time_us = response.actual_start_time_us();

// DOPO:
if (response.actual_start_time_us() > 0) {
    it->second.actual_start_time_us = response.actual_start_time_us() - start_time_us_;
}
```

#### 3. **Tempo di fine task** (riga 175)
```cpp
// PRIMA:
exec.end_time_us = notification.end_time_us();

// DOPO:
exec.end_time_us = notification.end_time_us() - start_time_us_;  // Relative to start
```

#### 4. **Gestione errori** (riga 380)
```cpp
// PRIMA:
exec.actual_start_time_us = get_current_time_us();

// DOPO:
exec.actual_start_time_us = get_current_time_us() - start_time_us_;  // Relative to start
```

## 📊 Output Atteso Ora

Con la modalità ibrida implementata, dovresti vedere:

```
=== Execution Summary ===
Task: task_1
  Scheduled: 0 us
  Started: ~100 us          ← Tempo reale dall'inizio!
  Ended: ~500100 us
  Duration: ~500000 us
  Result: 1

Task: task_3
  Scheduled: 0 us
  Started: ~500200 us       ← Parte dopo task_1
  Ended: ~2000200 us
  Duration: ~1500000 us
  Result: 1

Task: task_2
  Scheduled: 2000000 us
  Started: ~2000100 us      ← Parte a 2 secondi precisi!
  Ended: ~2800100 us
  Duration: ~800000 us
  Result: 1
```

## 🔍 Spiegazione dei Tempi

### Task 1 (Sequenziale, nessuna dipendenza):
- **Scheduled**: 0 us (parte subito)
- **Started**: ~100 us (piccolo delay per setup)
- **Ended**: ~500100 us (dopo 500ms di lavoro)
- **Duration**: 500000 us (500ms)

### Task 3 (Sequenziale, aspetta task_1):
- **Scheduled**: 0 us (non rilevante in modalità sequenziale)
- **Started**: ~500200 us (parte subito dopo task_1)
- **Ended**: ~2000200 us (dopo 1.5s di lavoro)
- **Duration**: 1500000 us (1.5s)

### Task 2 (Temporizzato a 2 secondi):
- **Scheduled**: 2000000 us (2 secondi)
- **Started**: ~2000100 us (parte esattamente a 2s)
- **Ended**: ~2800100 us (dopo 800ms di lavoro)
- **Duration**: 800000 us (800ms)

## 🎯 Vantaggi

1. **Tempi leggibili**: Ora puoi vedere esattamente quando ogni task è partito
2. **Verifica timing**: Puoi verificare che i task temporizzati partano al momento giusto
3. **Debug facile**: Puoi vedere se ci sono ritardi o problemi di scheduling
4. **Timeline chiara**: Puoi ricostruire la timeline completa dell'esecuzione

## 🚀 Come Testare

### Con Docker:
```bash
sudo docker-compose build
sudo docker-compose up
```

### Senza Docker:
```bash
cd build
cmake ..
make -j$(nproc)

# Poi avvia orchestrator e task in terminali separati
```

## 📝 Note Tecniche

- `start_time_us_` viene impostato quando l'orchestrator parte (riga 120)
- Tutti i tempi sono in **microsecondi** (1 secondo = 1,000,000 us)
- I tempi sono presi con `std::chrono::steady_clock` per evitare problemi con cambi di orario
- La conversione avviene sottraendo `start_time_us_` da ogni timestamp

## ✅ Verifica

Dopo aver eseguito, controlla l'Execution Summary:

1. **Started** non deve essere 0 (tranne se il task parte veramente a tempo 0)
2. **Duration** deve essere `Ended - Started`
3. Per task temporizzati, **Started** deve essere vicino a **Scheduled**
4. Per task sequenziali, **Started** deve essere dopo il completamento del task precedente

## 🎨 Esempio Completo

```
T=0ms:        Orchestrator PARTE (start_time_us_ = NOW)
T=~100ms:     Task 1 PARTE (Started = 100 us)
T=~600ms:     Task 1 FINISCE (Ended = 600000 us)
              Task 3 PARTE (Started = 600100 us)
T=2000ms:     Task 2 PARTE (Started = 2000100 us, Scheduled = 2000000 us)
T=~2100ms:    Task 3 FINISCE (Ended = 2100000 us)
T=~2800ms:    Task 2 FINISCE (Ended = 2800000 us)
```

Output:
```
/ciao        (task_1)
 Jacopo/     (task_3, dopo task_1)
 sono        (task_2, a 2 secondi)
```

## 🔧 File Modificati

- **`src/orchestrator.cpp`**: 4 modifiche per usare tempi relativi

Nessuna modifica agli header o ad altri file necessaria!

## 🎯 Conclusione

Ora l'Execution Summary mostra i **tempi reali** di esecuzione, permettendoti di:
- Verificare il timing preciso
- Debug problemi di scheduling
- Analizzare le performance
- Validare la modalità ibrida

Buon lavoro! 🚀
