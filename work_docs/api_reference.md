# Actly Direct API Reference

## Overview

This document provides a comprehensive API reference for the Actly Direct BEAM implementation. All functions are implemented in pure ARM64 assembly and are callable from C code for testing purposes.

## Core Scheduler API

### Scheduler Initialization

#### `scheduler_init(core_id)`
Initialize the scheduler for a specific core.

**Parameters:**
- `core_id` (uint64_t): Core ID to initialize (0-127)

**Returns:**
- `int`: 1 on success, 0 on failure

**Complexity:** O(1)

**Example:**
```c
int result = scheduler_init(0);  // Initialize core 0
```

#### `scheduler_schedule(core_id)`
Schedule the next process to run on the specified core.

**Parameters:**
- `core_id` (uint64_t): Core ID to schedule on

**Returns:**
- `void*`: Pointer to next process, or NULL if no processes available

**Complexity:** O(n) where n is number of priority levels

### Process Management

#### `process_alloc()`
Allocate a new process control block.

**Parameters:**
- None

**Returns:**
- `void*`: Pointer to allocated PCB, or NULL on failure

**Complexity:** O(1)

#### `process_free(pcb)`
Free a process control block.

**Parameters:**
- `pcb` (void*): Pointer to PCB to free

**Returns:**
- `int`: 1 on success, 0 on failure

**Complexity:** O(1)

#### `process_set_state(pcb, state)`
Set the state of a process.

**Parameters:**
- `pcb` (void*): Pointer to PCB
- `state` (uint32_t): New process state

**Returns:**
- `int`: 1 on success, 0 on failure

**Complexity:** O(1)

#### `process_get_state(pcb)`
Get the current state of a process.

**Parameters:**
- `pcb` (void*): Pointer to PCB

**Returns:**
- `uint32_t`: Current process state

**Complexity:** O(1)

### Yielding and Preemption

#### `process_yield(pcb)`
Voluntarily yield the current process.

**Parameters:**
- `pcb` (void*): Pointer to PCB to yield

**Returns:**
- `int`: 1 if yielded, 0 if continued

**Complexity:** O(1)

#### `process_yield_with_state(pcb, scheduler_states, core_id)`
Yield process with scheduler state management.

**Parameters:**
- `pcb` (void*): Pointer to PCB to yield
- `scheduler_states` (void*): Pointer to scheduler states
- `core_id` (uint64_t): Core ID

**Returns:**
- `void*`: Next process to run, or NULL

**Complexity:** O(1)

#### `process_preempt(pcb)`
Force preemption of a process.

**Parameters:**
- `pcb` (void*): Pointer to PCB to preempt

**Returns:**
- `void*`: Next process to run, or NULL

**Complexity:** O(1)

## Load Balancing API

### Work Stealing

#### `try_steal_work(scheduler_states, core_id)`
Attempt to steal work from another core.

**Parameters:**
- `scheduler_states` (void*): Pointer to scheduler states
- `core_id` (uint64_t): Current core ID

**Returns:**
- `void*`: Stolen process, or NULL if no work available

**Complexity:** O(n) where n is number of cores

#### `get_scheduler_load(core_id)`
Get the current load of a scheduler.

**Parameters:**
- `core_id` (uint64_t): Core ID to check

**Returns:**
- `uint32_t`: Current scheduler load

**Complexity:** O(1)

#### `select_victim_by_load(scheduler_states, current_core)`
Select a victim core for work stealing based on load.

**Parameters:**
- `scheduler_states` (void*): Pointer to scheduler states
- `current_core` (uint64_t): Current core ID

**Returns:**
- `uint64_t`: Victim core ID

**Complexity:** O(n) where n is number of cores

### Work Stealing Deque

#### `ws_deque_init(deque, size)`
Initialize a work stealing deque.

**Parameters:**
- `deque` (void*): Pointer to deque structure
- `size` (uint32_t): Initial deque size

**Returns:**
- `int`: 1 on success, 0 on failure

**Complexity:** O(1)

#### `ws_deque_push_bottom(deque, process)`
Push a process to the bottom of the deque.

**Parameters:**
- `deque` (void*): Pointer to deque structure
- `process` (void*): Process to push

**Returns:**
- `int`: 1 on success, 0 on failure

**Complexity:** O(1)

#### `ws_deque_pop_bottom(deque)`
Pop a process from the bottom of the deque.

**Parameters:**
- `deque` (void*): Pointer to deque structure

**Returns:**
- `void*`: Popped process, or NULL if empty

**Complexity:** O(1)

#### `ws_deque_pop_top(deque)`
Pop a process from the top of the deque (work stealing).

**Parameters:**
- `deque` (void*): Pointer to deque structure

**Returns:**
- `void*`: Popped process, or NULL if empty

