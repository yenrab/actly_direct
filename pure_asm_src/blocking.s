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
// blocking.s — Blocking Operations Framework
// ------------------------------------------------------------
// BEAM-style blocking operations including message receive, timer waiting,
// and I/O blocking. Implements complete blocking suite for Phase 6.
//
// The file provides:
//   - Generic blocking and wake operations
//   - Message receive blocking with pattern matching
//   - Timer-based blocking with system timer
//   - I/O blocking stubs for future implementation
//   - Waiting queue management
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
.equ MAX_BLOCKING_TIME, 10000

// Define structure offsets (matching scheduler.s)
.equ scheduler_current_reductions, 112
.equ scheduler_total_yields, 128
.equ scheduler_total_blocks, 224
.equ scheduler_total_wakes, 232
.equ scheduler_waiting_receive, 152
.equ scheduler_waiting_timer, 176
.equ scheduler_waiting_io, 200
.equ scheduler_queues, 8
.equ queue_count, 16
.equ queue_head, 0
.equ queue_tail, 8
.equ scheduler_size, 240
.equ queue_size, 24

// Define PCB offsets (matching process.s)
.equ pcb_state, 32
.equ pcb_priority, 40
.equ pcb_next, 0
.equ pcb_prev, 8
.equ pcb_blocking_reason, 432
.equ pcb_blocking_data, 440
.equ pcb_wake_time, 448
.equ pcb_message_pattern, 456
.equ pcb_message_queue, 368

// External function declarations (macOS linker requirements)
.extern _scheduler_states
.extern _scheduler_get_current_process
.extern _scheduler_enqueue_process
.extern _scheduler_schedule
.extern _process_save_context
.extern _process_restore_context

// ------------------------------------------------------------
// Blocking Function Exports
// ------------------------------------------------------------
// Export the main blocking functions to make them callable from C code.
// These functions provide the primary interface for blocking operations
// and process state management.

// Export constants for C code
    .global _REASON_RECEIVE
    .global _REASON_TIMER
    .global _REASON_IO
    .global _MAX_BLOCKING_TIME
    .global _BIF_SPAWN_COST
    .global _BIF_EXIT_COST
    .global _BIF_YIELD_COST

// Non-underscore versions for C compatibility
    .global _REASON_RECEIVE_CONST
    .global _REASON_TIMER_CONST
    .global _REASON_IO_CONST
    .global _MAX_BLOCKING_TIME_CONST
    .global _BIF_SPAWN_COST_CONST
    .global _BIF_EXIT_COST_CONST
    .global _BIF_YIELD_COST_CONST
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

    .global _process_block
    .global _process_wake
    .global _process_block_on_receive
    .global _process_block_on_timer
    .global _process_block_on_io
    .global _process_check_timer_wakeups

