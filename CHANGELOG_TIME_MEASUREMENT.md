# Changelog - Misurazione Centralizzata dei Tempi

## 📅 Data: 19 Gennaio 2026

## 🎯 Obiettivo

Centralizzare la misurazione dei tempi **solo nell'Orchestrator**, eliminando la dipendenza dai clock dei task container.

---

## ✅ Modifiche Implementate

### 1. Orchestrator - Start Time Measurement

**File**: `src/orchestrator.cpp`  
**Funzione**: `execute_task()`

**Prima**:
```cpp
void Orchestrator::execute_task(const ScheduledTask& task) {
    std::lock_guard<std::mutex> lock(mutex_);
    TaskExecution exec;
    exec.actual_start_time_us = get_current_time_us() - start_time_us_;
    // ...
    // Poi sovrascriveva con il tempo ricevuto dal task
    exec.actual_start_time_us = response.actual_start_time_us() - start_time_us_;
}
```

**Dopo**:
```cpp
void Orchestrator::execute_task(const ScheduledTask& task) {
    // Measure start time BEFORE any communication
    int64_t task_start_time = get_current_time_us() - start_time_us_;
    
    std::lock_guard<std::mutex> lock(mutex_);
    TaskExecution exec;
    exec.actual_start_time_us = task_start_time;  // Measured by orchestrator
    // ...
    // NON viene più sovrascritto con response.actual_start_time_us()
}
```

**Cambiamenti**:
- ✅ Il tempo viene misurato IMMEDIATAMENTE all'inizio della funzione
- ✅ Il tempo non viene più sovrascritto con quello ricevuto dal task
- ✅ Commento esplicito: "Measured by orchestrator"

---

### 2. Orchestrator - End Time Measurement

**File**: `src/orchestrator.cpp`  
**Funzione**: `on_task_end()`

**Prima**:
```cpp
void Orchestrator::on_task_end(const TaskEndNotification& notification) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = active_tasks_.find(notification.task_id());
    
    TaskExecution& exec = it->second;
    exec.end_time_us = notification.end_time_us() - start_time_us_;  // Dal task
    // ...
}
```

**Dopo**:
```cpp
void Orchestrator::on_task_end(const TaskEndNotification& notification) {
    // Measure end time IMMEDIATELY when notification arrives
    int64_t task_end_time = get_current_time_us() - start_time_us_;
    
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = active_tasks_.find(notification.task_id());
    
    TaskExecution& exec = it->second;
    exec.end_time_us = task_end_time;  // Measured by orchestrator
    // ...
}
```

**Cambiamenti**:
- ✅ Il tempo viene misurato IMMEDIATAMENTE all'inizio della funzione
- ✅ Non usa più `notification.end_time_us()` dal task
- ✅ Commento esplicito: "Measured by orchestrator"

---

### 3. Deploy Manager - New Method

**File**: `include/deploy_manager.h` & `src/deploy_manager.cpp`

**Aggiunto**:
```cpp
bool deploy_tasks_only();  // Deploy solo i task, senza orchestrator
```

Questo permette di deployare i task separatamente dall'orchestrator.

---

## 📊 Confronto: Prima vs Dopo

### Prima

```
┌─────────────┐                    ┌─────────────┐
│ Orchestrator│                    │    Task 1   │
└──────┬──────┘                    └──────┬──────┘
       │                                  │
       │  StartTask()                     │
       ├─────────────────────────────────>│
       │                                  │ start_time = NOW  (task clock)
       │                                  │ ... execute ...
       │                                  │ end_time = NOW    (task clock)
       │  TaskEnd(start_time, end_time)   │
       │<─────────────────────────────────┤
       │                                  │
    Usa tempi ricevuti dal task
    (dipendenza da sync clock)
```

### Dopo

```
┌─────────────┐                    ┌─────────────┐
│ Orchestrator│                    │    Task 1   │
└──────┬──────┘                    └──────┬──────┘
       │                                  │
    start_time = NOW                      │
       │  StartTask()                     │
       ├─────────────────────────────────>│
       │                                  │
       │                                  │ ... execute ...
       │                                  │
       │  TaskEnd(result, output)         │
       │<─────────────────────────────────┤
    end_time = NOW                        │
       │                                  │
    Calcola duration = end - start
    (NO dipendenza da task clock)
```

---

## 🎯 Vantaggi

### 1. Semplificazione
- ❌ **Prima**: Task dovevano misurare e inviare tempi
- ✅ **Dopo**: Task eseguono solo la logica, nessuna gestione tempi

### 2. Affidabilità
- ❌ **Prima**: Dipendenza da sincronizzazione clock tra container
- ✅ **Dopo**: Solo il clock dell'orchestrator è rilevante

### 3. Consistenza
- ❌ **Prima**: Possibili discrepanze tra clock diversi
- ✅ **Dopo**: Unico punto di misurazione

### 4. Realismo
- ❌ **Prima**: Duration non includeva latenza gRPC
- ✅ **Dopo**: Duration include tutto l'overhead reale

---

## 📝 Cosa Include la Duration Ora

La duration misurata dall'orchestrator include:

1. **Latenza gRPC andata** (orchestrator → task)
2. **Setup del task** (parsing parametri, preparazione)
3. **Esecuzione effettiva del task**
4. **Cleanup del task** (preparazione output)
5. **Latenza gRPC ritorno** (task → orchestrator)

Questo è il **tempo reale end-to-end** percepito dal sistema.

---

## 🔧 Impatto sul Codice Esistente

### File Modificati
1. `src/orchestrator.cpp` - Misurazione centralizzata
2. `include/deploy_manager.h` - Nuovo metodo `deploy_tasks_only()`
3. `docs/TIME_MEASUREMENT.md` - Documentazione aggiornata

### File Task (Nessuna Modifica Necessaria)
- I task continuano a funzionare normalmente
- I campi `start_time_us` e `end_time_us` nel protobuf esistono ancora
- Ma l'orchestrator non li usa più

---

## 📚 Documentazione

- **Guida completa**: `docs/TIME_MEASUREMENT.md`
- **Architettura**: `ARCHITETTURA_REFACTORED.md`
- **Questo file**: `CHANGELOG_TIME_MEASUREMENT.md`

---

## 🧪 Testing

### Verifica del Comportamento

1. **Compila**:
   ```bash
   cd build && cmake .. && make
   ```

2. **Deploy**:
   ```bash
   ./bin/deploy_manager_main deploy
   ```

3. **Verifica logs**:
   ```bash
   docker logs grpc_orchestrator
   ```

4. **Cerca**:
   - `actual_start_time_us` - misurato dall'orchestrator
   - `end_time_us` - misurato dall'orchestrator
   - Duration corrispondente

---

## ✅ Checklist

- [x] Orchestrator misura start time
- [x] Orchestrator misura end time
- [x] Rimosso uso di `response.actual_start_time_us()`
- [x] Rimosso uso di `notification.end_time_us()`
- [x] Aggiunto `deploy_tasks_only()` method
- [x] Aggiornata documentazione
- [x] Commentato il codice

---

## 🎉 Risultato

Ora hai un sistema con **misurazione centralizzata dei tempi** che è:
- ✅ Più semplice
- ✅ Più affidabile
- ✅ Più realistico
- ✅ Indipendente dai clock dei task