**Complexity:** O(1)

## CPU Affinity API

### Affinity Management

#### `set_process_affinity(pcb, core_mask)`
Set the CPU affinity mask for a process.

**Parameters:**
- `pcb` (void*): Pointer to PCB
- `core_mask` (uint64_t): Bitmask of allowed cores

**Returns:**
- `int`: 1 on success, 0 on failure

**Complexity:** O(1)

#### `get_process_affinity(pcb)`
Get the CPU affinity mask for a process.

**Parameters:**
- `pcb` (void*): Pointer to PCB

**Returns:**
- `uint64_t`: Affinity mask

**Complexity:** O(1)

#### `check_affinity(pcb, core_id)`
Check if a process can run on a specific core.

**Parameters:**
- `pcb` (void*): Pointer to PCB
- `core_id` (uint64_t): Core ID to check

**Returns:**
- `int`: 1 if allowed, 0 if not allowed

**Complexity:** O(1)

### Core Type Detection

#### `get_core_type(core_id)`
Get the type of a core (P-core or E-core).

**Parameters:**
- `core_id` (uint64_t): Core ID to check

**Returns:**
- `uint64_t`: Core type (0=PERFORMANCE, 1=EFFICIENCY, 2=UNKNOWN)

**Complexity:** O(1)

#### `is_performance_core(core_id)`
Check if a core is a performance core.

**Parameters:**
- `core_id` (uint64_t): Core ID to check

**Returns:**
- `int`: 1 if P-core, 0 if E-core or invalid

**Complexity:** O(1)

#### `get_optimal_core(process_type)`
Get the optimal core for a process type.

**Parameters:**
- `process_type` (uint64_t): Process type (0=CPU_INTENSIVE, 1=I/O_BOUND, 2=MIXED)

**Returns:**
- `uint64_t`: Optimal core ID

**Complexity:** O(1)

## Inter-Core Communication API

### Message Queues

#### `message_queue_init(queue_ptr, size)`
Initialize a message queue.

**Parameters:**
- `queue_ptr` (void*): Pointer to queue structure
- `size` (uint32_t): Queue size

**Returns:**
- `int`: 1 on success, 0 on failure

**Complexity:** O(1)

#### `send_message(queue_ptr, message)`
Send a message to a queue.

**Parameters:**
- `queue_ptr` (void*): Pointer to queue structure
- `message` (uint64_t): Message to send

**Returns:**
- `int`: 1 on success, 0 on failure

**Complexity:** O(1)

#### `try_receive_message(queue_ptr)`
Try to receive a message from a queue (non-blocking).

**Parameters:**
- `queue_ptr` (void*): Pointer to queue structure

**Returns:**
- `uint64_t`: Received message, or 0 if empty

**Complexity:** O(1)

#### `receive_message_blocking(queue_ptr, pcb_ptr)`
Receive a message from a queue (blocking).

**Parameters:**
- `queue_ptr` (void*): Pointer to queue structure
- `pcb_ptr` (void*): Pointer to PCB to block

**Returns:**
- `uint64_t`: Received message

**Complexity:** O(1)

#### `message_queue_is_empty(queue_ptr)`
Check if a message queue is empty.

**Parameters:**
- `queue_ptr` (void*): Pointer to queue structure

**Returns:**
- `int`: 1 if empty, 0 if not empty

**Complexity:** O(1)

#### `message_queue_size(queue_ptr)`
Get the current size of a message queue.

**Parameters:**
- `queue_ptr` (void*): Pointer to queue structure

**Returns:**
- `uint32_t`: Current queue size

**Complexity:** O(1)

## Timer System API

### Timer Management

#### `timer_init()`
Initialize the timer system.

**Parameters:**
- None

**Returns:**
- `int`: 1 on success, 0 on failure

**Complexity:** O(1)

#### `get_system_ticks()`
Get the current system tick count.

**Parameters:**
- None

**Returns:**
- `uint64_t`: Current system tick count

**Complexity:** O(1)

#### `insert_timer(expiry_ticks, callback, process_id)`
Insert a timer into the timer system.

**Parameters:**
- `expiry_ticks` (uint64_t): When the timer should expire
- `callback` (void*): Callback function to call
- `process_id` (uint64_t): Process ID associated with timer

**Returns:**
- `uint64_t`: Timer ID for cancellation, or 0 on failure

**Complexity:** O(1)

#### `cancel_timer(timer_id)`
Cancel a previously scheduled timer.

**Parameters:**
- `timer_id` (uint64_t): Timer ID to cancel

**Returns:**
- `int`: 1 on success, 0 on failure

**Complexity:** O(1)

#### `process_timers()`
Process all expired timers.

**Parameters:**
- None

