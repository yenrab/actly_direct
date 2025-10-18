<!-- 70fd4dac-18f4-4e75-b02e-df6cceb4c536 c36a1b5b-a337-4869-862e-59aa2ac3ed0b -->
# BEAM Multi-Core Threading Research & Implementation Plan

## Phase 1: Research and Analysis

### Research Topics

1. **BEAM Scheduler Architecture**

            - Study BEAM's per-scheduler run queues (one per core)
            - Understand reduction counting (2000 reductions per process time slice)
            - Analyze the four priority levels: max, high, normal, low
            - Document scheduler sleep/wake mechanisms

2. **Process Scheduling Behavior**

            - BEAM processes are lightweight (start at ~2KB)
            - Each process has its own stack and heap
            - Processes run until reduction count exhausted or blocking operation
            - Analyze process priority queues per scheduler

3. **Work Stealing Algorithm**

            - BEAM uses work stealing when schedulers become idle
            - Stealing happens from other scheduler's run queues
            - Understand migration limits and thresholds
            - Document balancing intervals

4. **CPU Affinity and Migration**

            - Process affinity to specific schedulers
            - Migration cost considerations
            - NUMA-aware scheduling principles
            - Scheduler binding strategies

5. **Yielding Mechanisms**

            - Voluntary yields (erlang:yield/0)
            - Reduction-based preemption
            - Blocking operation yields (receive, I/O)
            - BIF (Built-In Function) trap points

### Deliverables

- Document BEAM scheduler design principles
- Create data structure specifications
- Define scheduling algorithm pseudocode
- Identify ARM64 assembly requirements

---

## Phase 2: Core Scheduler Implementation

### Goals

Implement a per-core scheduler with run queues and basic scheduling logic

### Tasks

1. **Data Structures** (scheduler.s)

            - Define scheduler_state struct per core
            - Create priority-based run queues (4 levels: max, high, normal, low)
            - Implement reduction counter
            - Add current_process pointer

2. **Scheduler Initialization** (scheduler.s)

            - Initialize per-core scheduler_state in .bss
            - Set up run queue heads/tails
            - Initialize reduction counters
            - Link with boot.s core bring-up

3. **Basic Scheduling Logic**

            - Implement schedule() function: select next process from highest priority non-empty queue
            - Implement round-robin within priority level
            - Track reduction counts (start with 2000 per slice)
            - Handle empty queue scenarios (idle core)

4. **Process State Management**

            - Define process states: RUNNING, READY, WAITING, SUSPENDED
            - Implement state transition functions
            - Enqueue/dequeue operations for run queues

### Integration Points

- Modify runtime.s to call scheduler_init during core initialization
- Replace WFE idle loops with scheduler_idle() function

### Testing

- Create test with 2-3 dummy processes per core
- Verify round-robin behavior with UART output
- Confirm reduction counting works

---

## Phase 3: Process Model

### Goals

Design lightweight process control blocks and context switching

### Tasks

1. **Process Control Block (PCB)** (process.s)
   ```
   PCB structure (~128 bytes):
         - x0-x30 register save area (31 * 8 = 248 bytes)
         - sp, pc, pstate
         - process_id (PID)
         - priority level
         - state (RUNNING/READY/WAITING/SUSPENDED)
         - reduction_count
         - scheduler_id (affinity)
         - stack_base, stack_size
         - heap_base, heap_size
         - message_queue pointer
         - next/prev pointers for run queue
   ```

2. **Stack Management**

            - Define stack pool in .bss (e.g., 256 stacks of 8KB each)
            - Implement stack_alloc() and stack_free()
            - Add stack overflow detection (guard pages concept)

3. **Context Switching** (process.s)

            - Implement save_context(pcb): save x0-x30, sp, lr, pc to PCB
            - Implement restore_context(pcb): restore all registers from PCB
            - Ensure atomic context switch with interrupts disabled
            - Minimize context switch overhead (<100 cycles)

4. **Process Lifecycle**

            - process_create(entry_point, priority) -> PID
            - process_terminate(pid)
            - process_cleanup(pcb): free stack, heap

### Integration Points

- Link with scheduler.s for process enqueue/dequeue
- Update runtime_secondary_entry to create initial process per core

### Testing

- Create test processes with simple loops
- Verify context switching preserves register state
- Test process creation and termination

---

## Phase 4: Load Balancing

### Goals

Implement work stealing and dynamic load balancing across cores

### Tasks

1. **Work Stealing Queues** (loadbalancer.s)

            - Implement lock-free deque per scheduler (for stealing)
            - Use ARM64 LDXR/STXR for atomic operations
            - Implement push_bottom(), pop_bottom(), pop_top()

2. **Load Monitoring**

            - Track per-core metrics: run_queue_length, current_load
            - Implement load_check() called periodically (every 1000 reductions)
            - Define load imbalance thresholds (e.g., >50% difference)

3. **Migration Logic**

            - try_steal_work(): attempt to steal from random overloaded core
            - Implement migration_allowed(pcb): check affinity constraints
            - migrate_process(pcb, target_core): atomic process transfer

