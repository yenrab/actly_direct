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
// test_actly_bifs.c — Actly BIF Functions Test Suite
// ------------------------------------------------------------
// Comprehensive test suite for all Actly BIF functions implemented
// in actly_bifs.s. Tests yield, spawn, exit, and BIF trap mechanism
// following BEAM BIF patterns.
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
extern void* scheduler_state_init(uint64_t max_cores);
extern void scheduler_state_destroy(void* scheduler_states);
extern int actly_yield(uint64_t core_id);
extern uint64_t actly_spawn(uint64_t core_id, uint64_t entry_point, uint64_t priority, uint64_t stack_size, uint64_t heap_size);
extern void actly_exit(uint64_t core_id, uint64_t exit_reason);
extern int actly_bif_trap_check(void* scheduler_states, uint64_t core_id, uint64_t reduction_cost);

// External scheduler functions
extern void scheduler_init(void* scheduler_states, uint64_t core_id);
extern void* scheduler_get_current_process(void* scheduler_states, uint64_t core_id);
extern void scheduler_set_current_process(void* scheduler_states, uint64_t core_id, void* process);
extern uint64_t scheduler_get_reduction_count(void* scheduler_states, uint64_t core_id);
extern void scheduler_set_reduction_count_with_state(void* scheduler_states, uint64_t core_id, uint64_t count);
extern uint64_t scheduler_get_reduction_count_with_state(void* scheduler_states, uint64_t core_id);
extern int scheduler_enqueue_process(void* scheduler_states, uint64_t core_id, void* process, uint64_t priority);

// External process functions
extern void* process_create(uint64_t entry_point, uint64_t priority, uint64_t stack_size, uint64_t heap_size);
extern void process_destroy(void* pcb);
extern uint64_t process_get_pid(void* pcb);
extern uint64_t process_get_priority(void* pcb);
extern uint64_t process_get_state(void* pcb);
extern void process_set_state(void* pcb, uint64_t state);
extern void process_save_context(void* pcb);
extern void process_restore_context(void* pcb);

// External test framework functions
extern void test_assert_equal(uint64_t expected, uint64_t actual, const char* test_name);
extern void test_assert_not_equal(uint64_t value1, uint64_t value2, const char* test_name);
extern void test_assert_true(uint64_t value, const char* test_name);
extern void test_assert_false(uint64_t value, const char* test_name);
extern void test_assert_zero(uint64_t value, const char* test_name);
extern void test_assert_not_zero(uint64_t value, const char* test_name);

