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
//
// Version: 0.10
// Author: Lee Barney
// Last Modified: 2024-12-19
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
    // Initialize primary core
    mov x0, #0
    bl _runtime_init
    
    // Start scheduler
    bl _scheduler_init
    
    // Enter main loop
    bl _runtime_main_loop
    
    // Should never reach here
    b _runtime_halt

// ------------------------------------------------------------
// Secondary Core Entry Point
// ------------------------------------------------------------
// This is the entry point for secondary cores. Each core
// initializes its own scheduler instance.
//
    .global _secondary_core_start
_secondary_core_start:
    // Get core ID
    mrs x0, mpidr_el1
    and x0, x0, #0xFF
    
    // Initialize runtime for this core
    bl _runtime_init
    
    // Initialize scheduler for this core
    bl _scheduler_init
    
    // Enter scheduler loop
    bl _scheduler_main_loop
    
    // Should never reach here
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