4. **Balancing Strategy**

            - Check for work stealing when local queue empty
            - Random victim selection initially (later: least loaded)
            - Limit migration rate to prevent thrashing

### Integration Points

- Call load_check() from scheduler main loop
- Invoke try_steal_work() when scheduler idle

### Testing

- Create scenario with uneven process distribution
- Verify work stealing rebalances load
- Measure migration overhead

---

## Phase 5: CPU Affinity

### Goals

Implement CPU affinity management and NUMA-aware scheduling

### Tasks

1. **Affinity Data Structures** (affinity.s)

            - Add affinity_mask to PCB (64-bit bitmask for cores)
            - Create scheduler_to_domain mapping for NUMA
            - Track per-core cache locality hints

2. **Affinity Operations**

            - set_affinity(pid, core_mask): set allowed cores
            - get_affinity(pid): return current affinity
            - check_affinity(pcb, core_id): verify if allowed

3. **NUMA-Aware Scheduling**

            - Detect Apple Silicon cluster configuration (P-cores vs E-cores)
            - Prefer keeping processes on same cluster
            - Implement memory locality tracking

4. **Affinity Enforcement**

            - Block migrations that violate affinity
            - Rebalance respecting affinity constraints
            - Handle affinity changes for running processes

### Integration Points

- Modify migrate_process() to check affinity
- Update scheduler to respect affinity masks

### Testing

- Pin process to specific core, verify it doesn't migrate
- Test affinity with load balancing active
- Verify NUMA domain awareness

---

## Phase 6: Yielding and Preemption

### Goals

Implement cooperative yielding and timer-based preemption

### Tasks

1. **Reduction-Based Preemption** (yield.s)

            - Decrement reduction counter on every N instructions
            - Call yield_check() when counter reaches 0
            - Implement forced preemption: save context, enqueue, schedule()

2. **Voluntary Yield**

            - Implement process_yield(): explicit yield by process
            - Save context, reset reduction counter
            - Move to back of ready queue

3. **Timer-Based Preemption**

            - Set up ARM Generic Timer per core
            - Configure timer interrupt every 1ms (configurable)
            - Timer ISR: decrement global tick, check for preemption

4. **Preemption Points**

            - Add yield points in long-running operations
            - Implement trap mechanism for blocking operations
            - Handle preemption during critical sections

### Integration Points

- Add yield_check() calls in scheduler loop
- Configure timer in boot.s or apple_silicon.s

### Testing

- Test voluntary yield behavior
- Verify reduction-based preemption works
- Confirm timer preemption with busy processes

---

## Phase 7: Memory Management

### Goals

Implement per-process memory management with pools

### Tasks

1. **Heap Allocator** (memory.s)

            - Implement simple bump allocator per process
            - Define heap pool in .bss (e.g., 16MB total)
            - Add heap_alloc(pcb, size) -> pointer
            - Add heap_free(pcb, pointer)

2. **Memory Pools**

            - Create fixed-size pools for common allocations (64B, 256B, 1KB, 4KB)
            - Implement pool_alloc(size_class) -> pointer
            - Implement pool_free(pointer, size_class)

3. **Garbage Collection Preparation**

            - Add GC flags to PCB
            - Implement simple mark phase preparation
            - Track live heap usage per process

4. **Memory Limits**

            - Add max_heap_size to PCB
            - Enforce heap limits in heap_alloc()
            - Implement out-of-memory handling

### Integration Points

- Link with process creation to allocate initial heap
- Add memory cleanup to process_terminate()

### Testing

- Test heap allocation and deallocation
- Verify memory pool efficiency
- Test memory limit enforcement

---

## Phase 8: Inter-Core Communication

### Goals

Implement message passing and lock-free synchronization

### Tasks

1. **Message Queue** (communication.s)

            - Define message structure: sender_pid, receiver_pid, data
            - Implement per-process message queue (linked list)
            - Add send_message(sender, receiver, data)
            - Add receive_message(pid) -> message (blocking)

2. **Lock-Free Data Structures**

            - Implement lock-free MPSC queue (Multi-Producer Single-Consumer)
            - Use ARM64 LDXR/STXR for atomic operations
            - Implement DMB/DSB barriers for memory ordering

3. **Cross-Core Notifications**

            - Use SEV (Send Event) to wake idle cores
            - Implement core_signal(target_core_id)
            - Handle WFE (Wait For Event) in scheduler idle

4. **Synchronization Primitives**

            - Implement atomic_compare_and_swap()
            - Implement atomic_fetch_and_add()
            - Add memory barriers (acquire/release semantics)

### Integration Points

- Link message queue with scheduler (block on empty receive)
- Use notifications in work stealing

### Testing

- Test message passing between processes on different cores
- Verify lock-free queue correctness under contention
- Test SEV/WFE notification mechanism

---

## Phase 9: Timer System

### Goals

Implement system tick counter and timeout management

### Tasks

1. **System Tick Counter** (timer.s)

            - Use ARM Generic Timer (CNTPCT_EL0)
            - Implement get_system_ticks() -> 64-bit counter
            - Define tick frequency (e.g., 1ms per tick)

