# BEAM Process Scheduling Behavior Research

## Overview

This document provides detailed research and analysis of BEAM's process scheduling behavior, focusing on the lightweight process model, execution patterns, and scheduling mechanisms that enable efficient multi-core operation.

## 1. Lightweight Processes (Start at ~2KB)

### Process Size Characteristics
- **Initial size**: ~2KB per process (much smaller than OS threads)
- **Growth pattern**: Processes grow dynamically as needed
- **Memory efficiency**: Thousands of processes can exist simultaneously
- **Fast creation**: Process creation is extremely fast (~microseconds)

### Memory Layout per Process
```
Process Memory Structure (~2KB initial):
┌─────────────────────────────────────┐
│ Process Control Block (PCB)         │ 128 bytes
├─────────────────────────────────────┤
│ Stack (grows downward)              │ 1KB initial
├─────────────────────────────────────┤
│ Heap (grows upward)                 │ 512 bytes initial
├─────────────────────────────────────┤
│ Message Queue                       │ 256 bytes
├─────────────────────────────────────┤
│ Process-specific data               │ 128 bytes
└─────────────────────────────────────┘
Total: ~2KB initial
```

### Process Creation Overhead
- **No OS thread creation**: Processes are user-space constructs
- **No kernel context switching**: All scheduling happens in user space
- **Minimal memory allocation**: Only allocate what's needed
- **Fast initialization**: Simple data structure setup

### Benefits of Lightweight Processes
- **Massive concurrency**: Support for millions of processes
- **Low overhead**: Minimal memory and CPU overhead
- **Fast spawning**: Rapid process creation and destruction
- **Efficient resource usage**: Only use resources when needed

## 2. Each Process Has Its Own Stack and Heap

### Stack Management
- **Private stack**: Each process has its own stack space
- **Stack growth**: Stacks grow dynamically as needed
- **Stack overflow protection**: Guard pages or bounds checking
- **Stack sharing**: No sharing between processes (isolation)

### Stack Characteristics
```
Stack Properties:
- Initial size: 1KB
- Growth direction: Downward (toward lower addresses)
- Alignment: 16-byte aligned for ARM64
- Overflow detection: Stack pointer bounds checking
- Underflow protection: Guard pages or sentinel values
```

### Heap Management
- **Private heap**: Each process has its own heap space
- **Heap growth**: Heaps grow dynamically as needed
- **Garbage collection**: Per-process or generational GC
- **Memory isolation**: No direct memory sharing between processes

### Heap Characteristics
```
Heap Properties:
- Initial size: 512 bytes
- Growth direction: Upward (toward higher addresses)
- Allocation strategy: Bump allocator or free-list
- Garbage collection: Mark-and-sweep or copying
- Memory limits: Configurable per-process limits
```

### Memory Isolation Benefits
- **Fault isolation**: Process crashes don't affect others
- **Security**: No direct memory access between processes
- **Debugging**: Easier to debug individual processes
- **Reliability**: System remains stable despite process failures

## 3. Processes Run Until Reduction Count Exhausted or Blocking Operation

### Execution Model
- **Cooperative scheduling**: Processes yield control voluntarily or by preemption
- **Reduction-based preemption**: Processes are preempted after 2000 reductions
- **Blocking operations**: Processes block on I/O, messages, or synchronization
- **No time slicing**: No fixed time quantum, only reduction counting

### Execution Flow
```
Process Execution Cycle:
1. Process is scheduled and starts running
2. Process executes instructions, reducing reduction count
3. Process continues until:
   a) Reduction count reaches 0 (preemption)
   b) Process performs blocking operation (voluntary yield)
   c) Process terminates
4. Context is saved and process is enqueued
5. Next process is scheduled
```

### Blocking Operations
- **Message receive**: `receive` blocks until message arrives
- **I/O operations**: File, network, or device I/O
- **Synchronization**: Locks, semaphores, or barriers
- **Timer operations**: `sleep`, `delay`, or timeout operations
- **Resource waiting**: Waiting for memory, CPU, or other resources

### Preemption Points
- **Function calls**: Each function call decrements reduction count
- **Arithmetic operations**: Basic operations cost reductions
- **Control structures**: Loops and conditionals cost reductions
- **Built-in functions**: BIF calls are preemption points
- **Message operations**: Send/receive operations

