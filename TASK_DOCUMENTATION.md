# Documentazione dei Task del Sistema gRPC Orchestrator

## Panoramica
Il sistema implementa una pipeline di elaborazione dati composta da tre task specializzati che possono essere eseguiti in modalità sequenziale o temporizzata. Ogni task simula un'operazione realistica di elaborazione dati con carichi computazionali variabili.

---

## Task 1: Data Analyzer
**File sorgente:** `examples/task1_data_analyzer.cpp`  
**Porta gRPC:** 50051  
**Durata stimata:** ~4.1 secondi  
**CPU affinity:** Core 1  

### Funzionalità
Il Data Analyzer è un task di analisi statistica che:
1. **Genera dati casuali**: Crea un dataset di dimensione configurabile (default: 500.000 punti)
2. **Calcola statistiche**:
   - Media (mean)
   - Valore minimo (min)
   - Valore massimo (max)
   - Deviazione standard (std_dev)
3. **Simula elaborazione pesante**: Esegue operazioni matematiche intensive per simulare un carico realistico
4. **Produce output JSON**: Le statistiche calcolate vengono restituite come JSON per essere utilizzate da altri task

### Parametri di input
```yaml
parameters:
  data_size: 500000  # Numero di punti dati da analizzare
```

### Output esempio
```json
{
  "mean": 0.4998,
  "min": 0.0001,
  "max": 0.9999,
  "std_dev": 0.2887,
  "data_points": 500000,
  "processing_time_ms": 4012
}
```

### Simulazione del carico
- Genera numeri casuali con distribuzione uniforme
- Esegue calcoli statistici complessi
- Include delay artificiali per simulare I/O o accesso a database

---

## Task 2: Image Processor
**File sorgente:** `examples/task2_image_processor.cpp`  
**Porta gRPC:** 50052  
**Durata stimata:** ~3.3 secondi  
**CPU affinity:** Core 1  

### Funzionalità
L'Image Processor simula l'elaborazione di immagini con filtri:
1. **Crea immagine virtuale**: Genera una matrice di pixel delle dimensioni specificate
2. **Applica filtri**:
   - Gaussian blur (sfocatura gaussiana)
   - Edge detection (rilevamento bordi)
   - Sharpening (nitidezza)
3. **Calcola metriche**:
   - Luminosità media
   - Contrasto
   - Istogramma dei colori
4. **Ottimizzazione**: Simula operazioni di compressione e ottimizzazione

### Parametri di input
```yaml
parameters:
  image_width: 800     # Larghezza immagine in pixel
  image_height: 600    # Altezza immagine in pixel
  filter: "gaussian"   # Tipo di filtro da applicare
```

### Output esempio
```json
{
  "width": 800,
  "height": 600,
  "filter_applied": "gaussian",
  "avg_brightness": 127.5,
  "contrast": 0.65,
  "processing_time_ms": 3238,
  "pixels_processed": 480000
}
```

### Simulazione del carico
- Operazioni su matrici 2D
- Convoluzione per filtri
- Trasformazioni pixel-per-pixel
- Calcoli di istogrammi e statistiche immagine

---