**Returns:**
- `uint32_t`: Number of timers processed

**Complexity:** O(n) where n is number of expired timers

#### `timer_tick()`
Called each scheduler loop to process timers.

**Parameters:**
- None

**Returns:**
- None

**Complexity:** O(n) where n is number of expired timers

### Timeout Support

#### `schedule_timeout(timeout_ticks, process_id)`
Schedule a timeout for a blocking operation.

**Parameters:**
- `timeout_ticks` (uint64_t): Timeout duration in ticks
- `process_id` (uint64_t): Process ID to timeout

**Returns:**
- `uint64_t`: Timeout ID for cancellation

**Complexity:** O(1)

#### `cancel_timeout(timeout_id)`
Cancel a scheduled timeout.

**Parameters:**
- `timeout_id` (uint64_t): Timeout ID to cancel

**Returns:**
- `int`: 1 on success, 0 on failure

**Complexity:** O(1)

## Apple Silicon Optimization API

### Core Detection

#### `detect_apple_silicon_core_types(core_type_map)`
Detect and map P-cores and E-cores on Apple Silicon.

**Parameters:**
- `core_type_map` (void*): Pointer to array to store core types

**Returns:**
- `int`: 1 on success, 0 on failure

**Complexity:** O(n) where n is number of cores

#### `get_core_type_apple_silicon(core_id)`
Get the core type for a specific core on Apple Silicon.

**Parameters:**
- `core_id` (uint64_t): Core ID to check

**Returns:**
- `uint64_t`: Core type (0=PERFORMANCE, 1=EFFICIENCY, 2=UNKNOWN)

**Complexity:** O(1)

#### `get_core_cluster_apple_silicon(core_id)`
Get the cluster ID for a specific core on Apple Silicon.

**Parameters:**
- `core_id` (uint64_t): Core ID to check

**Returns:**
- `uint64_t`: Cluster ID (0 for P-cores, 1 for E-cores)

**Complexity:** O(1)

#### `is_performance_core_apple_silicon(core_id)`
Check if a core is a P-core on Apple Silicon.

**Parameters:**
- `core_id` (uint64_t): Core ID to check

**Returns:**
- `int`: 1 if P-core, 0 if E-core or invalid

**Complexity:** O(1)

#### `get_optimal_core_apple_silicon(process_type)`
Select the optimal core for a process on Apple Silicon.

**Parameters:**
- `process_type` (uint64_t): Process type (0=CPU_INTENSIVE, 1=I/O_BOUND, 2=MIXED)

**Returns:**
- `uint64_t`: Optimal core ID

**Complexity:** O(1)

### Cache Optimization

#### `get_cache_line_size_apple_silicon()`
Get the cache line size for Apple Silicon processors.

**Parameters:**
- None

**Returns:**
- `uint64_t`: Cache line size in bytes (128 for Apple Silicon)

**Complexity:** O(1)

#### `optimize_for_apple_silicon()`
Apply Apple Silicon specific optimizations.

**Parameters:**
- None

**Returns:**
- `int`: 1 on success, 0 on failure

**Complexity:** O(1)

## System Integration API

### Boot and Initialization

#### `scheduler_main_loop()`
Main scheduler loop that integrates all subsystems.

**Parameters:**
- None

**Returns:**
- None (infinite loop)

**Complexity:** O(1) per iteration

#### `process_messages()`
Process incoming messages for all cores.

**Parameters:**
- None

**Returns:**
- None

**Complexity:** O(1)

#### `check_load_balance()`
Periodic load balancing check.

**Parameters:**
- None

**Returns:**
- None

**Complexity:** O(1)

## Error Handling

All functions return appropriate error codes:
- `1`: Success
- `0`: Failure (invalid parameters, resource unavailable, etc.)

## Memory Management

- All functions use the system's memory management
- PCBs are allocated using `process_alloc()` and freed using `process_free()`
- Message queues and timers are managed internally
- Memory alignment follows ARM64 requirements (8-byte alignment)

## Thread Safety

- All functions are designed to be thread-safe
- Lock-free algorithms are used where possible
- Atomic operations are used for shared data structures
- Cross-core communication uses memory barriers

## Performance Characteristics

- Most operations are O(1) constant time
- Work stealing operations are O(n) where n is number of cores
- Timer processing is O(n) where n is number of expired timers
- Memory allocation is O(1) with O(n) garbage collection

## Platform Support

- **Primary Platform:** macOS on Apple Silicon (ARM64)
- **Architecture:** ARM64 (AArch64)
- **Compiler:** GCC with ARM64 support
- **Assembler:** GNU Assembler (as)

## Version Information

- **Version:** 0.10
- **Author:** Lee Barney
- **Last Modified:** 2025-01-19
- **License:** MIT License
