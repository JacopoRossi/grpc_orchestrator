# Time Measurement System

## Overview

The orchestrator uses a **globally synchronized time measurement system** based on `std::chrono::system_clock` to ensure accurate timing analysis across all containers.

## Clock Selection

### `system_clock` - Global Synchronization
- **Used for**: All timestamp measurements (start, end, duration)
- **Precision**: Microseconds (µs)
- **Scope**: Global across all containers
- **Synchronization**: Via host system clock sharing

### Why `system_clock` instead of `steady_clock`?

| Aspect | `steady_clock` | `system_clock` |
|--------|----------------|----------------|
| **Monotonic** | ✅ Yes | ⚠️ Can be adjusted |
| **Cross-container** | ❌ No (independent per container) | ✅ Yes (synchronized) |
| **Comparable timestamps** | ❌ No | ✅ Yes |
| **Best for** | Single-process duration | Distributed system timing |

In a **multi-container environment**, `system_clock` is essential for:
- Comparing timestamps between orchestrator and tasks
- Measuring end-to-end latency including gRPC communication
- Global event ordering
- Accurate performance analysis

## Time Synchronization

### Docker Configuration

All containers share the host's system clock via volume mounts:

```yaml
volumes:
  - /etc/localtime:/etc/localtime:ro
  - /etc/timezone:/etc/timezone:ro
environment:
  - TZ=UTC
```

This ensures:
- All containers use the same time reference
- Timestamps are directly comparable
- No clock drift between containers

## Measured Metrics

### 1. Task Execution Duration
```cpp
execution_duration_us = end_time_us - start_time_us
```
- **Measured in**: Task container
- **Precision**: Microseconds
- **Reliability**: ✅ High (same clock source)

### 2. Task Start Time (Absolute)
```cpp
start_time_us = get_current_time_us()  // system_clock
```
- **Measured in**: Task container
- **Sent to**: Orchestrator via gRPC
- **Comparable**: ✅ Yes (synchronized clock)

### 3. Task End Time (Absolute)
```cpp
end_time_us = get_current_time_us()  // system_clock
```
- **Measured in**: Task container
- **Sent to**: Orchestrator via gRPC
- **Comparable**: ✅ Yes (synchronized clock)

### 4. Orchestrator Relative Time
```cpp
relative_time_us = get_current_time_us() - start_time_us_
```
- **Reference**: Orchestrator start time
- **Used for**: Scheduling and internal timing
- **Comparable with task times**: ✅ Yes (same clock)

## Timing Guarantees

### What You Can Rely On

✅ **Task execution duration** - Accurate to microseconds
✅ **Task start/end timestamps** - Globally synchronized
✅ **Event ordering** - Correct across all containers
✅ **Scheduling precision** - Microsecond-level timing
✅ **gRPC latency measurement** - Accurate end-to-end timing
✅ **Performance analysis** - All metrics are comparable

### Potential Limitations

⚠️ **System clock adjustments** - If the host system clock is adjusted (e.g., NTP sync), it may affect measurements in progress
⚠️ **Container startup** - Small initial clock sync delay (~100ms)

### Best Practices

1. **Ensure host NTP is configured** for accurate time
2. **Use UTC timezone** to avoid DST issues
3. **Compare timestamps directly** - they are globally synchronized
4. **Log absolute timestamps** for debugging and analysis
5. **Calculate durations** using the provided `execution_duration_us` field

## Code References

### Orchestrator
```cpp
// src/orchestrator.cpp:450-454
int64_t Orchestrator::get_current_time_us() const {
    return std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
}
```

### Task Wrapper
```cpp
// src/task_wrapper.cpp:320-324
int64_t TaskWrapper::get_current_time_us() const {
    return std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
}
```

## Example Analysis

With synchronized clocks, you can now accurately measure:

```cpp
// Task 1 starts at: 1704729600000000 µs (absolute)
// Task 1 ends at:   1704729600500000 µs (absolute)
// Duration:         500000 µs (0.5 seconds)

// Orchestrator receives notification at: 1704729600500100 µs
// gRPC latency: 100 µs
```

All these timestamps are **directly comparable** and **globally accurate**.

## Migration Notes

### Changed from `steady_clock` to `system_clock`

**Before**: Each container had independent `steady_clock` - timestamps were not comparable
**After**: All containers share `system_clock` - full global synchronization

**Impact**: 
- ✅ More accurate cross-container timing
- ✅ Reliable performance metrics
- ✅ Correct event ordering
- ⚠️ Requires host clock synchronization (NTP recommended)