### Benefits of This Model
- **Predictable behavior**: Consistent execution patterns
- **Responsive system**: No process can monopolize CPU
- **Efficient blocking**: Processes don't consume CPU while waiting
- **Fair scheduling**: All processes get CPU time

## 4. Process Priority Queues Per Scheduler

### Priority Queue Structure
Each scheduler maintains four priority queues:
- **MAX priority**: System-critical processes
- **HIGH priority**: Interactive and real-time processes
- **NORMAL priority**: Regular application processes
- **LOW priority**: Background and batch processes

### Queue Implementation
```
Priority Queue per Scheduler:
struct priority_queue {
    struct process *head;        // First process in queue
    struct process *tail;        // Last process in queue
    uint32_t count;              // Number of processes
    uint32_t total_reductions;   // Total reductions executed
    uint64_t last_scheduled;     // Timestamp of last scheduling
};

struct scheduler {
    struct priority_queue queues[4];  // MAX, HIGH, NORMAL, LOW
    struct process *current_process;  // Currently running process
    uint32_t current_reductions;      // Remaining reductions
};
```

### Scheduling Algorithm
```
Priority-based Scheduling:
1. Check MAX priority queue first
2. If empty, check HIGH priority queue
3. If empty, check NORMAL priority queue
4. If empty, check LOW priority queue
5. If all empty, try work stealing
6. If still no work, go idle
```

### Round-Robin Within Priority
- **Same priority**: Processes of same priority share CPU time
- **Round-robin**: Processes are scheduled in FIFO order
- **Fair sharing**: Each process gets equal opportunity
- **Priority aging**: Long-waiting processes may get priority boost

### Queue Management Operations
```
Queue Operations:
- enqueue(process, priority): Add process to priority queue
- dequeue(priority): Remove and return next process
- is_empty(priority): Check if queue is empty
- queue_length(priority): Get number of processes in queue
- promote_process(process, new_priority): Move to higher priority
```

## Process Lifecycle and State Transitions

### Process States
- **CREATED**: Process created but not yet scheduled
- **READY**: Process ready to run, waiting in queue
- **RUNNING**: Process currently executing on CPU
- **WAITING**: Process blocked on I/O, message, or resource
- **SUSPENDED**: Process temporarily suspended
- **TERMINATED**: Process finished execution

### State Transition Diagram
```
Process State Transitions:
CREATED → READY → RUNNING → READY (preemption)
                ↓
            WAITING → READY (wake up)
                ↓
            TERMINATED
```

### State Transition Functions
```
State Management:
- process_create() → CREATED
- process_schedule() → READY → RUNNING
- process_preempt() → RUNNING → READY
- process_block() → RUNNING → WAITING
- process_wake() → WAITING → READY
- process_terminate() → RUNNING → TERMINATED
```

### Process Creation Process
```
Process Creation Flow:
1. Allocate memory for PCB, stack, and heap
2. Initialize process data structures
3. Set up initial context (registers, stack pointer)
4. Set process state to CREATED
5. Enqueue to appropriate priority queue (READY)
6. Signal scheduler if it was idle
```

## Context Switching Mechanisms

### Context Switch Overhead
- **Minimal overhead**: <100 CPU cycles per context switch
- **Register save/restore**: Save x0-x30, sp, lr, pc, pstate
- **Stack switching**: Switch to process's private stack
- **Cache effects**: Minimize cache pollution

### Context Switch Implementation
```
Context Switch Process:
1. Save current process context to PCB
2. Update process state to READY
3. Enqueue current process to run queue
4. Select next process from priority queue
5. Restore next process context from PCB
6. Update process state to RUNNING
7. Jump to process's program counter
```

### ARM64 Context Switch
```assembly
// Save context
stp x0, x1, [sp, #-16]!
stp x2, x3, [sp, #-16]!
// ... save all registers x0-x30
str sp, [pcb, #PCB_STACK_POINTER]
str lr, [pcb, #PCB_LINK_REGISTER]
str pc, [pcb, #PCB_PROGRAM_COUNTER]
mrs x0, SPSR_EL1
str x0, [pcb, #PCB_PROCESSOR_STATE]

// Restore context
ldr x0, [pcb, #PCB_PROCESSOR_STATE]
msr SPSR_EL1, x0
ldr x0, [pcb, #PCB_PROGRAM_COUNTER]
ldr lr, [pcb, #PCB_LINK_REGISTER]
ldr sp, [pcb, #PCB_STACK_POINTER]
ldp x30, x29, [sp], #16
ldp x28, x27, [sp], #16
// ... restore all registers x0-x30
```

