#include "rt_utils.h"
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/resource.h>
#include <sched.h>
#include <errno.h>
#include <algorithm>

namespace orchestrator {

bool RTUtils::lock_memory() {
    // Lock all current and future pages in memory
    if (mlockall(MCL_CURRENT | MCL_FUTURE) != 0) {
        std::cerr << "[RTUtils] Failed to lock memory: " << strerror(errno) << std::endl;
        std::cerr << "[RTUtils] Make sure to run with sufficient privileges or set ulimit -l" << std::endl;
        return false;
    }
    
    std::cout << "[RTUtils] Memory locked successfully" << std::endl;
    return true;
}

bool RTUtils::unlock_memory() {
    if (munlockall() != 0) {
        std::cerr << "[RTUtils] Failed to unlock memory: " << strerror(errno) << std::endl;
        return false;
    }
    
    std::cout << "[RTUtils] Memory unlocked successfully" << std::endl;
    return true;
}

void RTUtils::prefault_stack(size_t size) {
    // Allocate memory on stack and touch it to force page faults now
    unsigned char dummy[size];
    memset(dummy, 0, size);
    
    std::cout << "[RTUtils] Pre-faulted " << size << " bytes of stack" << std::endl;
}

bool RTUtils::set_thread_realtime(RTSchedulingPolicy policy, int priority) {
    return set_thread_realtime(pthread_self(), policy, priority);
}

bool RTUtils::set_thread_realtime(pthread_t thread, RTSchedulingPolicy policy, int priority) {
    if (policy == RT_POLICY_NONE) {
        std::cout << "[RTUtils] No real-time policy requested" << std::endl;
        return true;
    }
    
    int sched_policy = policy_to_sched_policy(policy);
    if (sched_policy == -1) {
        std::cerr << "[RTUtils] Invalid scheduling policy" << std::endl;
        return false;
    }
    
    // Validate priority range
    int min_prio = sched_get_priority_min(sched_policy);
    int max_prio = sched_get_priority_max(sched_policy);
    
    if (priority < min_prio || priority > max_prio) {
        std::cerr << "[RTUtils] Priority " << priority << " out of range [" 
                  << min_prio << ", " << max_prio << "]" << std::endl;
        return false;
    }
    
    // Set scheduling parameters
    struct sched_param param;
    memset(&param, 0, sizeof(param));
    param.sched_priority = priority;
    
    if (pthread_setschedparam(thread, sched_policy, &param) != 0) {
        std::cerr << "[RTUtils] Failed to set scheduling policy: " << strerror(errno) << std::endl;
        std::cerr << "[RTUtils] Make sure to run with CAP_SYS_NICE capability or as root" << std::endl;
        return false;
    }
    
    std::cout << "[RTUtils] Set thread to " << policy_to_string(policy) 
              << " with priority " << priority << std::endl;
    return true;
}

bool RTUtils::set_cpu_affinity(int cpu_id) {
    return set_cpu_affinity(pthread_self(), cpu_id);
}

bool RTUtils::set_cpu_affinity(pthread_t thread, int cpu_id) {
    if (cpu_id < 0) {
        std::cout << "[RTUtils] No CPU affinity requested" << std::endl;
        return true;
    }
    
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(cpu_id, &cpuset);
    
    if (pthread_setaffinity_np(thread, sizeof(cpu_set_t), &cpuset) != 0) {
        std::cerr << "[RTUtils] Failed to set CPU affinity to CPU " << cpu_id 
                  << ": " << strerror(errno) << std::endl;
        return false;
    }
    
    std::cout << "[RTUtils] Set CPU affinity to CPU " << cpu_id << std::endl;
    return true;
}

bool RTUtils::apply_rt_config(const RTConfig& config) {
    return apply_rt_config(pthread_self(), config);
}

bool RTUtils::apply_rt_config(pthread_t thread, const RTConfig& config) {
    std::cout << "[RTUtils] Applying real-time configuration:" << std::endl;
    std::cout << "  Policy: " << policy_to_string(config.policy) << std::endl;
    std::cout << "  Priority: " << config.priority << std::endl;
    std::cout << "  CPU Affinity: " << (config.cpu_affinity >= 0 ? std::to_string(config.cpu_affinity) : "none") << std::endl;
    std::cout << "  Lock Memory: " << (config.lock_memory ? "yes" : "no") << std::endl;
    std::cout << "  Prefault Stack: " << (config.prefault_stack ? "yes" : "no") << std::endl;
    
    bool success = true;
    
    // Lock memory if requested (process-wide operation)
    if (config.lock_memory) {
        if (!lock_memory()) {
            success = false;
        }
    }
    
    // Pre-fault stack if requested
    if (config.prefault_stack) {
        size_t stack_size = config.stack_size > 0 ? config.stack_size : 8 * 1024 * 1024;
        prefault_stack(stack_size);
    }
    
    // Set CPU affinity
    if (config.cpu_affinity >= 0) {
        if (!set_cpu_affinity(thread, config.cpu_affinity)) {
            success = false;
        }
    }
    
    // Set real-time scheduling policy
    if (config.policy != RT_POLICY_NONE) {
        if (!set_thread_realtime(thread, config.policy, config.priority)) {
            success = false;
        }
    }
    
    if (success) {
        std::cout << "[RTUtils] Real-time configuration applied successfully" << std::endl;
    } else {
        std::cerr << "[RTUtils] Some real-time configurations failed" << std::endl;
    }
    
    return success;
}

int RTUtils::get_max_priority(RTSchedulingPolicy policy) {
    int sched_policy = policy_to_sched_policy(policy);
    if (sched_policy == -1) {
        return -1;
    }
    return sched_get_priority_max(sched_policy);
}

int RTUtils::get_min_priority(RTSchedulingPolicy policy) {
    int sched_policy = policy_to_sched_policy(policy);
    if (sched_policy == -1) {
        return -1;
    }
    return sched_get_priority_min(sched_policy);
}

bool RTUtils::has_rt_capabilities() {
    // Try to get current scheduling policy
    int policy = sched_getscheduler(0);
    if (policy == -1) {
        return false;
    }
    
    // Try to set a real-time priority (will fail if no capabilities)
    struct sched_param param;
    param.sched_priority = 1;
    
    if (sched_setscheduler(0, SCHED_FIFO, &param) == 0) {
        // Restore original policy
        param.sched_priority = 0;
        sched_setscheduler(0, SCHED_OTHER, &param);
        return true;
    }
    
    return false;
}

std::string RTUtils::policy_to_string(RTSchedulingPolicy policy) {
    switch (policy) {
        case RT_POLICY_NONE:
            return "NONE";
        case RT_POLICY_FIFO:
            return "FIFO";
        case RT_POLICY_RR:
            return "RR";
        case RT_POLICY_DEADLINE:
            return "DEADLINE";
        default:
            return "UNKNOWN";
    }
}

RTSchedulingPolicy RTUtils::string_to_policy(const std::string& policy_str) {
    std::string lower = policy_str;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    
    if (lower == "fifo") {
        return RT_POLICY_FIFO;
    } else if (lower == "rr") {
        return RT_POLICY_RR;
    } else if (lower == "deadline") {
        return RT_POLICY_DEADLINE;
    } else if (lower == "none") {
        return RT_POLICY_NONE;
    }
    
    std::cerr << "[RTUtils] Unknown policy string: " << policy_str << std::endl;
    return RT_POLICY_NONE;
}

int RTUtils::policy_to_sched_policy(RTSchedulingPolicy policy) {
    switch (policy) {
        case RT_POLICY_FIFO:
            return SCHED_FIFO;
        case RT_POLICY_RR:
            return SCHED_RR;
        case RT_POLICY_DEADLINE:
#ifdef SCHED_DEADLINE
            return SCHED_DEADLINE;
#else
            std::cerr << "[RTUtils] SCHED_DEADLINE not supported on this system" << std::endl;
            return -1;
#endif
        case RT_POLICY_NONE:
            return SCHED_OTHER;
        default:
            return -1;
    }
}

} // namespace orchestrator
