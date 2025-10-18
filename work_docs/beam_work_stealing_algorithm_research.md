# BEAM Work Stealing Algorithm Research

## Overview

This document provides detailed research and analysis of BEAM's work stealing algorithm, which is a critical component for efficient load balancing across multiple CPU cores. Work stealing enables idle schedulers to "steal" work from busy schedulers, ensuring optimal CPU utilization and preventing cores from sitting idle.

## 1. Work Stealing When Schedulers Become Idle

### Idle Detection
- **Empty run queues**: Scheduler has no processes in any priority queue
- **No current process**: No process is currently running on the scheduler
- **Work stealing trigger**: Idle scheduler attempts to steal work from other schedulers
- **Immediate response**: Work stealing happens as soon as scheduler becomes idle

### Idle Scheduler Behavior
```
Idle Scheduler Flow:
1. Scheduler completes current process or process blocks
2. Check all priority queues (MAX, HIGH, NORMAL, LOW)
3. If all queues are empty:
   a) Attempt work stealing from other schedulers
   b) If stealing fails, go to idle state (WFE)
   c) Wait for new work or external wake-up
4. If work is stolen, immediately schedule the stolen process
```

### Benefits of Immediate Work Stealing
- **Zero idle time**: Cores never sit idle when work is available elsewhere
- **Optimal utilization**: Maximum CPU utilization across all cores
- **Responsive system**: Processes get scheduled as soon as possible
- **Load balancing**: Automatic distribution of work across cores

### Idle State Management
- **WFE (Wait For Event)**: Use ARM64 WFE instruction for power-efficient idle
- **SEV (Send Event)**: Other cores can wake idle cores with SEV
- **Interrupt handling**: Timer and I/O interrupts can wake idle cores
- **Work arrival**: New processes or messages can wake idle cores

## 2. Stealing from Other Scheduler's Run Queues

### Stealing Source
- **Run queues**: Steal from other schedulers' priority queues
- **All priority levels**: Can steal from MAX, HIGH, NORMAL, LOW queues
- **Queue selection**: Typically steal from highest priority non-empty queue
- **Process selection**: Steal from tail of queue (oldest processes first)

### Stealing Mechanism
```
Work Stealing Process:
1. Identify target scheduler with work available
2. Select appropriate priority queue to steal from
3. Atomically remove process from target queue
4. Transfer process to local scheduler
5. Update process affinity and statistics
6. Schedule stolen process immediately
```

### Queue Access Patterns
- **Push-bottom**: Local scheduler adds processes to bottom of queue
- **Pop-bottom**: Local scheduler removes processes from bottom of queue
- **Pop-top**: Remote scheduler steals processes from top of queue
- **Lock-free**: Use atomic operations to avoid locks

### Stealing Data Structures
```
Work Stealing Queue Structure:
struct work_stealing_queue {
    struct process *bottom;      // Bottom pointer (local access)
    struct process *top;         // Top pointer (remote access)
    uint32_t size;               // Current queue size
    uint32_t max_size;           // Maximum queue size
    uint32_t steal_count;        // Number of successful steals
    uint32_t steal_attempts;     // Total steal attempts
};
```

### Atomic Operations for Stealing
```assembly
// Load-linked, store-conditional for atomic operations
steal_process:
    ldxr x1, [x0]           // Load bottom pointer with exclusive access
    cmp x1, x2              // Compare with top pointer
    b.le steal_failed       // Queue is empty or has only one element
    sub x3, x1, #8          // Calculate new bottom pointer
    stxr w4, x3, [x0]       // Store new bottom pointer
    cbnz w4, steal_process  // Retry if store failed
    // Successfully stole process
```

## 3. Migration Limits and Thresholds

### Migration Limits
- **Per-process limit**: Maximum number of times a process can be migrated
- **Per-scheduler limit**: Maximum number of migrations per scheduler per interval
- **System-wide limit**: Global migration rate limit to prevent thrashing
- **Affinity constraints**: Respect process affinity when migrating

### Migration Thresholds
```
Migration Thresholds:
- Minimum queue size: Don't steal if target queue has < 2 processes
- Maximum steal rate: Limit steals per scheduler per second
- Process age threshold: Only steal processes that have been waiting > N ms
- Load difference threshold: Only steal if load difference > 50%
```

