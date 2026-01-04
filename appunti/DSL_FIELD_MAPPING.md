# DSL Field Mapping - C++ ↔ YAML

## Complete Field Mapping

This document shows the **exact mapping** between C++ `ScheduledTask` fields and YAML DSL fields.

## 📊 Field-by-Field Mapping

| C++ Field | YAML Field | Type | Required | Default | Notes |
|-----------|------------|------|----------|---------|-------|
| `task_id` | `id` | string | ✅ Yes | - | Unique identifier |
| `task_address` | `address` | string | ✅ Yes | - | Auto-converted for Docker/native |
| `execution_mode` | `mode` | enum | ✅ Yes | - | `"sequential"` or `"timed"` |
| `scheduled_time_us` | `scheduled_time_us` | int64 | ⚠️ Conditional | 0 | Required for `timed` mode |
| `deadline_us` | `deadline_us` | int64 | ❌ No | 1000000 | From defaults or built-in |
| `estimated_duration_us` | `estimated_duration_us` | int64 | ❌ No | 1000000 | Built-in default |
| `wait_for_task_id` | `depends_on` | string | ❌ No | "" | Only for `sequential` mode |
| `parameters` | `parameters` | map | ❌ No | {} | Key-value pairs |

## 🔍 Detailed Examples

### Example 1: Minimal Sequential Task

**C++ Code:**
```cpp
ScheduledTask task1;
task1.task_id = "task_1";
task1.task_address = "task1:50051";
task1.execution_mode = TASK_MODE_SEQUENTIAL;
task1.scheduled_time_us = 0;
task1.deadline_us = 1000000;        // default
task1.estimated_duration_us = 1000000; // default
task1.wait_for_task_id = "";
task1.parameters["task_id"] = "task_1"; // auto-added
```

**YAML Equivalent:**
```yaml
- id: task_1
  address: "task1:50051"
  mode: sequential
```

All other fields use defaults!

---

### Example 2: Full Sequential Task (All Parameters)

**C++ Code:**
```cpp
ScheduledTask task1;
task1.task_id = "task_1";
task1.task_address = "task1:50051";
task1.scheduled_time_us = 0;
task1.deadline_us = 3000000;
task1.priority = 10;
task1.parameters["mode"] = "fast";
task1.parameters["iterations"] = "100";
task1.parameters["task_id"] = "task_1";
task1.estimated_duration_us = 500000;
task1.critical = true;
task1.execution_mode = TASK_MODE_SEQUENTIAL;
task1.wait_for_task_id = "";
```

**YAML Equivalent:**
```yaml
- id: task_1
  address: "task1:50051"
  mode: sequential
  scheduled_time_us: 0
  deadline_us: 3000000
  priority: 10
  critical: true
  estimated_duration_us: 500000
  # depends_on: ""  # Empty or omitted
  parameters:
    mode: "fast"
    iterations: "100"
    # task_id added automatically
```

---

### Example 3: Timed Task

**C++ Code:**
```cpp
ScheduledTask task2;
task2.task_id = "task_2";
task2.task_address = "task2:50051";
task2.scheduled_time_us = 8000000;  // 8 seconds
task2.deadline_us = 1000000;
task2.priority = 10;
task2.parameters["mode"] = "normal";
task2.parameters["data_size"] = "1024";
task2.parameters["task_id"] = "task_2";
task2.estimated_duration_us = 800000;
task2.critical = false;
task2.execution_mode = TASK_MODE_TIMED;
task2.wait_for_task_id = "";  // Ignored for timed
```

**YAML Equivalent:**
```yaml
- id: task_2
  address: "task2:50051"
  mode: timed
  scheduled_time_us: 8000000
  deadline_us: 1000000
  priority: 10
  critical: false
  estimated_duration_us: 800000
  parameters:
    mode: "normal"
    data_size: "1024"
```

---

### Example 4: Sequential Task with Dependency

**C++ Code:**
```cpp
ScheduledTask task3;
task3.task_id = "task_3";
task3.task_address = "task3:50051";
task3.scheduled_time_us = 0;  // Ignored (uses dependency)
task3.deadline_us = 3000000;
task3.priority = 10;
task3.parameters["mode"] = "slow";
task3.parameters["quality"] = "high";
task3.parameters["task_id"] = "task_3";
task3.estimated_duration_us = 500000;
task3.critical = true;
task3.execution_mode = TASK_MODE_SEQUENTIAL;
task3.wait_for_task_id = "task_1";  // Wait for task_1
```

