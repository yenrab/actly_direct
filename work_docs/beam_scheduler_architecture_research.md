# BEAM Scheduler Architecture Research

## Overview

This document provides detailed research and analysis of the BEAM (Bogdan/Björn's Erlang Abstract Machine) scheduler architecture, focusing on the components needed to implement a multi-core threading system similar to BEAM's approach.

## 1. Per-Scheduler Run Queues (One Per Core)

### Architecture
- **One scheduler per CPU core**: Each core has its own dedicated scheduler instance
- **Independent run queues**: Each scheduler maintains its own set of priority queues
- **No shared scheduler state**: Eliminates contention and lock overhead
- **Core affinity**: Processes can be bound to specific schedulers/cores

### Implementation Details
```
Scheduler per Core Structure:
- Core ID (0 to N-1)
- 4 Priority Run Queues (max, high, normal, low)
- Current running process pointer
- Reduction counter for current process
- Work stealing queue (for load balancing)
- Statistics and metrics
```

### Benefits
- **Scalability**: Linear scaling with number of cores
- **Cache locality**: Processes stay on same core, improving cache performance
- **Reduced contention**: No shared scheduler locks
- **NUMA awareness**: Can optimize for memory locality

## 2. Reduction Counting (2000 Reductions Per Process Time Slice)

### Concept
- **Reduction**: A unit of work measurement in BEAM
- **Time slice**: Maximum number of reductions a process can execute before yielding
- **Default**: 2000 reductions per time slice (configurable)
- **Preemption**: Process is preempted when reduction count reaches zero

### Reduction Counting Mechanism
```
Process Execution:
1. Process starts with reduction_count = 2000
2. Each function call, arithmetic operation, or control structure decrements counter
3. When reduction_count reaches 0:
   - Save process context
   - Enqueue process back to run queue
   - Schedule next process
4. Reset reduction_count when process is scheduled again
```

### Implementation Strategy
- **Instruction counting**: Approximate reductions by counting certain instruction types
- **Function call overhead**: Each function call costs ~1 reduction
- **Arithmetic operations**: Basic operations cost ~1 reduction
- **Control structures**: Loops, conditionals cost ~1 reduction per iteration

### Benefits
- **Fair scheduling**: Prevents any single process from monopolizing CPU
- **Responsive system**: Ensures all processes get CPU time
- **Predictable behavior**: Consistent time slicing across all processes

## 3. Four Priority Levels: Max, High, Normal, Low

### Priority Hierarchy
```
Priority Levels (highest to lowest):
1. MAX    - System-critical processes (garbage collection, I/O drivers)
2. HIGH   - Interactive processes (user interfaces, real-time tasks)
3. NORMAL - Regular application processes (default priority)
4. LOW    - Background processes (batch jobs, maintenance tasks)
```

### Scheduling Behavior
- **Strict priority**: Higher priority processes always run before lower priority
- **Round-robin within priority**: Processes of same priority share CPU time
- **Priority inheritance**: Processes can temporarily inherit higher priority
- **Priority aging**: Long-waiting processes may get priority boost

### Implementation Structure
```
Priority Queue Structure:
struct priority_queue {
    struct process *head;     // First process in queue
    struct process *tail;     // Last process in queue
    uint32_t count;           // Number of processes in queue
};

struct scheduler {
    struct priority_queue queues[4];  // max, high, normal, low
    struct process *current_process;  // Currently running process
    uint32_t current_reductions;      // Remaining reductions for current process
    uint32_t core_id;                 // Core this scheduler runs on
};
```

### Priority Selection Algorithm
```
schedule_next_process():
    for priority in [MAX, HIGH, NORMAL, LOW]:
        if !is_empty(queues[priority]):
            return dequeue(queues[priority])
    return NULL  // No processes ready
```

## 4. Scheduler Sleep/Wake Mechanisms

### Sleep Conditions
- **Blocking operations**: I/O, message receive, synchronization
- **Voluntary yield**: Process explicitly yields CPU
- **Resource waiting**: Waiting for memory, locks, etc.
- **Timer expiration**: Process waiting for timeout

### Wake Conditions
- **Message arrival**: Process receives expected message
- **I/O completion**: Asynchronous I/O operation completes
- **Resource available**: Required resource becomes available
- **Timer expiration**: Timeout period ends
- **External signal**: Process receives signal or notification

### Implementation Mechanisms
```
Sleep/Wake Data Structures:
struct sleeping_process {
    struct process *process;
    uint64_t wake_condition;  // Condition that will wake this process
    uint64_t wake_data;       // Additional wake data
    struct sleeping_process *next;
};

struct scheduler {
    struct sleeping_process *sleeping_list;  // List of sleeping processes
    struct process *waiting_for_io;          // I/O waiting processes
    struct process *waiting_for_timer;       // Timer waiting processes
};
```

### Wake-up Process
```
wake_process(process, reason):
    remove_from_sleeping_list(process)
    process->state = READY
    enqueue_to_priority_queue(process)
    if (scheduler_idle):
        signal_scheduler()  // Wake up idle scheduler
```

## Data Structure Specifications

### Core Scheduler Structure
```c
struct scheduler_state {
    // Core identification
    uint32_t core_id;
    uint32_t scheduler_id;
    
    // Priority run queues
    struct priority_queue queues[4];  // max, high, normal, low
    
    // Current process
    struct process *current_process;
    uint32_t current_reductions;
    
    // Work stealing
    struct work_stealing_queue steal_queue;
    uint32_t steal_threshold;
    
    // Statistics
    uint64_t total_scheduled;
    uint64_t total_yields;
    uint64_t total_migrations;
    
    // Sleep/wake management
    struct sleeping_process *sleeping_list;
    struct timer_wheel *timer_wheel;
    
    // Memory management
    struct memory_pool *stack_pool;
    struct memory_pool *heap_pool;
};
```

### Process Control Block (PCB)
```c
struct process {
    // Process identification
    uint64_t pid;
    uint32_t scheduler_id;  // Affinity
    
    // Execution state
    uint32_t state;         // RUNNING, READY, WAITING, SUSPENDED
    uint32_t priority;      // MAX, HIGH, NORMAL, LOW
    uint32_t reduction_count;
    
    // Context (registers)
    uint64_t registers[31]; // x0-x30
    uint64_t sp;            // Stack pointer
    uint64_t pc;            // Program counter
    uint64_t pstate;        // Processor state
    
    // Memory management
    uint64_t stack_base;
    uint64_t stack_size;
    uint64_t heap_base;
    uint64_t heap_size;
    
    // Message passing
    struct message_queue *message_queue;
    
    // Scheduling
    struct process *next;   // For run queue
    struct process *prev;   // For run queue
    uint64_t last_scheduled;
    
    // Affinity and migration
    uint64_t affinity_mask; // Bitmask of allowed cores
    uint32_t migration_count;
};
```

## Scheduling Algorithm Pseudocode

### Main Scheduler Loop
```
scheduler_main_loop():
    while (true):
        current_process = get_next_process()
        if (current_process == NULL):
            // No processes ready, try work stealing
            current_process = try_steal_work()
            if (current_process == NULL):
                // Still no work, go idle
                scheduler_idle()
                continue
        
        // Set up process execution
        current_process->state = RUNNING
        current_process->reduction_count = 2000
        current_process->last_scheduled = get_system_ticks()
        
        // Context switch to process
        restore_context(current_process)
        
        // Process runs until:
        // 1. Reduction count reaches 0 (preemption)
        // 2. Process yields voluntarily
        // 3. Process blocks on I/O/message
        // 4. Process terminates
        
        // When process returns control:
        save_context(current_process)
        handle_process_state_change(current_process)
```

### Process Selection Algorithm
```
get_next_process():
    // Check each priority level from highest to lowest
    for priority in [MAX, HIGH, NORMAL, LOW]:
        if (!is_empty(queues[priority])):
            process = dequeue(queues[priority])
            return process
    
    return NULL  // No processes ready
```

### Work Stealing Algorithm
```
try_steal_work():
    // Try to steal from other schedulers
    for attempt in range(MAX_STEAL_ATTEMPTS):
        target_core = select_random_core()
        if (target_core != current_core):
            stolen_process = steal_from_core(target_core)
            if (stolen_process != NULL):
                return stolen_process
    
    return NULL  // No work to steal
```

## ARM64 Assembly Requirements

### Core Identification
```assembly
// Get current core ID
mrs x0, MPIDR_EL1
and x0, x0, #0xFF  // Extract core ID bits
```

### Atomic Operations for Work Stealing
```assembly
// Load-linked, store-conditional for atomic operations
ldxr x1, [x0]      // Load with exclusive access
// ... modify x1 ...
stxr w2, x1, [x0]  // Store conditionally
cbnz w2, retry     // Retry if store failed
```

### Memory Barriers
```assembly
// Data memory barrier (ensure all memory operations complete)
dmb sy
// Data synchronization barrier (wait for all memory operations)
dsb sy
// Instruction synchronization barrier (flush pipeline)
isb
```

### Context Switching
```assembly
// Save all general purpose registers
stp x0, x1, [sp, #-16]!
stp x2, x3, [sp, #-16]!
// ... continue for all registers ...

// Restore all general purpose registers
ldp x30, x29, [sp], #16
ldp x28, x27, [sp], #16
// ... continue for all registers ...
```

### Event Signaling (SEV/WFE)
```assembly
// Send event to wake other cores
sev

// Wait for event (idle loop)
wfe
```

### Timer Access
```assembly
// Read system timer
mrs x0, CNTPCT_EL0  // Physical counter value
mrs x1, CNTFRQ_EL0  // Counter frequency
```

## Implementation Considerations

### Memory Layout
- **Per-core data**: Each scheduler's data structures in separate memory regions
- **Cache line alignment**: Align structures to cache line boundaries (128 bytes for Apple Silicon)
- **NUMA awareness**: Place scheduler data close to the core it manages

### Performance Optimizations
- **Lock-free data structures**: Use atomic operations instead of locks where possible
- **Branch prediction**: Structure code to minimize branch mispredictions
- **Instruction cache**: Keep hot paths in instruction cache
- **Data cache**: Minimize cache misses in scheduler operations

### Error Handling
- **Stack overflow detection**: Check stack bounds during context switch
- **Invalid process states**: Validate process state transitions
- **Memory allocation failures**: Handle out-of-memory conditions gracefully
- **Core failure handling**: Detect and handle core failures

## Next Steps

This research provides the foundation for implementing the BEAM scheduler architecture. The next phase should focus on:

1. **Data structure implementation** in ARM64 assembly
2. **Basic scheduler initialization** and core bring-up
3. **Simple process creation** and context switching
4. **Priority queue management** and scheduling logic
5. **Testing framework** for validation

The implementation should start with a single-core scheduler and gradually add multi-core support, work stealing, and advanced features.
