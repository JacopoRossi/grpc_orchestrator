# Task Schedule DSL - Implementation Summary

## ✅ What Has Been Implemented

### 1. **YAML DSL Parser** (`src/schedule.cpp`)
- ✅ Full YAML parsing using `yaml-cpp` library
- ✅ Support for schedule metadata (name, description)
- ✅ Default values for common fields
- ✅ Task configuration parsing
- ✅ Sequential and timed execution modes
- ✅ Dependency resolution (`depends_on`)
- ✅ Parameter passing to tasks
- ✅ Automatic address conversion (Docker ↔ Native)
- ✅ Error handling with fallback to test schedule

### 2. **Example Schedules** (`schedules/`)
- ✅ `example_hybrid.yaml` - Mix of sequential and timed
- ✅ `example_sequential.yaml` - Pure sequential pipeline
- ✅ `example_timed.yaml` - Pure timed execution
- ✅ `README.md` - Quick start guide

### 3. **Documentation**
- ✅ `DSL_GUIDE.md` - Complete DSL reference
- ✅ Field reference table
- ✅ Execution mode explanations
- ✅ Complete examples
- ✅ Best practices
- ✅ Common pitfalls

### 4. **Build System**
- ✅ Added `yaml-cpp` dependency to CMakeLists.txt
- ✅ Updated Dockerfile.orchestrator with yaml-cpp
- ✅ Schedule files copied to Docker image
- ✅ Docker-compose configured to use YAML schedule

## 🎯 DSL Features

### Supported Fields

**Schedule Level:**
- `name` - Schedule name
- `description` - Description
- `defaults` - Default values for all tasks

**Task Level:**
- `id` - Unique identifier (required)
- `address` - gRPC address (required)
- `mode` - `sequential` or `timed` (required)
- `depends_on` - Task dependency (sequential mode)
- `scheduled_time_us` - Execution time (timed mode)
- `deadline_us` - Task deadline
- `estimated_duration_us` - Estimated duration
- `parameters` - Key-value parameters

### Execution Modes

**Sequential:**
```yaml
- id: task_1
  mode: sequential
  depends_on: previous_task  # Optional
```

**Timed:**
```yaml
- id: task_2
  mode: timed
  scheduled_time_us: 1000000  # Required
```

## 📂 File Structure

```
grpc_orchestrator/
├── schedules/
│   ├── README.md
│   ├── example_hybrid.yaml
│   ├── example_sequential.yaml
│   └── example_timed.yaml
├── src/
│   └── schedule.cpp  (YAML parser implementation)
├── include/
│   └── schedule.h
├── CMakeLists.txt  (yaml-cpp dependency)
├── Dockerfile.orchestrator  (yaml-cpp + schedules)
├── docker-compose.yml  (--schedule parameter)
├── DSL_GUIDE.md  (Complete reference)
└── DSL_SUMMARY.md  (This file)
```

## 🚀 How to Use

### 1. Create a Schedule

```yaml
# my_schedule.yaml
schedule:
  name: "My Custom Schedule"
  
  tasks:
    - id: init
      address: "task1:50051"
      mode: sequential
      priority: 90
      parameters:
        action: "initialize"
    
    - id: process
      address: "task2:50051"
      mode: sequential
      depends_on: init
      parameters:
        algorithm: "fft"
```

### 2. Run with Docker

```bash
# Edit docker-compose.yml to use your schedule
sudo docker-compose build
sudo docker-compose up
```

### 3. Run Natively

```bash
sudo ./bin/orchestrator_main \
  --address 0.0.0.0:50050 \
  --schedule schedules/my_schedule.yaml \
  --policy fifo \
  --priority 80 \
  --lock-memory
```

## 🔧 Build Instructions

### Native Build

```bash
# Install yaml-cpp
sudo apt-get install libyaml-cpp-dev

# Build
cd build
cmake ..
make -j$(nproc)
```

### Docker Build

```bash
# Build images (includes yaml-cpp)
sudo docker-compose build

# Run
sudo docker-compose up
```

## 📊 Example Output

When loading a YAML schedule:

```
[ScheduleParser] Parsing YAML file: schedules/example_hybrid.yaml
[ScheduleParser] Schedule name: Hybrid Schedule Example
[ScheduleParser] Description: Mix of sequential and timed tasks
[ScheduleParser] Loaded task: task_1 (sequential)
[ScheduleParser] Loaded task: task_2 (timed)
[ScheduleParser] Loaded task: task_3 (sequential)
[ScheduleParser] Successfully loaded 3 tasks from YAML
```

## ✨ Key Benefits

1. **Declarative**: Define what to run, not how
2. **Readable**: YAML is human-friendly
3. **Flexible**: Mix sequential and timed tasks
4. **Reusable**: Share schedules across environments
5. **Validated**: Parser checks required fields
6. **Portable**: Same file works in Docker and native

## 🎓 Advanced Usage

### Dynamic Schedules

Generate schedules programmatically:

```python
# generate_schedule.py
import yaml

schedule = {
    'schedule': {
        'name': 'Generated Schedule',
        'tasks': []
    }
}

# Generate 10 timed tasks
for i in range(10):
    schedule['schedule']['tasks'].append({
        'id': f'sensor_{i}',
        'address': f'task{i%3+1}:50051',
        'mode': 'timed',
        'scheduled_time_us': i * 1000000,  # Every second
        'parameters': {
            'sensor_id': str(i)
        }
    })

with open('schedules/generated.yaml', 'w') as f:
    yaml.dump(schedule, f)
```

### Environment-Specific Schedules

```bash
# Development
--schedule schedules/dev_schedule.yaml

# Production
--schedule schedules/prod_schedule.yaml

# Testing
--schedule schedules/test_schedule.yaml
```

## 🐛 Troubleshooting

### YAML Parse Error

```
[ScheduleParser] YAML parsing error: bad conversion
[ScheduleParser] Falling back to test schedule
```

**Solution**: Check YAML syntax, ensure all required fields are present

### Missing Schedule File

```
[ScheduleParser] YAML parsing error: bad file
```

**Solution**: Verify file path is correct relative to working directory

### Invalid Mode

```
[ScheduleParser] Loaded task: task_1 (unknown)
```

**Solution**: Use `mode: sequential` or `mode: timed`

## 📚 Related Documentation

- `DSL_GUIDE.md` - Complete DSL reference
- `MODALITA_IBRIDA.md` - Hybrid scheduling details
- `GUIDA_COMPLETA.md` - Full usage guide
- `schedules/README.md` - Schedule examples

## 🎯 Next Steps

1. **Try the examples**:
   ```bash
   sudo docker-compose up
   ```

2. **Create your own schedule**:
   - Copy an example
   - Modify for your use case
   - Test with orchestrator

3. **Read the full guide**:
   - See `DSL_GUIDE.md` for all features
   - Check examples in `schedules/`

---

**The DSL is ready to use!** 🚀

Create your schedules in YAML and let the orchestrator handle the rest.
