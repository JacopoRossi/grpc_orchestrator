# Task Schedule DSL Guide

## 📋 Overview

The orchestrator supports a **YAML-based DSL** (Domain Specific Language) for defining task schedules. This allows you to declaratively specify tasks, their execution modes, dependencies, and parameters.

## 🎯 DSL Structure

### Basic Structure

```yaml
schedule:
  name: "Schedule Name"
  description: "Schedule description"
  
  defaults:
    priority: 50
    max_retries: 3
    critical: false
    deadline_us: 1000000
    
  tasks:
    - id: task_1
      address: "task1:50051"
      mode: sequential
      # ... task configuration
```

## 📖 Field Reference

### Schedule Metadata

| Field | Type | Required | Description |
|-------|------|----------|-------------|
| `name` | string | No | Human-readable schedule name |
| `description` | string | No | Schedule description |

### Defaults Section

Default values applied to all tasks (can be overridden per task):

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `priority` | int | 50 | Task priority (0-100, higher = more important) |
| `max_retries` | int | 3 | Maximum retry attempts on failure |
| `critical` | bool | false | Whether task failure aborts the schedule |
| `deadline_us` | int64 | 1000000 | Task deadline in microseconds |

### Task Configuration

#### Required Fields

| Field | Type | Description |
|-------|------|-------------|
| `id` | string | Unique task identifier |
| `address` | string | Task gRPC address (e.g., `task1:50051` for Docker, `localhost:50051` for native) |
| `mode` | string | Execution mode: `sequential` or `timed` |

#### Mode-Specific Fields

**Sequential Mode:**
- `depends_on` (string, optional): ID of task to wait for before starting

**Timed Mode:**
- `scheduled_time_us` (int64, required): Time in microseconds from schedule start

#### Optional Fields

| Field | Type | Description |
|-------|------|-------------|
| `priority` | int | Task priority (overrides default) |
| `max_retries` | int | Max retries (overrides default) |
| `critical` | bool | Critical flag (overrides default) |
| `deadline_us` | int64 | Deadline (overrides default) |
| `estimated_duration_us` | int64 | Estimated task duration in microseconds |
| `parameters` | map | Key-value parameters passed to task |

## 🎨 Execution Modes

### 1. Sequential Mode

Tasks execute in order, optionally waiting for dependencies.

```yaml
tasks:
  - id: task_1
    address: "task1:50051"
    mode: sequential
    # No dependency, starts immediately
    
  - id: task_2
    address: "task2:50051"
    mode: sequential
    depends_on: task_1  # Waits for task_1 to complete
```

**Timeline:**
```
T=0:    task_1 starts
T=500:  task_1 completes
T=503:  task_2 starts (after task_1)
T=1003: task_2 completes
```

### 2. Timed Mode

Tasks execute at specific scheduled times, running concurrently.

```yaml
tasks:
  - id: task_1
    address: "task1:50051"
    mode: timed
    scheduled_time_us: 0  # Start immediately
    
  - id: task_2
    address: "task2:50051"
    mode: timed
    scheduled_time_us: 2000000  # Start at 2 seconds
```

**Timeline:**
```
T=0:    task_1 starts
T=2000: task_2 starts (concurrent with task_1 if still running)
```

### 3. Hybrid Mode

Mix sequential and timed tasks for complex workflows.

```yaml
tasks:
  - id: init
    mode: sequential
    # Starts immediately
    
  - id: periodic_monitor
    mode: timed
    scheduled_time_us: 1000000  # Every second (if repeated)
    
  - id: cleanup
    mode: sequential
    depends_on: init  # After init completes
```

## 📝 Complete Examples

### Example 1: Simple Sequential Pipeline

```yaml
schedule:
  name: "Data Processing Pipeline"
  description: "Sequential data processing"
  
  defaults:
    priority: 50
    max_retries: 2
    
  tasks:
    - id: acquire_data
      address: "task1:50051"
      mode: sequential
      priority: 90
      critical: true
      estimated_duration_us: 1000000
      parameters:
        source: "sensor_array"
        sample_rate: 1000
      
    - id: process_data
      address: "task2:50051"
      mode: sequential
      depends_on: acquire_data
      priority: 80
      estimated_duration_us: 500000
      parameters:
        algorithm: "fft"
        
    - id: store_results
      address: "task3:50051"
      mode: sequential
      depends_on: process_data
      priority: 70
      estimated_duration_us: 300000
      parameters:
        database: "timeseries_db"
```

### Example 2: Timed Monitoring

```yaml
schedule:
  name: "Periodic Monitoring"
  description: "Concurrent sensor monitoring"
  
  tasks:
    - id: temp_sensor
      address: "task1:50051"
      mode: timed
      scheduled_time_us: 0
      priority: 80
      parameters:
        sensor_type: "temperature"
      
    - id: pressure_sensor
      address: "task2:50051"
      mode: timed
      scheduled_time_us: 500000  # 500ms offset
      priority: 80
      parameters:
        sensor_type: "pressure"
      
    - id: aggregate
      address: "task3:50051"
      mode: timed
      scheduled_time_us: 1000000  # After 1 second
      priority: 70
      parameters:
        window_size: 1000
```

### Example 3: Hybrid Workflow