**YAML Equivalent:**
```yaml
- id: task_3
  address: "task3:50051"
  mode: sequential
  depends_on: task_1  # ← This sets wait_for_task_id
  scheduled_time_us: 0
  deadline_us: 3000000
  priority: 10
  critical: true
  estimated_duration_us: 500000
  parameters:
    mode: "slow"
    quality: "high"
```

## 🎯 Special Handling

### 1. Address Specification

**YAML (Docker):**
```yaml
address: "task1:50051"  # task1 on port 50051
address: "task2:50052"  # task2 on port 50052
address: "task3:50053"  # task3 on port 50053
```

**YAML (Native):**
```yaml
address: "localhost:50051"
address: "localhost:50052"
address: "localhost:50053"
```

**C++ (uses address as-is from YAML):**
```cpp
task.task_address = task_node["address"].as<std::string>();
```

### 2. Execution Mode

**YAML:**
```yaml
mode: sequential  # or "timed"
```

**C++ Enum:**
```cpp
task.execution_mode = TASK_MODE_SEQUENTIAL;  // or TASK_MODE_TIMED
```

### 3. Parameters

**YAML:**
```yaml
parameters:
  mode: "fast"
  iterations: "100"
```

**C++ Map:**
```cpp
task.parameters["mode"] = "fast";
task.parameters["iterations"] = "100";
task.parameters["task_id"] = "task_3";  // Auto-added by parser
```

### 4. Defaults

**YAML:**
```yaml
defaults:
  deadline_us: 1000000
```

**C++ (applied if field not specified):**
```cpp
int64_t default_deadline_us = 1000000;

// Applied like this:
task.priority = task_node["priority"] ? 
                task_node["priority"].as<int>() : 
                default_priority;
```

## 📋 Complete C++ to YAML Example

**C++ Test Schedule:**
```cpp
ScheduledTask task1;
task1.task_id = "task_1";
task1.task_address = use_docker_hostnames ? "task1:50051" : "localhost:50051";
task1.scheduled_time_us = 0;
task1.deadline_us = 3000000;
task1.priority = 10;
task1.parameters["mode"] = "fast";
task1.parameters["iterations"] = "100";
task1.parameters["task_id"] = "task_1";
task1.estimated_duration_us = 500000;
task1.critical = true;
task1.execution_mode = TASK_MODE_SEQUENTIAL;
task1.wait_for_task_id = "";
```

**Equivalent YAML:**
```yaml
schedule:
  name: "Test Schedule"
  
  tasks:
    - id: task_1
      address: "task1:50051"
      mode: sequential
      scheduled_time_us: 0
      deadline_us: 3000000
      priority: 10
      critical: true
      estimated_duration_us: 500000
      parameters:
        mode: "fast"
        iterations: "100"
```

## 🔄 Parser Logic

The parser in `src/schedule.cpp` does:

1. **Load YAML file**
   ```cpp
   YAML::Node config = YAML::LoadFile(yaml_path);
   ```

2. **Parse defaults**
   ```cpp
   int default_priority = 50;
   if (sched["defaults"]["priority"]) 
       default_priority = sched["defaults"]["priority"].as<int>();
   ```

3. **For each task:**
   ```cpp
   task.task_id = task_node["id"].as<std::string>();
   task.task_address = task_node["address"].as<std::string>();
   
   std::string mode = task_node["mode"].as<std::string>();
   if (mode == "sequential") {
       task.execution_mode = TASK_MODE_SEQUENTIAL;
   } else if (mode == "timed") {
       task.execution_mode = TASK_MODE_TIMED;
       task.scheduled_time_us = task_node["scheduled_time_us"].as<int64_t>();
   }
   
   task.priority = task_node["priority"] ? 
                   task_node["priority"].as<int>() : 
                   default_priority;
   
   if (task_node["depends_on"]) {
       task.wait_for_task_id = task_node["depends_on"].as<std::string>();
   }
   
   // ... etc
   ```

4. **Auto-add task_id to parameters**
   ```cpp
   task.parameters["task_id"] = task.task_id;
   ```

## ✅ Validation Checklist

When creating YAML schedules, ensure:

- [x] `id` is unique across all tasks
- [x] `address` includes port (e.g., `:50051`)
- [x] `mode` is either `sequential` or `timed`
- [x] `scheduled_time_us` is present for `timed` tasks
- [x] `depends_on` references existing task ID
- [x] All numeric values are valid (no negative times)
- [x] Parameters are key-value string pairs

## 📚 See Also

- `DSL_GUIDE.md` - Complete DSL reference
- `schedules/template_full.yaml` - Full template with all fields
- `schedules/example_hybrid.yaml` - Working example
- `src/schedule.cpp` - Parser implementation

---

**All C++ fields are supported in the YAML DSL!** 🎯
