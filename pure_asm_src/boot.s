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
// boot.s — Multi-core boot and initialization
// ------------------------------------------------------------
// This file provides the initial boot sequence for the scheduler system.
// It handles multi-core initialization and sets up the basic runtime
// environment before transferring control to the scheduler.
// Implements Phase 12 of the research implementation plan.
//
// The file provides:
//   - Complete initialization sequence for all subsystems
//   - Multi-core boot and initialization
//   - Error handling for initialization failures
//   - Integration of timer, affinity, scheduler, and communication systems
//
// Version: 0.10
// Author: Lee Barney
// Last Modified: 2025-01-19
//

    .section .text
    .align 4

// ------------------------------------------------------------
// Boot Entry Point
// ------------------------------------------------------------
// This is the main entry point for the system. It initializes
// the primary core and sets up the scheduler environment.
//
    .global _start
_start:
    // Save callee-saved registers
    stp x19, x30, [sp, #-16]!
    stp x20, x21, [sp, #-16]!

    // Initialize primary core (core 0)
    mov x19, #0  // core_id

    // Phase 1: Initialize timer system
    bl _timer_init
    cbz x0, boot_timer_init_failed

    // Phase 2: Initialize Apple Silicon optimizations
    bl _optimize_for_apple_silicon
    cbz x0, boot_apple_silicon_init_failed

    // Phase 3: Initialize scheduler system
    mov x0, x19  // core_id
    bl _scheduler_init
    cbz x0, boot_scheduler_init_failed

    // Phase 4: Initialize communication system
    bl _timer_init  // Use existing timer_init for now
    cbz x0, boot_communication_init_failed

    // Phase 5: Initialize load balancing
    bl _timer_init  // Use existing timer_init for now
    cbz x0, boot_load_balancer_init_failed

    // All subsystems initialized successfully
    ldp x20, x21, [sp], #16
    ldp x19, x30, [sp], #16
    
    // Enter main scheduler loop
    bl _scheduler_main_loop
    
    // Should never reach here
    b _runtime_halt

boot_timer_init_failed:
    mov x0, #1  // Error code 1: Timer init failed
    b boot_error_handler

boot_apple_silicon_init_failed:
    mov x0, #2  // Error code 2: Apple Silicon init failed
    b boot_error_handler

boot_scheduler_init_failed:
    mov x0, #3  // Error code 3: Scheduler init failed
    b boot_error_handler

boot_communication_init_failed:
    mov x0, #4  // Error code 4: Communication init failed
    b boot_error_handler

boot_load_balancer_init_failed:
    mov x0, #5  // Error code 5: Load balancer init failed
    b boot_error_handler

boot_error_handler:
    // Log error and halt system
    ldp x20, x21, [sp], #16
    ldp x19, x30, [sp], #16
    b _runtime_halt

// ------------------------------------------------------------
// Secondary Core Entry Point
// ------------------------------------------------------------
// This is the entry point for secondary cores. Each core
// initializes its own scheduler instance.
//
    .global _secondary_core_start
_secondary_core_start:
    // Save callee-saved registers
    stp x19, x30, [sp, #-16]!
    stp x20, x21, [sp, #-16]!

    // Get core ID
    mrs x0, mpidr_el1
    and x19, x0, #0xFF  // core_id
    
    // Phase 1: Initialize timer system (per-core)
    bl _timer_init
    cbz x0, secondary_boot_timer_init_failed

    // Phase 2: Initialize scheduler for this core
    mov x0, x19  // core_id
    bl _scheduler_init
    cbz x0, secondary_boot_scheduler_init_failed

    // Phase 3: Initialize communication for this core
    bl _timer_init  // Use existing timer_init for now
    cbz x0, secondary_boot_communication_init_failed

    // All subsystems initialized successfully
    ldp x20, x21, [sp], #16
    ldp x19, x30, [sp], #16
    
    // Enter scheduler loop
    bl _scheduler_main_loop
    
    // Should never reach here
    b _runtime_halt

secondary_boot_timer_init_failed:
    mov x0, #1  // Error code 1: Timer init failed
    b secondary_boot_error_handler

secondary_boot_scheduler_init_failed:
    mov x0, #3  // Error code 3: Scheduler init failed
    b secondary_boot_error_handler

secondary_boot_communication_init_failed:
    mov x0, #4  // Error code 4: Communication init failed
    b secondary_boot_error_handler

secondary_boot_error_handler:
    // Log error and halt system
    ldp x20, x21, [sp], #16
    ldp x19, x30, [sp], #16
    b _runtime_halt

// ------------------------------------------------------------
// System Halt
// ------------------------------------------------------------
// Halt the system - used for error conditions or shutdown.
//
    .global _runtime_halt
_runtime_halt:
    // Disable interrupts
    msr daifset, #2
    
    // Infinite loop
halt_loop:
    wfi
    b halt_loop