## Task 3: Report Generator
**File sorgente:** `examples/task3_report_generator.cpp`  
**Porta gRPC:** 50053  
**Durata stimata:** ~17 secondi  
**CPU affinity:** Core 1  
**Dipendenze:** Task 1 (riceve l'output come input)

### Funzionalità
Il Report Generator crea report completi basati sui dati ricevuti:
1. **Analisi dei dati di input**: Processa le statistiche ricevute da Task 1
2. **Machine Learning simulato**:
   - Clustering dei dati
   - Analisi predittiva
   - Rilevamento anomalie
3. **Generazione visualizzazioni**:
   - Grafici a barre
   - Grafici a torta
   - Serie temporali
4. **Creazione report**:
   - Sommario esecutivo
   - Analisi dettagliata
   - Raccomandazioni
5. **Aggregazioni temporali**: Simula l'analisi di dati storici

### Parametri di input
```yaml
parameters:
  report_depth: 5      # Numero di sezioni/grafici da generare
  # Riceve automaticamente dep_output da Task 1
```

### Input da dipendenze
Il task riceve automaticamente l'output di Task 1 nel campo `dep_output`:
```json
{
  "report_depth": 5,
  "dep_output": {
    "mean": 0.4998,
    "min": 0.0001,
    "max": 0.9999,
    "std_dev": 0.2887
  }
}
```

### Output esempio
```json
{
  "report_id": "RPT-2024-001",
  "sections_generated": 5,
  "charts": ["bar", "pie", "line", "scatter", "heatmap"],
  "ml_models": {
    "clustering": "k-means",
    "clusters_found": 3,
    "anomalies_detected": 2
  },
  "recommendations": [
    "Optimize data collection frequency",
    "Implement caching strategy",
    "Scale horizontally for peak loads"
  ],
  "processing_time_ms": 16099,
  "total_data_analyzed": 500000
}
```

### Simulazione del carico
- Algoritmi di ML (clustering K-means simulato)
- Generazione di grafici complessi
- Analisi multi-dimensionale
- Aggregazioni e join di dati
- Generazione di testo per report

---

## Modalità di Esecuzione

### Sequential Mode
I task vengono eseguiti uno dopo l'altro, rispettando le dipendenze:
- Task 1 → Task 3 (Task 3 attende il completamento di Task 1)
- Utilizzato quando l'output di un task è necessario come input per il successivo

### Timed Mode
I task vengono lanciati a tempi prestabiliti:
- Task 1: avvio a 5 secondi
- Task 2: avvio a 90 secondi
- Indipendenti tra loro, esecuzione parallela possibile

---

## Configurazione Real-Time

Tutti i task supportano configurazioni real-time per garantire prestazioni deterministiche:

```yaml
rt_policy: "fifo"      # Politica di scheduling (SCHED_FIFO)
rt_priority: 30        # Priorità RT (1-99)
cpu_affinity: 1        # Core CPU dedicato
```

### Politiche disponibili:
- **none**: Scheduling normale del kernel
- **fifo**: First-In-First-Out, esecuzione fino al completamento
- **rr**: Round-Robin, time-slicing con priorità
- **deadline**: Deadline scheduling per vincoli temporali stretti

---

## Metriche e Monitoraggio

Il sistema traccia per ogni task:
- **scheduled_time_us**: Tempo pianificato di avvio
- **actual_start_time_us**: Tempo effettivo di avvio
- **end_time_us**: Tempo di completamento
- **duration**: Durata effettiva dell'esecuzione
- **estimated_duration_us**: Durata stimata per il controllo prestazioni

### Controllo Prestazioni
Al termine dell'esecuzione, il sistema verifica:
- **OKOK**: Task completato entro il tempo stimato ✓
- **KOKO**: Task ha superato il tempo stimato ✗

---

## Esempio di Pipeline Completa

```
[Time: 0ms]      Orchestrator avviato
[Time: 5000ms]   → Task 1 (Data Analyzer) inizia
[Time: 9013ms]   ← Task 1 completato (4013ms di esecuzione)
[Time: 9016ms]   → Task 3 (Report Generator) inizia (riceve output Task 1)
[Time: 25115ms]  ← Task 3 completato (16099ms di esecuzione)
[Time: 90003ms]  → Task 2 (Image Processor) inizia
[Time: 93242ms]  ← Task 2 completato (3239ms di esecuzione)

=== Duration Check ===
task_1: OKOK (actual: 4012808 us <= estimated: 4100000 us)
task_3: OKOK (actual: 16099261 us <= estimated: 17000000 us)
task_2: OKOK (actual: 3238885 us <= estimated: 3300000 us)
```

---

## Note Implementative

1. **Simulazione realistica**: Ogni task simula operazioni computazionali reali con sleep e calcoli intensivi
2. **Scalabilità**: I parametri permettono di aumentare il carico (data_size, image dimensions, report_depth)
3. **Fault tolerance**: Gestione errori con retry e timeout configurabili
4. **Containerizzazione**: Ogni task gira in un container Docker separato per isolamento
5. **Comunicazione gRPC**: Protocollo efficiente per comunicazione inter-processo

---

## Utilizzo Tipico

1. **Data Pipeline**: Task 1 → Task 3 per analisi e reportistica
2. **Batch Processing**: Task 2 indipendente per elaborazione immagini in batch
3. **Real-time Analytics**: Configurazione RT per garantire latenze basse
4. **Load Testing**: Aumento parametri per stress test del sistema
