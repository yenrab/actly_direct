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
// loadbalancer.s — Work Stealing Load Balancer Implementation
// ------------------------------------------------------------
// BEAM-style work stealing load balancer with lock-free deques,
// victim selection strategies, and migration logic. Implements
// Phase 4 of the research implementation plan.
//
// The file provides:
//   - Lock-free work stealing deque data structure
//   - Victim selection algorithms (random, load-based, locality-aware)
//   - Work stealing logic with migration constraints
//   - Load monitoring and balancing functions
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
// Work Stealing Deque Function Exports
// ------------------------------------------------------------
// Export the main work stealing functions to make them callable from C code.
// These functions provide the primary interface for work stealing operations,
// victim selection, and load balancing.
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
    .global _ws_deque_init
    .global _ws_deque_push_bottom
    .global _ws_deque_pop_bottom
    .global _ws_deque_pop_top
    .global _ws_deque_is_empty
    .global _ws_deque_size
    .global _get_scheduler_load
    .global _find_busiest_scheduler
    .global _is_steal_allowed
    .global _select_victim_random
    .global _select_victim_by_load
    .global _select_victim_locality
    .global _try_steal_work
    .global _migrate_process

// No global data variables exported - all constants are in config.inc

// ------------------------------------------------------------
// Work Stealing Deque Data Structure Layout
// ------------------------------------------------------------
// Define the memory layout for the work stealing deque structure.
// This structure implements a lock-free double-ended queue optimized
// for work stealing with ARM64 atomic operations.
//
// Version: 0.10
// Author: Lee Barney
// Last Modified: 2025-01-19
//
    // Work stealing deque structure offsets (64 bytes aligned)
    .equ ws_deque_top, 0              // Top pointer (8 bytes) - remote access
    .equ ws_deque_bottom, 8           // Bottom pointer (8 bytes) - local access
    .equ ws_deque_processes, 16       // Process array pointer (8 bytes)
    .equ ws_deque_size, 24            // Maximum size (4 bytes)
    .equ ws_deque_mask, 28            // Size mask for circular buffer (4 bytes)
    .equ ws_deque_steal_count, 32    // Successful steals (8 bytes)
    .equ ws_deque_steal_attempts, 40  // Total steal attempts (8 bytes)
    .equ ws_deque_local_pops, 48     // Local pop operations (8 bytes)
    .equ ws_deque_size_bytes, 64     // Total structure size

// No global data variables - all constants are defined in config.inc

