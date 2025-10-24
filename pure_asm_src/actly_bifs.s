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
// actly_bifs.s — Actly BIF Functions Implementation
// ------------------------------------------------------------
// Core Actly BIF functions (yield, spawn, exit) implemented in pure
// ARM64 assembly following BEAM BIF patterns. These functions provide
// high-level process control operations with reduction counting.
//
// The file provides:
//   - actly_yield: Yield current process (erlang:yield/0 equivalent)
//   - actly_spawn: Spawn new process (erlang:spawn/1 equivalent)
//   - actly_exit: Terminate current process (erlang:exit/1 equivalent)
//   - BIF trap mechanism for preemption checking
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
.equ pcb_blocking_data, 440
.equ pcb_message_queue, 368
.equ pcb_stack_base, 336
.equ pcb_heap_base, 352

// Define size constants
.equ MAX_STACK_SIZE, 65536
.equ DEFAULT_STACK_SIZE, 8192
.equ MAX_HEAP_SIZE, 1048576
.equ DEFAULT_HEAP_SIZE, 4096

// External function declarations (macOS linker requirements)
.extern _scheduler_get_current_process
.extern _scheduler_decrement_reductions
.extern _scheduler_enqueue_process
.extern _scheduler_schedule
.extern _process_save_context
.extern _process_restore_context
.extern _process_create
.extern _process_preempt

// ------------------------------------------------------------
// Actly BIF Function Exports
// ------------------------------------------------------------
// Export the main Actly BIF functions to make them callable from C code.
// These functions provide the primary interface for high-level process
// control operations following BEAM's BIF patterns.
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

    .global _actly_yield
    .global _actly_spawn
    .global _actly_exit
    .global _actly_bif_trap_check