### Context Switch Optimization
- **Register pairs**: Use STP/LDP for efficient register save/restore
- **Cache optimization**: Keep PCB in cache-friendly memory layout
- **Branch prediction**: Minimize branch mispredictions
- **Pipeline efficiency**: Avoid pipeline stalls

## Inter-Process Communication

### Message Passing Model
- **Asynchronous**: Send operations are non-blocking
- **Synchronous receive**: Receive operations block until message arrives
- **Mailbox model**: Each process has a message queue
- **Pattern matching**: Messages can be selectively received

### Message Queue Structure
```
Message Queue per Process:
struct message_queue {
    struct message *head;        // First message
    struct message *tail;        // Last message
    uint32_t count;              // Number of messages
    uint32_t max_size;           // Maximum queue size
    struct process *waiting;     // Process waiting for messages
};

struct message {
    struct process *sender;      // Sending process
    void *data;                  // Message data
    uint32_t size;               // Message size
    struct message *next;        // Next message in queue
};
```

### Communication Patterns
- **One-to-one**: Direct process-to-process communication
- **One-to-many**: Broadcast messages to multiple processes
- **Many-to-one**: Multiple processes sending to one process
- **Pattern matching**: Selective message reception

### Message Operations
```
Message Operations:
- send_message(pid, data, size): Send message to process
- receive_message(pattern): Receive message matching pattern
- receive_message_timeout(pattern, timeout): Receive with timeout
- peek_message(): Check for messages without blocking
- flush_messages(): Clear all messages from queue
```

## Process Scheduling Statistics

### Per-Process Statistics
- **Total execution time**: Cumulative CPU time used
- **Scheduling count**: Number of times scheduled
- **Blocking count**: Number of times blocked
- **Message count**: Messages sent and received
- **Memory usage**: Current stack and heap usage

### Per-Scheduler Statistics
- **Total processes**: Number of processes managed
- **Queue lengths**: Processes in each priority queue
- **Load metrics**: CPU utilization and load balancing
- **Migration count**: Number of processes migrated
- **Work stealing**: Steal attempts and successes

### Performance Metrics
- **Context switch time**: Average time per context switch
- **Process creation time**: Time to create new process
- **Message latency**: Time for message delivery
- **Throughput**: Processes scheduled per second
- **Response time**: Time from ready to running

## Implementation Considerations

### Memory Management
- **Stack pools**: Pre-allocate stacks for fast process creation
- **Heap pools**: Use memory pools for efficient allocation
- **Garbage collection**: Implement per-process or generational GC
- **Memory limits**: Enforce per-process memory limits

### Error Handling
- **Stack overflow**: Detect and handle stack overflow
- **Heap exhaustion**: Handle out-of-memory conditions
- **Invalid states**: Validate process state transitions
- **Resource limits**: Enforce system resource limits

### Debugging Support
- **Process tracing**: Track process execution and state changes
- **Message tracing**: Log message passing between processes
- **Performance profiling**: Measure scheduling and communication overhead
- **Deadlock detection**: Detect and resolve deadlocks

### Scalability Considerations
- **Lock-free data structures**: Minimize contention in multi-core systems
- **Cache-friendly layouts**: Optimize memory layout for cache performance
- **NUMA awareness**: Consider memory locality in multi-socket systems
- **Load balancing**: Distribute processes evenly across cores

## Next Steps

This research provides the foundation for implementing BEAM's process scheduling behavior. The next phase should focus on:

1. **Process Control Block implementation** in ARM64 assembly
2. **Stack and heap management** for lightweight processes
3. **Context switching mechanisms** with minimal overhead
4. **Priority queue management** and scheduling algorithms
5. **Message passing system** for inter-process communication
6. **Testing framework** for process scheduling validation

The implementation should start with basic process creation and scheduling, then add advanced features like message passing and load balancing.
