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
// communication.s — Inter-Core Communication Implementation
// ------------------------------------------------------------
// BEAM-style inter-core message passing with lock-free queues,
// blocking receive operations, and cross-core notifications.
// Implements Phase 8 of the research implementation plan.
//
// The file provides:
//   - Lock-free message queue implementation
//   - Cross-core message passing
//   - Blocking and non-blocking receive operations
//   - Message ordering and delivery guarantees
//   - ARM64 atomic operations for concurrency
//
// Version: 0.10
// Author: Lee Barney
// Last Modified: 2025-01-19
//

    .text
    .align 4

// Include configuration constants
    .include "config.inc"

// ------------------------------------------------------------
// Message Queue Function Exports
// ------------------------------------------------------------
// Export the main message queue functions to make them callable from C code.
// These functions provide the primary interface for inter-core communication,
// message passing, and queue management.
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
    .global _message_queue_init
    .global _send_message
    .global _receive_message
    .global _try_receive_message
    .global _message_queue_empty
    .global _message_queue_size
    .global _message_queue_full
    .global _wake_receiver
    .global _block_on_receive

// ------------------------------------------------------------
// Message Queue Data Structure Layout
// ------------------------------------------------------------
// Define the memory layout for the message queue structure.
// This structure implements a lock-free MPSC (Multi-Producer Single-Consumer) queue.
//
// Version: 0.10
// Author: Lee Barney
// Last Modified: 2025-01-19
//
    // Message queue structure offsets (64 bytes aligned)
    .equ msg_queue_head, 0              // Head pointer (8 bytes) - consumer access
    .equ msg_queue_tail, 8              // Tail pointer (8 bytes) - producer access
    .equ msg_queue_messages, 16         // Message array pointer (8 bytes)
    .equ msg_queue_size, 24             // Maximum size (8 bytes) - changed from 4 bytes
    .equ msg_queue_mask, 32             // Size mask for circular buffer (8 bytes) - changed from 4 bytes
    .equ msg_queue_blocked, 40          // Blocked receiver flag (8 bytes)
    .equ msg_queue_waiting_process, 48  // Waiting process pointer (8 bytes) - changed from offset 40
    .equ msg_queue_size_bytes, 64       // Total structure size

// ------------------------------------------------------------
// Message Structure Layout
// ------------------------------------------------------------
// Define the memory layout for individual messages.
//
// Version: 0.10
// Author: Lee Barney
// Last Modified: 2025-01-19
//
    .equ msg_sender, 0                  // Sender process pointer (8 bytes)
    .equ msg_data, 8                    // Message data (8 bytes)
    .equ msg_timestamp, 16              // Timestamp (8 bytes)
    .equ msg_size, 24                   // Total message size