### Migration Cost Considerations
- **Cache locality**: Migration may cause cache misses
- **Memory access**: Remote memory access is slower than local
- **Context switching**: Additional overhead for process migration
- **NUMA effects**: Cross-NUMA migration is expensive

### Migration Policies
```
Migration Policy:
1. Check if migration is allowed (limits and thresholds)
2. Verify process affinity allows migration
3. Calculate migration cost vs. benefit
4. Only migrate if benefit > cost
5. Update migration statistics
```

### Anti-Thrashing Measures
- **Migration history**: Track recent migrations to prevent oscillation
- **Cooling period**: Wait between migrations from same source
- **Load stability**: Only migrate if load difference is significant
- **Process characteristics**: Consider process behavior (CPU vs. I/O bound)

## 4. Balancing Intervals

### Balancing Timing
- **Immediate balancing**: Steal work as soon as scheduler becomes idle
- **Periodic balancing**: Check for load imbalance every N milliseconds
- **Event-driven balancing**: Balance when significant load changes occur
- **Adaptive intervals**: Adjust balancing frequency based on system load

### Balancing Intervals
```
Balancing Schedule:
- Immediate: When scheduler becomes idle
- Periodic: Every 10ms during normal operation
- High load: Every 5ms when system is heavily loaded
- Low load: Every 50ms when system is lightly loaded
- Event-driven: When new processes are created or processes terminate
```

### Load Monitoring
- **Queue lengths**: Monitor number of processes in each priority queue
- **CPU utilization**: Track CPU usage per scheduler
- **Response times**: Monitor process scheduling latency
- **Migration rates**: Track work stealing frequency and success

### Adaptive Balancing
```
Adaptive Balancing Algorithm:
1. Monitor system load and migration success rate
2. If migration success rate > 80%:
   - Increase balancing frequency
   - Lower migration thresholds
3. If migration success rate < 20%:
   - Decrease balancing frequency
   - Raise migration thresholds
4. If system is stable:
   - Use default balancing parameters
```

## Lock-Free Data Structures

### Lock-Free Deque Implementation
- **Double-ended queue**: Support push-bottom, pop-bottom, pop-top operations
- **Atomic pointers**: Use atomic operations for head and tail pointers
- **Memory ordering**: Use appropriate memory barriers for consistency
- **ABA problem**: Handle ABA problem with version numbers or hazard pointers

### Lock-Free Queue Structure
```
Lock-Free Deque:
struct lockfree_deque {
    volatile uint64_t top;      // Top pointer (for stealing)
    volatile uint64_t bottom;   // Bottom pointer (for local access)
    struct process *processes;  // Array of process pointers
    uint32_t size;              // Queue size
    uint32_t mask;              // Size mask for circular buffer
};
```

### Atomic Operations
```assembly
// Push-bottom operation (local scheduler)
push_bottom:
    ldr x1, [x0, #BOTTOM_OFFSET]    // Load current bottom
    add x2, x1, #1                  // Calculate new bottom
    and x2, x2, x3                  // Apply size mask
    str x4, [x5, x1, lsl #3]        // Store process pointer
    dmb sy                          // Memory barrier
    str x2, [x0, #BOTTOM_OFFSET]    // Update bottom pointer

// Pop-top operation (remote scheduler)
pop_top:
    ldxr x1, [x0, #TOP_OFFSET]      // Load top with exclusive access
    ldr x2, [x0, #BOTTOM_OFFSET]    // Load bottom
    cmp x1, x2                      // Compare top and bottom
    b.ge pop_failed                 // Queue is empty
    ldr x3, [x4, x1, lsl #3]        // Load process pointer
    add x5, x1, #1                  // Calculate new top
    and x5, x5, x6                  // Apply size mask
    stxr w7, x5, [x0, #TOP_OFFSET]  // Update top pointer
    cbnz w7, pop_top                // Retry if store failed
```

### Memory Ordering
- **Acquire semantics**: Use LDAXR for acquiring locks
- **Release semantics**: Use STLXR for releasing locks
- **Sequential consistency**: Use DMB/DSB for memory ordering
- **Relaxed ordering**: Use LDXR/STXR for relaxed atomic operations

