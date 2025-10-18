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
// test_yielding.c — Yielding Functions Test Suite
// ------------------------------------------------------------
// Comprehensive test suite for all yielding and preemption functions
// implemented in yield.s. Tests reduction-based preemption, voluntary
// yields, and conditional yields following BEAM behavior.
//
// Version: 0.10
// Author: Lee Barney
// Last Modified: 2025-01-19
//

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>

// External assembly functions
extern int process_yield_check(uint64_t core_id, void* pcb);
extern void* process_preempt(uint64_t core_id, void* pcb);
extern int process_decrement_reductions_with_check(uint64_t core_id);
extern void* process_yield(uint64_t core_id, void* pcb);
extern int process_yield_conditional(uint64_t core_id, void* pcb);

// External scheduler functions
extern void scheduler_init(uint64_t core_id);
extern void* scheduler_get_current_process(uint64_t core_id);
extern void scheduler_set_current_process(uint64_t core_id, void* process);
extern uint64_t scheduler_get_reduction_count(uint64_t core_id);
extern void scheduler_set_reduction_count(uint64_t core_id, uint64_t count);
extern int scheduler_enqueue_process(uint64_t core_id, void* process, uint64_t priority);

// External process functions
extern void* process_create(uint64_t entry_point, uint64_t priority, uint64_t stack_size, uint64_t heap_size);
extern void process_destroy(void* pcb);
extern uint64_t process_get_pid(void* pcb);
extern uint64_t process_get_priority(void* pcb);
extern uint64_t process_get_state(void* pcb);
extern void process_set_state(void* pcb, uint64_t state);

// External test framework functions
extern void test_assert_equal(uint64_t expected, uint64_t actual, const char* test_name);
extern void test_assert_not_equal(uint64_t value1, uint64_t value2, const char* test_name);
extern void test_assert_true(uint64_t value, const char* test_name);
extern void test_assert_false(uint64_t value, const char* test_name);
extern void test_assert_zero(uint64_t value, const char* test_name);
extern void test_assert_not_zero(uint64_t value, const char* test_name);

// External constants
extern const uint64_t DEFAULT_REDUCTIONS;
extern const uint64_t PROCESS_STATE_READY;
extern const uint64_t PROCESS_STATE_RUNNING;
extern const uint64_t PROCESS_STATE_WAITING;
extern const uint64_t PRIORITY_NORMAL;
extern const uint64_t PRIORITY_HIGH;

// Test process structure for testing
typedef struct {
    void* next;          // Offset 0: Next pointer in queue (8 bytes)
    void* prev;          // Offset 8: Previous pointer in queue (8 bytes)
    uint64_t pid;        // Offset 16: Process ID (8 bytes)
    uint64_t scheduler_id; // Offset 24: Scheduler ID (8 bytes)
    uint64_t state;      // Offset 32: Process state (8 bytes)
    uint64_t priority;   // Offset 40: Priority level (8 bytes)
    uint64_t reduction_count; // Offset 48: Reduction counter (8 bytes)
    uint64_t registers[31]; // Offset 56: x0-x30 register save area (31 * 8 = 248 bytes)
    uint64_t sp;         // Offset 304: Stack pointer (8 bytes)
    uint64_t lr;         // Offset 312: Link register (8 bytes)
    uint64_t pc;         // Offset 320: Program counter (8 bytes)
    uint64_t pstate;     // Offset 328: Processor state (8 bytes)
    uint64_t stack_base; // Offset 336: Stack base address (8 bytes)
    uint64_t stack_size; // Offset 344: Stack size (8 bytes)
    uint64_t heap_base;  // Offset 352: Heap base address (8 bytes)
    uint64_t heap_size;  // Offset 360: Heap size (8 bytes)
    void* message_queue; // Offset 368: Message queue pointer (8 bytes)
    uint64_t last_scheduled; // Offset 376: Last scheduled timestamp (8 bytes)
    uint64_t affinity_mask; // Offset 384: CPU affinity mask (8 bytes)
    uint64_t migration_count; // Offset 392: Migration count (8 bytes)
    uint64_t stack_pointer; // Offset 400: Current stack pointer (8 bytes)
    uint64_t stack_limit; // Offset 408: Stack limit (8 bytes)
    uint64_t heap_pointer; // Offset 416: Current heap pointer (8 bytes)
    uint64_t heap_limit; // Offset 424: Heap limit (8 bytes)
    uint64_t blocking_reason; // Offset 432: Blocking reason (8 bytes)
    uint64_t blocking_data; // Offset 440: Blocking data (8 bytes)
    uint64_t wake_time; // Offset 448: Timer wake time (8 bytes)
    uint64_t message_pattern; // Offset 456: Receive pattern (8 bytes)
    // Total size: 464 bytes
} test_process_t;