2. **Timer Wheel**

            - Implement hierarchical timing wheel (4 levels)
            - Add timer entries: expire_time, callback, process_id
            - Implement insert_timer(ticks, callback, pid)
            - Implement cancel_timer(timer_id)

3. **Timeout Processing**

            - Check timer wheel each scheduler tick
            - Execute expired timer callbacks
            - Re-queue processes waiting on timeout

4. **Periodic Tasks**

            - Implement register_periodic_task(interval, callback)
            - Execute load balancing checks periodically
            - Add scheduler statistics collection

### Integration Points

- Call timer_tick() from scheduler main loop
- Link timeouts with receive_message blocking

### Testing

- Test basic timer expiration
- Verify timer wheel efficiency with many timers
- Test periodic task execution

---

## Phase 10: Apple Silicon Optimization

### Goals

Optimize for Apple Silicon specific features

### Tasks

1. **Core Type Detection** (apple_silicon.s)

            - Read CPU ID registers to detect P-cores vs E-cores
            - Create core_type array (PERFORMANCE or EFFICIENCY)
            - Implement get_core_type(core_id)

2. **Intelligent Core Assignment**

            - Assign CPU-intensive processes to P-cores
            - Assign I/O-bound processes to E-cores
            - Track process behavior (CPU vs I/O bound)

3. **Apple Silicon Instructions**

            - Use Apple-specific ARM instructions where beneficial
            - Optimize memory barriers for Apple Silicon
            - Use efficient cache management instructions

4. **Unified Memory Optimization**

            - Optimize for unified memory architecture
            - Minimize cache conflicts between cores
            - Use appropriate cache line sizes (128 bytes for Apple Silicon)

### Integration Points

- Use core type info in scheduler affinity decisions
- Apply optimizations throughout codebase

### Testing

- Verify P-core and E-core detection
- Benchmark performance differences
- Test on real Apple Silicon hardware

---

## Phase 11: Testing and Validation

### Goals

Build comprehensive testing framework

### Tasks

1. **Unit Tests** (tests/)

            - Create test harness in assembly
            - Test individual components (scheduler, process, memory)
            - Implement assertion framework with UART output

2. **Stress Testing**

            - Create test with 1000+ processes
            - Test work stealing under heavy load
            - Test memory allocation stress scenarios

3. **Performance Benchmarking**

            - Measure context switch overhead
            - Measure message passing latency
            - Compare with BEAM VM benchmarks

4. **Regression Tests**

            - Create automated test suite
            - Add tests for each bug found
            - Ensure consistency across test runs

### Integration Points

- Add test entry points in runtime.s
- Create separate test build target in Makefile

### Testing

- Run all tests and ensure 100% pass rate
- Achieve performance within 20% of native BEAM

---

## Phase 12: Integration

### Goals

Integrate all components into cohesive system

### Tasks

1. **System Initialization** (boot.s, runtime.s)

            - Define initialization order: memory -> timer -> scheduler -> processes
            - Initialize all subsystems from runtime_init()
            - Ensure proper dependencies between components

2. **Configuration System** (config.inc)

            - Add configuration parameters (MAX_PROCESSES, STACK_SIZE, etc.)
            - Make system tunable without recompilation where possible

3. **Error Handling**

            - Add consistent error handling across all modules
            - Implement panic() function for fatal errors
            - Add error codes and reporting

4. **Documentation**

            - Document all assembly functions
            - Create architecture diagrams
            - Write usage guide for the system

### Integration Points

- Update Makefile with all new .s files
- Update link.ld with necessary memory sections

### Testing

- Integration test with all components active
- Full system stress test
- Performance validation

---

## Implementation Order

1. Phase 1: Research (analysis and planning)
2. Phase 3: Process Model (foundation)
3. Phase 2: Core Scheduler (basic scheduling)
4. Phase 7: Memory Management (needed for processes)
5. Phase 9: Timer System (needed for preemption)
6. Phase 6: Yielding and Preemption (complete scheduling)
7. Phase 4: Load Balancing (multi-core optimization)
8. Phase 5: CPU Affinity (advanced scheduling)
9. Phase 8: Inter-Core Communication (process interaction)
10. Phase 10: Apple Silicon Optimization (platform-specific)
11. Phase 11: Testing and Validation (verification)
12. Phase 12: Integration (finalization)

## Key ARM64 Instructions to Use

- **Atomic Operations**: LDXR, STXR, LDAXR, STLXR, CAS
- **Memory Barriers**: DMB, DSB, ISB
- **Event Signaling**: SEV, WFE
- **System Registers**: MPIDR_EL1 (core ID), CNTPCT_EL0 (timer)
- **Context Switching**: STP, LDP (store/load pair for efficiency)

## Apple Silicon Specifics

- P-cores (Firestorm/Avalanche): 8-wide decode, optimized for performance
- E-cores (Icestorm/Blizzard): 4-wide decode, optimized for efficiency
- Unified L2 cache per cluster
- 128-byte cache lines
- Relaxed memory ordering on ARM (use barriers appropriately)