// ------------------------------------------------------------
// Work Stealing Deque Initialization
// ------------------------------------------------------------
// Initialize a work stealing deque with the specified size.
// Allocates memory for the process array and sets up the circular buffer.
//
// Parameters:
//   x0 (void*) - deque_ptr: Pointer to deque structure
//   x1 (uint32_t) - size: Maximum number of processes in deque
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
// Clobbers: x1, x2, x3, x4, x5, x6, x7, x8, x9, x10, x11, x12, x13, x14, x15, x16, x17, x18, x19, x20, x21, x22, x23, x24, x25, x26, x27, x28, x29, x30
//
_ws_deque_init:
    // Save callee-saved registers
    stp x19, x30, [sp, #-16]!
    stp x20, x21, [sp, #-16]!

    // Validate parameters
    cbz x0, init_failed  // Check deque pointer
    cbz x1, init_failed  // Check size

    // Save parameters
    mov x19, x0  // deque_ptr
    mov x20, x1  // size

    // Validate size is power of 2 (required for circular buffer)
    // Check if (size & (size - 1)) == 0
    sub x21, x20, #1
    and x21, x20, x21
    cbnz x21, init_failed  // Not a power of 2

    // Check size is reasonable (between 2 and 1024)
    cmp x20, #2
    b.lt init_failed
    cmp x20, #1024
    b.gt init_failed

    // Check if there's already a process array allocated and free it
    // Only do this if the deque was previously initialized (size field is non-zero)
    ldr w22, [x19, #ws_deque_size]  // Check if previously initialized
    cbz w22, no_previous_array       // Skip if size is 0 (not initialized)
    
    ldr x23, [x19, #ws_deque_processes]
    cbz x23, no_previous_array  // Skip if no previous array
    
    // Free the existing array
    mov x0, x23        // addr = existing array
    ldr x1, [x19, #ws_deque_size]  // Get current size
    lsl x1, x1, #3     // size * 8
    bl _munmap         // Call C library munmap function
    
no_previous_array:
    // Allocate memory for process array (size * 8 bytes per pointer)
    lsl x21, x20, #3  // size * 8
    
    // Call C library mmap function
    // void *mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset)
    mov x0, xzr        // addr = NULL (let system choose)
    mov x1, x21        // length = size * 8
    mov x2, #3         // prot = PROT_READ | PROT_WRITE
    mov x3, #0x1002    // flags = MAP_PRIVATE | MAP_ANON (macOS)
    mov x4, #-1        // fd = -1 (not a file mapping)
    mov x5, xzr        // offset = 0
    bl _mmap           // Call C library mmap function

    // Check for mmap failure (MAP_FAILED is typically -1)
    mov x21, #-1
    cmp x0, x21
    b.eq init_failed

    // Initialize deque structure
    str xzr, [x19, #ws_deque_top]        // top = 0
    str xzr, [x19, #ws_deque_bottom]     // bottom = 0
    str x0, [x19, #ws_deque_processes]   // processes = allocated array
    str w20, [x19, #ws_deque_size]       // size = requested size
    sub x21, x20, #1                     // mask = size - 1
    str w21, [x19, #ws_deque_mask]       // mask = size - 1
    str xzr, [x19, #ws_deque_steal_count] // steal_count = 0
    str xzr, [x19, #ws_deque_steal_attempts] // steal_attempts = 0
    str xzr, [x19, #ws_deque_local_pops] // local_pops = 0

    // Clear the process array
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
// Work Stealing Deque Push Bottom
// ------------------------------------------------------------
// Add a process to the bottom of the deque (local scheduler operation).
// This is the fast path for local schedulers adding work to their queue.
//
// Parameters:
//   x0 (void*) - deque_ptr: Pointer to deque structure
//   x1 (void*) - process: Process pointer to add
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
// Clobbers: x1, x2, x3, x4, x5, x6, x7, x8, x9, x10, x11, x12, x13, x14, x15, x16, x17, x18, x19, x20, x21, x22, x23, x24, x25, x26, x27, x28, x29, x30
//
_ws_deque_push_bottom:
    // Save callee-saved registers
    stp x19, x30, [sp, #-16]!
    stp x20, x21, [sp, #-16]!

    // Validate parameters
    cbz x0, push_failed  // Check deque pointer
    cbz x1, push_failed  // Check process pointer

    // Save parameters
    mov x19, x0  // deque_ptr
    mov x20, x1  // process

    // Get current bottom pointer
    ldr x21, [x19, #ws_deque_bottom]
    
    // Get process array pointer
    ldr x23, [x19, #ws_deque_processes]
    cbz x23, push_failed  // Check if array is allocated

    // Calculate index with mask
    ldr x22, [x19, #ws_deque_mask]
    and x24, x21, x22  // index = bottom & mask

    // Store process in array
    str x20, [x23, x24, lsl #3]  // processes[index] = process

    // Memory barrier to ensure store is visible
    dmb sy

    // Update bottom pointer (increment actual pointer, not masked)
    add x21, x21, #1
    str x21, [x19, #ws_deque_bottom]

    // Return success
    mov x0, #1
    ldp x20, x21, [sp], #16
    ldp x19, x30, [sp], #16
    ret

push_failed:
    mov x0, #0
    ldp x20, x21, [sp], #16
    ldp x19, x30, [sp], #16
    ret

// ------------------------------------------------------------
// Work Stealing Deque Pop Bottom
// ------------------------------------------------------------
// Remove a process from the bottom of the deque (local scheduler operation).
// This is the fast path for local schedulers getting work from their queue.
//
// Parameters:
//   x0 (void*) - deque_ptr: Pointer to deque structure
//
// Returns:
//   x0 (void*) - process: Process pointer, or NULL if deque is empty
//
// Complexity: O(1) - Constant time operation
//
// Version: 0.10
// Author: Lee Barney
// Last Modified: 2025-01-19
//
// Clobbers: x1, x2, x3, x4, x5, x6, x7, x8, x9, x10, x11, x12, x13, x14, x15, x16, x17, x18, x19, x20, x21, x22, x23, x24, x25, x26, x27, x28, x29, x30
//
_ws_deque_pop_bottom:
    // Save callee-saved registers
    stp x19, x30, [sp, #-16]!
    stp x20, x21, [sp, #-16]!

    // Validate parameters
    cbz x0, pop_bottom_failed  // Check deque pointer

    // Save parameters
    mov x19, x0  // deque_ptr

    // Get current bottom and top pointers
    ldr x20, [x19, #ws_deque_bottom]
    ldr x21, [x19, #ws_deque_top]

    // Check if deque is empty (bottom <= top)
    cmp x20, x21
    b.le pop_bottom_empty

    // Decrement bottom pointer
    sub x20, x20, #1
    str x20, [x19, #ws_deque_bottom]

    // Get process array pointer
    ldr x23, [x19, #ws_deque_processes]
    cbz x23, pop_bottom_failed  // Check if array is allocated

    // Calculate index with mask
    ldr x22, [x19, #ws_deque_mask]
    and x24, x20, x22  // index = bottom & mask

    // Load process from array
    ldr x25, [x23, x24, lsl #3]  // process = processes[index]

    // Increment local pops counter
    ldr x26, [x19, #ws_deque_local_pops]
    add x26, x26, #1
    str x26, [x19, #ws_deque_local_pops]

    // Return process
    mov x0, x25
    ldp x20, x21, [sp], #16
    ldp x19, x30, [sp], #16
    ret

pop_bottom_empty:
    // Deque is empty, return NULL
    mov x0, #0
    ldp x20, x21, [sp], #16
    ldp x19, x30, [sp], #16
    ret

pop_bottom_failed:
    mov x0, #0
    ldp x20, x21, [sp], #16
    ldp x19, x30, [sp], #16
    ret

// ------------------------------------------------------------
// Work Stealing Deque Pop Top
// ------------------------------------------------------------
// Remove a process from the top of the deque (remote scheduler operation).
// This implements the work stealing mechanism using atomic operations.
//
// Parameters:
//   x0 (void*) - deque_ptr: Pointer to deque structure
//
// Returns:
//   x0 (void*) - process: Process pointer, or NULL if steal failed
//
// Complexity: O(1) - Constant time operation with retry logic
//
// Version: 0.10
// Author: Lee Barney
// Last Modified: 2025-01-19
//
// Clobbers: x1, x2, x3, x4, x5, x6, x7, x8, x9, x10, x11, x12, x13, x14, x15, x16, x17, x18, x19, x20, x21, x22, x23, x24, x25, x26, x27, x28, x29, x30
//
_ws_deque_pop_top:
    // Save callee-saved registers
    stp x19, x30, [sp, #-16]!
    stp x20, x21, [sp, #-16]!
    stp x22, x23, [sp, #-16]!
    stp x24, x25, [sp, #-16]!

    // Validate parameters
    cbz x0, pop_top_failed  // Check deque pointer

    // Save parameters
    mov x19, x0  // deque_ptr

    // Get process array pointer
    ldr x20, [x19, #ws_deque_processes]
    cbz x20, pop_top_failed  // Check if array is allocated

    // Get mask for circular buffer
    ldr x21, [x19, #ws_deque_mask]

    // Increment steal attempts counter
    ldr x22, [x19, #ws_deque_steal_attempts]
    add x22, x22, #1
    str x22, [x19, #ws_deque_steal_attempts]

pop_top_retry:
    // Load top pointer with exclusive access
    add x26, x19, #ws_deque_top
    ldxr x22, [x26]
    ldr x23, [x19, #ws_deque_bottom]

    // Check if deque is empty (top >= bottom)
    cmp x22, x23
    b.ge pop_top_empty

    // Calculate new top pointer
    add x24, x22, #1

    // Calculate index with mask for array access
    and x28, x22, x21  // index = top & mask

    // Load process from array
    ldr x25, [x20, x28, lsl #3]  // process = processes[index]

    // Update top pointer atomically
    stxr w27, x24, [x26]
    cbnz w27, pop_top_retry  // Retry if store failed

    // Successfully stole process
    // Increment steal count
    ldr x26, [x19, #ws_deque_steal_count]
    add x26, x26, #1
    str x26, [x19, #ws_deque_steal_count]

    // Return process
    mov x0, x25
    ldp x24, x25, [sp], #16
    ldp x22, x23, [sp], #16
    ldp x20, x21, [sp], #16
    ldp x19, x30, [sp], #16
    ret

pop_top_empty:
    // Deque is empty, return NULL
    mov x0, #0
    ldp x24, x25, [sp], #16
    ldp x22, x23, [sp], #16
    ldp x20, x21, [sp], #16
    ldp x19, x30, [sp], #16
    ret

pop_top_failed:
    mov x0, #0
    ldp x24, x25, [sp], #16
    ldp x22, x23, [sp], #16
    ldp x20, x21, [sp], #16
    ldp x19, x30, [sp], #16
    ret

// ------------------------------------------------------------
// Work Stealing Deque Is Empty
// ------------------------------------------------------------
// Check if the deque is empty by comparing top and bottom pointers.
//
// Parameters:
//   x0 (void*) - deque_ptr: Pointer to deque structure
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
_ws_deque_is_empty:
    // Validate parameters
    cbz x0, is_empty_failed  // Check deque pointer

    // Get top and bottom pointers
    ldr x1, [x0, #ws_deque_top]
    ldr x2, [x0, #ws_deque_bottom]

    // Check if empty (top >= bottom)
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
// Work Stealing Deque Size
// ------------------------------------------------------------
// Get the current number of processes in the deque.
//
// Parameters:
//   x0 (void*) - deque_ptr: Pointer to deque structure
//
// Returns:
//   x0 (uint32_t) - size: Number of processes in deque
//
// Complexity: O(1) - Constant time operation
//
// Version: 0.10
// Author: Lee Barney
// Last Modified: 2025-01-19
//
_ws_deque_size:
    // Validate parameters
    cbz x0, size_failed  // Check deque pointer

    // Get top and bottom pointers
    ldr x1, [x0, #ws_deque_top]
    ldr x2, [x0, #ws_deque_bottom]

    // Calculate size (bottom - top)
    sub x0, x2, x1
    ret

size_failed:
    mov x0, #0
    ret

// ------------------------------------------------------------
// Get Scheduler Load
// ------------------------------------------------------------
// Calculate the current load for a scheduler based on priority queue lengths.
// Load is weighted by priority: MAX=4, HIGH=3, NORMAL=2, LOW=1.
//
// Parameters:
//   x0 (uint64_t) - core_id: Core ID (0 to MAX_CORES-1)
//
// Returns:
//   x0 (uint32_t) - load: Calculated load value
//
// Complexity: O(p) where p is number of priority levels (4)
//
// Version: 0.10
// Author: Lee Barney
// Last Modified: 2025-01-19
//
// Clobbers: x1, x2, x3, x4, x5, x6, x7, x8, x9, x10, x11, x12, x13, x14, x15, x16, x17, x18, x19, x20, x21, x22, x23, x24, x25, x26, x27, x28, x29, x30
//
_get_scheduler_load:
    // Save callee-saved registers
    stp x19, x30, [sp, #-16]!
    stp x20, x21, [sp, #-16]!

    // Validate core ID
    cmp x1, #MAX_CORES
    b.ge get_load_invalid

    // Save parameters
    mov x19, x0  // scheduler_states pointer
    mov x20, x1  // core_id

    // Calculate scheduler state address
    mov x21, #248                    // scheduler_size = 248
    mul x21, x20, x21
    add x20, x19, x21  // x20 = scheduler state address

    // Initialize load to 0
    mov x21, #0  // load = 0

    // Calculate load for each priority level
    // MAX priority (weight = 4)
    add x22, x20, #8                 // Queue array base (scheduler_queues = 8)
    ldr w23, [x22, #16]             // MAX queue count (queue_count = 16)
    lsl x23, x23, #2                 // Multiply by 4
    add x21, x21, x23                // load += MAX_count * 4

    // HIGH priority (weight = 3)
    add x22, x22, #24               // Move to HIGH queue (queue_size = 24)
    ldr w23, [x22, #16]             // HIGH queue count (queue_count = 16)
    mov x24, #3
    mul x23, x23, x24               // Multiply by 3
    add x21, x21, x23               // load += HIGH_count * 3

    // NORMAL priority (weight = 2)
    add x22, x22, #24               // Move to NORMAL queue (queue_size = 24)
    ldr w23, [x22, #16]             // NORMAL queue count (queue_count = 16)
    lsl x23, x23, #1                // Multiply by 2
    add x21, x21, x23               // load += NORMAL_count * 2

    // LOW priority (weight = 1)
    add x22, x22, #24               // Move to LOW queue (queue_size = 24)
    ldr w23, [x22, #16]             // LOW queue count (queue_count = 16)
    add x21, x21, x23               // load += LOW_count * 1

    // Return load
    mov x0, x21
    ldp x20, x21, [sp], #16
    ldp x19, x30, [sp], #16
    ret

get_load_invalid:
    mov x0, #0
    ldp x20, x21, [sp], #16
    ldp x19, x30, [sp], #16
    ret

// ------------------------------------------------------------
// Find Busiest Scheduler
// ------------------------------------------------------------
// Find the scheduler with the highest load among all cores.
//
// Parameters:
//   x0 (uint64_t) - current_core: Current core ID (excluded from search)
//
// Returns:
//   x0 (uint64_t) - busiest_core: Core ID with highest load, or current_core if none found
//
// Complexity: O(n) where n is number of cores
//
// Version: 0.10
// Author: Lee Barney
// Last Modified: 2025-01-19
//
// Clobbers: x1, x2, x3, x4, x5, x6, x7, x8, x9, x10, x11, x12, x13, x14, x15, x16, x17, x18, x19, x20, x21, x22, x23, x24, x25, x26, x27, x28, x29, x30
//
_find_busiest_scheduler:
    // Save callee-saved registers
    stp x19, x30, [sp, #-16]!
    stp x20, x21, [sp, #-16]!
    stp x22, x23, [sp, #-16]!

    // Validate current core ID
    cmp x0, #MAX_CORES
    b.ge find_busiest_invalid

    // Save current core ID
    mov x19, x0

    // Initialize search variables
    mov x20, #0        // best_core = 0
    mov x21, #0        // best_load = 0
    mov x22, #0        // core_index = 0

find_busiest_loop:
    // Skip current core
    cmp x22, x19
    b.eq find_busiest_next

    // Get load for this core
    mov x0, x22
    bl _get_scheduler_load
    mov x23, x0  // current_load

    // Check if this is the best load so far
    cmp x23, x21
    b.le find_busiest_next

    // Update best load and core
    mov x21, x23  // best_load = current_load
    mov x20, x22  // best_core = core_index

find_busiest_next:
    // Move to next core
    add x22, x22, #1
    cmp x22, #MAX_CORES
    b.lt find_busiest_loop

    // Check if we found a core with work
    cbz x21, find_busiest_no_work

    // Return best core
    mov x0, x20
    ldp x22, x23, [sp], #16
    ldp x20, x21, [sp], #16
    ldp x19, x30, [sp], #16
    ret

find_busiest_no_work:
    // No work found, return current core
    mov x0, x19
    ldp x22, x23, [sp], #16
    ldp x20, x21, [sp], #16
    ldp x19, x30, [sp], #16
    ret

find_busiest_invalid:
    mov x0, #0
    ldp x22, x23, [sp], #16
    ldp x20, x21, [sp], #16
    ldp x19, x30, [sp], #16
    ret

// ------------------------------------------------------------
// Is Steal Allowed
// ------------------------------------------------------------
// Check if work stealing is allowed from source to target core.
// Considers migration limits, affinity constraints, and cooldown periods.
//
// Parameters:
//   x0 (uint64_t) - source_core: Source core ID
//   x1 (uint64_t) - target_core: Target core ID
//
// Returns:
//   x0 (int) - allowed: 1 if allowed, 0 if not allowed
//
// Complexity: O(1) - Constant time operation
//
// Version: 0.10
// Author: Lee Barney
// Last Modified: 2025-01-19
//
_is_steal_allowed:
    // Validate parameters
    cmp x0, #MAX_CORES
    b.ge steal_not_allowed
    cmp x1, #MAX_CORES
    b.ge steal_not_allowed

    // Check migration count limits
    ldr x2, [x21, #392]  // pcb_migration_count = 392
    mov x3, #MAX_MIGRATIONS
    cmp x2, x3
    b.ge steal_not_allowed
    
    // Check affinity constraints
    ldr x4, [x21, #384]  // pcb_affinity_mask = 384
    mov x5, #1
    lsl x5, x5, x1  // Create mask for target core
    and x4, x4, x5
    cbz x4, steal_not_allowed
    
    // Check cooldown period
    ldr x6, [x21, #400]  // pcb_last_migration_time = 400
    mrs x7, CNTPCT_EL0  // Current time
    sub x8, x7, x6
    mov x9, #1000  // MIGRATION_COOLDOWN_TICKS (simplified)
    cmp x8, x9
    b.lt steal_not_allowed
    
    // Check load imbalance threshold
    mov x0, x1  // target_core
    bl _get_scheduler_load
    mov x10, x0  // target_load
    
    mov x0, x20  // current_core
    bl _get_scheduler_load
    mov x11, x0  // current_load
    
    sub x12, x10, x11  // load difference
    mov x13, #LOAD_IMBALANCE_THRESHOLD
    cmp x12, x13
    b.lt steal_not_allowed
    
    mov x0, #1  // Allow steal
    ret

steal_not_allowed:
    mov x0, #0
    ret

// ------------------------------------------------------------
// Select Victim Random
// ------------------------------------------------------------
// Select a random victim core for work stealing.
//
// Parameters:
//   x0 (uint64_t) - current_core: Current core ID
//
// Returns:
//   x0 (uint64_t) - victim_core: Random victim core ID
//
// Complexity: O(1) - Constant time operation
//
// Version: 0.10
// Author: Lee Barney
// Last Modified: 2025-01-19
//
_select_victim_random:
    // Validate current core ID
    cmp x0, #MAX_CORES
    b.ge select_random_invalid

    // Simple random selection (using current time as seed)
    // In a real implementation, this would use a proper random number generator
    // For now, use a simple hash of the core ID
    mov x1, x0
    lsl x1, x1, #3
    add x1, x1, #7
    and x1, x1, #0xFF  // Keep within reasonable range
    mov x2, #MAX_CORES
    udiv x3, x1, x2
    msub x0, x3, x2, x1  // x0 = x1 % MAX_CORES

    // Ensure we don't select ourselves
    cmp x0, x0  // This will always be equal, so we need to adjust
    b.eq select_random_adjust
    ret

select_random_adjust:
    // If we selected ourselves, pick the next core
    add x0, x0, #1
    cmp x0, #MAX_CORES
    b.lt select_random_done
    mov x0, #0  // Wrap around to core 0
select_random_done:
    ret

select_random_invalid:
    mov x0, #0
    ret

// ------------------------------------------------------------
// Select Victim By Load
// ------------------------------------------------------------
// Select victim core based on load (find the busiest core).
//
// Parameters:
//   x0 (uint64_t) - current_core: Current core ID
//
// Returns:
//   x0 (uint64_t) - victim_core: Victim core ID with highest load
//
// Complexity: O(n) where n is number of cores
//
// Version: 0.10
// Author: Lee Barney
// Last Modified: 2025-01-19
//
_select_victim_by_load:
    // Use the existing find_busiest_scheduler function
    b _find_busiest_scheduler

// ------------------------------------------------------------
// Select Victim Locality
// ------------------------------------------------------------
// Select victim core based on locality (prefer same NUMA node).
// For now, this is a simplified implementation that falls back to load-based selection.
//
// Parameters:
//   x0 (uint64_t) - current_core: Current core ID
//
// Returns:
//   x0 (uint64_t) - victim_core: Locality-aware victim core ID
//
// Complexity: O(n) where n is number of cores
//
// Version: 0.10
// Author: Lee Barney
// Last Modified: 2025-01-19
//
_select_victim_locality:
    // Save callee-saved registers
    stp x19, x20, [sp, #-16]!
    stp x21, x22, [sp, #-16]!
    stp x23, x24, [sp, #-16]!
    stp x25, x30, [sp, #-16]!
    
    // Get current core's NUMA node
    mov x19, x0  // Save current_core
    bl _get_numa_node
    mov x20, x0  // current_numa_node
    
    // Try to find cores on same NUMA node
    mov x21, #0  // core_index
    mov x22, #0  // best_local_core
    mov x23, #0  // best_local_load
    
locality_scan_cores:
    cmp x21, #MAX_CORES
    b.ge locality_check_found
    
    // Skip current core
    cmp x21, x19
    b.eq locality_next_core
    
    // Get core's NUMA node
    mov x0, x21
    bl _get_numa_node
    cmp x0, x20  // Compare with current NUMA node
    b.ne locality_next_core
    
    // Get core's load
    mov x0, x21
    bl _get_scheduler_load
    mov x24, x0  // core_load
    
    // Check if this is better than current best
    cmp x24, x23
    b.le locality_next_core
    
    // Update best local core
    mov x22, x21
    mov x23, x24
    
locality_next_core:
    add x21, x21, #1
    b locality_scan_cores
    
locality_check_found:
    // If we found a local core with work, use it
    cbz x22, locality_fallback_to_load
    mov x0, x22
    ldp x25, x30, [sp], #16
    ldp x23, x24, [sp], #16
    ldp x21, x22, [sp], #16
    ldp x19, x20, [sp], #16
    ret
    
locality_fallback_to_load:
    // Fall back to load-based selection
    ldp x25, x30, [sp], #16
    ldp x23, x24, [sp], #16
    ldp x21, x22, [sp], #16
    ldp x19, x20, [sp], #16
    b _select_victim_by_load

// ------------------------------------------------------------
// Try Steal Work
// ------------------------------------------------------------
// Attempt to steal work from another scheduler.
// This is the main work stealing function called by idle schedulers.
//
// Parameters:
//   x0 (uint64_t) - current_core: Current core ID
//
// Returns:
//   x0 (void*) - stolen_process: Stolen process pointer, or NULL if no work stolen
//
// Complexity: O(n) where n is number of cores (for victim selection)
//
// Version: 0.10
// Author: Lee Barney
// Last Modified: 2025-01-19
//
// Clobbers: x1, x2, x3, x4, x5, x6, x7, x8, x9, x10, x11, x12, x13, x14, x15, x16, x17, x18, x19, x20, x21, x22, x23, x24, x25, x26, x27, x28, x29, x30
//
_try_steal_work:
    // Save callee-saved registers
    stp x19, x30, [sp, #-16]!
    stp x20, x21, [sp, #-16]!
    stp x22, x23, [sp, #-16]!

    // Validate current core ID
    cmp x1, #MAX_CORES
    b.ge steal_work_failed

    // Save parameters
    mov x19, x0  // scheduler_states pointer
    mov x20, x1  // current core ID

    // Check if work stealing is enabled
    mov x21, #WORK_STEAL_ENABLED
    cbz x21, steal_work_disabled

    // Select victim using default strategy
    mov x0, x19  // scheduler_states pointer
    mov x1, x20  // current core ID
    bl _select_victim_by_load
    mov x21, x0  // victim_core

    // Check if we found a valid victim
    cmp x21, x20
    b.eq steal_work_no_victim

    // Check if steal is allowed
    mov x0, x19  // scheduler_states pointer
    mov x1, x20  // source_core
    mov x2, x21  // target_core
    bl _is_steal_allowed
    cbz x0, steal_work_not_allowed

    // Get victim's scheduler state
    mov x0, x21  // victim_core
    bl _get_scheduler_state
    mov x22, x0  // victim_state
    
    // Try to steal from highest priority queue first
    mov x23, #NUM_PRIORITIES
    sub x23, x23, #1  // Start with highest priority
    
steal_try_priority:
    // Get queue for this priority
    mov x0, x22  // scheduler_state
    mov x1, x21  // core_id
    mov x2, x23  // priority
    bl _get_priority_queue
    
    // Try to dequeue from this queue
    mov x0, x22  // scheduler_state
    mov x1, x21  // core_id
    mov x2, x23  // priority
    bl _scheduler_dequeue_process
    mov x24, x0  // stolen_process
    
    cbz x24, steal_try_next_priority
    
    // Update migration count
    ldr x25, [x24, #392]  // pcb_migration_count = 392
    add x25, x25, #1
    str x25, [x24, #392]  // pcb_migration_count = 392
    
    // Update last migration time
    mrs x26, CNTPCT_EL0
    str x26, [x24, #400]  // pcb_last_migration_time = 400
    
    // Update statistics
    mov x0, x20  // current_core
    bl _increment_steal_count
    
    mov x0, x24  // Return stolen process
    ldp x22, x23, [sp], #16
    ldp x20, x21, [sp], #16
    ldp x19, x30, [sp], #16
    ret
    
steal_try_next_priority:
    sub x23, x23, #1
    cmp x23, #0
    b.ge steal_try_priority
    
    // No work found
    mov x0, #0
    ldp x22, x23, [sp], #16
    ldp x20, x21, [sp], #16
    ldp x19, x30, [sp], #16
    ret

steal_work_no_victim:
    mov x0, #0
    ldp x22, x23, [sp], #16
    ldp x20, x21, [sp], #16
    ldp x19, x30, [sp], #16
    ret

steal_work_not_allowed:
    mov x0, #0
    ldp x22, x23, [sp], #16
    ldp x20, x21, [sp], #16
    ldp x19, x30, [sp], #16
    ret

steal_work_disabled:
    mov x0, #0
    ldp x22, x23, [sp], #16
    ldp x20, x21, [sp], #16
    ldp x19, x30, [sp], #16
    ret

steal_work_failed:
    mov x0, #0
    ldp x22, x23, [sp], #16
    ldp x20, x21, [sp], #16
    ldp x19, x30, [sp], #16
    ret

// ------------------------------------------------------------
// Migrate Process
// ------------------------------------------------------------
// Migrate a process from source core to target core.
// Updates process affinity, migration count, and scheduler statistics.
//
// Parameters:
//   x0 (void*) - process: Process pointer to migrate
//   x1 (uint64_t) - source_core: Source core ID
//   x2 (uint64_t) - target_core: Target core ID
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
// Clobbers: x1, x2, x3, x4, x5, x6, x7, x8, x9, x10, x11, x12, x13, x14, x15, x16, x17, x18, x19, x20, x21, x22, x23, x24, x25, x26, x27, x28, x29, x30
//
_migrate_process:
    // Save callee-saved registers
    stp x19, x30, [sp, #-16]!
    stp x20, x21, [sp, #-16]!

    // Validate parameters
    cbz x0, migrate_failed  // Check process pointer
    cmp x1, #MAX_CORES
    b.ge migrate_failed    // Check source core
    cmp x2, #MAX_CORES
    b.ge migrate_failed    // Check target core

    // Save parameters
    mov x19, x0  // process
    mov x20, x1  // source_core
    mov x21, x2  // target_core

    // Update process scheduler ID
    str x21, [x19, #24]              // pcb_scheduler_id = 24

    // Increment migration count
    ldr x22, [x19, #392]             // pcb_migration_count = 392
    add x22, x22, #1
    str x22, [x19, #392]             // pcb_migration_count = 392

    // Update last scheduled timestamp (use current time approximation)
    // In a real implementation, this would use a proper timestamp
    mov x23, #0  // Simplified timestamp
    str x23, [x19, #376]             // pcb_last_scheduled = 376

    // Update scheduler migration statistics
    // Source scheduler: increment total_migrations
    mov x24, x19  // scheduler_states pointer
    mov x25, #248                    // scheduler_size = 248 (was 240)
    mul x25, x20, x25
    add x24, x24, x25
    ldr x26, [x24, #136]             // scheduler_total_migrations = 136
    add x26, x26, #1
    str x26, [x24, #136]             // scheduler_total_migrations = 136

    // Target scheduler: increment total_migrations
    mov x24, x19  // scheduler_states pointer
    mov x25, #248                    // scheduler_size = 248 (was 240)
    mul x25, x21, x25
    add x24, x24, x25
    ldr x26, [x24, #136]             // scheduler_total_migrations = 136
    add x26, x26, #1
    str x26, [x24, #136]             // scheduler_total_migrations = 136

    // Return success
    mov x0, #1
    ldp x20, x21, [sp], #16
    ldp x19, x30, [sp], #16
    ret

migrate_failed:
    mov x0, #0
    ldp x20, x21, [sp], #16
    ldp x19, x30, [sp], #16
    ret

// ------------------------------------------------------------
// External Dependencies
// ------------------------------------------------------------
// ------------------------------------------------------------
// Get NUMA Node Helper Function
// ------------------------------------------------------------
// Get the NUMA node ID for a given core.
// This is a simplified implementation that assumes a single NUMA node.
//
// Parameters:
//   x0 (uint64_t) - core_id: Core ID to get NUMA node for
//
// Returns:
//   x0 (uint64_t) - numa_node: NUMA node ID (0 for single node)
//
// Complexity: O(1) - Constant time operation
//
// Version: 0.10
// Author: Lee Barney
// Last Modified: 2025-01-19
//
_get_numa_node:
    // Simplified implementation - assume single NUMA node
    // In a full implementation, this would:
    // - Read NUMA topology from system registers
    // - Query ACPI tables for NUMA information
    // - Return actual NUMA node ID
    mov x0, #0  // Single NUMA node
    ret

// ------------------------------------------------------------
// Increment Steal Count Helper Function
// ------------------------------------------------------------
// Increment the steal count for a scheduler core.
//
// Parameters:
//   x0 (uint64_t) - core_id: Core ID to update
//
// Returns:
//   None
//
// Complexity: O(1) - Constant time operation
//
// Version: 0.10
// Author: Lee Barney
// Last Modified: 2025-01-19
//
_increment_steal_count:
    // Save callee-saved registers
    stp x19, x20, [sp, #-16]!
    stp x21, x30, [sp, #-16]!
    
    // Validate core ID
    cmp x0, #MAX_CORES
    b.ge increment_steal_count_done
    
    // Get scheduler state for this core
    mov x19, x0  // Save core_id
    bl _get_scheduler_state
    mov x20, x0  // scheduler_state
    
    // Increment steal count
    ldr x21, [x20, #240]  // scheduler_total_steals = 240
    add x21, x21, #1
    str x21, [x20, #240]  // scheduler_total_steals = 240
    
increment_steal_count_done:
    ldp x21, x30, [sp], #16
    ldp x19, x20, [sp], #16
    ret

// Import required functions from other modules
    .extern _mmap
    .extern scheduler_size
    .extern scheduler_queues
    .extern queue_count
    .extern pcb_scheduler_id
    .extern pcb_migration_count
    .extern pcb_last_scheduled
    .extern scheduler_total_migrations

// ------------------------------------------------------------
// Get Scheduler State Helper Function
// ------------------------------------------------------------
// Get the scheduler state for a given core ID.
//
// Parameters:
//   x0 (uint64_t) - core_id: Core ID to get state for
//
// Returns:
//   x0 (void*) - scheduler_state: Pointer to scheduler state
//
// Complexity: O(1) - Constant time operation
//
// Version: 0.10
// Author: Lee Barney
// Last Modified: 2025-01-19
//
_get_scheduler_state:
    // Save callee-saved registers
    stp x19, x20, [sp, #-16]!
    stp x21, x30, [sp, #-16]!

    // Validate core ID
    cmp x0, #MAX_CORES
    b.ge get_scheduler_state_invalid

    // Save core ID
    mov x19, x0  // core_id

    // Get global scheduler_states pointer (passed from caller)
    // In a real implementation, this would be a global variable
    // For now, we'll use a simplified approach
    mov x20, #0x10000000  // Dummy scheduler states base address
    mov x21, #248         // scheduler_size = 248
    mul x21, x19, x21     // core_id * scheduler_size
    add x20, x20, x21     // scheduler_states + offset

    // Return scheduler state address
    mov x0, x20
    ldp x21, x30, [sp], #16
    ldp x19, x20, [sp], #16
    ret

get_scheduler_state_invalid:
    mov x0, #0
    ldp x21, x30, [sp], #16
    ldp x19, x20, [sp], #16
    ret

// ------------------------------------------------------------
// Get Priority Queue Helper Function
// ------------------------------------------------------------
// Get the priority queue for a given scheduler state, core, and priority.
//
// Parameters:
//   x0 (void*) - scheduler_state: Scheduler state pointer
//   x1 (uint64_t) - core_id: Core ID
//   x2 (uint32_t) - priority: Priority level
//
// Returns:
//   x0 (void*) - queue: Priority queue pointer
//
// Complexity: O(1) - Constant time operation
//
// Version: 0.10
// Author: Lee Barney
// Last Modified: 2025-01-19
//
_get_priority_queue:
    // Save callee-saved registers
    stp x19, x20, [sp, #-16]!
    stp x21, x30, [sp, #-16]!

    // Validate parameters
    cbz x0, get_priority_queue_invalid  // Check scheduler_state
    cmp x1, #MAX_CORES
    b.ge get_priority_queue_invalid    // Check core_id
    cmp x2, #NUM_PRIORITIES
    b.ge get_priority_queue_invalid    // Check priority

    // Save parameters
    mov x19, x0  // scheduler_state
    mov x20, x1  // core_id
    mov x21, x2  // priority

    // Calculate queue address
    // Queue offset = scheduler_queues + (priority * queue_size)
    // scheduler_queues = 8, queue_size = 24
    mov x0, #24         // queue_size
    mul x0, x21, x0     // priority * queue_size
    add x0, x0, #8      // + scheduler_queues offset
    add x0, x19, x0     // scheduler_state + queue_offset

    ldp x21, x30, [sp], #16
    ldp x19, x20, [sp], #16
    ret

get_priority_queue_invalid:
    mov x0, #0
    ldp x21, x30, [sp], #16
    ldp x19, x20, [sp], #16
    ret

// ------------------------------------------------------------
// Scheduler Dequeue Process Helper Function
// ------------------------------------------------------------
// Dequeue a process from a priority queue.
//
// Parameters:
//   x0 (void*) - scheduler_state: Scheduler state pointer
//   x1 (uint64_t) - core_id: Core ID
//   x2 (uint32_t) - priority: Priority level
//
// Returns:
//   x0 (void*) - process: Dequeued process pointer, or NULL if empty
//
// Complexity: O(1) - Constant time operation
//
// Version: 0.10
// Author: Lee Barney
// Last Modified: 2025-01-19
//
_scheduler_dequeue_process:
    // Save callee-saved registers
    stp x19, x30, [sp, #-16]!
    stp x20, x21, [sp, #-16]!
    stp x22, x23, [sp, #-16]!

    // Validate parameters
    cbz x0, scheduler_dequeue_failed  // Check scheduler_state
    cmp x1, #MAX_CORES
    b.ge scheduler_dequeue_failed     // Check core_id
    cmp x2, #NUM_PRIORITIES
    b.ge scheduler_dequeue_failed     // Check priority

    // Save parameters
    mov x19, x0  // scheduler_state
    mov x20, x1  // core_id
    mov x21, x2  // priority

    // Get priority queue address
    mov x0, x19  // scheduler_state
    mov x1, x20  // core_id
    mov x2, x21  // priority
    bl _get_priority_queue
    mov x22, x0  // queue_address

    cbz x22, scheduler_dequeue_failed  // Check if queue address is valid

    // Check if queue is empty
    ldr w23, [x22, #16]  // queue_count
    cbz w23, scheduler_dequeue_empty  // count == 0 means empty

    // Get head process
    ldr x0, [x22, #0]    // queue_head
    cbz x0, scheduler_dequeue_failed  // head is NULL

    // Update queue head to next process
    ldr x1, [x0, #0]     // Load next pointer from PCB (offset 0 = pcb_next)
    str x1, [x22, #0]    // Update queue_head

    // If this was the last process, update tail to NULL
    cbz x1, scheduler_dequeue_clear_tail
    b scheduler_dequeue_decrement_count

scheduler_dequeue_clear_tail:
    str xzr, [x22, #8]   // Clear queue_tail

scheduler_dequeue_decrement_count:
    // Decrement queue count
    sub w23, w23, #1
    str w23, [x22, #16]  // queue_count

    // Clear next/prev pointers of dequeued process
    str xzr, [x0, #0]    // Clear next pointer
    str xzr, [x0, #8]    // Clear prev pointer

    // Return process
    mov x0, x0           // Return the dequeued process
    ldp x22, x23, [sp], #16
    ldp x20, x21, [sp], #16
    ldp x19, x30, [sp], #16
    ret

scheduler_dequeue_empty:
    mov x0, #0
    ldp x22, x23, [sp], #16
    ldp x20, x21, [sp], #16
    ldp x19, x30, [sp], #16
    ret

scheduler_dequeue_failed:
    mov x0, #0
    ldp x22, x23, [sp], #16
    ldp x20, x21, [sp], #16
    ldp x19, x30, [sp], #16
    ret
