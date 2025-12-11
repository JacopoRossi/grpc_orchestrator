#ifndef RT_UTILS_H
#define RT_UTILS_H

#include <string>
#include <pthread.h>

namespace orchestrator {

/**
 * Real-time scheduling policies
 */
enum RTSchedulingPolicy {
    RT_POLICY_NONE = 0,      // No real-time scheduling
    RT_POLICY_FIFO = 1,      // SCHED_FIFO - First In First Out
    RT_POLICY_RR = 2,        // SCHED_RR - Round Robin
    RT_POLICY_DEADLINE = 3   // SCHED_DEADLINE (Linux 3.14+)
};

/**
 * Real-time configuration parameters
 */
struct RTConfig {
    RTSchedulingPolicy policy;
    int priority;              // Priority: 1-99 (99 = highest)
    int cpu_affinity;          // CPU core to bind to (-1 = no affinity)
    bool lock_memory;          // Lock all memory pages
    bool prefault_stack;       // Pre-fault stack to avoid page faults
    size_t stack_size;         // Thread stack size (0 = default)
    
    RTConfig()
        : policy(RT_POLICY_NONE)
        , priority(50)
        , cpu_affinity(-1)
        , lock_memory(false)
        , prefault_stack(false)
        , stack_size(0) {}
};

/**
 * Real-time utility functions
 */
class RTUtils {
public:
    /**
     * Lock all current and future memory pages to prevent page faults
     * This is critical for real-time performance
     * @return true on success, false on failure
     */
    static bool lock_memory();
    
    /**
     * Unlock all locked memory pages
     * @return true on success, false on failure
     */
    static bool unlock_memory();
    
    /**
     * Pre-fault stack memory to avoid page faults during execution
     * @param size Size of stack to pre-fault in bytes
     */
    static void prefault_stack(size_t size = 8 * 1024 * 1024);
    
    /**
     * Set real-time scheduling policy for current thread
     * @param policy Scheduling policy (FIFO, RR, DEADLINE)
     * @param priority Priority level (1-99, 99 = highest)
     * @return true on success, false on failure
     */
    static bool set_thread_realtime(RTSchedulingPolicy policy, int priority);
    
    /**
     * Set real-time scheduling policy for specific thread
     * @param thread Thread handle
     * @param policy Scheduling policy
     * @param priority Priority level (1-99)
     * @return true on success, false on failure
     */
    static bool set_thread_realtime(pthread_t thread, RTSchedulingPolicy policy, int priority);
    
    /**
     * Set CPU affinity for current thread
     * @param cpu_id CPU core ID to bind to
     * @return true on success, false on failure
     */
    static bool set_cpu_affinity(int cpu_id);
    
    /**
     * Set CPU affinity for specific thread
     * @param thread Thread handle
     * @param cpu_id CPU core ID to bind to
     * @return true on success, false on failure
     */
    static bool set_cpu_affinity(pthread_t thread, int cpu_id);
    
    /**
     * Apply complete real-time configuration to current thread
     * @param config Real-time configuration
     * @return true on success, false on failure
     */
    static bool apply_rt_config(const RTConfig& config);
    
    /**
     * Apply complete real-time configuration to specific thread
     * @param thread Thread handle
     * @param config Real-time configuration
     * @return true on success, false on failure
     */
    static bool apply_rt_config(pthread_t thread, const RTConfig& config);
    
    /**
     * Get maximum priority for a scheduling policy
     * @param policy Scheduling policy
     * @return Maximum priority value
     */
    static int get_max_priority(RTSchedulingPolicy policy);
    
    /**
     * Get minimum priority for a scheduling policy
     * @param policy Scheduling policy
     * @return Minimum priority value
     */
    static int get_min_priority(RTSchedulingPolicy policy);
    
    /**
     * Check if current process has real-time capabilities
     * @return true if RT capabilities are available
     */
    static bool has_rt_capabilities();
    
    /**
     * Get string representation of scheduling policy
     * @param policy Scheduling policy
     * @return Policy name as string
     */
    static std::string policy_to_string(RTSchedulingPolicy policy);
    
    /**
     * Parse scheduling policy from string
     * @param policy_str Policy name ("fifo", "rr", "deadline", "none")
     * @return Scheduling policy enum
     */
    static RTSchedulingPolicy string_to_policy(const std::string& policy_str);

private:
    static int policy_to_sched_policy(RTSchedulingPolicy policy);
};

} // namespace orchestrator

#endif // RT_UTILS_H
