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
// yield.s — Yielding and Preemption Implementation
// ------------------------------------------------------------
// BEAM-style yielding and preemption mechanisms with reduction-based
// preemption, voluntary yields, and cooperative scheduling. Implements
// Phase 6 of the research implementation plan.
//
// The file provides:
//   - Reduction-based preemption at function boundaries
//   - Voluntary yield functions (BEAM-compatible)
//   - Conditional yield functions
//   - Preemption and context switching
//   - Integration with scheduler and process management
//
// Version: 0.10
// Author: Lee Barney
// Last Modified: 2025-01-19
//

    .text
    .align 4

// Define constants (matching scheduler.s and config.inc)
.equ MAX_CORES, 128
.equ PRIORITY_LEVELS, 4
.equ DEFAULT_REDUCTIONS, 2000
.equ PROCESS_STATE_READY, 1
.equ PROCESS_STATE_RUNNING, 2
.equ PROCESS_STATE_WAITING, 3
.equ PROCESS_STATE_TERMINATED, 5
.equ REASON_RECEIVE, 1
.equ REASON_TIMER, 2
.equ REASON_IO, 3
.equ BIF_SPAWN_COST, 10
.equ BIF_EXIT_COST, 1
.equ BIF_YIELD_COST, 1
.equ MAX_BLOCKING_TIME, 1000000

// Define structure offsets (matching scheduler.s)
.equ scheduler_current_reductions, 112
.equ scheduler_total_yields, 128
.equ scheduler_queues, 8
.equ queue_count, 16
    .equ scheduler_size, 248
.equ queue_size, 24

// Define PCB offsets (matching process.s)
.equ pcb_state, 32
.equ pcb_priority, 40
.equ pcb_next, 0
.equ pcb_prev, 8

// External function declarations (macOS linker requirements)
.extern _scheduler_get_current_process
.extern _scheduler_decrement_reductions
.extern _scheduler_enqueue_process
.extern _scheduler_schedule
.extern _process_save_context
.extern _process_restore_context

// ------------------------------------------------------------
// Yield Function Exports
// ------------------------------------------------------------
// Export the main yielding functions to make them callable from C code.
// These functions provide the primary interface for yielding and preemption
// operations, following BEAM's cooperative scheduling model.
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

    .global _process_yield_check
    .global _process_preempt
    .global _process_decrement_reductions_with_check
    .global _process_yield_with_state
    .global _process_yield_conditional_with_state

// Non-underscore versions for C compatibility
    .global process_yield_check
    .global process_preempt
    .global process_decrement_reductions_with_check
    .global process_yield_with_state
    .global process_yield_conditional_with_state