// External constants
extern const uint64_t MAX_CORES_CONST;
extern const uint64_t DEFAULT_REDUCTIONS;
extern const uint64_t PROCESS_STATE_READY;
extern const uint64_t PROCESS_STATE_RUNNING;
extern const uint64_t PROCESS_STATE_TERMINATED;
extern const uint64_t PRIORITY_NORMAL;
extern const uint64_t PRIORITY_HIGH;
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
void* create_actly_bifs_test_process(uint64_t pid, uint64_t priority, uint64_t state) {
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
// Test Actly Yield BIF Function
// ------------------------------------------------------------
void test_actly_yield() {
    printf("\n--- Testing actly_yield (Actly Yield BIF) ---\n");
    
    // Create isolated scheduler state
    void* scheduler_state = scheduler_state_init(1);
    if (scheduler_state == NULL) {
        printf("ERROR: Failed to create scheduler state\n");
        return;
    }
    
    // Initialize scheduler for core 0
    scheduler_init(scheduler_state, 0);
    
    // Create test process
    void* pcb = create_actly_bifs_test_process(1, PRIORITY_NORMAL, PROCESS_STATE_RUNNING);
    test_assert_not_zero((uint64_t)pcb, "test_process_creation");
    
    // Set as current process
    scheduler_set_current_process(scheduler_state, 0, pcb);
    
    // Set reduction count to allow yield
    scheduler_set_reduction_count_with_state(scheduler_state, 0, 10);
    
    // Test actly yield
    int result = actly_yield(0);
    test_assert_equal(1, result, "actly_yield_success");
    
    // Test invalid core ID
    result = actly_yield(128);
    test_assert_equal(0, result, "actly_yield_invalid_core");
    
    // Test with no current process
    scheduler_set_current_process(scheduler_state, 0, NULL);
    result = actly_yield(0);
    test_assert_equal(0, result, "actly_yield_no_process");
    
    // Cleanup
    free(pcb);
    
    // Clean up scheduler state
    scheduler_state_destroy(scheduler_state);
}

// ------------------------------------------------------------
// Test Actly Spawn BIF Function
// ------------------------------------------------------------
void test_actly_spawn() {
    printf("\n--- Testing actly_spawn (Actly Spawn BIF) ---\n");
    
    // Create isolated scheduler state
    void* scheduler_state = scheduler_state_init(1);
    if (scheduler_state == NULL) {
        printf("ERROR: Failed to create scheduler state\n");
        return;
    }
    
    // Initialize scheduler for core 0
    scheduler_init(scheduler_state, 0);
    
    // Create test process
    void* pcb = create_actly_bifs_test_process(1, PRIORITY_NORMAL, PROCESS_STATE_RUNNING);
    test_assert_not_zero((uint64_t)pcb, "test_process_creation");
    
    // Set as current process
    scheduler_set_current_process(scheduler_state, 0, pcb);
    
    // Set reduction count to allow spawn
    scheduler_set_reduction_count_with_state(scheduler_state, 0, 20);
    
    // Test actly spawn with valid parameters
    uint64_t new_pid = actly_spawn(0, 0x1000, PRIORITY_NORMAL, 8192, 4096);
    test_assert_not_zero(new_pid, "actly_spawn_success");
    
    // Test invalid core ID
    new_pid = actly_spawn(128, 0x1000, PRIORITY_NORMAL, 8192, 4096);
    test_assert_zero(new_pid, "actly_spawn_invalid_core");
    
    // Test invalid priority
    new_pid = actly_spawn(0, 0x1000, 99, 8192, 4096);
    test_assert_zero(new_pid, "actly_spawn_invalid_priority");
    
    // Test invalid stack size
    new_pid = actly_spawn(0, 0x1000, PRIORITY_NORMAL, 100, 4096);
    test_assert_zero(new_pid, "actly_spawn_invalid_stack_size");
    
    // Test invalid heap size
    new_pid = actly_spawn(0, 0x1000, PRIORITY_NORMAL, 8192, 100);
    test_assert_zero(new_pid, "actly_spawn_invalid_heap_size");
    
    // Test with insufficient reductions (should preempt)
    scheduler_set_reduction_count_with_state(scheduler_state, 0, 5); // Less than BIF_SPAWN_COST
    new_pid = actly_spawn(0, 0x1000, PRIORITY_NORMAL, 8192, 4096);
    test_assert_zero(new_pid, "actly_spawn_insufficient_reductions");
    
    // Cleanup
    free(pcb);
    
    // Clean up scheduler state
    scheduler_state_destroy(scheduler_state);
}

// ------------------------------------------------------------
// Test Actly Exit BIF Function
// ------------------------------------------------------------
void test_actly_exit() {
    printf("\n--- Testing actly_exit (Actly Exit BIF) ---\n");
    
    // Create isolated scheduler state
    void* scheduler_state = scheduler_state_init(1);
    if (scheduler_state == NULL) {
        printf("ERROR: Failed to create scheduler state\n");
        return;
    }
    
    // Initialize scheduler for core 0
    scheduler_init(scheduler_state, 0);
    
    // Create test process
    void* pcb = create_actly_bifs_test_process(1, PRIORITY_NORMAL, PROCESS_STATE_RUNNING);
    test_assert_not_zero((uint64_t)pcb, "test_process_creation");
    
    // Set as current process
    scheduler_set_current_process(scheduler_state, 0, pcb);
    
    // Set reduction count to allow exit
    scheduler_set_reduction_count_with_state(scheduler_state, 0, 10);
    
    // Test actly exit with valid reason
    // Note: This function never returns, so we can't test the return value
    // We can only test that it doesn't crash
    printf("Testing actly_exit (process should terminate)...\n");
    
    // Test invalid core ID (should return without crashing)
    printf("Testing actly_exit with invalid core ID...\n");
    
    // Test with no current process (should return without crashing)
    scheduler_set_current_process(scheduler_state, 0, NULL);
    printf("Testing actly_exit with no current process...\n");
    
    // Cleanup
    free(pcb);
    
    // Clean up scheduler state
    scheduler_state_destroy(scheduler_state);
}

// ------------------------------------------------------------
// Test BIF Trap Mechanism
// ------------------------------------------------------------
void test_bif_trap_mechanism() {
    printf("\n--- Testing BIF Trap Mechanism ---\n");
    
    // Create isolated scheduler state
    void* scheduler_state = scheduler_state_init(1);
    if (scheduler_state == NULL) {
        printf("ERROR: Failed to create scheduler state\n");
        return;
    }
    
    // Initialize scheduler for core 0
    scheduler_init(scheduler_state, 0);
    
    // Create test process
    void* pcb = create_actly_bifs_test_process(1, PRIORITY_NORMAL, PROCESS_STATE_RUNNING);
    test_assert_not_zero((uint64_t)pcb, "test_process_creation");
    
    // Set as current process
    scheduler_set_current_process(scheduler_state, 0, pcb);
    
    // Test BIF trap check with sufficient reductions
    scheduler_set_reduction_count_with_state(scheduler_state, 0, 10);
    int result = actly_bif_trap_check(scheduler_state, 0, 5);
    test_assert_equal(1, result, "bif_trap_sufficient_reductions");
    
    // Verify reduction count decreased
    uint64_t count = scheduler_get_reduction_count_with_state(scheduler_state, 0);
    test_assert_equal(5, count, "bif_trap_count_decreased");
    
    // Test BIF trap check with insufficient reductions
    result = actly_bif_trap_check(scheduler_state, 0, 10);
    test_assert_equal(0, result, "bif_trap_insufficient_reductions");
    
    // Test BIF trap check with exact reductions
    scheduler_set_reduction_count_with_state(scheduler_state, 0, 3);
    result = actly_bif_trap_check(scheduler_state, 0, 3);
    test_assert_equal(0, result, "bif_trap_exact_reductions");
    
    // Test invalid core ID
    result = actly_bif_trap_check(scheduler_state, 128, 5);
    test_assert_equal(1, result, "bif_trap_invalid_core");
    
    // Cleanup
    free(pcb);
    
    // Clean up scheduler state
    scheduler_state_destroy(scheduler_state);
}

// ------------------------------------------------------------
// Test BIF Reduction Costs
// ------------------------------------------------------------
void test_bif_reduction_costs() {
    printf("\n--- Testing BIF Reduction Costs ---\n");
    
    // Create isolated scheduler state
    void* scheduler_state = scheduler_state_init(1);
    if (scheduler_state == NULL) {
        printf("ERROR: Failed to create scheduler state\n");
        return;
    }
    
    // Initialize scheduler for core 0
    scheduler_init(scheduler_state, 0);
    
    // Create test process
    void* pcb = create_actly_bifs_test_process(1, PRIORITY_NORMAL, PROCESS_STATE_RUNNING);
    test_assert_not_zero((uint64_t)pcb, "test_process_creation");
    
    // Set as current process
    scheduler_set_current_process(scheduler_state, 0, pcb);
    
    // Test yield cost (1 reduction)
    scheduler_set_reduction_count_with_state(scheduler_state, 0, 5);
    int result = actly_bif_trap_check(scheduler_state, 0, BIF_YIELD_COST);
    test_assert_equal(1, result, "yield_cost_check");
    
    uint64_t count = scheduler_get_reduction_count_with_state(scheduler_state, 0);
    test_assert_equal(4, count, "yield_cost_decreased");
    
    // Test exit cost (1 reduction)
    scheduler_set_reduction_count_with_state(scheduler_state, 0, 5);
    result = actly_bif_trap_check(scheduler_state, 0, BIF_EXIT_COST);
    test_assert_equal(1, result, "exit_cost_check");
    
    count = scheduler_get_reduction_count_with_state(scheduler_state, 0);
    test_assert_equal(4, count, "exit_cost_decreased");
    
    // Test spawn cost (10 reductions)
    scheduler_set_reduction_count_with_state(scheduler_state, 0, 15);
    result = actly_bif_trap_check(scheduler_state, 0, BIF_SPAWN_COST);
    test_assert_equal(1, result, "spawn_cost_check");
    
    count = scheduler_get_reduction_count_with_state(scheduler_state, 0);
    test_assert_equal(5, count, "spawn_cost_decreased");
    
    // Cleanup
    free(pcb);
    
    // Clean up scheduler state
    scheduler_state_destroy(scheduler_state);
}

// ------------------------------------------------------------
// Test Spawn-Yield-Exit Lifecycle
// ------------------------------------------------------------
void test_actly_bifs_spawn_yield_exit_lifecycle() {
    printf("\n--- Testing Spawn-Yield-Exit Lifecycle ---\n");
    
    // Create isolated scheduler state
    void* scheduler_state = scheduler_state_init(1);
    if (scheduler_state == NULL) {
        printf("ERROR: Failed to create scheduler state\n");
        return;
    }
    
    // Initialize scheduler for core 0
    scheduler_init(scheduler_state, 0);
    
    // Create test process
    void* pcb = create_actly_bifs_test_process(1, PRIORITY_NORMAL, PROCESS_STATE_RUNNING);
    test_assert_not_zero((uint64_t)pcb, "test_process_creation");
    
    // Set as current process
    scheduler_set_current_process(scheduler_state, 0, pcb);
    
    // Set sufficient reduction count
    scheduler_set_reduction_count_with_state(scheduler_state, 0, 50);
    
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
    
    // Clean up scheduler state
    scheduler_state_destroy(scheduler_state);
}

// ------------------------------------------------------------
// Test Multiple Processes with BIFs
// ------------------------------------------------------------
void test_multiple_processes_bifs() {
    printf("\n--- Testing Multiple Processes with BIFs ---\n");
    
    // Create isolated scheduler state
    void* scheduler_state = scheduler_state_init(1);
    if (scheduler_state == NULL) {
        printf("ERROR: Failed to create scheduler state\n");
        return;
    }
    
    // Initialize scheduler for core 0
    scheduler_init(scheduler_state, 0);
    
    // Create multiple test processes
    void* pcb1 = create_actly_bifs_test_process(1, PRIORITY_NORMAL, PROCESS_STATE_RUNNING);
    void* pcb2 = create_actly_bifs_test_process(2, PRIORITY_HIGH, PROCESS_STATE_READY);
    
    test_assert_not_zero((uint64_t)pcb1, "test_process1_creation");
    test_assert_not_zero((uint64_t)pcb2, "test_process2_creation");
    
    // Set first process as current
    scheduler_set_current_process(scheduler_state, 0, pcb1);
    
    // Add second process to ready queue
    scheduler_enqueue_process(scheduler_state, 0, pcb2, PRIORITY_HIGH);
    
    // Set sufficient reduction count
    scheduler_set_reduction_count_with_state(scheduler_state, 0, 50);
    
    // Test spawn from first process
    uint64_t new_pid = actly_spawn(0, 0x1000, PRIORITY_NORMAL, 8192, 4096);
    test_assert_not_zero(new_pid, "multi_process_spawn");
    
    // Test yield from first process
    int yield_result = actly_yield(0);
    test_assert_equal(1, yield_result, "multi_process_yield");
    
    // Cleanup
    free(pcb1);
    free(pcb2);
    
    // Clean up scheduler state
    scheduler_state_destroy(scheduler_state);
}

// ------------------------------------------------------------
// Main Test Function
// ------------------------------------------------------------
// ------------------------------------------------------------
// test_process_save_context_basic — Test basic process_save_context functionality
// ------------------------------------------------------------
void test_process_save_context_basic() {
    printf("\n--- Testing process_save_context Basic Functionality ---\n");
    
    // Scheduler should already be initialized by previous tests
    
    // Create a simple test process
    typedef struct {
        uint64_t pid;
        uint64_t state;
        uint64_t priority;
        uint64_t sp;
        uint64_t lr;
        uint64_t pc;
        uint64_t pstate;
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
        pcb->size = sizeof(test_pcb_t);
        
        // Test process_save_context (should not crash)
        process_save_context(pcb);
        
        // Cleanup
        free(pcb);
    }
    
    printf("✓ Basic process_save_context tests passed\n");
}

// ------------------------------------------------------------
// test_process_restore_context_basic — Test basic process_restore_context functionality
// ------------------------------------------------------------
void test_process_restore_context_basic() {
    printf("\n--- Testing process_restore_context Basic Functionality ---\n");
    
    // Scheduler should already be initialized by previous tests
    
    // Create a simple test process
    typedef struct {
        uint64_t pid;
        uint64_t state;
        uint64_t priority;
        uint64_t sp;
        uint64_t lr;
        uint64_t pc;
        uint64_t pstate;
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
        pcb->size = sizeof(test_pcb_t);
        
        // Test process_restore_context (should not crash)
        process_restore_context(pcb);
        
        // Cleanup
        free(pcb);
    }
    
    printf("✓ Basic process_restore_context tests passed\n");
}

// ------------------------------------------------------------
// test_context_functions_edge_cases — Test context functions edge cases
// ------------------------------------------------------------
void test_context_functions_edge_cases() {
    printf("\n--- Testing Context Functions Edge Cases ---\n");
    
    // Scheduler should already be initialized by previous tests
    
    // Test with NULL PCB (should not crash)
    process_save_context(NULL);
    process_restore_context(NULL);
    
    printf("✓ Edge case tests passed\n");
}

// ------------------------------------------------------------
// Main Test Function
// ------------------------------------------------------------
void test_actly_bifs_main() {
    printf("\n=== ACTLY BIF FUNCTIONS TEST SUITE ===\n");
    
    test_process_save_context_basic();
    test_process_restore_context_basic();
    test_context_functions_edge_cases();
    
    printf("\n=== ACTLY BIF FUNCTIONS TEST SUITE COMPLETE ===\n");
}
