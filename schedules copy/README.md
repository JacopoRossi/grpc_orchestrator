# Task Schedules

This directory contains YAML schedule files for the orchestrator.

## 📁 Available Schedules

### `example_hybrid.yaml`
Mix of sequential and timed tasks. Demonstrates:
- Sequential task with no dependencies
- Timed task at 8 seconds
- Sequential task depending on first task

**Use case**: Testing hybrid scheduling

### `example_sequential.yaml`
Pure sequential pipeline. Demonstrates:
- Data acquisition → Preprocessing → Analysis → Storage
- Each task waits for previous to complete
- Different priorities and parameters

**Use case**: Data processing pipelines

### `example_timed.yaml`
Pure timed execution. Demonstrates:
- Immediate initialization
- Periodic sensor reads at 1s, 2s
- Data aggregation at 3s
- Report generation at 5s

**Use case**: Periodic monitoring and reporting

## 🚀 How to Use

### With Docker

Edit `docker-compose.yml` to change the schedule:

```yaml
command: >
  ./orchestrator_main
  --address 0.0.0.0:50050
  --schedule schedules/example_sequential.yaml  # ← Change this
  --policy fifo
  --priority 80
  --lock-memory
```

Then:
```bash
sudo docker-compose build
sudo docker-compose up
```

### With Native Execution

```bash
sudo ./bin/orchestrator_main \
  --address 0.0.0.0:50050 \
  --schedule schedules/example_timed.yaml \
  --policy fifo \
  --priority 80 \
  --cpu-affinity 0 \
  --lock-memory
```

## ✏️ Creating Custom Schedules

1. Copy an example file:
   ```bash
   cp schedules/example_hybrid.yaml schedules/my_schedule.yaml
   ```

2. Edit the YAML file with your tasks

3. Validate syntax (optional):
   ```bash
   yamllint schedules/my_schedule.yaml
   ```

4. Run with your schedule:
   ```bash
   sudo docker-compose run orchestrator \
     ./orchestrator_main \
     --schedule schedules/my_schedule.yaml \
     ...
   ```

## 📖 Documentation

See `DSL_GUIDE.md` for complete DSL reference and examples.

## 🎯 Quick Reference

```yaml
schedule:
  name: "My Schedule"
  
  defaults:
    priority: 50
    
  tasks:
    # Sequential task
    - id: task_1
      address: "task1:50051"
      mode: sequential
      depends_on: previous_task  # optional
      
    # Timed task
    - id: task_2
      address: "task2:50051"
      mode: timed
      scheduled_time_us: 1000000  # 1 second
      
      parameters:
        key: "value"
```
