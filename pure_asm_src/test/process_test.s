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
// process_test.s — Simple process creation for testing
// ------------------------------------------------------------
// Simple process creation function for testing scheduler functionality.
// This provides a minimal working process creation that can be used
// by scheduler tests without the complexity of the full PCB implementation.

    .text
    .align 4

    .global _process_create_fixed

    // Simple PCB structure for testing
    .equ test_pcb_pid, 0
    .equ test_pcb_priority, 8
    .equ test_pcb_scheduler_id, 16
    .equ test_pcb_state, 24
    .equ test_pcb_size, 32

    // Simple process pool for testing
    .bss
    .align 3
    .global _test_process_pool
_test_process_pool:
    .space 32 * 10  // test_pcb_size * 10

    .global _test_next_process_id
_test_next_process_id:
    .space 8

    .global _test_process_pool_bitmap
_test_process_pool_bitmap:
    .space 2  // 10 bits / 8 bytes per byte

    .text
    .align 4

// ------------------------------------------------------------
// process_create_fixed — Create a simple process for testing
// ------------------------------------------------------------
_process_create_fixed:
    // Save callee-saved registers
    stp x19, x30, [sp, #-16]!
    stp x20, x21, [sp, #-16]!

    // Save parameters
    mov x19, x0  // entry_point
    mov x20, x1  // priority
    mov x21, x2  // scheduler_id

    // Find available PCB in test pool
    adrp x0, _test_process_pool@PAGE
    add x0, x0, _test_process_pool@PAGEOFF
    
    adrp x1, _test_process_pool_bitmap@PAGE
    add x1, x1, _test_process_pool_bitmap@PAGEOFF
    
    // Search for available PCB
    mov x2, #0  // PCB index
    mov x3, #10  // MAX_PROCESSES
    
find_pcb_loop:
    // Check if PCB is available
    ldrb w4, [x1]  // Load bitmap byte
    mov w5, #1
    lsl w5, w5, w2  // Create bit mask
    tst w4, w5
    b.ne find_pcb_next
    
    // PCB is available, allocate it
    orr w4, w4, w5  // Set bit
    strb w4, [x1]    // Store updated bitmap
    
    // Calculate PCB address
    mov x4, #32  // test_pcb_size
    mul x4, x2, x4
    add x0, x0, x4  // PCB address
    
    // Get next process ID
    adrp x1, _test_next_process_id@PAGE
    add x1, x1, _test_next_process_id@PAGEOFF
    ldr x2, [x1]
    cbz x2, init_test_process_id
    add x2, x2, #1
    str x2, [x1]
    b test_process_id_ready
init_test_process_id:
    mov x2, #1
    str x2, [x1]
test_process_id_ready:
    
    // Initialize PCB fields
    str x2, [x0, #test_pcb_pid]
    str x20, [x0, #test_pcb_priority]
    str x21, [x0, #test_pcb_scheduler_id]
    mov x3, #0  // PROCESS_STATE_CREATED
    str x3, [x0, #test_pcb_state]
    
    // Restore registers and return
    ldp x20, x21, [sp], #16
    ldp x19, x30, [sp], #16
    ret
    
find_pcb_next:
    add x2, x2, #1
    cmp x2, x3
    b.lt find_pcb_loop
    
    // No available PCB found
    mov x0, #0
    ldp x20, x21, [sp], #16
    ldp x19, x30, [sp], #16
    ret