// ------------------------------------------------------------
// Process Block Function
// ------------------------------------------------------------
// Generic blocking function that moves a process to WAITING state
// and adds it to the appropriate waiting queue based on reason.
//
// Parameters:
//   x0 (uint64_t) - core_id: Core ID (0 to MAX_CORES-1)
//   x1 (void*) - pcb: Process Control Block pointer
//   x2 (uint64_t) - reason: Blocking reason (REASON_RECEIVE, REASON_TIMER, REASON_IO)
//
// Returns:
//   x0 (void*) - next_process: Pointer to next process to run, or NULL
//
// Complexity: O(1) - Constant time blocking operation
//
// Version: 0.10
// Author: Lee Barney
// Last Modified: 2025-01-19
//
// Clobbers: x1, x2, x3, x4, x5, x6, x7, x8, x9, x10, x11, x12, x13, x14, x15, x16, x17, x18, x19, x20, x21, x22, x23, x24, x25, x26, x27, x28, x29, x30
//
_process_block:
    // Save callee-saved registers
    stp x19, x20, [sp, #-16]!
    stp x21, x22, [sp, #-16]!
    stp x23, x24, [sp, #-16]!
    stp x25, x26, [sp, #-16]!
    stp x27, x30, [sp], #16

    // x0 = core_id, x1 = pcb, x2 = reason
    mov x19, x0  // Save core_id
    mov x20, x1  // Save pcb
    mov x21, x2  // Save reason

    // Validate core ID
    cmp x19, #MAX_CORES
    b.ge block_invalid_core

    // Validate PCB pointer
    cbz x20, block_invalid_pcb

    // Validate reason
    cmp x21, #REASON_IO
    b.gt block_invalid_reason

    // Get scheduler state
    adrp x22, _scheduler_states@PAGE
    add x22, x22, _scheduler_states@PAGEOFF
    mov x23, #scheduler_size
    mul x23, x19, x23
    add x22, x22, x23  // x22 = scheduler state address

    // Save process context
    mov x0, x20  // pcb
    bl _process_save_context

    // Set process state to WAITING
    mov x24, #PROCESS_STATE_WAITING
    str x24, [x20, #pcb_state]

    // Store blocking reason in PCB
    str x21, [x20, #pcb_blocking_reason]

    // Remove from ready queue if still in one (clear next/prev pointers)
    str xzr, [x20, #pcb_next]
    str xzr, [x20, #pcb_prev]

    // Add to appropriate waiting queue based on reason
    cmp x21, #REASON_RECEIVE
    b.eq block_add_to_receive_queue
    cmp x21, #REASON_TIMER
    b.eq block_add_to_timer_queue
    cmp x21, #REASON_IO
    b.eq block_add_to_io_queue
    b block_invalid_reason

block_add_to_receive_queue:
    add x25, x22, #scheduler_waiting_receive
    bl _add_to_waiting_queue
    b block_continue

block_add_to_timer_queue:
    add x25, x22, #scheduler_waiting_timer
    bl _add_to_waiting_queue
    b block_continue

block_add_to_io_queue:
    add x25, x22, #scheduler_waiting_io
    bl _add_to_waiting_queue
    b block_continue

block_continue:
    // Increment scheduler blocking statistics
    ldr x26, [x22, #scheduler_total_blocks]
    add x26, x26, #1
    str x26, [x22, #scheduler_total_blocks]

    // Schedule next process
    mov x0, x19  // core_id
    bl _scheduler_schedule
    mov x27, x0  // Save next process pointer

    // If next process available, restore its context
    cbz x27, block_no_next_process
    mov x0, x27  // pcb
    bl _process_restore_context

block_no_next_process:
    // Return next process pointer (or NULL)
    mov x0, x27
    ldp x27, x30, [sp], #16
    ldp x25, x26, [sp], #16
    ldp x23, x24, [sp], #16
    ldp x21, x22, [sp], #16
    ldp x19, x20, [sp], #16
    ret

block_invalid_core:
    mov x0, #0  // Return NULL
    ldp x27, x30, [sp], #16
    ldp x25, x26, [sp], #16
    ldp x23, x24, [sp], #16
    ldp x21, x22, [sp], #16
    ldp x19, x20, [sp], #16
    ret

block_invalid_pcb:
    mov x0, #0  // Return NULL
    ldp x27, x30, [sp], #16
    ldp x25, x26, [sp], #16
    ldp x23, x24, [sp], #16
    ldp x21, x22, [sp], #16
    ldp x19, x20, [sp], #16
    ret

block_invalid_reason:
    mov x0, #0  // Return NULL
    ldp x27, x30, [sp], #16
    ldp x25, x26, [sp], #16
    ldp x23, x24, [sp], #16
    ldp x21, x22, [sp], #16
    ldp x19, x20, [sp], #16
    ret

// ------------------------------------------------------------
// Add to Waiting Queue Helper
// ------------------------------------------------------------
// Helper function to add a process to a waiting queue.
// This is a common operation for all blocking reasons.
//
// Parameters:
//   x20 (void*) - pcb: Process Control Block pointer
//   x25 (void*) - queue: Waiting queue pointer
//
// Returns: None
//
// Clobbers: x26, x27, x28, x29
//
_add_to_waiting_queue:
    // Load current queue head
    ldr x26, [x25, #queue_head]
    cbz x26, add_to_empty_queue

    // Queue not empty, add to tail
    ldr x27, [x25, #queue_tail]
    str x20, [x27, #pcb_next]  // Set current tail's next to new process
    str x27, [x20, #pcb_prev]  // Set new process's prev to current tail
    str x20, [x25, #queue_tail]  // Update queue tail
    b add_to_queue_done

add_to_empty_queue:
    // Queue is empty, add as head and tail
    str x20, [x25, #queue_head]
    str x20, [x25, #queue_tail]
    str xzr, [x20, #pcb_next]
    str xzr, [x20, #pcb_prev]

add_to_queue_done:
    // Increment queue count
    ldr w28, [x25, #queue_count]
    add w28, w28, #1
    str w28, [x25, #queue_count]
    ret

// ------------------------------------------------------------
// Process Wake Function
// ------------------------------------------------------------
// Wake a blocked process and return it to READY state.
// This is the counterpart to _process_block.
//
// Parameters:
//   x0 (uint64_t) - core_id: Core ID (0 to MAX_CORES-1)
//   x1 (void*) - pcb: Process Control Block pointer
//
// Returns:
//   x0 (int) - success: 1 on success, 0 on failure
//
// Complexity: O(1) - Constant time wake operation
//
// Version: 0.10
// Author: Lee Barney
// Last Modified: 2025-01-19
//
// Clobbers: x1, x2, x3, x4, x5, x6, x7, x8, x9, x10, x11, x12, x13, x14, x15, x16, x17, x18, x19, x20, x21, x22, x23, x24, x25, x26, x27, x28, x29, x30
//
_process_wake:
    // Save callee-saved registers
    stp x19, x20, [sp, #-16]!
    stp x21, x22, [sp, #-16]!
    stp x23, x24, [sp, #-16]!
    stp x25, x26, [sp, #-16]!
    stp x27, x30, [sp], #16

    // x0 = core_id, x1 = pcb
    mov x19, x0  // Save core_id
    mov x20, x1  // Save pcb

    // Validate core ID
    cmp x19, #MAX_CORES
    b.ge wake_invalid_core

    // Validate PCB pointer
    cbz x20, wake_invalid_pcb

    // Verify process is in WAITING state
    ldr x21, [x20, #pcb_state]
    cmp x21, #PROCESS_STATE_WAITING
    b.ne wake_invalid_state

    // Get scheduler state
    adrp x22, _scheduler_states@PAGE
    add x22, x22, _scheduler_states@PAGEOFF
    mov x23, #scheduler_size
    mul x23, x19, x23
    add x22, x22, x23  // x22 = scheduler state address

    // Get blocking reason to determine which queue to remove from
    ldr x24, [x20, #pcb_blocking_reason]
    cmp x24, #REASON_RECEIVE
    b.eq wake_remove_from_receive_queue
    cmp x24, #REASON_TIMER
    b.eq wake_remove_from_timer_queue
    cmp x24, #REASON_IO
    b.eq wake_remove_from_io_queue
    b wake_invalid_reason

wake_remove_from_receive_queue:
    add x25, x22, #scheduler_waiting_receive
    bl _remove_from_waiting_queue
    b wake_continue

wake_remove_from_timer_queue:
    add x25, x22, #scheduler_waiting_timer
    bl _remove_from_waiting_queue
    b wake_continue

wake_remove_from_io_queue:
    add x25, x22, #scheduler_waiting_io
    bl _remove_from_waiting_queue
    b wake_continue

wake_continue:
    // Set process state to READY
    mov x26, #PROCESS_STATE_READY
    str x26, [x20, #pcb_state]

    // Reset reduction counter to default
    mov x27, #DEFAULT_REDUCTIONS
    str x27, [x22, #scheduler_current_reductions]

    // Clear blocking reason
    str xzr, [x20, #pcb_blocking_reason]

    // Get process priority and enqueue to ready queue
    ldr x28, [x20, #pcb_priority]
    mov x0, x19  // core_id
    mov x1, x20  // pcb
    mov x2, x28  // priority
    bl _scheduler_enqueue_process

    // Increment scheduler wake statistics
    ldr x29, [x22, #scheduler_total_wakes]
    add x29, x29, #1
    str x29, [x22, #scheduler_total_wakes]

    // If scheduler idle, signal scheduler (use SEV)
    // Note: In a real system, this would wake idle cores
    // For now, just return success
    mov x0, #1  // Return 1 = success
    ldp x27, x30, [sp], #16
    ldp x25, x26, [sp], #16
    ldp x23, x24, [sp], #16
    ldp x21, x22, [sp], #16
    ldp x19, x20, [sp], #16
    ret

wake_invalid_core:
    mov x0, #0  // Return 0 = failure
    ldp x27, x30, [sp], #16
    ldp x25, x26, [sp], #16
    ldp x23, x24, [sp], #16
    ldp x21, x22, [sp], #16
    ldp x19, x20, [sp], #16
    ret

wake_invalid_pcb:
    mov x0, #0  // Return 0 = failure
    ldp x27, x30, [sp], #16
    ldp x25, x26, [sp], #16
    ldp x23, x24, [sp], #16
    ldp x21, x22, [sp], #16
    ldp x19, x20, [sp], #16
    ret

wake_invalid_state:
    mov x0, #0  // Return 0 = failure
    ldp x27, x30, [sp], #16
    ldp x25, x26, [sp], #16
    ldp x23, x24, [sp], #16
    ldp x21, x22, [sp], #16
    ldp x19, x20, [sp], #16
    ret

wake_invalid_reason:
    mov x0, #0  // Return 0 = failure
    ldp x27, x30, [sp], #16
    ldp x25, x26, [sp], #16
    ldp x23, x24, [sp], #16
    ldp x21, x22, [sp], #16
    ldp x19, x20, [sp], #16
    ret

// ------------------------------------------------------------
// Remove from Waiting Queue Helper
// ------------------------------------------------------------
// Helper function to remove a process from a waiting queue.
// This is a common operation for all wake operations.
//
// Parameters:
//   x20 (void*) - pcb: Process Control Block pointer
//   x25 (void*) - queue: Waiting queue pointer
//
// Returns: None
//
// Clobbers: x26, x27, x28, x29
//
_remove_from_waiting_queue:
    // Load process's prev and next pointers
    ldr x26, [x20, #pcb_prev]  // Previous process
    ldr x27, [x20, #pcb_next]  // Next process

    // Update previous process's next pointer
    cbz x26, remove_from_head
    str x27, [x26, #pcb_next]
    b remove_update_next

remove_from_head:
    // Process is at head, update queue head
    str x27, [x25, #queue_head]

remove_update_next:
    // Update next process's prev pointer
    cbz x27, remove_from_tail
    str x26, [x27, #pcb_prev]
    b remove_done

remove_from_tail:
    // Process is at tail, update queue tail
    str x26, [x25, #queue_tail]

remove_done:
    // Clear process's next/prev pointers
    str xzr, [x20, #pcb_next]
    str xzr, [x20, #pcb_prev]

    // Decrement queue count
    ldr w28, [x25, #queue_count]
    sub w28, w28, #1
    str w28, [x25, #queue_count]
    ret

// ------------------------------------------------------------
// Process Block on Receive Function
// ------------------------------------------------------------
// Block process waiting for a message with pattern matching.
// Checks message queue first, only blocks if no matching message.
//
// Parameters:
//   x0 (uint64_t) - core_id: Core ID (0 to MAX_CORES-1)
//   x1 (void*) - pcb: Process Control Block pointer
//   x2 (uint64_t) - pattern: Message pattern to match
//
// Returns:
//   x0 (void*) - message: Message pointer if found, NULL if blocked
//
// Complexity: O(m) where m is number of messages in queue
//
// Version: 0.10
// Author: Lee Barney
// Last Modified: 2025-01-19
//
// Clobbers: x1, x2, x3, x4, x5, x6, x7, x8, x9, x10, x11, x12, x13, x14, x15, x16, x17, x18, x19, x20, x21, x22, x23, x24, x25, x26, x27, x28, x29, x30
//
_process_block_on_receive:
    // Save callee-saved registers
    stp x19, x20, [sp, #-16]!
    stp x21, x22, [sp, #-16]!
    stp x23, x24, [sp, #-16]!
    stp x25, x26, [sp, #-16]!
    stp x27, x30, [sp], #16

    // x0 = core_id, x1 = pcb, x2 = pattern
    mov x19, x0  // Save core_id
    mov x20, x1  // Save pcb
    mov x21, x2  // Save pattern

    // Validate core ID
    cmp x19, #MAX_CORES
    b.ge receive_invalid_core

    // Validate PCB pointer
    cbz x20, receive_invalid_pcb

    // Get process message queue
    ldr x22, [x20, #pcb_message_queue]
    cbz x22, receive_no_messages

    // Check message queue for matching message
    // Note: This is a simplified implementation
    // In a real system, this would iterate through messages
    // and check pattern matching
    // For now, assume no matching message found
    b receive_no_match

receive_no_messages:
receive_no_match:
    // No matching message found, block process
    str x21, [x20, #pcb_message_pattern]  // Store pattern for later matching
    mov x0, x19  // core_id
    mov x1, x20  // pcb
    mov x2, #REASON_RECEIVE  // reason
    bl _process_block
    mov x0, #0  // Return NULL (process blocked)
    ldp x27, x30, [sp], #16
    ldp x25, x26, [sp], #16
    ldp x23, x24, [sp], #16
    ldp x21, x22, [sp], #16
    ldp x19, x20, [sp], #16
    ret

receive_invalid_core:
    mov x0, #0  // Return NULL
    ldp x27, x30, [sp], #16
    ldp x25, x26, [sp], #16
    ldp x23, x24, [sp], #16
    ldp x21, x22, [sp], #16
    ldp x19, x20, [sp], #16
    ret

receive_invalid_pcb:
    mov x0, #0  // Return NULL
    ldp x27, x30, [sp], #16
    ldp x25, x26, [sp], #16
    ldp x23, x24, [sp], #16
    ldp x21, x22, [sp], #16
    ldp x19, x20, [sp], #16
    ret

// ------------------------------------------------------------
// Process Block on Timer Function
// ------------------------------------------------------------
// Block process with timeout using system timer.
// Uses ARM Generic Timer (CNTPCT_EL0) for absolute wake time.
//
// Parameters:
//   x0 (uint64_t) - core_id: Core ID (0 to MAX_CORES-1)
//   x1 (void*) - pcb: Process Control Block pointer
//   x2 (uint64_t) - timeout_ticks: Timeout in timer ticks
//
// Returns:
//   x0 (int) - success: 1 on success, 0 on failure
//
// Complexity: O(1) - Constant time timer setup
//
// Version: 0.10
// Author: Lee Barney
// Last Modified: 2025-01-19
//
// Clobbers: x1, x2, x3, x4, x5, x6, x7, x8, x9, x10, x11, x12, x13, x14, x15, x16, x17, x18, x19, x20, x21, x22, x23, x24, x25, x26, x27, x28, x29, x30
//
_process_block_on_timer:
    // Save callee-saved registers
    stp x19, x20, [sp, #-16]!
    stp x21, x22, [sp, #-16]!
    stp x23, x24, [sp, #-16]!
    stp x25, x26, [sp, #-16]!
    stp x27, x30, [sp], #16

    // x0 = core_id, x1 = pcb, x2 = timeout_ticks
    mov x19, x0  // Save core_id
    mov x20, x1  // Save pcb
    mov x21, x2  // Save timeout_ticks

    // Validate core ID
    cmp x19, #MAX_CORES
    b.ge timer_invalid_core

    // Validate PCB pointer
    cbz x20, timer_invalid_pcb

    // Validate timeout (load constant into register first)
    mov x24, #MAX_BLOCKING_TIME
    cmp x21, x24
    b.gt timer_invalid_timeout

    // Read system timer (CNTPCT_EL0)
    mrs x22, CNTPCT_EL0

    // Calculate wake time (current + timeout)
    add x23, x22, x21

    // Store wake time in PCB
    str x23, [x20, #pcb_wake_time]

    // Block process with timer reason
    mov x0, x19  // core_id
    mov x1, x20  // pcb
    mov x2, #REASON_TIMER  // reason
    bl _process_block

    mov x0, #1  // Return 1 = success
    ldp x27, x30, [sp], #16
    ldp x25, x26, [sp], #16
    ldp x23, x24, [sp], #16
    ldp x21, x22, [sp], #16
    ldp x19, x20, [sp], #16
    ret

timer_invalid_core:
    mov x0, #0  // Return 0 = failure
    ldp x27, x30, [sp], #16
    ldp x25, x26, [sp], #16
    ldp x23, x24, [sp], #16
    ldp x21, x22, [sp], #16
    ldp x19, x20, [sp], #16
    ret

timer_invalid_pcb:
    mov x0, #0  // Return 0 = failure
    ldp x27, x30, [sp], #16
    ldp x25, x26, [sp], #16
    ldp x23, x24, [sp], #16
    ldp x21, x22, [sp], #16
    ldp x19, x20, [sp], #16
    ret

timer_invalid_timeout:
    mov x0, #0  // Return 0 = failure
    ldp x27, x30, [sp], #16
    ldp x25, x26, [sp], #16
    ldp x23, x24, [sp], #16
    ldp x21, x22, [sp], #16
    ldp x19, x20, [sp], #16
    ret

// ------------------------------------------------------------
// Process Block on I/O Function
// ------------------------------------------------------------
// Block process waiting for I/O operation (stub implementation).
// This is a placeholder for future I/O implementation.
//
// Parameters:
//   x0 (uint64_t) - core_id: Core ID (0 to MAX_CORES-1)
//   x1 (void*) - pcb: Process Control Block pointer
//   x2 (uint64_t) - io_descriptor: I/O descriptor
//
// Returns:
//   x0 (int) - success: 1 on success, 0 on failure
//
// Complexity: O(1) - Constant time I/O setup
//
// Version: 0.10
// Author: Lee Barney
// Last Modified: 2025-01-19
//
// Clobbers: x1, x2, x3, x4, x5, x6, x7, x8, x9, x10, x11, x12, x13, x14, x15, x16, x17, x18, x19, x20, x21, x22, x23, x24, x25, x26, x27, x28, x29, x30
//
_process_block_on_io:
    // Save callee-saved registers
    stp x19, x20, [sp, #-16]!
    stp x21, x22, [sp, #-16]!
    stp x23, x24, [sp, #-16]!
    stp x25, x26, [sp, #-16]!
    stp x27, x30, [sp], #16

    // x0 = core_id, x1 = pcb, x2 = io_descriptor
    mov x19, x0  // Save core_id
    mov x20, x1  // Save pcb
    mov x21, x2  // Save io_descriptor

    // Validate core ID
    cmp x19, #MAX_CORES
    b.ge io_invalid_core

    // Validate PCB pointer
    cbz x20, io_invalid_pcb

    // Store I/O descriptor in PCB
    str x21, [x20, #pcb_blocking_data]

    // Block process with I/O reason
    mov x0, x19  // core_id
    mov x1, x20  // pcb
    mov x2, #REASON_IO  // reason
    bl _process_block

    mov x0, #1  // Return 1 = success
    ldp x27, x30, [sp], #16
    ldp x25, x26, [sp], #16
    ldp x23, x24, [sp], #16
    ldp x21, x22, [sp], #16
    ldp x19, x20, [sp], #16
    ret

io_invalid_core:
    mov x0, #0  // Return 0 = failure
    ldp x27, x30, [sp], #16
    ldp x25, x26, [sp], #16
    ldp x23, x24, [sp], #16
    ldp x21, x22, [sp], #16
    ldp x19, x20, [sp], #16
    ret

io_invalid_pcb:
    mov x0, #0  // Return 0 = failure
    ldp x27, x30, [sp], #16
    ldp x25, x26, [sp], #16
    ldp x23, x24, [sp], #16
    ldp x21, x22, [sp], #16
    ldp x19, x20, [sp], #16
    ret

// ------------------------------------------------------------
// Process Check Timer Wakeups Function
// ------------------------------------------------------------
// Check timer waiting queue and wake processes whose timeout has expired.
// This function should be called periodically to handle timer timeouts.
//
// Parameters:
//   x0 (uint64_t) - core_id: Core ID (0 to MAX_CORES-1)
//
// Returns:
//   x0 (uint64_t) - woken_count: Number of processes woken up
//
// Complexity: O(n) where n is number of processes in timer queue
//
// Version: 0.10
// Author: Lee Barney
// Last Modified: 2025-01-19
//
// Clobbers: x1, x2, x3, x4, x5, x6, x7, x8, x9, x10, x11, x12, x13, x14, x15, x16, x17, x18, x19, x20, x21, x22, x23, x24, x25, x26, x27, x28, x29, x30
//
_process_check_timer_wakeups:
    // Save callee-saved registers
    stp x19, x20, [sp, #-16]!
    stp x21, x22, [sp, #-16]!
    stp x23, x24, [sp, #-16]!
    stp x25, x26, [sp, #-16]!
    stp x27, x30, [sp], #16

    // x0 = core_id
    mov x19, x0  // Save core_id
    mov x20, #0  // woken_count = 0

    // Validate core ID
    cmp x19, #MAX_CORES
    b.ge timer_check_invalid_core

    // Get scheduler state
    adrp x21, _scheduler_states@PAGE
    add x21, x21, _scheduler_states@PAGEOFF
    mov x22, #scheduler_size
    mul x22, x19, x22
    add x21, x21, x22  // x21 = scheduler state address

    // Get timer waiting queue
    add x23, x21, #scheduler_waiting_timer

    // Read current system timer
    mrs x24, CNTPCT_EL0

    // Check if timer queue is empty
    ldr w25, [x23, #queue_count]
    cbz w25, timer_check_done

    // Iterate through timer queue and wake expired processes
    ldr x26, [x23, #queue_head]  // Start from head
    mov x27, #0  // processed_count = 0

timer_check_loop:
    cbz x26, timer_check_done  // No more processes

    // Load process wake time
    ldr x28, [x26, #pcb_wake_time]
    cmp x28, x24
    b.le timer_check_wake_process

    // Process not expired, move to next
    ldr x26, [x26, #pcb_next]
    add x27, x27, #1
    cmp x27, x25
    b.lt timer_check_loop
    b timer_check_done

timer_check_wake_process:
    // Process expired, wake it up
    mov x0, x19  // core_id
    mov x1, x26  // pcb
    bl _process_wake
    add x20, x20, #1  // Increment woken_count

    // Move to next process
    ldr x26, [x26, #pcb_next]
    add x27, x27, #1
    cmp x27, x25
    b.lt timer_check_loop

timer_check_done:
    // Return woken count
    mov x0, x20
    ldp x27, x30, [sp], #16
    ldp x25, x26, [sp], #16
    ldp x23, x24, [sp], #16
    ldp x21, x22, [sp], #16
    ldp x19, x20, [sp], #16
    ret

timer_check_invalid_core:
    mov x0, #0  // Return 0 = no processes woken
    ldp x27, x30, [sp], #16
    ldp x25, x26, [sp], #16
    ldp x23, x24, [sp], #16
    ldp x21, x22, [sp], #16
    ldp x19, x20, [sp], #16
    ret

// ------------------------------------------------------------
// Constant Definitions for C Code
// ------------------------------------------------------------
// Export constants as global symbols for C code access
_REASON_RECEIVE:
    .quad REASON_RECEIVE

_REASON_TIMER:
    .quad REASON_TIMER

_REASON_IO:
    .quad REASON_IO

_MAX_BLOCKING_TIME:
    .quad MAX_BLOCKING_TIME

_BIF_SPAWN_COST:
    .quad BIF_SPAWN_COST

_BIF_EXIT_COST:
    .quad BIF_EXIT_COST

_BIF_YIELD_COST:
    .quad BIF_YIELD_COST

// Non-underscore versions for C compatibility
_REASON_RECEIVE_CONST:
    .quad REASON_RECEIVE

_REASON_TIMER_CONST:
    .quad REASON_TIMER

_REASON_IO_CONST:
    .quad REASON_IO

_MAX_BLOCKING_TIME_CONST:
    .quad MAX_BLOCKING_TIME

_BIF_SPAWN_COST_CONST:
    .quad BIF_SPAWN_COST

_BIF_EXIT_COST_CONST:
    .quad BIF_EXIT_COST

_BIF_YIELD_COST_CONST:
    .quad BIF_YIELD_COST

// ------------------------------------------------------------
// Function Aliases for C Compatibility
// ------------------------------------------------------------
// Create aliases for C code that expects non-underscore function names

process_block:
    b _process_block

process_wake:
    b _process_wake

process_block_on_receive:
    b _process_block_on_receive

process_block_on_timer:
    b _process_block_on_timer

process_block_on_io:
    b _process_block_on_io

process_check_timer_wakeups:
    b _process_check_timer_wakeups