// ------------------------------------------------------------
// Actly Yield BIF Function
// ------------------------------------------------------------
// Yield current process unconditionally. This implements BEAM's
// erlang:yield/0 behavior with reduction counting.
//
// Parameters:
//   x0 (void*) - scheduler_states: Pointer to scheduler states array
//   x1 (uint64_t) - core_id: Core ID (0 to MAX_CORES-1)
//
// Returns:
//   x0 (int) - success: 1 on success, 0 on failure
//
// Complexity: O(1) - Constant time yield operation
//
// Version: 0.10
// Author: Lee Barney
// Last Modified: 2025-01-19
//
// Clobbers: x1, x2, x3, x4, x5, x6, x7, x8, x9, x10, x11, x12, x13, x14, x15, x16, x17, x18, x19, x20, x21, x22, x23, x24, x25, x26, x27, x28, x29, x30
//
_actly_yield:
    // Save callee-saved registers
    stp x19, x20, [sp, #-16]!
    stp x21, x22, [sp, #-16]!
    stp x23, x24, [sp, #-16]!
    stp x25, x30, [sp], #16

    // x0 = core_id
    mov x19, x0  // Save core_id

    // Validate core ID
    cmp x19, #MAX_CORES
    b.ge actly_yield_invalid_core

    // Get current process from scheduler
    mov x0, x19  // core_id
    bl _scheduler_get_current_process
    mov x20, x0  // Save current process

    // If no current process, return failure
    cbz x20, actly_yield_no_process

    // Decrement reduction count (yield costs 1 reduction)
    mov x0, x19  // core_id
    bl _scheduler_decrement_reductions

    // Get current process again (in case it changed)
    mov x0, x19  // core_id
    bl _scheduler_get_current_process
    mov x20, x0  // Save current process

    // Yield the process unconditionally
    mov x0, x19  // core_id
    mov x1, x20  // pcb
    bl _process_yield_with_state

    mov x0, #1  // Return 1 = success
    ldp x25, x30, [sp], #16
    ldp x23, x24, [sp], #16
    ldp x21, x22, [sp], #16
    ldp x19, x20, [sp], #16
    ret

actly_yield_invalid_core:
    mov x0, #0  // Return 0 = failure
    ldp x25, x30, [sp], #16
    ldp x23, x24, [sp], #16
    ldp x21, x22, [sp], #16
    ldp x19, x20, [sp], #16
    ret

actly_yield_no_process:
    mov x0, #0  // Return 0 = failure
    ldp x25, x30, [sp], #16
    ldp x23, x24, [sp], #16
    ldp x21, x22, [sp], #16
    ldp x19, x20, [sp], #16
    ret

// ------------------------------------------------------------
// Actly Spawn BIF Function
// ------------------------------------------------------------
// Spawn new process with specified parameters. This implements BEAM's
// erlang:spawn/1 behavior with reduction counting.
//
// Parameters:
//   x0 (uint64_t) - core_id: Core ID (0 to MAX_CORES-1)
//   x1 (uint64_t) - entry_point: Process entry point address
//   x2 (uint64_t) - priority: Process priority level
//   x3 (uint64_t) - stack_size: Stack size in bytes
//   x4 (uint64_t) - heap_size: Heap size in bytes
//
// Returns:
//   x0 (uint64_t) - pid: New process PID on success, 0 on failure
//
// Complexity: O(1) - Constant time spawn operation
//
// Version: 0.10
// Author: Lee Barney
// Last Modified: 2025-01-19
//
// Clobbers: x1, x2, x3, x4, x5, x6, x7, x8, x9, x10, x11, x12, x13, x14, x15, x16, x17, x18, x19, x20, x21, x22, x23, x24, x25, x26, x27, x28, x29, x30
//
_actly_spawn:
    // Save callee-saved registers
    stp x19, x20, [sp, #-16]!
    stp x21, x22, [sp, #-16]!
    stp x23, x24, [sp, #-16]!
    stp x25, x26, [sp, #-16]!
    stp x27, x30, [sp], #16

    // x0 = core_id, x1 = entry_point, x2 = priority, x3 = stack_size, x4 = heap_size
    mov x19, x0  // Save core_id
    mov x20, x1  // Save entry_point
    mov x21, x2  // Save priority
    mov x22, x3  // Save stack_size
    mov x23, x4  // Save heap_size

    // Validate core ID
    cmp x19, #MAX_CORES
    b.ge actly_spawn_invalid_core

    // Validate priority
    cmp x21, #PRIORITY_LEVELS
    b.ge actly_spawn_invalid_priority

    // Validate stack size
    cmp x22, #MAX_STACK_SIZE
    b.gt actly_spawn_invalid_stack_size
    cmp x22, #DEFAULT_STACK_SIZE
    b.lt actly_spawn_invalid_stack_size

    // Validate heap size
    cmp x23, #MAX_HEAP_SIZE
    b.gt actly_spawn_invalid_heap_size
    cmp x23, #DEFAULT_HEAP_SIZE
    b.lt actly_spawn_invalid_heap_size

    // Decrement reduction count (spawn costs 10 reductions)
    mov x0, x19  // core_id
    mov x1, #BIF_SPAWN_COST  // reduction cost
    bl _actly_bif_trap_check
    cbz x0, actly_spawn_preempted  // If preempted, return 0

    // Create new process
    mov x0, x20  // entry_point
    mov x1, x21  // priority
    mov x2, x22  // stack_size
    mov x3, x23  // heap_size
    bl _process_create
    mov x24, x0  // Save new process PID

    // If creation failed, return 0
    cbz x24, actly_spawn_creation_failed

    // Enqueue new process to ready queue
    mov x0, x19  // core_id
    mov x1, x24  // pcb (PID is also PCB pointer in our implementation)
    mov x2, x21  // priority
    bl _scheduler_enqueue_process

    // Return new process PID
    mov x0, x24
    ldp x27, x30, [sp], #16
    ldp x25, x26, [sp], #16
    ldp x23, x24, [sp], #16
    ldp x21, x22, [sp], #16
    ldp x19, x20, [sp], #16
    ret

actly_spawn_invalid_core:
    mov x0, #0  // Return 0 = failure
    ldp x27, x30, [sp], #16
    ldp x25, x26, [sp], #16
    ldp x23, x24, [sp], #16
    ldp x21, x22, [sp], #16
    ldp x19, x20, [sp], #16
    ret

actly_spawn_invalid_priority:
    mov x0, #0  // Return 0 = failure
    ldp x27, x30, [sp], #16
    ldp x25, x26, [sp], #16
    ldp x23, x24, [sp], #16
    ldp x21, x22, [sp], #16
    ldp x19, x20, [sp], #16
    ret

actly_spawn_invalid_stack_size:
    mov x0, #0  // Return 0 = failure
    ldp x27, x30, [sp], #16
    ldp x25, x26, [sp], #16
    ldp x23, x24, [sp], #16
    ldp x21, x22, [sp], #16
    ldp x19, x20, [sp], #16
    ret

actly_spawn_invalid_heap_size:
    mov x0, #0  // Return 0 = failure
    ldp x27, x30, [sp], #16
    ldp x25, x26, [sp], #16
    ldp x23, x24, [sp], #16
    ldp x21, x22, [sp], #16
    ldp x19, x20, [sp], #16
    ret

actly_spawn_preempted:
    mov x0, #0  // Return 0 = failure (preempted)
    ldp x27, x30, [sp], #16
    ldp x25, x26, [sp], #16
    ldp x23, x24, [sp], #16
    ldp x21, x22, [sp], #16
    ldp x19, x20, [sp], #16
    ret

actly_spawn_creation_failed:
    mov x0, #0  // Return 0 = failure
    ldp x27, x30, [sp], #16
    ldp x25, x26, [sp], #16
    ldp x23, x24, [sp], #16
    ldp x21, x22, [sp], #16
    ldp x19, x20, [sp], #16
    ret

// ------------------------------------------------------------
// Actly Exit BIF Function
// ------------------------------------------------------------
// Terminate current process with exit reason. This implements BEAM's
// erlang:exit/1 behavior with process cleanup.
//
// Parameters:
//   x0 (uint64_t) - core_id: Core ID (0 to MAX_CORES-1)
//   x1 (uint64_t) - exit_reason: Exit reason code
//
// Returns:
//   x0 (void) - Never returns (process terminates)
//
// Complexity: O(1) - Constant time exit operation
//
// Version: 0.10
// Author: Lee Barney
// Last Modified: 2025-01-19
//
// Clobbers: x1, x2, x3, x4, x5, x6, x7, x8, x9, x10, x11, x12, x13, x14, x15, x16, x17, x18, x19, x20, x21, x22, x23, x24, x25, x26, x27, x28, x29, x30
//
_actly_exit:
    // Save callee-saved registers
    stp x19, x20, [sp, #-16]!
    stp x21, x22, [sp, #-16]!
    stp x23, x24, [sp, #-16]!
    stp x25, x26, [sp, #-16]!
    stp x27, x30, [sp], #16

    // x0 = core_id, x1 = exit_reason
    mov x19, x0  // Save core_id
    mov x20, x1  // Save exit_reason

    // Validate core ID
    cmp x19, #MAX_CORES
    b.ge actly_exit_invalid_core

    // Get current process from scheduler
    mov x0, x19  // core_id
    bl _scheduler_get_current_process
    mov x21, x0  // Save current process

    // If no current process, return failure
    cbz x21, actly_exit_no_process

    // Decrement reduction count (exit costs 1 reduction)
    mov x0, x19  // core_id
    mov x1, #BIF_EXIT_COST  // reduction cost
    bl _actly_bif_trap_check
    cbz x0, actly_exit_preempted  // If preempted, return 0

    // Save exit reason in PCB
    str x20, [x21, #pcb_blocking_data]  // Use blocking_data for exit reason

    // Set process state to TERMINATED
    mov x22, #PROCESS_STATE_TERMINATED
    str x22, [x21, #pcb_state]

    // Cleanup process resources
    // Free message queue (if any)
    ldr x23, [x21, #pcb_message_queue]
    cbz x23, actly_exit_no_message_queue
    // Note: In a real system, this would free the message queue
    // For now, just clear the pointer
    str xzr, [x21, #pcb_message_queue]

actly_exit_no_message_queue:
    // Mark stack/heap for GC (in a real system)
    // For now, just clear the pointers
    str xzr, [x21, #pcb_stack_base]
    str xzr, [x21, #pcb_heap_base]

    // Do NOT enqueue process (it's terminated)
    // Schedule next process immediately
    mov x0, x19  // core_id
    bl _scheduler_schedule
    mov x24, x0  // Save next process pointer

    // If next process available, restore its context
    cbz x24, actly_exit_no_next_process
    mov x0, x24  // pcb
    bl _process_restore_context

    // Never return (context switch away)
    // This is the expected behavior for exit
    b actly_exit_done

actly_exit_no_next_process:
    // No next process, go to idle
    // In a real system, this would go to scheduler idle loop
    b actly_exit_done

actly_exit_done:
    // This should never be reached in normal operation
    // The process should have context switched away
    ldp x27, x30, [sp], #16
    ldp x25, x26, [sp], #16
    ldp x23, x24, [sp], #16
    ldp x21, x22, [sp], #16
    ldp x19, x20, [sp], #16
    ret

actly_exit_invalid_core:
    mov x0, #0  // Return 0 = failure
    ldp x27, x30, [sp], #16
    ldp x25, x26, [sp], #16
    ldp x23, x24, [sp], #16
    ldp x21, x22, [sp], #16
    ldp x19, x20, [sp], #16
    ret

actly_exit_no_process:
    mov x0, #0  // Return 0 = failure
    ldp x27, x30, [sp], #16
    ldp x25, x26, [sp], #16
    ldp x23, x24, [sp], #16
    ldp x21, x22, [sp], #16
    ldp x19, x20, [sp], #16
    ret

actly_exit_preempted:
    mov x0, #0  // Return 0 = failure (preempted)
    ldp x27, x30, [sp], #16
    ldp x25, x26, [sp], #16
    ldp x23, x24, [sp], #16
    ldp x21, x22, [sp], #16
    ldp x19, x20, [sp], #16
    ret

// ------------------------------------------------------------
// Actly BIF Trap Check Function
// ------------------------------------------------------------
// BIF trap mechanism helper that decrements reductions and checks for preemption.
// This implements BEAM's BIF trap mechanism for reduction-based preemption.
//
// Parameters:
//   x0 (uint64_t) - core_id: Core ID (0 to MAX_CORES-1)
//   x1 (uint64_t) - reduction_cost: Number of reductions to decrement
//
// Returns:
//   x0 (int) - status: 0 = preempted, 1 = continued
//
// Complexity: O(1) - Constant time trap check
//
// Version: 0.10
// Author: Lee Barney
// Last Modified: 2025-01-19
//
// Clobbers: x1, x2, x3, x4, x5, x6, x7, x8, x9, x10, x11, x12, x13, x14, x15, x16, x17, x18, x19, x20, x21, x22, x23, x24, x25, x26, x27, x28, x29, x30
//
_actly_bif_trap_check:
    // Save callee-saved registers
    stp x19, x20, [sp, #-16]!
    stp x21, x22, [sp, #-16]!
    stp x23, x24, [sp, #-16]!
    stp x25, x30, [sp], #16

    // x0 = scheduler_states, x1 = core_id, x2 = reduction_cost
    mov x19, x0  // Save scheduler_states pointer
    mov x20, x1  // Save core_id
    mov x21, x2  // Save reduction_cost

    // Validate core ID
    cmp x20, #MAX_CORES
    b.ge bif_trap_invalid_core

    // Get scheduler state
    mov x22, #scheduler_size
    mul x22, x20, x22
    add x22, x19, x22  // x22 = scheduler state address

    // Get current reduction count
    ldr x23, [x22, #scheduler_current_reductions]

    // Check if we have enough reductions
    cmp x23, x21
    b.lt bif_trap_insufficient_reductions

    // Decrement reductions by cost
    sub x23, x23, x21
    str x23, [x22, #scheduler_current_reductions]

    // Check if reductions exhausted
    cbz x23, bif_trap_preempt

    // Reductions still available, continue
    mov x0, #1  // Return 1 = continued
    ldp x25, x30, [sp], #16
    ldp x23, x24, [sp], #16
    ldp x21, x22, [sp], #16
    ldp x19, x20, [sp], #16
    ret

bif_trap_preempt:
    // Reductions exhausted, need to preempt
    // Get current process
    mov x0, x19  // core_id
    bl _scheduler_get_current_process
    mov x24, x0  // Save current process

    // If no current process, return continued
    cbz x24, bif_trap_no_process

    // Preempt the process
    mov x0, x19  // core_id
    mov x1, x24  // pcb
    bl _process_preempt

    mov x0, #0  // Return 0 = preempted
    ldp x25, x30, [sp], #16
    ldp x23, x24, [sp], #16
    ldp x21, x22, [sp], #16
    ldp x19, x20, [sp], #16
    ret

bif_trap_insufficient_reductions:
    // Not enough reductions, preempt immediately
    mov x0, x19  // core_id
    bl _scheduler_get_current_process
    mov x24, x0  // Save current process

    // If no current process, return continued
    cbz x24, bif_trap_no_process

    // Preempt the process
    mov x0, x19  // core_id
    mov x1, x24  // pcb
    bl _process_preempt

    mov x0, #0  // Return 0 = preempted
    ldp x25, x30, [sp], #16
    ldp x23, x24, [sp], #16
    ldp x21, x22, [sp], #16
    ldp x19, x20, [sp], #16
    ret

bif_trap_no_process:
    mov x0, #1  // Return 1 = continued
    ldp x25, x30, [sp], #16
    ldp x23, x24, [sp], #16
    ldp x21, x22, [sp], #16
    ldp x19, x20, [sp], #16
    ret

bif_trap_invalid_core:
    mov x0, #1  // Return 1 = continued (invalid core)
    ldp x25, x30, [sp], #16
    ldp x23, x24, [sp], #16
    ldp x21, x22, [sp], #16
    ldp x19, x20, [sp], #16
    ret

// ------------------------------------------------------------
// Function Aliases for C Compatibility
// ------------------------------------------------------------
// Create aliases for C code that expects non-underscore function names

actly_yield:
    b _actly_yield

actly_spawn:
    b _actly_spawn

actly_exit:
    b _actly_exit

actly_bif_trap_check:
    b _actly_bif_trap_check