// Helper function to create a test process
void* create_yielding_test_process(uint64_t pid, uint64_t priority, uint64_t state) {
    test_process_t* pcb = (test_process_t*)malloc(512); // Allocate full PCB size
    if (pcb == NULL) {
        return NULL;
    }
    
    // Initialize the entire PCB to zero
    memset(pcb, 0, 512);
    
    // Set up the PCB fields
    pcb->pid = pid;
    pcb->scheduler_id = 0;
    pcb->state = state;
    pcb->priority = priority;
    pcb->reduction_count = 2000;
    pcb->stack_base = (uint64_t)pcb + 512; // Place stack after PCB
    pcb->stack_size = 8192;
    pcb->heap_base = (uint64_t)pcb + 512 + 8192; // Place heap after stack
    pcb->heap_size = 4096;
    pcb->affinity_mask = 0xFFFFFFFFFFFFFFFF; // All cores allowed
    
    return pcb;
}

// ------------------------------------------------------------
// Test Process Yield Check Function
// ------------------------------------------------------------
void test_process_yield_check(void) {
    printf("\n--- Testing process_yield_check (Reduction-based Preemption) ---\n");
    
    // Initialize scheduler
    scheduler_init(0);
    
    // Create test process
    void* pcb = create_yielding_test_process(1, PRIORITY_NORMAL, PROCESS_STATE_RUNNING);
    test_assert_not_zero((uint64_t)pcb, "test_process_creation");
    
    // Set as current process
    scheduler_set_current_process(0, pcb);
    
    // Set reduction count to 1 (should not yield)
    scheduler_set_reduction_count(0, 1);
    
    // Test yield check with reductions available
    int result = process_yield_check(0, pcb);
    test_assert_equal(0, result, "yield_check_with_reductions");
    
    // Set reduction count to 0 (should yield)
    scheduler_set_reduction_count(0, 0);
    
    // Test yield check with no reductions
    result = process_yield_check(0, pcb);
    test_assert_equal(1, result, "yield_check_no_reductions");
    
    // Test invalid core ID
    result = process_yield_check(128, pcb);
    test_assert_equal(0, result, "yield_check_invalid_core");
    
    // Test invalid PCB
    result = process_yield_check(0, NULL);
    test_assert_equal(0, result, "yield_check_invalid_pcb");
    
    // Cleanup
    free(pcb);
}

// ------------------------------------------------------------
// Test Process Preempt Function
// ------------------------------------------------------------
void test_process_preempt(void) {
    printf("\n--- Testing process_preempt (Force Preemption) ---\n");
    
    // Initialize scheduler
    scheduler_init(0);
    
    // Create test process
    void* pcb = create_yielding_test_process(1, PRIORITY_NORMAL, PROCESS_STATE_RUNNING);
    test_assert_not_zero((uint64_t)pcb, "test_process_creation");
    
    // Set as current process
    scheduler_set_current_process(0, pcb);
    
    // Test preemption
    void* next_process = process_preempt(0, pcb);
    test_assert_zero((uint64_t)next_process, "preempt_no_next_process");
    
    // Verify process state changed to READY
    uint64_t state = process_get_state(pcb);
    test_assert_equal(PROCESS_STATE_READY, state, "preempt_state_change");
    
    // Test invalid core ID
    next_process = process_preempt(128, pcb);
    test_assert_zero((uint64_t)next_process, "preempt_invalid_core");
    
    // Test invalid PCB
    next_process = process_preempt(0, NULL);
    test_assert_zero((uint64_t)next_process, "preempt_invalid_pcb");
    
    // Cleanup
    free(pcb);
}

// ------------------------------------------------------------
// Test Process Decrement Reductions with Check
// ------------------------------------------------------------
void test_process_decrement_reductions_with_check(void) {
    printf("\n--- Testing process_decrement_reductions_with_check (Combined Operation) ---\n");
    
    // Initialize scheduler
    scheduler_init(0);
    
    // Create test process
    void* pcb = create_yielding_test_process(1, PRIORITY_NORMAL, PROCESS_STATE_RUNNING);
    test_assert_not_zero((uint64_t)pcb, "test_process_creation");
    
    // Set as current process
    scheduler_set_current_process(0, pcb);
    
    // Set reduction count to 2
    scheduler_set_reduction_count(0, 2);
    
    // Test decrement with reductions available
    int result = process_decrement_reductions_with_check(0);
    test_assert_equal(0, result, "decrement_with_reductions");
    
    // Verify reduction count decreased
    uint64_t count = scheduler_get_reduction_count(0);
    test_assert_equal(1, count, "decrement_count_decreased");
    
    // Test decrement with no reductions (should preempt)
    result = process_decrement_reductions_with_check(0);
    test_assert_equal(1, result, "decrement_no_reductions");
    
    // Test invalid core ID
    result = process_decrement_reductions_with_check(128);
    test_assert_equal(0, result, "decrement_invalid_core");
    
    // Cleanup
    free(pcb);
}

