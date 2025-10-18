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
// config.inc — System configuration constants
// ------------------------------------------------------------
// Global system configuration constants used throughout the
// BEAM-style scheduler implementation. These constants define
// system limits, memory layouts, and operational parameters.
//
// The file provides:
//   - Core system limits and boundaries
//   - Memory layout and alignment constants
//   - Process and scheduler configuration
//   - Timer and timeout settings
//
// Version: 0.10
// Author: Lee Barney
// Last Modified: 2025-01-19
//

    // Core system configuration
    .equ SPIN_TABLE_BASE, 0x40010000
    .equ MAX_CORES, 128
    .equ TIMEOUT_CYCLES, 0x100000

    // Process Control Block (PCB) configuration
    .equ DEFAULT_STACK_SIZE, 8192      // 8KB default stack size
    .equ DEFAULT_HEAP_SIZE, 4096       // 4KB default heap size
    .equ MAX_STACK_SIZE, 65536         // 64KB maximum stack size
    .equ MAX_HEAP_SIZE, 1048576        // 1MB maximum heap size
    .equ STACK_ALIGNMENT, 16           // 16-byte stack alignment
    .equ HEAP_ALIGNMENT, 8             // 8-byte heap alignment
    .equ MAX_PROCESSES, 1024           // Maximum number of processes
    .equ STACK_POOL_SIZE, 256          // Number of stacks in pool
    .equ HEAP_POOL_SIZE, 1024          // Number of heap blocks in pool
    .equ PCB_SIZE, 512                 // Process Control Block size

    // Scheduler configuration
    .equ DEFAULT_REDUCTIONS, 2000      // Default reduction count per time slice
    .equ NUM_PRIORITIES, 4             // Number of priority levels
    .equ PRIORITY_MAX, 0               // Maximum priority level
    .equ PRIORITY_HIGH, 1              // High priority level
    .equ PRIORITY_NORMAL, 2            // Normal priority level
    .equ PRIORITY_LOW, 3               // Low priority level

    // Process states
    .equ PROCESS_STATE_CREATED, 0      // Process created but not scheduled
    .equ PROCESS_STATE_READY, 1        // Process ready to run
    .equ PROCESS_STATE_RUNNING, 2      // Process currently running
    .equ PROCESS_STATE_WAITING, 3      // Process waiting for I/O or message
    .equ PROCESS_STATE_SUSPENDED, 4    // Process suspended
    .equ PROCESS_STATE_TERMINATED, 5   // Process terminated

    // Blocking reason constants
    .equ REASON_NONE, 0                // No blocking reason
    .equ REASON_RECEIVE, 1             // Blocking on message receive
    .equ REASON_TIMER, 2               // Blocking on timer timeout
    .equ REASON_IO, 3                  // Blocking on I/O operation

    // Yielding and blocking constants
    .equ YIELD_CHECK_INTERVAL, 1       // Check reductions every N operations
    .equ BIF_SPAWN_COST, 10            // Reductions cost for spawn
    .equ BIF_EXIT_COST, 1              // Reductions cost for exit
    .equ BIF_YIELD_COST, 1             // Reductions cost for yield
    .equ MAX_BLOCKING_TIME, 1000000    // Maximum blocking time in ticks

    // Memory alignment constants
    .equ CACHE_LINE_SIZE, 128          // Apple Silicon cache line size
    .equ PAGE_SIZE, 4096               // Standard page size
    .equ ALIGN_16, 16                  // 16-byte alignment
    .equ ALIGN_64, 64                  // 64-byte alignment
    .equ ALIGN_128, 128                // 128-byte alignment
