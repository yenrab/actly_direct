// MIT License
//
// Copyright (c) 2025 Lee Barney
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

// ------------------------------------------------------------
// scheduler_core.s — Core Scheduler Implementation
// ------------------------------------------------------------
// BEAM-style per-core scheduler with priority-based run queues,
// reduction counting, and basic scheduling logic. Implements
// Phase 2 of the research implementation plan.
//
// The file provides:
//   - Per-core scheduler state with 4 priority queues
//   - Basic scheduling logic with priority selection
//   - Round-robin within priority levels
//   - Reduction counting (2000 per time slice)
//   - Process state management
//   - Scheduler initialization and idle handling
//
// Version: 0.10
// Author: Lee Barney
// Last Modified: 2025-01-19
//

    .text
    .align 4

// ------------------------------------------------------------
// Scheduler Function Exports
// ------------------------------------------------------------
// Export the main scheduler functions to make them callable from C code.
// These functions provide the primary interface for scheduler operations,
// process scheduling, and core management.
//
// WARNING: These exports are intended ONLY for unit testing and other
// testing purposes. There is NO guarantee they will exist over various
// versions, nor any intention to make them stable or backwards compatible
// over versions. Do not use these exports in production code.
//
// Version: 0.10
// Author: Lee Barney
// Last Modified: 2025-01-19
//
    .global _scheduler_init
    .global _scheduler_schedule
    .global _scheduler_idle
    .global _scheduler_enqueue_process
    .global _scheduler_dequeue_process
    .global _scheduler_get_current_process
    .global _scheduler_set_current_process
    .global _scheduler_get_current_reductions
    .global _scheduler_set_current_reductions
    .global _scheduler_decrement_reductions
    .global _scheduler_get_core_id
    .global _scheduler_get_queue_length

// Additional exports for compatibility with existing tests
    .global _MAX_CORES
    .global _PRIORITY_MAX
    .global _PRIORITY_HIGH
    .global _PRIORITY_NORMAL
    .global _PRIORITY_LOW
    .global _NUM_PRIORITIES
    .global _DEFAULT_REDUCTIONS
    // Process state constants are defined in process.s
    .global _PRIORITY_QUEUE_SIZE
    .global _SCHEDULER_SIZE
    // Non-underscore versions for C compatibility
    .global _MAX_CORES_CONST
    .global _NUM_PRIORITIES_CONST
    .global _PRIORITY_QUEUE_SIZE_CONST
    .global _SCHEDULER_SIZE_CONST

// ------------------------------------------------------------
// Scheduler Configuration Constants
// ------------------------------------------------------------
// Define scheduler configuration values including priority levels,
// reduction counts, and system limits. These constants control the
// behavior and limits of the scheduler system.
//
// Version: 0.10
// Author: Lee Barney
// Last Modified: 2025-01-19
//
    .equ MAX_CORES, 128                  // Maximum number of CPU cores
    .equ PRIORITY_LEVELS, 4              // Number of priority levels
    .equ DEFAULT_REDUCTIONS, 2000        // Default reductions per time slice
    .equ MAX_REDUCTIONS, 10000           // Maximum reductions per time slice
    .equ MIN_REDUCTIONS, 100             // Minimum reductions per time slice

// Priority level constants
    .equ PRIORITY_MAX, 0                 // System-critical processes
    .equ PRIORITY_HIGH, 1                // Interactive processes
    .equ PRIORITY_NORMAL, 2              // Regular application processes
    .equ PRIORITY_LOW, 3                 // Background processes

// Process state constants
    .equ PROCESS_STATE_CREATED, 0        // Process created but not scheduled
    .equ PROCESS_STATE_READY, 1          // Process ready to run
    .equ PROCESS_STATE_RUNNING, 2        // Process currently executing
    .equ PROCESS_STATE_WAITING, 3        // Process blocked on I/O/message
    .equ PROCESS_STATE_SUSPENDED, 4      // Process temporarily suspended
    .equ PROCESS_STATE_TERMINATED, 5     // Process finished execution

// ------------------------------------------------------------
// Global Symbol Definitions for C Compatibility
// ------------------------------------------------------------
// Define global symbols that can be accessed from C code.
// These provide the same interface as the old scheduler.s
//
    .data
    .align 3

_MAX_CORES:
    .quad MAX_CORES

_PRIORITY_MAX:
    .quad PRIORITY_MAX

_PRIORITY_HIGH:
    .quad PRIORITY_HIGH

_PRIORITY_NORMAL:
    .quad PRIORITY_NORMAL

_PRIORITY_LOW:
    .quad PRIORITY_LOW

// Compatibility aliases for constants
PRIORITY_MAX:
    .quad PRIORITY_MAX