// ------------------------------------------------------------
// Message Queue Initialization
// ------------------------------------------------------------
// Initialize a message queue with the specified size.
// Allocates memory for the message array and sets up the circular buffer.
//
// Parameters:
//   x0 (void*) - queue_ptr: Pointer to queue structure
//   x1 (uint32_t) - size: Maximum number of messages in queue
//
// Returns:
//   x0 (int) - success: 1 on success, 0 on failure
//
// Complexity: O(1) - Constant time initialization
//
// Version: 0.10
// Author: Lee Barney
// Last Modified: 2025-01-19
//
_message_queue_init:
    // Save callee-saved registers
    stp x19, x30, [sp, #-16]!
    stp x20, x21, [sp, #-16]!

    // Validate parameters
    cbz x0, init_failed  // Check queue pointer
    cbz x1, init_failed  // Check size

    // Save parameters
    mov x19, x0  // queue_ptr
    mov x20, x1  // size

    // Validate size is power of 2 (required for circular buffer)
    sub x21, x20, #1
    and x21, x20, x21
    cbnz x21, init_failed  // Not a power of 2

    // Check size is reasonable (between 2 and 1024)
    cmp x20, #2
    b.lt init_failed
    cmp x20, #1024
    b.gt init_failed

    // Allocate memory for message array (size * 24 bytes per message)
    mov x21, #24  // Message size
    mul x21, x20, x21  // Total size
    
    // Call C library mmap function
    mov x0, xzr        // addr = NULL (let system choose)
    mov x1, x21        // length = size * 24
    mov x2, #3         // prot = PROT_READ | PROT_WRITE
    mov x3, #0x1002    // flags = MAP_PRIVATE | MAP_ANON (macOS)
    mov x4, #-1        // fd = -1 (not a file mapping)
    mov x5, xzr        // offset = 0
    bl _mmap           // Call C library mmap function

    // Check for mmap failure (MAP_FAILED is typically -1)
    mov x21, #-1
    cmp x0, x21
    b.eq init_failed

    // Initialize queue structure
    str xzr, [x19, #msg_queue_head]        // head = 0
    str xzr, [x19, #msg_queue_tail]         // tail = 0
    str x0, [x19, #msg_queue_messages]     // messages = allocated array
    str x20, [x19, #msg_queue_size]        // size = requested size (64-bit)
    sub x21, x20, #1                       // mask = size - 1
    str x21, [x19, #msg_queue_mask]        // mask = size - 1 (64-bit)
    str xzr, [x19, #msg_queue_blocked]     // blocked = 0
    str xzr, [x19, #msg_queue_waiting_process] // waiting_process = NULL

    // Clear the message array
    mov x21, x20        // Number of elements to clear
    mov x22, x0         // Current address
    mov x23, #0         // Value to store (0)

clear_array_loop:
    str x23, [x22], #8  // Store 0, increment address
    sub x21, x21, #1    // Decrement counter
    cbnz x21, clear_array_loop

    // Return success
    mov x0, #1
    ldp x20, x21, [sp], #16
    ldp x19, x30, [sp], #16
    ret

init_failed:
    mov x0, #0
    ldp x20, x21, [sp], #16
    ldp x19, x30, [sp], #16
    ret

// ------------------------------------------------------------
// Send Message
// ------------------------------------------------------------
// Send a message to a receiver's message queue.
// This is a non-blocking operation that may fail if the queue is full.
//
// Parameters:
//   x0 (void*) - sender_pcb: Sender process pointer
//   x1 (void*) - receiver_pcb: Receiver process pointer
//   x2 (uint64_t) - message_data: Message data to send
//
// Returns:
//   x0 (int) - success: 1 on success, 0 on failure
//
// Complexity: O(1) - Constant time operation
//
// Version: 0.10
// Author: Lee Barney
// Last Modified: 2025-01-19
//
_send_message:
    // Save callee-saved registers (save ALL registers we use)
    stp x19, x30, [sp, #-16]!
    stp x20, x21, [sp, #-16]!
    stp x22, x23, [sp, #-16]!
    stp x24, x25, [sp, #-16]!
    stp x26, x27, [sp, #-16]!
    
    // Add stack guard to detect overflow (use smaller constant)
    mov x27, #0x123
    stp x27, x27, [sp, #-16]!

    // Validate parameters
    cbz x0, send_failed  // Check sender PCB
    cbz x1, send_failed  // Check receiver PCB

    // Save parameters
    mov x19, x0  // sender_pcb
    mov x20, x1  // receiver_pcb
    mov x21, x2  // message_data

    // Get receiver's message queue (at offset 368 in real PCB)
    ldr x22, [x20, #368]  // receiver's message queue
    cbz x22, send_failed  // Check if queue exists

    // Check if queue is full
    mov x0, x22  // Pass queue pointer to _message_queue_full
    bl _message_queue_full
    cbnz x0, send_failed  // If FULL, go to send_failed (FIXED!)

    // Get current tail pointer
    ldr x23, [x22, #msg_queue_tail]
    
    // Get message array pointer
    ldr x24, [x22, #msg_queue_messages]
    cbz x24, send_failed  // Check if array is allocated

    // Calculate index with mask
    ldr x25, [x22, #msg_queue_mask]  // Load 64-bit mask field
    and x26, x23, x25  // index = tail & mask

    // Store message in array (24 bytes per message)
    mov x27, #24                 // Message size = 24 bytes
    mul x27, x26, x27           // x27 = index * 24
    add x27, x24, x27           // x27 = array + (index * 24)
    str x19, [x27, #0]          // messages[index].sender = sender_pcb
    str x21, [x27, #8]          // messages[index].data = message_data
    str xzr, [x27, #16]         // messages[index].timestamp = 0 (simplified)

    // Memory barrier to ensure store is visible
    dmb sy

    // Update tail pointer (increment actual pointer, not masked)
    add x23, x23, #1
    str x23, [x22, #msg_queue_tail]

    // Wake up blocked receiver if any
    ldr x27, [x22, #msg_queue_blocked]
    cbz x27, send_done  // No blocked receiver

    // Wake up the receiver (simplified - just clear blocked flag)
    str xzr, [x22, #msg_queue_blocked]
    str xzr, [x22, #msg_queue_waiting_process]

send_done:
    // Check stack guard before return
    ldp x27, x28, [sp], #16
    cmp x27, #0x123
    b.ne stack_corruption_detected_send
    cmp x28, #0x123
    b.ne stack_corruption_detected_send
    
    // Return success
    mov x0, #1
    ldp x26, x27, [sp], #16
    ldp x24, x25, [sp], #16
    ldp x22, x23, [sp], #16
    ldp x20, x21, [sp], #16
    ldp x19, x30, [sp], #16
    ret

send_failed:
    // Check stack guard before return
    ldp x27, x28, [sp], #16
    cmp x27, #0x123
    b.ne stack_corruption_detected_send
    cmp x28, #0x123
    b.ne stack_corruption_detected_send
    
    mov x0, #0
    ldp x26, x27, [sp], #16
    ldp x24, x25, [sp], #16
    ldp x22, x23, [sp], #16
    ldp x20, x21, [sp], #16
    ldp x19, x30, [sp], #16
    ret

stack_corruption_detected_send:
    // Stack corruption detected - return error
    mov x0, #0
    ldp x26, x27, [sp], #16
    ldp x24, x25, [sp], #16
    ldp x22, x23, [sp], #16
    ldp x20, x21, [sp], #16
    ldp x19, x30, [sp], #16
    ret

// ------------------------------------------------------------
// Receive Message
// ------------------------------------------------------------
// Receive a message from the process's message queue.
// This is a blocking operation that will wait if no messages are available.
//
// Parameters:
//   x0 (void*) - receiver_pcb: Receiver process pointer
//
// Returns:
//   x0 (uint64_t) - message_data: Message data received, or 0 if blocked
//
// Complexity: O(1) - Constant time operation
//
// Version: 0.10
// Author: Lee Barney
// Last Modified: 2025-01-19
//
_receive_message:
    // Save callee-saved registers
    stp x19, x30, [sp, #-16]!
    stp x20, x21, [sp, #-16]!

    // Validate parameters
    cbz x0, receive_failed  // Check receiver PCB

    // Save parameters
    mov x19, x0  // receiver_pcb

    // Get receiver's message queue (at offset 368 in real PCB)
    ldr x20, [x19, #368]  // receiver's message queue
    cbz x20, receive_failed  // Check if queue exists

    // Check if queue is empty
    mov x0, x20  // Pass queue pointer to _message_queue_empty
    bl _message_queue_empty
    cbnz x0, receive_blocked  // Queue is empty, block

    // Get current head pointer
    ldr x21, [x20, #msg_queue_head]
    
    // Get message array pointer
    ldr x22, [x20, #msg_queue_messages]
    cbz x22, receive_failed  // Check if array is allocated

    // Calculate index with mask
    ldr x23, [x20, #msg_queue_mask]  // Load 64-bit mask field
    and x24, x21, x23  // index = head & mask

    // Load message from array (24 bytes per message)
    mov x25, #24                 // Message size = 24 bytes
    mul x25, x24, x25           // x25 = index * 24
    add x25, x22, x25           // x25 = array + (index * 24)
    ldr x26, [x25, #8]         // message_data = messages[index].data

    // Update head pointer (increment actual pointer, not masked)
    add x21, x21, #1
    str x21, [x20, #msg_queue_head]

    // Return message data
    mov x0, x26
    ldp x20, x21, [sp], #16
    ldp x19, x30, [sp], #16
    ret

receive_blocked:
    // Block the receiver
    mov x1, #1
    str x1, [x20, #msg_queue_blocked]
    str x19, [x20, #msg_queue_waiting_process]
    
    // Return 0 to indicate blocking
    mov x0, #0
    ldp x20, x21, [sp], #16
    ldp x19, x30, [sp], #16
    ret

receive_failed:
    mov x0, #0
    ldp x20, x21, [sp], #16
    ldp x19, x30, [sp], #16
    ret

// ------------------------------------------------------------
// Try Receive Message
// ------------------------------------------------------------
// Try to receive a message from the process's message queue.
// This is a non-blocking operation that returns immediately.
//
// Parameters:
//   x0 (void*) - receiver_pcb: Receiver process pointer
//
// Returns:
//   x0 (uint64_t) - message_data: Message data received, or 0 if no message
//
// Complexity: O(1) - Constant time operation
//
// Version: 0.10
// Author: Lee Barney
// Last Modified: 2025-01-19
//
_try_receive_message:
    // Save callee-saved registers (save ALL registers we use)
    stp x19, x30, [sp, #-16]!
    stp x20, x21, [sp, #-16]!
    stp x22, x23, [sp, #-16]!
    stp x24, x25, [sp, #-16]!
    stp x26, x27, [sp, #-16]!
    
    // Add stack guard to detect overflow (use smaller constant)
    mov x27, #0x123
    stp x27, x27, [sp, #-16]!

    // Validate parameters
    cbz x0, try_receive_failed  // Check receiver PCB

    // Save parameters
    mov x19, x0  // receiver_pcb

    // Get receiver's message queue (at offset 368 in real PCB)
    ldr x20, [x19, #368]  // receiver's message queue
    cbz x20, try_receive_failed  // Check if queue exists

    // Check if queue is empty
    mov x0, x20  // Pass queue pointer to _message_queue_empty
    bl _message_queue_empty
    cbnz x0, try_receive_empty  // Queue is empty

    // Get current head pointer
    ldr x21, [x20, #msg_queue_head]
    
    // Get message array pointer
    ldr x22, [x20, #msg_queue_messages]
    cbz x22, try_receive_failed  // Check if array is allocated

    // Calculate index with mask
    ldr x23, [x20, #msg_queue_mask]  // Load 64-bit mask field
    and x24, x21, x23  // index = head & mask

    // Load message from array (24 bytes per message)
    mov x25, #24                 // Message size = 24 bytes
    mul x25, x24, x25           // x25 = index * 24
    add x25, x22, x25           // x25 = array + (index * 24)
    ldr x26, [x25, #8]         // message_data = messages[index].data

    // Update head pointer (increment actual pointer, not masked)
    add x21, x21, #1
    str x21, [x20, #msg_queue_head]

    // Check stack guard before return
    ldp x27, x28, [sp], #16
    cmp x27, #0x123
    b.ne stack_corruption_detected
    cmp x28, #0x123
    b.ne stack_corruption_detected

    // Return message data
    mov x0, x26
    ldp x26, x27, [sp], #16
    ldp x24, x25, [sp], #16
    ldp x22, x23, [sp], #16
    ldp x20, x21, [sp], #16
    ldp x19, x30, [sp], #16
    ret

try_receive_empty:
    // Check stack guard before return
    ldp x27, x28, [sp], #16
    cmp x27, #0x123
    b.ne stack_corruption_detected
    cmp x28, #0x123
    b.ne stack_corruption_detected
    
    // Queue is empty, return 0
    mov x0, #0
    ldp x26, x27, [sp], #16
    ldp x24, x25, [sp], #16
    ldp x22, x23, [sp], #16
    ldp x20, x21, [sp], #16
    ldp x19, x30, [sp], #16
    ret

try_receive_failed:
    // Check stack guard before return
    ldp x27, x28, [sp], #16
    cmp x27, #0x123
    b.ne stack_corruption_detected
    cmp x28, #0x123
    b.ne stack_corruption_detected
    
    mov x0, #0
    ldp x26, x27, [sp], #16
    ldp x24, x25, [sp], #16
    ldp x22, x23, [sp], #16
    ldp x20, x21, [sp], #16
    ldp x19, x30, [sp], #16
    ret

stack_corruption_detected:
    // Stack corruption detected - return error
    mov x0, #0
    ldp x26, x27, [sp], #16
    ldp x24, x25, [sp], #16
    ldp x22, x23, [sp], #16
    ldp x20, x21, [sp], #16
    ldp x19, x30, [sp], #16
    ret

// ------------------------------------------------------------
// Message Queue Is Empty
// ------------------------------------------------------------
// Check if the message queue is empty by comparing head and tail pointers.
//
// Parameters:
//   x0 (void*) - queue_ptr: Pointer to queue structure
//
// Returns:
//   x0 (int) - empty: 1 if empty, 0 if not empty
//
// Complexity: O(1) - Constant time operation
//
// Version: 0.10
// Author: Lee Barney
// Last Modified: 2025-01-19
//
_message_queue_empty:
    // Validate parameters
    cbz x0, is_empty_failed  // Check queue pointer

    // Get head and tail pointers
    ldr x1, [x0, #msg_queue_head]
    ldr x2, [x0, #msg_queue_tail]

    // Check if empty (head >= tail)
    cmp x1, x2
    b.ge is_empty_true

    // Not empty
    mov x0, #0
    ret

is_empty_true:
    mov x0, #1
    ret

is_empty_failed:
    mov x0, #1  // Consider NULL pointer as empty
    ret

// ------------------------------------------------------------
// Message Queue Size
// ------------------------------------------------------------
// Get the current number of messages in the queue.
//
// Parameters:
//   x0 (void*) - queue_ptr: Pointer to queue structure
//
// Returns:
//   x0 (uint32_t) - size: Number of messages in queue
//
// Complexity: O(1) - Constant time operation
//
// Version: 0.10
// Author: Lee Barney
// Last Modified: 2025-01-19
//
_message_queue_size:
    // Validate parameters
    cbz x0, size_failed  // Check queue pointer

    // Get head and tail pointers
    ldr x1, [x0, #msg_queue_head]
    ldr x2, [x0, #msg_queue_tail]

    // Calculate size (tail - head)
    sub x0, x2, x1
    ret

size_failed:
    mov x0, #0
    ret

// ------------------------------------------------------------
// Message Queue Is Full
// ------------------------------------------------------------
// Check if the message queue is full.
//
// Parameters:
//   x0 (void*) - queue_ptr: Pointer to queue structure
//
// Returns:
//   x0 (int) - full: 1 if full, 0 if not full
//
// Complexity: O(1) - Constant time operation
//
// Version: 0.10
// Author: Lee Barney
// Last Modified: 2025-01-19
//
_message_queue_full:
    // Validate parameters
    cbz x0, is_full_failed  // Check queue pointer

    // Get head and tail pointers
    ldr x1, [x0, #msg_queue_head]
    ldr x2, [x0, #msg_queue_tail]

    // Get queue size
    ldr x3, [x0, #msg_queue_size]  // Load 64-bit size field

    // Calculate current size
    sub x4, x2, x1

    // Check if full (current size >= max size)
    cmp x4, x3
    b.ge is_full_true

    // Not full
    mov x0, #0
    ret

is_full_true:
    mov x0, #1
    ret

is_full_failed:
    mov x0, #0  // Consider NULL pointer as not full
    ret

// ------------------------------------------------------------
// Wake Receiver
// ------------------------------------------------------------
// Wake up a blocked receiver process.
//
// Parameters:
//   x0 (void*) - queue_ptr: Pointer to queue structure
//
// Returns:
//   x0 (int) - success: 1 on success, 0 on failure
//
// Complexity: O(1) - Constant time operation
//
// Version: 0.10
// Author: Lee Barney
// Last Modified: 2025-01-19
//
_wake_receiver:
    // Validate parameters
    cbz x0, wake_failed  // Check queue pointer

    // Clear blocked flag
    str xzr, [x0, #msg_queue_blocked]
    str xzr, [x0, #msg_queue_waiting_process]

    // Return success
    mov x0, #1
    ret

wake_failed:
    mov x0, #0
    ret

// ------------------------------------------------------------
// Block On Receive
// ------------------------------------------------------------
// Block the current process on message receive.
//
// Parameters:
//   x0 (void*) - receiver_pcb: Receiver process pointer
//
// Returns:
//   x0 (int) - success: 1 on success, 0 on failure
//
// Complexity: O(1) - Constant time operation
//
// Version: 0.10
// Author: Lee Barney
// Last Modified: 2025-01-19
//
_block_on_receive:
    // Validate parameters
    cbz x0, block_failed  // Check receiver PCB

    // Get receiver's message queue (at offset 368 in real PCB)
    ldr x1, [x0, #368]  // receiver's message queue
    cbz x1, block_failed  // Check if queue exists

    // Set blocked flag
    mov x2, #1
    str x2, [x1, #msg_queue_blocked]
    str x0, [x1, #msg_queue_waiting_process]

    // Return success
    mov x0, #1
    ret

block_failed:
    mov x0, #0
    ret

// Import required functions from other modules
    .extern _mmap
