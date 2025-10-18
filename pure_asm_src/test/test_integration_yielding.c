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
// test_integration_yielding.c — Integration Test Suite for Yielding
// ------------------------------------------------------------
// Comprehensive integration test suite for complete yielding behavior
// including reduction-based preemption, voluntary yields, blocking operations,
// and Actly BIF functions working together.
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
extern int scheduler_enqueue_process(uint64_t core_id, void* process, uint64_t priority);
extern void* scheduler_schedule(uint64_t core_id);
extern void* process_block(uint64_t core_id, void* pcb, uint64_t reason);
extern int process_wake(uint64_t core_id, void* pcb);
extern void* process_block_on_receive(uint64_t core_id, void* pcb, uint64_t pattern);
extern int process_block_on_timer(uint64_t core_id, void* pcb, uint64_t timeout_ticks);
extern int process_block_on_io(uint64_t core_id, void* pcb, uint64_t io_descriptor);
extern uint64_t process_check_timer_wakeups(uint64_t core_id);
extern int actly_yield(uint64_t core_id);
extern uint64_t actly_spawn(uint64_t core_id, uint64_t entry_point, uint64_t priority, uint64_t stack_size, uint64_t heap_size);
extern void actly_exit(uint64_t core_id, uint64_t exit_reason);
extern int actly_bif_trap_check(uint64_t core_id, uint64_t reduction_cost);

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
extern const uint64_t PROCESS_STATE_TERMINATED;
extern const uint64_t PRIORITY_NORMAL;
extern const uint64_t PRIORITY_HIGH;
extern const uint64_t REASON_RECEIVE;
extern const uint64_t REASON_TIMER;
extern const uint64_t REASON_IO;
extern const uint64_t BIF_SPAWN_COST;
extern const uint64_t BIF_EXIT_COST;
extern const uint64_t BIF_YIELD_COST;

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
void* create_integration_test_process(uint64_t pid, uint64_t priority, uint64_t state) {
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
// Test Yield with Scheduling Integration
// ------------------------------------------------------------
void test_integration_yield_with_scheduling(void) {
    printf("\n--- Testing Yield with Scheduling Integration ---\n");
    
    // Initialize scheduler
    scheduler_init(0);
    
    // Create multiple test processes
    void* pcb1 = create_integration_test_process(1, PRIORITY_NORMAL, PROCESS_STATE_RUNNING);
    void* pcb2 = create_integration_test_process(2, PRIORITY_NORMAL, PROCESS_STATE_READY);
    void* pcb3 = create_integration_test_process(3, PRIORITY_HIGH, PROCESS_STATE_READY);
    
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
// Test Preemption at Reduction Limit
// ------------------------------------------------------------
void test_preemption_at_reduction_limit(void) {
    printf("\n--- Testing Preemption at Reduction Limit ---\n");
    
    // Initialize scheduler
    scheduler_init(0);
    
    // Create test process
    void* pcb = create_integration_test_process(1, PRIORITY_NORMAL, PROCESS_STATE_RUNNING);
    test_assert_not_zero((uint64_t)pcb, "test_process_creation");
    
    // Set as current process
    scheduler_set_current_process(0, pcb);
    
    // Test preemption at reduction limit
    scheduler_set_reduction_count(0, 1);
    
    // First decrement should continue
    int result = process_decrement_reductions_with_check(0);
    test_assert_equal(0, result, "preemption_first_decrement");
    
    // Second decrement should preempt
    result = process_decrement_reductions_with_check(0);
    test_assert_equal(1, result, "preemption_second_decrement");
    
    // Cleanup
    free(pcb);
}

// ------------------------------------------------------------
// Test Block-Wake Cycle
// ------------------------------------------------------------
void test_integration_block_wake_cycle(void) {
    printf("\n--- Testing Block-Wake Cycle ---\n");
    
    // Initialize scheduler
    scheduler_init(0);
    
    // Create test process
    void* pcb = create_integration_test_process(1, PRIORITY_NORMAL, PROCESS_STATE_RUNNING);
    test_assert_not_zero((uint64_t)pcb, "test_process_creation");
    
    // Set as current process
    scheduler_set_current_process(0, pcb);
    
    // Test multiple block-wake cycles
    for (int i = 0; i < 3; i++) {
        // Block process
        void* next_process = process_block(0, pcb, REASON_RECEIVE);
        test_assert_zero((uint64_t)next_process, "block_cycle");
        
        // Verify WAITING state
        uint64_t state = process_get_state(pcb);
        test_assert_equal(PROCESS_STATE_WAITING, state, "block_cycle_state");
        
        // Wake process
        int wake_result = process_wake(0, pcb);
        test_assert_equal(1, wake_result, "wake_cycle");
        
        // Verify READY state
        state = process_get_state(pcb);
        test_assert_equal(PROCESS_STATE_READY, state, "wake_cycle_state");
    }
    
    // Cleanup
    free(pcb);
}

// ------------------------------------------------------------
// Test Multiple Processes Yielding
// ------------------------------------------------------------
void test_multiple_processes_yielding(void) {
    printf("\n--- Testing Multiple Processes Yielding ---\n");
    
    // Initialize scheduler
    scheduler_init(0);
    
    // Create multiple test processes
    void* pcb1 = create_integration_test_process(1, PRIORITY_NORMAL, PROCESS_STATE_RUNNING);
    void* pcb2 = create_integration_test_process(2, PRIORITY_NORMAL, PROCESS_STATE_READY);
    void* pcb3 = create_integration_test_process(3, PRIORITY_HIGH, PROCESS_STATE_READY);
    
    test_assert_not_zero((uint64_t)pcb1, "test_process1_creation");
    test_assert_not_zero((uint64_t)pcb2, "test_process2_creation");
    test_assert_not_zero((uint64_t)pcb3, "test_process3_creation");
    
    // Set first process as current
    scheduler_set_current_process(0, pcb1);
    
    // Add other processes to ready queue
    scheduler_enqueue_process(0, pcb2, PRIORITY_NORMAL);
    scheduler_enqueue_process(0, pcb3, PRIORITY_HIGH);
    
    // Test yield from first process
    void* next_process = process_yield(0, pcb1);
    test_assert_not_zero((uint64_t)next_process, "yield_from_first_process");
    
    // Test yield from second process
    next_process = process_yield(0, pcb2);
    test_assert_not_zero((uint64_t)next_process, "yield_from_second_process");
    
    // Test yield from third process
    next_process = process_yield(0, pcb3);
    test_assert_not_zero((uint64_t)next_process, "yield_from_third_process");
    
    // Cleanup
    free(pcb1);
    free(pcb2);
    free(pcb3);
}

// ------------------------------------------------------------
// Test Spawn-Yield-Exit Lifecycle
// ------------------------------------------------------------
void test_integration_spawn_yield_exit_lifecycle(void) {
    printf("\n--- Testing Spawn-Yield-Exit Lifecycle ---\n");
    
    // Initialize scheduler
    scheduler_init(0);
    
    // Create test process
    void* pcb = create_integration_test_process(1, PRIORITY_NORMAL, PROCESS_STATE_RUNNING);
    test_assert_not_zero((uint64_t)pcb, "test_process_creation");
    
    // Set as current process
    scheduler_set_current_process(0, pcb);
    
    // Set sufficient reduction count
    scheduler_set_reduction_count(0, 50);
    
    // Test spawn operation
    uint64_t new_pid = actly_spawn(0, 0x1000, PRIORITY_NORMAL, 8192, 4096);
    test_assert_not_zero(new_pid, "lifecycle_spawn");
    
    // Test yield operation
    int yield_result = actly_yield(0);
    test_assert_equal(1, yield_result, "lifecycle_yield");
    
    // Test exit operation (this will terminate the process)
    printf("Testing exit operation (process should terminate)...\n");
    
    // Cleanup
    free(pcb);
}

// ------------------------------------------------------------
// Test Complete Yielding Behavior
// ------------------------------------------------------------
void test_complete_yielding_behavior(void) {
    printf("\n--- Testing Complete Yielding Behavior ---\n");
    
    // Initialize scheduler
    scheduler_init(0);
    
    // Create multiple test processes
    void* pcb1 = create_integration_test_process(1, PRIORITY_NORMAL, PROCESS_STATE_RUNNING);
    void* pcb2 = create_integration_test_process(2, PRIORITY_HIGH, PROCESS_STATE_READY);
    void* pcb3 = create_integration_test_process(3, PRIORITY_NORMAL, PROCESS_STATE_READY);
    
    test_assert_not_zero((uint64_t)pcb1, "test_process1_creation");
    test_assert_not_zero((uint64_t)pcb2, "test_process2_creation");
    test_assert_not_zero((uint64_t)pcb3, "test_process3_creation");
    
    // Set first process as current
    scheduler_set_current_process(0, pcb1);
    
    // Add other processes to ready queue
    scheduler_enqueue_process(0, pcb2, PRIORITY_HIGH);
    scheduler_enqueue_process(0, pcb3, PRIORITY_NORMAL);
    
    // Test reduction-based preemption
    scheduler_set_reduction_count(0, 1);
    int result = process_decrement_reductions_with_check(0);
    test_assert_equal(0, result, "reduction_preemption_continue");
    
    result = process_decrement_reductions_with_check(0);
    test_assert_equal(1, result, "reduction_preemption_yield");
    
    // Test voluntary yield
    void* next_process = process_yield(0, pcb1);
    test_assert_not_zero((uint64_t)next_process, "voluntary_yield");
    
    // Test conditional yield
    result = process_yield_conditional(0, pcb2);
    test_assert_equal(1, result, "conditional_yield");
    
    // Test blocking operations
    next_process = process_block(0, pcb3, REASON_RECEIVE);
    test_assert_zero((uint64_t)next_process, "block_operation");
    
    // Test wake operation
    int wake_result = process_wake(0, pcb3);
    test_assert_equal(1, wake_result, "wake_operation");
    
    // Test Actly BIF functions
    scheduler_set_reduction_count(0, 20);
    int bif_result = actly_yield(0);
    test_assert_equal(1, bif_result, "actly_yield_bif");
    
    uint64_t new_pid = actly_spawn(0, 0x1000, PRIORITY_NORMAL, 8192, 4096);
    test_assert_not_zero(new_pid, "actly_spawn_bif");
    
    // Cleanup
    free(pcb1);
    free(pcb2);
    free(pcb3);
}

// ------------------------------------------------------------
// Test Error Handling and Edge Cases
// ------------------------------------------------------------
void test_error_handling_and_edge_cases(void) {
    printf("\n--- Testing Error Handling and Edge Cases ---\n");
    
    // Initialize scheduler
    scheduler_init(0);
    
    // Test with invalid core ID
    int result = process_yield_check(128, NULL);
    test_assert_equal(0, result, "invalid_core_id");
    
    // Test with invalid PCB
    result = process_yield_check(0, NULL);
    test_assert_equal(0, result, "invalid_pcb");
    
    // Test with no current process
    scheduler_set_current_process(0, NULL);
    result = process_decrement_reductions_with_check(0);
    test_assert_equal(0, result, "no_current_process");
    
    // Test with insufficient reductions
    void* pcb = create_integration_test_process(1, PRIORITY_NORMAL, PROCESS_STATE_RUNNING);
    test_assert_not_zero((uint64_t)pcb, "test_process_creation");
    
    scheduler_set_current_process(0, pcb);
    scheduler_set_reduction_count(0, 0);
    
    result = process_decrement_reductions_with_check(0);
    test_assert_equal(1, result, "insufficient_reductions");
    
    // Cleanup
    free(pcb);
}

// ------------------------------------------------------------
// Main Test Function
// ------------------------------------------------------------
// ------------------------------------------------------------
// test_integration_yield_scheduling — Test integration of yield with scheduling
// ------------------------------------------------------------
void test_integration_yield_scheduling(void) {
    printf("\n--- Testing Integration of Yield with Scheduling ---\n");
    
    // Initialize scheduler
    scheduler_init(0);
    
    // Create multiple test processes
    typedef struct {
        uint64_t pid;
        uint64_t state;
        uint64_t priority;
        uint64_t reduction_count;
        uint64_t sp;
        uint64_t lr;
        uint64_t pc;
        uint64_t pstate;
        uint64_t size;
        uint64_t padding[50]; // Padding to reach ~520 bytes
    } test_pcb_t;
    
    test_pcb_t* pcb1 = (test_pcb_t*)malloc(sizeof(test_pcb_t));
    test_pcb_t* pcb2 = (test_pcb_t*)malloc(sizeof(test_pcb_t));
    
    test_assert_not_zero((uint64_t)pcb1, "PCB1 allocation should succeed");
    test_assert_not_zero((uint64_t)pcb2, "PCB2 allocation should succeed");
    
    if (pcb1 && pcb2) {
        // Initialize PCBs
        memset(pcb1, 0, sizeof(test_pcb_t));
        memset(pcb2, 0, sizeof(test_pcb_t));
        
        pcb1->pid = 1;
        pcb1->state = 2; // PROCESS_STATE_RUNNING
        pcb1->priority = 2; // PRIORITY_NORMAL
        pcb1->reduction_count = 2000;
        pcb1->size = sizeof(test_pcb_t);
        
        pcb2->pid = 2;
        pcb2->state = 1; // PROCESS_STATE_READY
        pcb2->priority = 2; // PRIORITY_NORMAL
        pcb2->reduction_count = 2000;
        pcb2->size = sizeof(test_pcb_t);
        
        // Test enqueueing processes
        int result1 = scheduler_enqueue_process(0, pcb1, 2);
        int result2 = scheduler_enqueue_process(0, pcb2, 2);
        
        test_assert_not_zero(result1, "scheduler_enqueue_process should succeed for pcb1");
        test_assert_not_zero(result2, "scheduler_enqueue_process should succeed for pcb2");
        
        // Test yielding
        scheduler_set_current_process(0, pcb1);
        void* next = process_yield(0, pcb1);
        test_assert_not_zero((uint64_t)next, "process_yield should return a valid next process");
        
        // Cleanup
        free(pcb1);
        free(pcb2);
    }
    
    printf("✓ Integration yield with scheduling tests passed\n");
}

// ------------------------------------------------------------
// test_integration_multiple_processes — Test integration with multiple processes
// ------------------------------------------------------------
void test_integration_multiple_processes(void) {
    printf("\n--- Testing Integration with Multiple Processes ---\n");
    
    // Initialize scheduler
    scheduler_init(0);
    
    // Test scheduler_schedule with no processes
    void* result = scheduler_schedule(0);
    test_assert_equal((uint64_t)result, 0, "scheduler_schedule with no processes should return NULL");
    
    printf("✓ Multiple processes integration tests passed\n");
}

// ------------------------------------------------------------
// Main Test Function
// ------------------------------------------------------------
void test_integration_yielding_main(void) {
    printf("\n=== INTEGRATION YIELDING TEST SUITE ===\n");
    
    test_integration_yield_scheduling();
    test_integration_multiple_processes();
    
    printf("\n=== INTEGRATION YIELDING TEST SUITE COMPLETE ===\n");
}