```yaml
schedule:
  name: "Hybrid Schedule"
  description: "Mix of sequential and timed tasks"
  
  tasks:
    # Sequential initialization
    - id: init_system
      address: "task1:50051"
      mode: sequential
      priority: 95
      critical: true
      parameters:
        action: "initialize"
    
    # Timed background monitoring (runs concurrently)
    - id: monitor
      address: "task2:50051"
      mode: timed
      scheduled_time_us: 0
      priority: 60
      parameters:
        interval_ms: 100
    
    # Sequential processing after init
    - id: main_task
      address: "task3:50051"
      mode: sequential
      depends_on: init_system
      priority: 85
      parameters:
        workload: "heavy"
    
    # Timed report at 5 seconds
    - id: generate_report
      address: "task1:50051"
      mode: timed
      scheduled_time_us: 5000000
      priority: 70
      parameters:
        format: "json"
```

## 🚀 Usage

### Docker

```bash
# Use default schedule (example_hybrid.yaml)
sudo docker-compose up

# Use custom schedule
sudo docker-compose run orchestrator \
  ./orchestrator_main \
  --address 0.0.0.0:50050 \
  --schedule schedules/my_schedule.yaml \
  --policy fifo \
  --priority 80 \
  --lock-memory
```

### Native

```bash
sudo ./bin/orchestrator_main \
  --address 0.0.0.0:50050 \
  --schedule /path/to/schedule.yaml \
  --policy fifo \
  --priority 80 \
  --cpu-affinity 0 \
  --lock-memory
```

## 📂 Schedule Files Location

- **Docker**: `/app/schedules/` inside container
- **Native**: `./schedules/` relative to working directory

Provided examples:
- `schedules/example_hybrid.yaml` - Mix of sequential and timed
- `schedules/example_sequential.yaml` - Pure sequential pipeline
- `schedules/example_timed.yaml` - Pure timed execution

## 🎯 Best Practices

### 1. Use Meaningful IDs
```yaml
# Good
- id: data_acquisition
- id: preprocessing
- id: analysis

# Bad
- id: task1
- id: task2
```

### 2. Set Realistic Durations
```yaml
- id: heavy_computation
  estimated_duration_us: 5000000  # 5 seconds
  deadline_us: 6000000  # 6 seconds (20% margin)
```

### 3. Use Priorities Wisely
```yaml
- id: critical_safety_check
  priority: 95
  critical: true

- id: optional_logging
  priority: 30
  critical: false
```

### 4. Group Related Parameters
```yaml
parameters:
  # Sensor configuration
  sensor_id: "temp_01"
  sensor_type: "thermocouple"
  sample_rate: 1000
  
  # Processing configuration
  filter: "lowpass"
  cutoff_freq: 100
```

### 5. Document Complex Schedules
```yaml
schedule:
  name: "Production Pipeline v2.1"
  description: |
    Multi-stage data processing pipeline:
    1. Acquire data from sensors
    2. Apply filters and preprocessing
    3. Run ML inference
    4. Store results in database
    
    Critical tasks: data_acquisition, ml_inference
    Expected duration: ~3 seconds
```

## ⚠️ Common Pitfalls

### 1. Circular Dependencies
```yaml
# ❌ WRONG - Circular dependency
- id: task_a
  depends_on: task_b
  
- id: task_b
  depends_on: task_a
```

### 2. Invalid Addresses
```yaml
# ❌ WRONG - Missing port
- id: task_1
  address: "task1"

# ✅ CORRECT
- id: task_1
  address: "task1:50051"
```

### 3. Missing Required Fields
```yaml
# ❌ WRONG - Missing mode
- id: task_1
  address: "task1:50051"

# ✅ CORRECT
- id: task_1
  address: "task1:50051"
  mode: sequential
```

### 4. Timed Task Without Schedule Time
```yaml
# ❌ WRONG
- id: task_1
  mode: timed
  # Missing scheduled_time_us!

# ✅ CORRECT
- id: task_1
  mode: timed
  scheduled_time_us: 1000000
```

## 🔧 Validation

The parser will:
- ✅ Validate required fields
- ✅ Apply defaults for missing optional fields
- ✅ Convert addresses for Docker/native execution
- ✅ Report parsing errors with line numbers
- ⚠️ Fall back to test schedule on error

## 📊 Time Units

All times are in **microseconds (µs)**:

| Unit | Microseconds | Example |
|------|--------------|---------|
| 1 ms | 1,000 | `1000` |
| 100 ms | 100,000 | `100000` |
| 1 second | 1,000,000 | `1000000` |
| 5 seconds | 5,000,000 | `5000000` |
| 1 minute | 60,000,000 | `60000000` |

## 🎓 Advanced Features

### Dynamic Parameters

Parameters are passed as strings to tasks:

```yaml
parameters:
  mode: "fast"
  iterations: "100"  # Will be converted by task
  enable_logging: "true"  # Will be parsed as bool by task
```

### Address Mapping

The parser automatically converts addresses:

```yaml
# In YAML (Docker hostnames)
address: "task1:50051"

# Converted to (native execution)
address: "localhost:50051"
```

Mapping:
- `task1:50051` → `localhost:50051`
- `task2:50051` → `localhost:50052`
- `task3:50051` → `localhost:50053`

## 📚 See Also

- `MODALITA_IBRIDA.md` - Hybrid scheduling details
- `GUIDA_COMPLETA.md` - Complete usage guide
- `FIX_RACE_CONDITION.md` - Dependency handling

---

**Happy Scheduling!** 🚀