// ------------------------------------------------------------
// Test Process Yield Function
// ------------------------------------------------------------
void test_process_yield_voluntary(void) {
    printf("\n--- Testing process_yield (Voluntary Yield) ---\n");
    
    // Initialize scheduler
    scheduler_init(0);
    
    // Create test process
    void* pcb = create_yielding_test_process(1, PRIORITY_NORMAL, PROCESS_STATE_RUNNING);
    test_assert_not_zero((uint64_t)pcb, "test_process_creation");
    
    // Set as current process
    scheduler_set_current_process(0, pcb);
    
    // Test voluntary yield
    void* next_process = process_yield(0, pcb);
    test_assert_zero((uint64_t)next_process, "yield_no_next_process");
    
    // Verify process state changed to READY
    uint64_t state = process_get_state(pcb);
    test_assert_equal(PROCESS_STATE_READY, state, "yield_state_change");
    
    // Test invalid core ID
    next_process = process_yield(128, pcb);
    test_assert_zero((uint64_t)next_process, "yield_invalid_core");
    
    // Test invalid PCB
    next_process = process_yield(0, NULL);
    test_assert_zero((uint64_t)next_process, "yield_invalid_pcb");
    
    // Cleanup
    free(pcb);
}

// ------------------------------------------------------------
// Test Process Yield Conditional Function
// ------------------------------------------------------------
void test_process_yield_conditional(void) {
    printf("\n--- Testing process_yield_conditional (Conditional Yield) ---\n");
    
    // Initialize scheduler
    scheduler_init(0);
    
    // Create test process
    void* pcb = create_yielding_test_process(1, PRIORITY_NORMAL, PROCESS_STATE_RUNNING);
    test_assert_not_zero((uint64_t)pcb, "test_process_creation");
    
    // Set as current process
    scheduler_set_current_process(0, pcb);
    
    // Test conditional yield with no other processes (should not yield)
    int result = process_yield_conditional(0, pcb);
    test_assert_equal(0, result, "yield_conditional_no_other_processes");
    
    // Add another process to ready queue
    void* pcb2 = create_yielding_test_process(2, PRIORITY_NORMAL, PROCESS_STATE_READY);
    test_assert_not_zero((uint64_t)pcb2, "test_process2_creation");
    
    int enqueue_result = scheduler_enqueue_process(0, pcb2, PRIORITY_NORMAL);
    test_assert_equal(1, enqueue_result, "enqueue_process2");
    
    // Test conditional yield with other processes (should yield)
    result = process_yield_conditional(0, pcb);
    test_assert_equal(1, result, "yield_conditional_with_other_processes");
    
    // Test invalid core ID
    result = process_yield_conditional(128, pcb);
    test_assert_equal(0, result, "yield_conditional_invalid_core");
    
    // Test invalid PCB
    result = process_yield_conditional(0, NULL);
    test_assert_equal(0, result, "yield_conditional_invalid_pcb");
    
    // Cleanup
    free(pcb);
    free(pcb2);
}

// ------------------------------------------------------------
// Test Reduction Counting Integration
// ------------------------------------------------------------
void test_reduction_counting_integration(void) {
    printf("\n--- Testing Reduction Counting Integration ---\n");
    
    // Initialize scheduler
    scheduler_init(0);
    
    // Create test process
    void* pcb = create_yielding_test_process(1, PRIORITY_NORMAL, PROCESS_STATE_RUNNING);
    test_assert_not_zero((uint64_t)pcb, "test_process_creation");
    
    // Set as current process
    scheduler_set_current_process(0, pcb);
    
    // Test multiple decrements
    for (int i = 0; i < 5; i++) {
        int result = process_decrement_reductions_with_check(0);
        if (i < 4) {
            test_assert_equal(0, result, "decrement_continued");
        } else {
            test_assert_equal(1, result, "decrement_preempted");
        }
    }
    
    // Cleanup
    free(pcb);
}

