# Realistic Task Implementation

Questo documento descrive i tre task realistici implementati nel sistema di orchestrazione.

## Overview

Il sistema implementa una pipeline di elaborazione dati completa con **tre eseguibili separati**, ognuno specializzato in un compito specifico:

1. **`task1_data_analyzer`**: Analizza grandi dataset e calcola statistiche
2. **`task2_image_processor`**: Elabora immagini con filtri avanzati
3. **`task3_report_generator`**: Genera report completi dai risultati di Task 1

### Architettura

Ogni task è un **eseguibile indipendente** con il proprio file sorgente:
- `examples/task1_data_analyzer.cpp` → eseguibile `task1_data_analyzer`
- `examples/task2_image_processor.cpp` → eseguibile `task2_image_processor`
- `examples/task3_report_generator.cpp` → eseguibile `task3_report_generator`

Questo approccio offre:
- ✅ **Separazione delle responsabilità**: Ogni task è completamente autonomo
- ✅ **Manutenibilità**: Modifiche a un task non impattano gli altri
- ✅ **Deployment flessibile**: Puoi compilare/deployare solo i task necessari
- ✅ **Scalabilità**: Architettura più realistica per sistemi distribuiti

## Task 1: Data Analyzer

### Descrizione
Analizza un array di dati numerici e calcola statistiche descrittive complete.

### Input Parameters
- `data_size` (int): Numero di data points da analizzare (es. 500000)

### Output
```json
{
  "mean": 124.5,
  "min": 100.0,
  "max": 149.0,
  "std_dev": 14.2,
  "samples_analyzed": 500000
}
```

### Elaborazione
1. Genera dataset simulato (in produzione: lettura da file/database)
2. **Prima passata**: Calcola somma, minimo, massimo
   - Simula carico computazionale ogni 100k elementi
   - Sleep di 500ms per simulare I/O
3. **Seconda passata**: Calcola deviazione standard
   - Itera su tutti i dati per calcolare varianza
   - Simula elaborazione pesante

### Tempo di Esecuzione
- ~8 secondi per 500k data points
- Scalabile linearmente con data_size

### Use Case Reale
- Analisi di dati sensoriali IoT
- Preprocessing di dataset per ML
- Quality control su dati di produzione

---

## Task 2: Image Processor

### Descrizione
Elabora immagini applicando filtri Gaussian blur e edge detection (Sobel operator).

### Input Parameters
- `image_width` (int): Larghezza immagine in pixel (es. 800)
- `image_height` (int): Altezza immagine in pixel (es. 600)
- `filter` (string, optional): Tipo di filtro (default: "gaussian")

### Output
```json
{
  "pixels_processed": 480000,
  "edges_detected": 45230,
  "edge_density": 9.42,
  "filter_applied": "gaussian"
}
```

### Elaborazione
1. **Caricamento**: Genera immagine simulata (in produzione: lettura da file)
2. **Gaussian Blur**: Applica convoluzione 3x3
   - Itera su ogni pixel (eccetto bordi)
   - Simula carico computazionale ogni 50k pixel
3. **Edge Detection**: Applica operatore Sobel
   - Calcola gradienti Gx e Gy
   - Conta pixel con gradiente > threshold
4. **Output**: Statistiche sull'elaborazione

### Tempo di Esecuzione
- ~12 secondi per immagine 800x600
- Complessità O(width × height)

### Use Case Reale
- Computer vision per quality inspection
- Preprocessing per object detection
- Medical imaging analysis

---

## Task 3: Report Generator

### Descrizione
Genera un report completo e dettagliato partendo dai risultati di Task 1. Include analisi avanzate, simulazione ML, aggregazioni temporali e generazione visualizzazioni.

### Input Parameters
- `dep_output` (object): Output di Task 1 (automatico via dependency)
- `report_depth` (int, optional): Numero di grafici da generare (default: 3)