## Victim Selection Strategies

### Random Selection
- **Simple implementation**: Randomly select target scheduler
- **Load distribution**: Spread stealing attempts across all schedulers
- **Avoid hotspots**: Prevent always stealing from same scheduler
- **Fairness**: Give all schedulers equal chance to be victims

### Load-Based Selection
- **Least loaded**: Steal from scheduler with most work
- **Load balancing**: Steal from overloaded schedulers
- **Efficiency**: Maximize work transfer per steal attempt
- **Adaptive**: Adjust selection based on success rate

### Locality-Aware Selection
- **NUMA awareness**: Prefer stealing from same NUMA node
- **Cache locality**: Consider cache effects of migration
- **Memory distance**: Minimize memory access latency
- **Core affinity**: Respect process affinity constraints

### Selection Algorithms
```
Victim Selection Strategies:

1. Random Selection:
   target = random() % num_schedulers

2. Load-Based Selection:
   target = scheduler_with_most_work()

3. Locality-Aware Selection:
   target = closest_scheduler_with_work()

4. Hybrid Selection:
   if (random() < 0.7):
       target = load_based_selection()
   else:
       target = random_selection()
```

### Selection Optimization
- **Success tracking**: Track which schedulers are good victims
- **Adaptive weights**: Adjust selection probabilities based on success
- **Cache effects**: Consider cache line sharing between cores
- **Migration cost**: Factor in migration overhead in selection

## Work Stealing Overhead Analysis

### Stealing Overhead Components
- **Queue access**: Time to access remote scheduler's queue
- **Atomic operations**: Cost of lock-free operations
- **Memory barriers**: Overhead of memory ordering
- **Cache effects**: Cache misses from remote memory access
- **Context switching**: Additional overhead for stolen processes

### Performance Metrics
```
Work Stealing Metrics:
- Steal attempt rate: Steals attempted per second
- Steal success rate: Percentage of successful steals
- Steal latency: Time from steal attempt to success
- Migration overhead: Additional cost per migrated process
- Load balance quality: Distribution of work across cores
```

### Overhead Minimization
- **Batch stealing**: Steal multiple processes in one operation
- **Local work preference**: Prefer local work over stealing
- **Steal threshold**: Only steal when local queue is empty
- **Efficient data structures**: Optimize queue implementation
- **Cache-friendly layouts**: Minimize cache misses

### Performance Tuning
```
Performance Tuning Parameters:
- Steal threshold: Minimum queue size before stealing
- Steal rate limit: Maximum steals per scheduler per second
- Batch size: Number of processes to steal per attempt
- Retry limit: Maximum retry attempts per steal
- Cooling period: Minimum time between steals from same source
```

## Implementation Considerations

### Error Handling
- **Steal failures**: Handle failed steal attempts gracefully
- **Queue corruption**: Detect and recover from queue corruption
- **Memory allocation**: Handle out-of-memory during migration
- **Process state**: Validate process state before migration

### Debugging Support
- **Steal tracing**: Log all steal attempts and results
- **Performance counters**: Track stealing statistics
- **Load monitoring**: Monitor load distribution across cores
- **Migration tracking**: Track process migration history

### Scalability
- **Core count**: Scale efficiently with number of cores
- **Process count**: Handle large numbers of processes
- **Memory usage**: Minimize memory overhead per scheduler
- **Cache effects**: Optimize for cache hierarchy

### Testing
- **Unit tests**: Test individual stealing operations
- **Stress tests**: Test under heavy load conditions
- **Correctness tests**: Verify no work is lost or duplicated
- **Performance tests**: Measure stealing overhead and effectiveness

## Next Steps

This research provides the foundation for implementing BEAM's work stealing algorithm. The next phase should focus on:

1. **Lock-free deque implementation** in ARM64 assembly
2. **Atomic operations** for work stealing
3. **Victim selection algorithms** and load balancing
4. **Migration limits and thresholds** implementation
5. **Performance monitoring** and optimization
6. **Testing framework** for work stealing validation

The implementation should start with basic work stealing and gradually add advanced features like adaptive balancing and NUMA awareness.