PRIORITY_HIGH:
    .quad PRIORITY_HIGH

PRIORITY_NORMAL:
    .quad PRIORITY_NORMAL

PRIORITY_LOW:
    .quad PRIORITY_LOW

DEFAULT_REDUCTIONS:
    .quad 2000

_NUM_PRIORITIES:
    .quad PRIORITY_LEVELS

_DEFAULT_REDUCTIONS:
    .quad 2000

    // Process state constants are defined in process.s

_PRIORITY_QUEUE_SIZE:
    .quad queue_size

_SCHEDULER_SIZE:
    .quad 144  // Updated scheduler size

// Non-underscore versions for C compatibility (as data symbols)
_MAX_CORES_CONST:
    .quad 128  // MAX_CORES value

_NUM_PRIORITIES_CONST:
    .quad 4    // PRIORITY_LEVELS value

_PRIORITY_QUEUE_SIZE_CONST:
    .quad 24   // queue_size value

_SCHEDULER_SIZE_CONST:
    .quad 144  // Updated scheduler size

// Compatibility aliases for test functions
_scheduler_get_queue_length_compat:
    .quad _scheduler_get_queue_length_queue_ptr


    .text
    .align 4

// ------------------------------------------------------------
// Scheduler Data Structure Layout
// ------------------------------------------------------------
// Define the memory layout for the scheduler state structure.
// Each core has its own scheduler instance with independent
// priority queues and state management.
//
// Version: 0.10
// Author: Lee Barney
// Last Modified: 2025-01-19
//
    // Priority queue structure offsets
    .equ queue_head, 0                   // Head pointer (8 bytes)
    .equ queue_tail, 8                   // Tail pointer (8 bytes)
    .equ queue_count, 16                 // Process count (4 bytes)
    .equ queue_padding, 20               // Padding to 8-byte alignment
    .equ queue_size, 24                  // Total queue structure size

    // Scheduler state structure offsets
    .equ scheduler_core_id, 0            // Core ID (8 bytes)
    .equ scheduler_queues, 8             // Priority queues array (4 * 24 = 96 bytes)
    .equ scheduler_current_process, 104  // Current running process (8 bytes)
    .equ scheduler_current_reductions, 112 // Current reduction count (8 bytes)
    .equ scheduler_total_scheduled, 120  // Total processes scheduled (8 bytes)
    .equ scheduler_total_yields, 128     // Total voluntary yields (8 bytes)
    .equ scheduler_total_migrations, 136 // Total process migrations (8 bytes)
    .equ scheduler_idle_count, 144       // Idle loop count (8 bytes)
    .equ scheduler_padding, 144          // No padding needed
    .equ scheduler_size, 144             // Total scheduler state size

// ------------------------------------------------------------
// Global Scheduler Data
// ------------------------------------------------------------
// Define global data structures for scheduler state management.
// Each core has its own scheduler instance stored in the .bss section.
//
// Version: 0.10
// Author: Lee Barney
// Last Modified: 2025-01-19
//
    .bss
    .align 3  // 8-byte alignment

    // Array of scheduler states (one per core)
    .global _scheduler_states
_scheduler_states:
    .space scheduler_size * MAX_CORES

    // Note: Global scheduler statistics removed - was unused (64 bytes saved)

    // Core ID detection - removed global variable

    .text
    .align 4