### Output
```json
{
  "samples": 500000,
  "mean": 124.5,
  "range": 49.0,
  "std_dev": 14.2,
  "coefficient_of_variation": 11.4,
  "z_score_min": -1.73,
  "z_score_max": 1.73,
  "prediction_accuracy": 0.87,
  "moving_averages_count": 50,
  "charts_generated": 5,
  "report_status": "complete"
}
```

### Elaborazione

#### Fase 1: Metriche Avanzate (2 secondi)
- Calcola range, coefficient of variation
- Calcola z-scores per min/max
- Identifica outliers potenziali

#### Fase 2: Analisi Predittiva ML (4 secondi)
- Simula training di modello ML
- 100 iterazioni con sleep progressivi
- Calcola prediction accuracy
- Progress logging ogni 20%

#### Fase 3: Aggregazioni Temporali (3 secondi)
- Calcola moving averages
- Window size adattivo basato su sample size
- Genera 50 punti di aggregazione

#### Fase 4: Visualizzazioni (report_depth × 1 secondo)
- Genera grafici (simulato)
- Export in formato report
- Progress per ogni chart generato

### Tempo di Esecuzione
- Base: ~9 secondi
- + 1 secondo per ogni chart (report_depth)
- Totale: ~14 secondi con report_depth=5

### Use Case Reale
- Business intelligence dashboards
- Automated reporting systems
- Data quality monitoring
- Predictive maintenance reports

---

## Pipeline Completa

### Flusso di Esecuzione

```
T=0s:  Task 1 (Data Analyzer) starts
       └─> Analizza 500k data points
       
T=2s:  Task 2 (Image Processor) starts (timed)
       └─> Elabora immagine 800x600 in parallelo
       
T=8s:  Task 1 completes
       └─> Output: statistiche
       
T=8s:  Task 3 (Report Generator) starts (depends on Task 1)
       └─> Fase 1: Metriche avanzate
       └─> Fase 2: ML simulation
       └─> Fase 3: Aggregazioni
       └─> Fase 4: Visualizzazioni
       
T=14s: Task 2 completes
       
T=23s: Task 3 completes
       └─> Output: report completo
```

### Caratteristiche
- **Parallelismo**: Task 2 esegue in parallelo con Task 1 e 3
- **Dependency**: Task 3 dipende da Task 1 (usa i suoi risultati)
- **CPU Affinity**: Ogni task su core diverso (0, 1, 2)
- **Real-time**: Tutti con SCHED_FIFO priority 30

---

## Testing

### Build e Run

```bash
# Build del progetto
docker-compose build

# Avvia i task
docker-compose up
```

### Verifica Output

Dovresti vedere:
1. Task 1 che processa 500k elementi con progress ogni 100k
2. Task 2 che elabora pixel con progress percentuale
3. Task 3 con 4 fasi di elaborazione e report finale dettagliato

### Modifica Parametri

Edita `schedules/example_parametrized.yaml`:

```yaml
# Aumenta dataset size
parameters:
  data_size: 1000000  # 1M data points

# Aumenta risoluzione immagine
parameters:
  image_width: 1920
  image_height: 1080

# Più grafici nel report
parameters:
  report_depth: 10
```

---

## Performance Tuning

### CPU Affinity
Ogni task è assegnato a un core specifico per evitare context switching:
- Task 1: Core 0
- Task 2: Core 1
- Task 3: Core 2

### Real-time Priority
Tutti i task usano SCHED_FIFO con priority 30 per garantire esecuzione deterministica.

### Deadline
- Task 1: 15s (analisi dati)
- Task 2: 20s (image processing)
- Task 3: 25s (report generation)

### Scalabilità
- **Task 1**: Lineare con data_size
- **Task 2**: Quadratica con dimensioni immagine
- **Task 3**: Lineare con report_depth, costante per ML simulation

---

## Estensioni Future

1. **Task 1**: Lettura da file CSV/database reale
2. **Task 2**: Supporto formati immagine reali (JPEG, PNG)
3. **Task 3**: Export report in PDF/HTML
4. **Pipeline**: Aggiungere Task 4 che combina risultati di Task 2 e 3