// ------------------------------------------------------------
// Test Yield with Scheduling Integration
// ------------------------------------------------------------
void test_yield_with_scheduling(void) {
    printf("\n--- Testing Yield with Scheduling Integration ---\n");
    
    // Initialize scheduler
    scheduler_init(0);
    
    // Create multiple test processes
    void* pcb1 = create_yielding_test_process(1, PRIORITY_NORMAL, PROCESS_STATE_RUNNING);
    void* pcb2 = create_yielding_test_process(2, PRIORITY_NORMAL, PROCESS_STATE_READY);
    void* pcb3 = create_yielding_test_process(3, PRIORITY_HIGH, PROCESS_STATE_READY);
    
    test_assert_not_zero((uint64_t)pcb1, "test_process1_creation");
    test_assert_not_zero((uint64_t)pcb2, "test_process2_creation");
    test_assert_not_zero((uint64_t)pcb3, "test_process3_creation");
    
    // Set first process as current
    scheduler_set_current_process(0, pcb1);
    
    // Add other processes to ready queue
    scheduler_enqueue_process(0, pcb2, PRIORITY_NORMAL);
    scheduler_enqueue_process(0, pcb3, PRIORITY_HIGH);
    
    // Test yield with multiple processes
    void* next_process = process_yield(0, pcb1);
    test_assert_not_zero((uint64_t)next_process, "yield_with_multiple_processes");
    
    // Cleanup
    free(pcb1);
    free(pcb2);
    free(pcb3);
}

// ------------------------------------------------------------
// Main Test Function
// ------------------------------------------------------------
// ------------------------------------------------------------
// test_process_yield_basic — Test basic process_yield functionality
// ------------------------------------------------------------
void test_process_yield_basic(void) {
    printf("\n--- Testing process_yield Basic Functionality ---\n");
    
    // Initialize scheduler
    scheduler_init(0);
    
    // Test with NULL PCB (should return NULL)
    void* result = process_yield(0, NULL);
    test_assert_equal((uint64_t)result, 0, "process_yield with NULL PCB should return NULL");
    
    // Test with invalid core ID (should return NULL)
    result = process_yield(128, NULL);
    test_assert_equal((uint64_t)result, 0, "process_yield with invalid core ID should return NULL");
    
    printf("✓ Basic process_yield tests passed\n");
}

// ------------------------------------------------------------
// test_process_yield_with_pcb — Test process_yield with proper PCB
// ------------------------------------------------------------
void test_process_yield_with_pcb(void) {
    printf("\n--- Testing process_yield with Proper PCB ---\n");
    
    // Initialize scheduler
    scheduler_init(0);
    
    // Create a proper PCB structure (simplified for testing)
    typedef struct {
        uint64_t pid;
        uint64_t state;
        uint64_t priority;
        uint64_t reduction_count;
        uint64_t sp;
        uint64_t lr;
        uint64_t pc;
        uint64_t pstate;
        uint64_t stack_base;
        uint64_t stack_size;
        uint64_t heap_base;
        uint64_t heap_size;
        uint64_t size;
        uint64_t padding[50]; // Padding to reach ~520 bytes
    } test_pcb_t;
    
    test_pcb_t* pcb = (test_pcb_t*)malloc(sizeof(test_pcb_t));
    test_assert_not_zero((uint64_t)pcb, "PCB allocation should succeed");
    
    if (pcb) {
        // Initialize PCB
        memset(pcb, 0, sizeof(test_pcb_t));
        pcb->pid = 1;
        pcb->state = 2; // PROCESS_STATE_RUNNING
        pcb->priority = 2; // PRIORITY_NORMAL
        pcb->reduction_count = 2000; // DEFAULT_REDUCTIONS
        pcb->size = sizeof(test_pcb_t);
        
        // Set as current process
        scheduler_set_current_process(0, pcb);
        
        // Test process_yield
        void* result = process_yield(0, pcb);
        test_assert_not_zero((uint64_t)result, "process_yield should return a valid result");
        
        // Cleanup
        free(pcb);
    }
    
    printf("✓ process_yield with PCB tests passed\n");
}

// ------------------------------------------------------------
// Main Test Function
// ------------------------------------------------------------
void test_yielding_main(void) {
    printf("\n=== YIELDING FUNCTIONS TEST SUITE ===\n");
    
    test_process_yield_basic();
    test_process_yield_with_pcb();
    
    printf("\n=== YIELDING FUNCTIONS TEST SUITE COMPLETE ===\n");
}