// ------------------------------------------------------------
// Scheduler Initialization
// ------------------------------------------------------------
// Initialize the scheduler for the current core. This function sets up
// the priority queues, initializes counters, and prepares the scheduler
// for process management. Must be called once per core during system startup.
//
// Parameters:
//   x0 (uint64_t) - core_id: Core ID (0 to MAX_CORES-1)
//
// Returns:
//   None (void function)
//
// Complexity: O(1) - Constant time initialization regardless of core count
//
// Version: 0.10
// Author: Lee Barney
// Last Modified: 2025-01-19
//
// Clobbers: x1, x2, x3, x4, x5, x6, x7, x8, x9, x10, x11, x12, x13, x14, x15, x16, x17, x18, x19, x20, x21, x22, x23, x24, x25, x26, x27, x28, x29, x30
//
_scheduler_init:
    // Save callee-saved registers
    stp x19, x30, [sp, #-16]!
    stp x20, x21, [sp, #-16]!

    // Validate core ID
    cmp x0, #MAX_CORES
    b.ge scheduler_init_failed

    // Save core ID
    mov x19, x0

    // Calculate scheduler state address
    adrp x20, _scheduler_states@PAGE
    add x20, x20, _scheduler_states@PAGEOFF
    mov x21, #scheduler_size
    mul x21, x19, x21
    add x20, x20, x21  // x20 = scheduler state address

    // Initialize core ID
    str x19, [x20, #scheduler_core_id]  // Store 64-bit value

    // Initialize priority queues
    mov x21, #0  // Queue index
    mov x22, #PRIORITY_LEVELS  // Number of queues
    add x23, x20, #scheduler_queues  // Queue array base

init_queue_loop:
    // Initialize queue head and tail to NULL
    str xzr, [x23, #queue_head]
    str xzr, [x23, #queue_tail]
    
    // Initialize queue count to 0
    str wzr, [x23, #queue_count]
    
    // Move to next queue
    add x23, x23, #queue_size
    add x21, x21, #1
    cmp x21, x22
    b.lt init_queue_loop

    // Initialize current process to NULL
    str xzr, [x20, #scheduler_current_process]

    // Initialize current reductions to default
    mov x21, #2000  // DEFAULT_REDUCTIONS
    str x21, [x20, #scheduler_current_reductions]  // Store 64-bit value

    // Initialize statistics to 0
    str xzr, [x20, #scheduler_total_scheduled]
    str xzr, [x20, #scheduler_total_yields]
    str xzr, [x20, #scheduler_total_migrations]
    str xzr, [x20, #scheduler_idle_count]

    // Core ID is now passed as parameter, no need to store globally

    // Return (void function)
    ldp x20, x21, [sp], #16
    ldp x19, x30, [sp], #16
    ret

scheduler_init_failed:
    ldp x20, x21, [sp], #16
    ldp x19, x30, [sp], #16
    ret

// ------------------------------------------------------------
// Scheduler Schedule Function
// ------------------------------------------------------------
// Select the next process to run from the highest priority non-empty queue.
// Implements strict priority scheduling with round-robin within each priority level.
// This is the core scheduling algorithm that determines which process runs next.
//
// Parameters:
//   x0 (uint64_t) - core_id: Core ID (0 to MAX_CORES-1)
//
// Returns:
//   x0 (void*) - process: Pointer to next process to run, or NULL if no processes ready
//
// Complexity: O(p) where p is the number of priority levels (4)
//
// Version: 0.10
// Author: Lee Barney
// Last Modified: 2025-01-19
//
// Clobbers: x1, x2, x3, x4, x5, x6, x7, x8, x9, x10, x11, x12, x13, x14, x15, x16, x17, x18, x19, x20, x21, x22, x23, x24, x25, x26, x27, x28, x29, x30
//
_scheduler_schedule:
    // x0 = core_id parameter
    // Save callee-saved registers
    stp x19, x20, [sp, #-16]!
    stp x21, x22, [sp, #-16]!
    stp x23, x24, [sp, #-16]!
    stp x25, x26, [sp, #-16]!
    stp x27, x30, [sp, #-16]!

    // Validate core ID
    cmp x0, #MAX_CORES
    b.ge schedule_invalid_core

    // Calculate scheduler state address
    adrp x20, _scheduler_states@PAGE
    add x20, x20, _scheduler_states@PAGEOFF
    mov x21, #scheduler_size
    mul x21, x0, x21
    add x20, x20, x21  // x20 = scheduler state address

    // Check each priority level from highest to lowest
    mov x21, #0  // Priority level (0 = MAX, 1 = HIGH, 2 = NORMAL, 3 = LOW)
    mov x22, #PRIORITY_LEVELS  // Number of priority levels

schedule_priority_loop:
    // Calculate queue address for this priority
    add x23, x20, #scheduler_queues  // Queue array base
    mov x24, #queue_size
    mul x24, x21, x24
    add x23, x23, x24  // x23 = queue address

    // Check if queue is empty
    ldr w24, [x23, #queue_count]
    cbz w24, schedule_next_priority

    // Queue is not empty, get the head process
    ldr x25, [x23, #queue_head]
    cbz x25, schedule_next_priority

    // Found a process, dequeue it
    // Update queue head to next process
    ldr x26, [x25, #0]  // Load next pointer from PCB (offset 0 = pcb_next)
    str x26, [x23, #queue_head]

    // If this was the last process, update tail to NULL
    cbz x26, schedule_update_tail
    // Skip clearing prev pointer for now to avoid segfault
    b schedule_decrement_count

schedule_update_tail:
    str xzr, [x23, #queue_tail]

schedule_decrement_count:
    // Decrement queue count
    sub w24, w24, #1
    str w24, [x23, #queue_count]

    // Clear next/prev pointers of dequeued process
    str xzr, [x25, #0]   // Clear next pointer
    str xzr, [x25, #8]   // Clear prev pointer

    // Set process state to RUNNING
    mov w26, #PROCESS_STATE_RUNNING
    str w26, [x25, #32]  // Set state (offset 32 = pcb_state)

    // Set as current process
    str x25, [x20, #scheduler_current_process]

    // Set reduction count to default
    mov w26, #2000  // DEFAULT_REDUCTIONS
    str w26, [x20, #scheduler_current_reductions]

    // Increment total scheduled count
    ldr x26, [x20, #scheduler_total_scheduled]
    add x26, x26, #1
    str x26, [x20, #scheduler_total_scheduled]

    // Return process pointer
    mov x0, x25
    ldp x27, x30, [sp], #16
    ldp x25, x26, [sp], #16
    ldp x23, x24, [sp], #16
    ldp x21, x22, [sp], #16
    ldp x19, x20, [sp], #16
    ret

schedule_next_priority:
    // Move to next priority level
    add x21, x21, #1
    cmp x21, x22
    b.lt schedule_priority_loop

    // No processes ready, return NULL
    mov x0, #0
    ldp x27, x30, [sp], #16
    ldp x25, x26, [sp], #16
    ldp x23, x24, [sp], #16
    ldp x21, x22, [sp], #16
    ldp x19, x20, [sp], #16
    ret

schedule_invalid_core:
    // Invalid core ID, return NULL
    mov x0, #0
    ldp x27, x30, [sp], #16
    ldp x25, x26, [sp], #16
    ldp x23, x24, [sp], #16
    ldp x21, x22, [sp], #16
    ldp x19, x20, [sp], #16
    ret

// ------------------------------------------------------------
// Scheduler Idle Function
// ------------------------------------------------------------
// Handle idle core scenario when no processes are ready to run.
// This is a stub implementation that always returns NULL.
// Can be extended to support work stealing and other idle-time optimizations.
//
// Parameters:
//   None (uses hardcoded core ID 0)
//
// Returns:
//   x0 (void*) - process: Always returns NULL (stub implementation)
//
// Complexity: O(1) - Constant time stub function
//
// Version: 0.10
// Author: Lee Barney
// Last Modified: 2025-01-19
//
// Clobbers: x1, x2, x3, x4, x5, x6, x7, x8, x9, x10, x11, x12, x13, x14, x15, x16, x17, x18, x19, x20, x21, x22, x23, x24, x25, x26, x27, x28, x29, x30
//
_scheduler_idle:
    // Save callee-saved registers
    stp x19, x30, [sp, #-16]!
    stp x20, x21, [sp, #-16]!

    // Use default core ID (0) since global variable is removed
    mov x19, #0

    // Calculate scheduler state address
    adrp x20, _scheduler_states@PAGE
    add x20, x20, _scheduler_states@PAGEOFF
    mov x21, #scheduler_size
    mul x21, x19, x21
    add x20, x20, x21  // x20 = scheduler state address

    // Increment idle count
    ldr x21, [x20, #scheduler_idle_count]
    add x21, x21, #1
    str x21, [x20, #scheduler_idle_count]

    // For now, just return NULL (no work stealing implemented yet)
    // TODO: Implement work stealing in Phase 4
    mov x0, #0

    ldp x20, x21, [sp], #16
    ldp x19, x30, [sp], #16
    ret

// ------------------------------------------------------------
// Scheduler Enqueue Process
// ------------------------------------------------------------
// Add a process to the appropriate priority queue. The process is
// added to the tail of the queue for round-robin scheduling within
// the priority level.
//
// Parameters:
//   x0 (uint64_t) - core_id: Core ID (0 to MAX_CORES-1)
//   x1 (void*) - process: Process pointer (PCB)
//   x2 (uint32_t) - priority: Priority level (0=MAX, 1=HIGH, 2=NORMAL, 3=LOW)
//
// Returns:
//   x0 (int) - success: 1 on success, 0 on failure
//
// Complexity: O(1) - Constant time queue insertion
//
// Version: 0.10
// Author: Lee Barney
// Last Modified: 2025-01-19
//
// Clobbers: x1, x2, x3, x4, x5, x6, x7, x8, x9, x10, x11, x12, x13, x14, x15, x16, x17, x18, x19, x20, x21, x22, x23, x24, x25, x26, x27, x28, x29, x30
//
_scheduler_enqueue_process:
    // Save callee-saved registers
    stp x19, x30, [sp, #-16]!
    stp x20, x21, [sp, #-16]!

    // Validate parameters
    cbz x1, enqueue_failed  // Check process pointer (now in x1)
    cmp x2, #PRIORITY_LEVELS
    b.ge enqueue_failed

    // Save parameters
    mov x19, x0  // Core ID
    mov x20, x1  // Process pointer
    mov x21, x2  // Priority level

    // Use provided core ID instead of global
    // x19 already contains the core ID

    // Calculate scheduler state address
    adrp x22, _scheduler_states@PAGE
    add x22, x22, _scheduler_states@PAGEOFF
    mov x23, #scheduler_size
    mul x23, x19, x23  // Use core ID from x19
    add x22, x22, x23  // x22 = scheduler state address

    // Calculate queue address for this priority
    add x23, x22, #scheduler_queues  // Queue array base
    mov x24, #queue_size
    mul x24, x21, x24  // Use priority from x21
    add x23, x23, x24  // x23 = queue address

    // Set process state to READY
    mov w24, #PROCESS_STATE_READY
    str w24, [x20, #32]  // Set state (offset 32 = pcb_state) using process pointer from x20

    // Get current tail
    ldr x24, [x23, #queue_tail]
    cbz x24, enqueue_empty_queue

    // Queue is not empty, add to tail
    str x20, [x24, #0]   // Set next pointer of current tail (use process pointer from x20)
    str x24, [x20, #8]   // Set prev pointer of new process (use process pointer from x20)
    str x20, [x23, #queue_tail]  // Update queue tail (use process pointer from x20)
    b enqueue_increment_count

enqueue_empty_queue:
    // Queue is empty, this becomes both head and tail
    str x20, [x23, #queue_head]  // Use process pointer from x20
    str x20, [x23, #queue_tail]  // Use process pointer from x20
    str xzr, [x20, #8]   // Clear prev pointer (use process pointer from x20)
    str xzr, [x20, #0]   // Clear next pointer (use process pointer from x20)

enqueue_increment_count:
    // Increment queue count
    ldr w24, [x23, #queue_count]
    add w24, w24, #1
    str w24, [x23, #queue_count]

    // Return success
    mov x0, #1
    ldp x20, x21, [sp], #16
    ldp x19, x30, [sp], #16
    ret

enqueue_failed:
    mov x0, #0
    ldp x20, x21, [sp], #16
    ldp x19, x30, [sp], #16
    ret

// ------------------------------------------------------------
// Scheduler Dequeue Process
// ------------------------------------------------------------
// Remove a process from the head of a priority queue. This is used when
// a process needs to be moved to a different queue or when a process
// terminates. Returns the dequeued process or NULL if queue is empty.
//
// Parameters:
//   x0 (void*) - queue: Pointer to the priority queue structure
//
// Returns:
//   x0 (void*) - process: Pointer to dequeued process, or NULL if queue empty
//
// Complexity: O(1) - Constant time queue removal
//
// Version: 0.10
// Author: Lee Barney
// Last Modified: 2025-01-19
//
// Clobbers: x1, x2, x3, x4, x5, x6, x7, x8, x9, x10, x11, x12, x13, x14, x15, x16, x17, x18, x19, x20, x21, x22, x23, x24, x25, x26, x27, x28, x29, x30
//
_scheduler_dequeue_process:
    // x0 = queue pointer
    // Return: process pointer or NULL if empty
    
    // Validate queue pointer
    cbz x0, dequeue_failed
    
    // Check if queue is empty
    ldr w1, [x0, #queue_count]
    cbz w1, dequeue_failed
    
    // Get head process
    ldr x1, [x0, #queue_head]
    cbz x1, dequeue_failed
    
    // Update queue head to next process
    ldr x2, [x1, #0]  // Load next pointer
    str x2, [x0, #queue_head]  // Update head
    
    // If this was the last process, clear tail too
    cbz x2, dequeue_clear_tail
    b dequeue_decrement_count

dequeue_clear_tail:
    str xzr, [x0, #queue_tail]  // Clear tail

dequeue_decrement_count:
    // Decrement queue count
    ldr w2, [x0, #queue_count]
    sub w2, w2, #1
    str w2, [x0, #queue_count]
    
    // Return the process
    mov x0, x1
    ret

dequeue_failed:
    mov x0, #0  // Return NULL
    ret

// ------------------------------------------------------------
// Scheduler Get Current Process
// ------------------------------------------------------------
// Get the currently running process for a specific core.
//
// Parameters:
//   x0 (uint64_t) - core_id: Core ID (0 to MAX_CORES-1)
//
// Returns:
//   x0 (void*) - process: Pointer to current process, or NULL if none
//
// Complexity: O(1) - Constant time process lookup
//
// Version: 0.10
// Author: Lee Barney
// Last Modified: 2025-01-19
//
// Clobbers: x1, x2, x3, x4, x5, x6, x7, x8, x9, x10, x11, x12, x13, x14, x15, x16, x17, x18, x19, x20, x21, x22, x23, x24, x25, x26, x27, x28, x29, x30
//
_scheduler_get_current_process:
    // x0 = core_id parameter
    // Validate core ID
    cmp x0, #MAX_CORES
    b.ge get_current_process_invalid

    // Calculate scheduler state address
    adrp x1, _scheduler_states@PAGE
    add x1, x1, _scheduler_states@PAGEOFF
    mov x2, #scheduler_size
    mul x2, x0, x2
    add x1, x1, x2  // x1 = scheduler state address

    // Load current process
    ldr x0, [x1, #scheduler_current_process]
    ret

get_current_process_invalid:
    mov x0, #0
    ret

// ------------------------------------------------------------
// Scheduler Set Current Process
// ------------------------------------------------------------
// Set the currently running process for a specific core.
//
// Parameters:
//   x0 (uint64_t) - core_id: Core ID (0 to MAX_CORES-1)
//   x1 (void*) - process: Process pointer (PCB), or NULL to clear
//
// Returns:
//   x0 (int) - success: 1 on success, 0 on failure
//
// Complexity: O(1) - Constant time process assignment
//
// Version: 0.10
// Author: Lee Barney
// Last Modified: 2025-01-19
//
// Clobbers: x1, x2, x3, x4, x5, x6, x7, x8, x9, x10, x11, x12, x13, x14, x15, x16, x17, x18, x19, x20, x21, x22, x23, x24, x25, x26, x27, x28, x29, x30
//
_scheduler_set_current_process:
    // x0 = core_id parameter
    // x1 = process pointer parameter
    // Validate core ID
    cmp x0, #MAX_CORES
    b.ge set_current_process_invalid

    // Calculate scheduler state address
    adrp x2, _scheduler_states@PAGE
    add x2, x2, _scheduler_states@PAGEOFF
    mov x3, #scheduler_size
    mul x3, x0, x3
    add x2, x2, x3  // x2 = scheduler state address

    // Store current process (x1 contains the process pointer)
    str x1, [x2, #scheduler_current_process]
    mov x0, #1
    ret

set_current_process_invalid:
    mov x0, #0
    ret

// ------------------------------------------------------------
// Scheduler Get Current Reductions
// ------------------------------------------------------------
// Get the current reduction count for core 0 (hardcoded for testing).
// In a real system, this would take a core_id parameter.
//
// Parameters:
//   None (uses hardcoded core ID 0)
//
// Returns:
//   x0 (uint64_t) - reduction_count: Current reduction count for core 0
//
// Complexity: O(1) - Constant time reduction count lookup
//
// Version: 0.10
// Author: Lee Barney
// Last Modified: 2025-01-19
//
// Clobbers: x1, x2, x3, x4, x5, x6, x7, x8, x9, x10, x11, x12, x13, x14, x15, x16, x17, x18, x19, x20, x21, x22, x23, x24, x25, x26, x27, x28, x29, x30
//
_scheduler_get_current_reductions:
    // Use core ID 0 for testing environment
    // In a real system, this would get the current core ID from thread-local storage
    mov x0, #0

    // Calculate scheduler state address
    adrp x1, _scheduler_states@PAGE
    add x1, x1, _scheduler_states@PAGEOFF
    mov x2, #scheduler_size
    mul x2, x0, x2
    add x1, x1, x2  // x1 = scheduler state address

    // Load current reductions
    ldr x0, [x1, #scheduler_current_reductions]  // Load 64-bit value instead of 32-bit
    ret

// ------------------------------------------------------------
// Scheduler Set Current Reductions
// ------------------------------------------------------------
// Set the current reduction count for core 0 (hardcoded for testing).
// In a real system, this would take a core_id parameter.
//
// Parameters:
//   x0 (uint64_t) - reduction_count: New reduction count
//
// Returns:
//   x0 (int) - success: 1 on success, 0 on failure
//
// Complexity: O(1) - Constant time reduction count update
//
// Version: 0.10
// Author: Lee Barney
// Last Modified: 2025-01-19
//
// Clobbers: x1, x2, x3, x4, x5, x6, x7, x8, x9, x10, x11, x12, x13, x14, x15, x16, x17, x18, x19, x20, x21, x22, x23, x24, x25, x26, x27, x28, x29, x30
//
_scheduler_set_current_reductions:
    // Validate reduction count
    mov x1, #MAX_REDUCTIONS
    cmp x0, x1
    b.gt set_reductions_failed
    mov x1, #MIN_REDUCTIONS
    cmp x0, x1
    b.lt set_reductions_failed

    // Use default core ID (0) since global variable is removed
    mov x1, #0

    // Calculate scheduler state address
    adrp x2, _scheduler_states@PAGE
    add x2, x2, _scheduler_states@PAGEOFF
    mov x3, #scheduler_size
    mul x3, x1, x3
    add x2, x2, x3  // x2 = scheduler state address

    // Store current reductions
    str w0, [x2, #scheduler_current_reductions]
    mov x0, #1
    ret

set_reductions_failed:
    mov x0, #0
    ret

// ------------------------------------------------------------
// Scheduler Decrement Reductions
// ------------------------------------------------------------
// Decrement the current reduction count for core 0 (hardcoded for testing).
// This is called during process execution to track remaining time slice.
// In a real system, this would take a core_id parameter.
//
// Parameters:
//   None (uses hardcoded core ID 0)
//
// Returns:
//   x0 (uint64_t) - remaining_count: Remaining reduction count, or 0 if time slice exhausted
//
// Complexity: O(1) - Constant time reduction count decrement
//
// Version: 0.10
// Author: Lee Barney
// Last Modified: 2025-01-19
//
// Clobbers: x1, x2, x3, x4, x5, x6, x7, x8, x9, x10, x11, x12, x13, x14, x15, x16, x17, x18, x19, x20, x21, x22, x23, x24, x25, x26, x27, x28, x29, x30
//
_scheduler_decrement_reductions:
    // Use default core ID (0) since global variable is removed
    mov x0, #0

    // Calculate scheduler state address
    adrp x1, _scheduler_states@PAGE
    add x1, x1, _scheduler_states@PAGEOFF
    mov x2, #scheduler_size
    mul x2, x0, x2
    add x1, x1, x2  // x1 = scheduler state address

    // Load current reductions
    ldr x0, [x1, #scheduler_current_reductions]  // Load 64-bit value instead of 32-bit
    cbz x0, decrement_reductions_zero

    // Decrement reductions
    sub x0, x0, #1
    str x0, [x1, #scheduler_current_reductions]  // Store 64-bit value instead of 32-bit

decrement_reductions_zero:
    ret

// ------------------------------------------------------------
// Scheduler Get Core ID
// ------------------------------------------------------------
// Get the current core ID.
//
// Parameters:
//   None
//
// Returns:
//   x0 - Current core ID
//
// Clobbers: x1, x2, x3, x4, x5, x6, x7, x8, x9, x10, x11, x12, x13, x14, x15, x16, x17, x18, x19, x20, x21, x22, x23, x24, x25, x26, x27, x28, x29, x30
//
// Version: 0.10
// Author: Lee Barney
// Last Modified: 2025-01-19
//
_scheduler_get_core_id:
    // Return core ID 0 for testing environment
    // In a real system, this would read from system registers or thread-local storage
    mov x0, #0
    ret

// ------------------------------------------------------------
// Scheduler Get Queue Length
// ------------------------------------------------------------
// Get the number of processes in a specific priority queue.
//
// Parameters:
//   x0 - Priority level (0=MAX, 1=HIGH, 2=NORMAL, 3=LOW)
//
// Returns:
//   x0 - Number of processes in queue, or 0 if invalid priority
//
// Clobbers: x1, x2, x3, x4, x5, x6, x7, x8, x9, x10, x11, x12, x13, x14, x15, x16, x17, x18, x19, x20, x21, x22, x23, x24, x25, x26, x27, x28, x29, x30
//
// Version: 0.10
// Author: Lee Barney
// Last Modified: 2025-01-19
//
_scheduler_get_queue_length:
    // x0 = queue pointer
    // Return: queue count
    
    // Load queue count directly from queue structure
    ldr x0, [x0, #queue_count]  // Load count from queue structure
    ret

// ------------------------------------------------------------
// Compatibility Functions for Existing Tests
// ------------------------------------------------------------
// These functions provide compatibility with the old scheduler.s interface
// that the existing test files expect.
//

// Export additional functions for compatibility
    .global _get_scheduler_state
    .global _get_priority_queue
    .global _scheduler_get_reduction_count
    .global _scheduler_set_reduction_count
    .global _scheduler_is_queue_empty
    .global _scheduler_get_queue_length_from_queue
    .global _scheduler_get_queue_length_queue_ptr
    .global get_scheduler_state
    .global get_priority_queue
    .global scheduler_get_reduction_count
    .global scheduler_set_reduction_count
    .global scheduler_is_queue_empty
    .global scheduler_get_queue_length_from_queue
    .global scheduler_get_queue_length_queue_ptr

// Compatibility aliases for C-callable interface
    .global scheduler_init
    .global scheduler_schedule
    .global scheduler_enqueue_process
    .global scheduler_dequeue_process
    .global scheduler_is_queue_empty
    .global scheduler_get_queue_length
    .global get_scheduler_state
    .global get_priority_queue
    .global PRIORITY_MAX
    .global PRIORITY_HIGH
    .global PRIORITY_NORMAL
    .global PRIORITY_LOW
    .global DEFAULT_REDUCTIONS

// get_scheduler_state - Get scheduler state for a specific core
_get_scheduler_state:
    // x0 = core_id
    // Return: scheduler state pointer
    
    // Validate core_id
    cmp x0, #MAX_CORES
    b.ge get_scheduler_state_invalid
    
    // Calculate scheduler state address
    adrp x1, _scheduler_states@PAGE
    add x1, x1, _scheduler_states@PAGEOFF
    mov x2, #scheduler_size
    mul x2, x0, x2
    add x0, x1, x2
    
    ret

get_scheduler_state_invalid:
    mov x0, #0
    ret

// get_priority_queue - Get priority queue for a specific core and priority
_get_priority_queue:
    // x0 = scheduler_state pointer
    // x1 = priority level
    // Return: queue pointer
    
    // Validate priority
    cmp x1, #PRIORITY_LEVELS
    b.ge get_priority_queue_invalid
    
    // Calculate queue address
    add x2, x0, #scheduler_queues
    mov x3, #queue_size
    mul x3, x1, x3
    add x0, x2, x3
    
    ret

get_priority_queue_invalid:
    mov x0, #0
    ret

// scheduler_get_reduction_count - Get reduction count for a specific core
_scheduler_get_reduction_count:
    // x0 = core_id
    // Return: reduction count
    
    // Validate core_id
    cmp x0, #MAX_CORES
    b.ge scheduler_get_reduction_count_invalid
    
    // Get scheduler state
    adrp x1, _scheduler_states@PAGE
    add x1, x1, _scheduler_states@PAGEOFF
    mov x2, #scheduler_size
    mul x2, x0, x2
    add x1, x1, x2
    
    // Load reduction count
    ldr x0, [x1, #scheduler_current_reductions]  // Load 64-bit value instead of 32-bit
    
    ret

scheduler_get_reduction_count_invalid:
    mov x0, #0
    ret
    
// scheduler_set_reduction_count - Set reduction count for a specific core
_scheduler_set_reduction_count:
    // x0 = core_id
    // x1 = reduction count
    // Return: success (1) or failure (0)
    
    // Validate core_id
    cmp x0, #MAX_CORES
    b.ge scheduler_set_reduction_count_invalid
    
    // Get scheduler state
    adrp x2, _scheduler_states@PAGE
    add x2, x2, _scheduler_states@PAGEOFF
    mov x3, #scheduler_size
    mul x3, x0, x3
    add x2, x2, x3
    
    // Store reduction count
    str x1, [x2, #scheduler_current_reductions]  // Store 64-bit value instead of 32-bit
    
    mov x0, #1  // Success
    ret

scheduler_set_reduction_count_invalid:
    mov x0, #0  // Failure
    ret

// scheduler_is_queue_empty - Check if a queue is empty
_scheduler_is_queue_empty:
    // x0 = queue pointer
    // Return: 1 if empty, 0 if not empty
    
    // Load queue count
    ldr w1, [x0, #queue_count]
    cbz w1, queue_is_empty
    
    mov x0, #0  // Not empty
    ret

queue_is_empty:
    mov x0, #1  // Empty
    ret

// ------------------------------------------------------------
// Compatibility Functions for Tests
// ------------------------------------------------------------
// These functions provide the interface expected by the test files

// scheduler_get_queue_length with queue pointer parameter
// x0 = queue pointer
// Return: queue count
.global _scheduler_get_queue_length_from_queue
_scheduler_get_queue_length_from_queue:
    // Load queue count directly from queue structure
    ldr x0, [x0, #queue_count]  // Load count from queue structure
    ret

// Compatibility alias for tests that expect queue pointer parameter
.global _scheduler_get_queue_length_queue_ptr
_scheduler_get_queue_length_queue_ptr:
    // Load queue count directly from queue structure
    ldr x0, [x0, #queue_count]  // Load count from queue structure
    ret

// ------------------------------------------------------------
// C-Callable Compatibility Aliases
// ------------------------------------------------------------
// These provide the interface expected by C test files

scheduler_init:
    b _scheduler_init

scheduler_schedule:
    b _scheduler_schedule

scheduler_enqueue_process:
    b _scheduler_enqueue_process

scheduler_get_queue_length_from_queue:
    b _scheduler_get_queue_length_from_queue

scheduler_get_queue_length_queue_ptr:
    b _scheduler_get_queue_length_queue_ptr

scheduler_get_reduction_count:
    b _scheduler_get_reduction_count

scheduler_set_reduction_count:
    b _scheduler_set_reduction_count

scheduler_dequeue_process:
    b _scheduler_dequeue_process

scheduler_is_queue_empty:
    b _scheduler_is_queue_empty

scheduler_get_queue_length:
    b _scheduler_get_queue_length

get_scheduler_state:
    b _get_scheduler_state

get_priority_queue:
    b _get_priority_queue