// ------------------------------------------------------------
// Process Yield Check Function
// ------------------------------------------------------------
// Check if reductions are exhausted and yield if needed.
// This is the core preemption mechanism that checks reduction count
// and triggers preemption when the time slice is exhausted.
//
// Parameters:
//   x0 (void*) - scheduler_states: Pointer to scheduler states array
//   x1 (uint64_t) - core_id: Core ID (0 to MAX_CORES-1)
//   x2 (void*) - pcb: Process Control Block pointer
//
// Returns:
//   x0 (int) - status: 0 = continued, 1 = yielded
//
// Complexity: O(1) - Constant time reduction check
//
// Version: 0.10
// Author: Lee Barney
// Last Modified: 2025-01-19
//
// Clobbers: x1, x2, x3, x4, x5, x6, x7, x8, x9, x10, x11, x12, x13, x14, x15, x16, x17, x18, x19, x20, x21, x22, x23, x24, x25, x26, x27, x28, x29, x30
//
_process_yield_check:
    // Save callee-saved registers
    stp x19, x20, [sp, #-16]!
    stp x21, x22, [sp, #-16]!
    stp x23, x24, [sp, #-16]!
    stp x25, x30, [sp, #-16]!

    // x0 = scheduler_states, x1 = core_id, x2 = pcb
    mov x19, x0  // Save scheduler_states pointer
    mov x20, x1  // Save core_id
    mov x21, x2  // Save pcb

    // Validate core ID
    cmp x20, #MAX_CORES
    b.ge yield_check_invalid_core

    // Validate PCB pointer
    cbz x21, yield_check_invalid_pcb

    // Get scheduler state
    mov x22, #scheduler_size
    mul x22, x20, x22
    add x22, x19, x22  // x22 = scheduler state address

    // Load current reduction count
    ldr x23, [x22, #scheduler_current_reductions]
    cbz x23, yield_check_exhausted

    // Reductions still available, continue
    mov x0, #0  // Return 0 = continued
    ldp x25, x30, [sp], #16
    ldp x23, x24, [sp], #16
    ldp x21, x22, [sp], #16
    ldp x19, x20, [sp], #16
    ret

yield_check_exhausted:
    // Reductions exhausted, need to preempt
    mov x0, x19  // scheduler_states pointer
    mov x1, x20  // core_id
    mov x2, x21  // pcb
    bl _process_preempt
    mov x0, #1  // Return 1 = yielded
    ldp x25, x30, [sp], #16
    ldp x23, x24, [sp], #16
    ldp x21, x22, [sp], #16
    ldp x19, x20, [sp], #16
    ret

yield_check_invalid_core:
    mov x0, #0  // Return 0 = continued (invalid core)
    ldp x25, x30, [sp], #16
    ldp x23, x24, [sp], #16
    ldp x21, x22, [sp], #16
    ldp x19, x20, [sp], #16
    ret

yield_check_invalid_pcb:
    mov x0, #0  // Return 0 = continued (invalid PCB)
    ldp x25, x30, [sp], #16
    ldp x23, x24, [sp], #16
    ldp x21, x22, [sp], #16
    ldp x19, x20, [sp], #16
    ret

// ------------------------------------------------------------
// Process Preempt Function
// ------------------------------------------------------------
// Force preemption of current process. Saves context, enqueues process,
// and schedules next process. This is the core preemption mechanism.
//
// Parameters:
//   x0 (uint64_t) - core_id: Core ID (0 to MAX_CORES-1)
//   x1 (void*) - pcb: Process Control Block pointer
//
// Returns:
//   x0 (void*) - next_process: Pointer to next process to run, or NULL
//
// Complexity: O(1) - Constant time preemption
//
// Version: 0.10
// Author: Lee Barney
// Last Modified: 2025-01-19
//
// Clobbers: x1, x2, x3, x4, x5, x6, x7, x8, x9, x10, x11, x12, x13, x14, x15, x16, x17, x18, x19, x20, x21, x22, x23, x24, x25, x26, x27, x28, x29, x30
//
_process_preempt:
    // Save callee-saved registers
    stp x19, x20, [sp, #-16]!
    stp x21, x22, [sp, #-16]!
    stp x23, x24, [sp, #-16]!
    stp x25, x26, [sp, #-16]!
    stp x27, x30, [sp, #-16]!

    // x0 = scheduler_states, x1 = core_id, x2 = pcb
    mov x19, x0  // Save scheduler_states pointer
    mov x20, x1  // Save core_id
    mov x21, x2  // Save pcb

    // Validate core ID
    cmp x20, #MAX_CORES
    b.ge preempt_invalid_core

    // Validate PCB pointer
    cbz x21, preempt_invalid_pcb

    // Get scheduler state
    mov x22, #scheduler_size
    mul x22, x20, x22
    add x22, x19, x22  // x22 = scheduler state address

    // Save process context
    mov x0, x19  // scheduler_states pointer
    mov x1, x20  // core_id
    mov x2, x21  // pcb
    bl _process_save_context

    // Set process state to READY
    mov x23, #PROCESS_STATE_READY
    str x23, [x21, #pcb_state]

    // Get process priority
    ldr x24, [x21, #pcb_priority]

    // Enqueue process to appropriate priority queue
    mov x0, x19  // scheduler_states pointer
    mov x1, x20  // core_id
    mov x2, x21  // pcb
    mov x3, x24  // priority
    bl _scheduler_enqueue_process

    // Increment scheduler yield statistics
    ldr x25, [x22, #scheduler_total_yields]
    add x25, x25, #1
    str x25, [x22, #scheduler_total_yields]

    // Schedule next process
    mov x0, x19  // scheduler_states pointer
    mov x1, x20  // core_id
    bl _scheduler_schedule
    mov x26, x0  // Save next process pointer

    // If next process available, restore its context
    cbz x26, preempt_no_next_process
    mov x0, x19  // scheduler_states pointer
    mov x1, x20  // core_id
    mov x2, x26  // pcb
    bl _process_restore_context

preempt_no_next_process:
    // Return next process pointer (or NULL)
    mov x0, x26
    ldp x27, x30, [sp], #16
    ldp x25, x26, [sp], #16
    ldp x23, x24, [sp], #16
    ldp x21, x22, [sp], #16
    ldp x19, x20, [sp], #16
    ret

preempt_invalid_core:
    mov x0, #0  // Return NULL
    ldp x27, x30, [sp], #16
    ldp x25, x26, [sp], #16
    ldp x23, x24, [sp], #16
    ldp x21, x22, [sp], #16
    ldp x19, x20, [sp], #16
    ret

preempt_invalid_pcb:
    mov x0, #0  // Return NULL
    ldp x27, x30, [sp], #16
    ldp x25, x26, [sp], #16
    ldp x23, x24, [sp], #16
    ldp x21, x22, [sp], #16
    ldp x19, x20, [sp], #16
    ret

// ------------------------------------------------------------
// Process Decrement Reductions with Check
// ------------------------------------------------------------
// Decrement reduction count and check if preemption is needed.
// This combines reduction decrement with preemption check in one operation.
//
// Parameters:
//   x0 (uint64_t) - core_id: Core ID (0 to MAX_CORES-1)
//
// Returns:
//   x0 (int) - status: 0 = continued, 1 = yielded
//
// Complexity: O(1) - Constant time operation
//
// Version: 0.10
// Author: Lee Barney
// Last Modified: 2025-01-19
//
// Clobbers: x1, x2, x3, x4, x5, x6, x7, x8, x9, x10, x11, x12, x13, x14, x15, x16, x17, x18, x19, x20, x21, x22, x23, x24, x25, x26, x27, x28, x29, x30
//
_process_decrement_reductions_with_check:
    // Save callee-saved registers
    stp x19, x20, [sp, #-16]!
    stp x21, x22, [sp, #-16]!
    stp x23, x24, [sp, #-16]!
    stp x25, x30, [sp, #-16]!

    // x0 = core_id
    mov x19, x0  // Save core_id

    // Decrement reductions
    bl _scheduler_decrement_reductions

    // Get current process from scheduler
    mov x0, x19  // core_id
    bl _scheduler_get_current_process
    mov x20, x0  // Save current process

    // If no current process, return continued
    cbz x20, decrement_check_continued

    // Check if reduction count is 0
    // x19 already contains scheduler_states pointer
    mov x22, #scheduler_size
    mul x22, x20, x22
    add x21, x19, x22  // x21 = scheduler state address

    ldr x23, [x21, #scheduler_current_reductions]
    cbz x23, decrement_check_preempt

    // Reductions still available, continue
    mov x0, #0  // Return 0 = continued
    ldp x25, x30, [sp], #16
    ldp x23, x24, [sp], #16
    ldp x21, x22, [sp], #16
    ldp x19, x20, [sp], #16
    ret

decrement_check_preempt:
    // Reductions exhausted, preempt
    mov x0, x19  // core_id
    mov x1, x20  // pcb
    bl _process_preempt
    mov x0, #1  // Return 1 = yielded
    ldp x25, x30, [sp], #16
    ldp x23, x24, [sp], #16
    ldp x21, x22, [sp], #16
    ldp x19, x20, [sp], #16
    ret

decrement_check_continued:
    mov x0, #0  // Return 0 = continued
    ldp x25, x30, [sp], #16
    ldp x23, x24, [sp], #16
    ldp x21, x22, [sp], #16
    ldp x19, x20, [sp], #16
    ret

// ------------------------------------------------------------
// Process Yield Function
// ------------------------------------------------------------
// Explicit voluntary yield. Always yields regardless of reduction count.
// This implements BEAM's erlang:yield/0 behavior.
//
// Parameters:
//   x0 (uint64_t) - core_id: Core ID (0 to MAX_CORES-1)
//   x1 (void*) - pcb: Process Control Block pointer
//
// Returns:
//   x0 (void*) - next_process: Pointer to next process to run, or NULL
//
// Complexity: O(1) - Constant time voluntary yield
//
// Version: 0.10
// Author: Lee Barney
// Last Modified: 2025-01-19
//
// Clobbers: x1, x2, x3, x4, x5, x6, x7, x8, x9, x10, x11, x12, x13, x14, x15, x16, x17, x18, x19, x20, x21, x22, x23, x24, x25, x26, x27, x28, x29, x30
//
// REMOVED: Old API _process_yield function - replaced with _process_yield_with_state

// ------------------------------------------------------------
// Process Yield Conditional Function
// ------------------------------------------------------------
// Yield only if other processes are waiting. This implements BEAM's
// conditional yield behavior for cooperative scheduling.
//
// Parameters:
//   x0 (uint64_t) - core_id: Core ID (0 to MAX_CORES-1)
//   x1 (void*) - pcb: Process Control Block pointer
//
// Returns:
//   x0 (int) - status: 0 = no yield, 1 = yielded
//
// Complexity: O(p) where p is number of priority levels (4)
//
// Version: 0.10
// Author: Lee Barney
// Last Modified: 2025-01-19
//
// Clobbers: x1, x2, x3, x4, x5, x6, x7, x8, x9, x10, x11, x12, x13, x14, x15, x16, x17, x18, x19, x20, x21, x22, x23, x24, x25, x26, x27, x28, x29, x30
//
_process_yield_conditional_with_state:
    // Save callee-saved registers
    stp x19, x20, [sp, #-16]!
    stp x21, x22, [sp, #-16]!
    stp x23, x24, [sp, #-16]!
    stp x25, x26, [sp, #-16]!
    stp x27, x30, [sp, #-16]!

    // x0 = scheduler_states, x1 = core_id, x2 = pcb
    mov x19, x0  // Save scheduler_states pointer
    mov x20, x1  // Save core_id
    mov x21, x2  // Save pcb

    // Validate core ID
    cmp x20, #MAX_CORES
    b.ge yield_conditional_invalid_core

    // Validate PCB pointer
    cbz x21, yield_conditional_invalid_pcb

    // Get scheduler state
    // x19 already contains scheduler_states pointer
    mov x22, #scheduler_size
    mul x22, x20, x22
    add x23, x19, x22  // x23 = scheduler state address (keep PCB in x21)

    // Check if any processes in ready queues
    mov x24, #0  // Priority level
    mov x25, #PRIORITY_LEVELS  // Number of priority levels
    add x26, x23, #scheduler_queues  // Queue array base (use scheduler state x23)

yield_conditional_check_loop:
    // Calculate queue address for this priority
    add x27, x26, #0  // Base queue address
    mov x28, #queue_size
    mul x28, x24, x28
    add x27, x27, x28  // x27 = queue address

    // Check if queue is empty
    ldr w28, [x27, #queue_count]
    cbnz w28, yield_conditional_has_processes

    // Move to next priority level
    add x24, x24, #1
    cmp x24, x25
    b.lt yield_conditional_check_loop

    // No processes in any queue, don't yield
    mov x0, #0  // Return 0 = no yield
    ldp x27, x30, [sp], #16
    ldp x25, x26, [sp], #16
    ldp x23, x24, [sp], #16
    ldp x21, x22, [sp], #16
    ldp x19, x20, [sp], #16
    ret

yield_conditional_has_processes:
    // Other processes are waiting, yield
    mov x0, x19  // core_id
    mov x1, x20  // pcb
    bl _process_yield_with_state
    mov x0, #1  // Return 1 = yielded
    ldp x27, x30, [sp], #16
    ldp x25, x26, [sp], #16
    ldp x23, x24, [sp], #16
    ldp x21, x22, [sp], #16
    ldp x19, x20, [sp], #16
    ret

yield_conditional_invalid_core:
    mov x0, #0  // Return 0 = no yield
    ldp x27, x30, [sp], #16
    ldp x25, x26, [sp], #16
    ldp x23, x24, [sp], #16
    ldp x21, x22, [sp], #16
    ldp x19, x20, [sp], #16
    ret

yield_conditional_invalid_pcb:
    mov x0, #0  // Return 0 = no yield
    ldp x27, x30, [sp], #16
    ldp x25, x26, [sp], #16
    ldp x23, x24, [sp], #16
    ldp x21, x22, [sp], #16
    ldp x19, x20, [sp], #16
    ret

// ------------------------------------------------------------
// Function Aliases for C Compatibility
// ------------------------------------------------------------
// Create aliases for C code that expects non-underscore function names
// These aliases simply jump to the underscore versions

process_yield_check:
    b _process_yield_check

process_preempt:
    b _process_preempt

process_decrement_reductions_with_check:
    b _process_decrement_reductions_with_check

process_yield_with_state:
    b _process_yield_with_state

_process_yield_with_state:
    // BINARY SEARCH: Comment out first half of function
    // Save callee-saved registers
    stp x19, x20, [sp, #-16]!
    stp x21, x22, [sp, #-16]!
    stp x23, x24, [sp, #-16]!
    stp x25, x26, [sp, #-16]!
    stp x27, x30, [sp, #-16]!

    // x0 = scheduler_states, x1 = core_id, x2 = pcb
    mov x19, x0  // Save scheduler_states pointer
    mov x20, x1  // Save core_id
    mov x21, x2  // Save pcb

    // Validate core ID
    cmp x20, #MAX_CORES
    b.ge yield_with_state_invalid_core

    // Validate PCB pointer
    cbz x21, yield_with_state_invalid_pcb

    // Get scheduler state
    // x19 already contains scheduler_states pointer
    mov x22, #scheduler_size
    mul x22, x20, x22
    add x23, x19, x22  // x23 = scheduler state address (keep PCB in x21)

    // BINARY SEARCH DEBUG: Testing first part of second half
    // Save process context
    mov x0, x21  // pcb (use original PCB pointer)
    bl _process_save_context

    // Set process state to READY
    mov x24, #PROCESS_STATE_READY
    str x24, [x21, #pcb_state]  // Use PCB pointer for PCB fields

    // Reset reduction counter to default
    mov x25, #DEFAULT_REDUCTIONS
    str x25, [x23, #scheduler_current_reductions]  // Use scheduler state for scheduler fields

    // Get process priority
    ldr x26, [x21, #pcb_priority]  // Use PCB pointer for PCB fields

    // BINARY SEARCH DEBUG: Testing enqueue operation only
    // Enqueue process to back of its priority queue
    mov x0, x19  // scheduler_states
    mov x1, x20  // core_id
    mov x2, x21  // pcb (use original PCB pointer)
    mov x3, x26  // priority
    bl _scheduler_enqueue_process

    // BINARY SEARCH DEBUG: Testing schedule operation
    // Increment scheduler yield statistics
    ldr x26, [x23, #scheduler_total_yields]  // Use scheduler state for scheduler fields
    add x26, x26, #1
    str x26, [x23, #scheduler_total_yields]  // Use scheduler state for scheduler fields

    // Schedule next process
    mov x0, x19  // scheduler_states
    mov x1, x20  // core_id
    bl _scheduler_schedule
    mov x27, x0  // Save next process pointer

    // BINARY SEARCH DEBUG: Testing context restore operation
    // If next process available, restore its context
    cbz x27, yield_with_state_no_next_process
    mov x0, x27  // pcb
    bl _process_restore_context

    // Return the next process pointer
    mov x0, x27
    ldp x27, x30, [sp], #16
    ldp x25, x26, [sp], #16
    ldp x23, x24, [sp], #16
    ldp x21, x22, [sp], #16
    ldp x19, x20, [sp], #16
    ret
    
    // BINARY SEARCH DEBUG: Return early to test first half
    mov x0, #0  // Return NULL for now
    ldp x27, x30, [sp], #16
    ldp x25, x26, [sp], #16
    ldp x23, x24, [sp], #16
    ldp x21, x22, [sp], #16
    ldp x19, x20, [sp], #16
    ret

yield_with_state_no_next_process:
    // No next process available, return NULL
    mov x0, #0
    ldp x27, x30, [sp], #16
    ldp x25, x26, [sp], #16
    ldp x23, x24, [sp], #16
    ldp x21, x22, [sp], #16
    ldp x19, x20, [sp], #16
    ret

yield_with_state_invalid_core:
    // Return NULL for invalid core
    mov x0, #0
    ldp x27, x30, [sp], #16
    ldp x25, x26, [sp], #16
    ldp x23, x24, [sp], #16
    ldp x21, x22, [sp], #16
    ldp x19, x20, [sp], #16
    ret

yield_with_state_invalid_pcb:
    // Return NULL for invalid PCB
    mov x0, #0
    ldp x27, x30, [sp], #16
    ldp x25, x26, [sp], #16
    ldp x23, x24, [sp], #16
    ldp x21, x22, [sp], #16
    ldp x19, x20, [sp], #16
    ret

process_yield_conditional_with_state:
    b _process_yield_conditional_with_state

