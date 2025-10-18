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
// test_process_yield_standalone.c — Focused test for process_yield function
// ------------------------------------------------------------
// Isolated test for the process_yield function to debug and validate
// the core voluntary yielding mechanism. This test focuses specifically
// on process_yield without the complexity of the full yielding test suite.
//
// Version: 0.10
// Author: Lee Barney
// Last Modified: 2025-01-19
//

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

// External assembly functions
extern void* process_yield(uint64_t core_id, void* pcb);

// External scheduler functions
extern void scheduler_init(uint64_t core_id);
extern void* scheduler_get_current_process(uint64_t core_id);
extern void scheduler_set_current_process(uint64_t core_id, void* process);
extern uint64_t scheduler_get_reduction_count(uint64_t core_id);
extern void scheduler_set_reduction_count(uint64_t core_id, uint64_t count);

// External process functions
extern void* process_create(uint64_t entry_point, uint64_t priority, uint64_t stack_size, uint64_t heap_size);
extern void process_destroy(void* pcb);
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
extern const uint64_t PROCESS_STATE_READY;
extern const uint64_t PROCESS_STATE_RUNNING;
extern const uint64_t PRIORITY_NORMAL;
extern const uint64_t DEFAULT_REDUCTIONS;

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
void* create_test_process(uint64_t pid, uint64_t priority, uint64_t state) {
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
// test_process_yield_basic — Test basic process_yield functionality
// ------------------------------------------------------------
void test_process_yield_basic(void) {
    printf("\n--- Testing process_yield (Basic Functionality) ---\n");
    
    // Initialize scheduler
    scheduler_init(0);
    
    // Create test process
    void* pcb = create_test_process(1, PRIORITY_NORMAL, PROCESS_STATE_RUNNING);
    test_assert_not_zero((uint64_t)pcb, "test_process_creation");
    
    // Set as current process
    scheduler_set_current_process(0, pcb);
    
    // Verify initial state
    void* current = scheduler_get_current_process(0);
    test_assert_equal((uint64_t)pcb, (uint64_t)current, "initial_current_process");
    
    uint64_t state = process_get_state(pcb);
    test_assert_equal(PROCESS_STATE_RUNNING, state, "initial_process_state");
    
    // Test voluntary yield
    printf("Calling process_yield(0, pcb)...\n");
    void* next_process = process_yield(0, pcb);
    
    // Verify yield results
    test_assert_zero((uint64_t)next_process, "yield_no_next_process");
    
    // Verify process state changed to READY
    state = process_get_state(pcb);
    test_assert_equal(PROCESS_STATE_READY, state, "yield_state_change");
    
    // Verify current process is cleared
    current = scheduler_get_current_process(0);
    test_assert_zero((uint64_t)current, "yield_current_process_cleared");
    
    // Cleanup
    free(pcb);
}

// ------------------------------------------------------------
// test_process_yield_error_handling — Test error handling
// ------------------------------------------------------------
void test_process_yield_error_handling(void) {
    printf("\n--- Testing process_yield (Error Handling) ---\n");
    
    // Initialize scheduler
    scheduler_init(0);
    
    // Create test process
    void* pcb = create_test_process(1, PRIORITY_NORMAL, PROCESS_STATE_RUNNING);
    test_assert_not_zero((uint64_t)pcb, "test_process_creation");
    
    // Test invalid core ID (should handle gracefully)
    printf("Testing invalid core ID (128)...\n");
    void* result = process_yield(128, pcb);
    test_assert_zero((uint64_t)result, "yield_invalid_core");
    
    // Test NULL PCB (should handle gracefully)
    printf("Testing NULL PCB...\n");
    result = process_yield(0, NULL);
    test_assert_zero((uint64_t)result, "yield_null_pcb");
    
    // Cleanup
    free(pcb);
}

// ------------------------------------------------------------
// test_process_yield_reduction_reset — Test reduction counter reset
// ------------------------------------------------------------
void test_process_yield_reduction_reset(void) {
    printf("\n--- Testing process_yield (Reduction Counter Reset) ---\n");
    
    // Initialize scheduler
    scheduler_init(0);
    
    // Create test process
    void* pcb = create_test_process(1, PRIORITY_NORMAL, PROCESS_STATE_RUNNING);
    test_assert_not_zero((uint64_t)pcb, "test_process_creation");
    
    // Set as current process
    scheduler_set_current_process(0, pcb);
    
    // Set reduction count to a low value
    scheduler_set_reduction_count(0, 5);
    uint64_t initial_count = scheduler_get_reduction_count(0);
    test_assert_equal(5, initial_count, "initial_reduction_count");
    
    // Test yield
    void* next_process = process_yield(0, pcb);
    test_assert_zero((uint64_t)next_process, "yield_no_next_process");
    
    // Verify reduction count is reset to default
    uint64_t final_count = scheduler_get_reduction_count(0);
    test_assert_equal(DEFAULT_REDUCTIONS, final_count, "yield_reduction_reset");
    
    // Cleanup
    free(pcb);
}

// ------------------------------------------------------------
// test_process_yield_state_transitions — Test state transitions
// ------------------------------------------------------------
void test_process_yield_state_transitions(void) {
    printf("\n--- Testing process_yield (State Transitions) ---\n");
    
    // Initialize scheduler
    scheduler_init(0);
    
    // Create test process in RUNNING state
    void* pcb = create_test_process(1, PRIORITY_NORMAL, PROCESS_STATE_RUNNING);
    test_assert_not_zero((uint64_t)pcb, "test_process_creation");
    
    // Set as current process
    scheduler_set_current_process(0, pcb);
    
    // Verify initial RUNNING state
    uint64_t state = process_get_state(pcb);
    test_assert_equal(PROCESS_STATE_RUNNING, state, "initial_running_state");
    
    // Test yield
    void* next_process = process_yield(0, pcb);
    test_assert_zero((uint64_t)next_process, "yield_no_next_process");
    
    // Verify state changed to READY
    state = process_get_state(pcb);
    test_assert_equal(PROCESS_STATE_READY, state, "yield_ready_state");
    
    // Cleanup
    free(pcb);
}

// ------------------------------------------------------------
// Main Test Function
// ------------------------------------------------------------
int main(void) {
    printf("\n========================================\n");
    printf("    PROCESS_YIELD STANDALONE TEST\n");
    printf("========================================\n");
    
    printf("\n--- System Information ---\n");
    printf("DEFAULT_REDUCTIONS: %llu\n", DEFAULT_REDUCTIONS);
    printf("PROCESS_STATE_READY: %llu\n", PROCESS_STATE_READY);
    printf("PROCESS_STATE_RUNNING: %llu\n", PROCESS_STATE_RUNNING);
    printf("PRIORITY_NORMAL: %llu\n", PRIORITY_NORMAL);
    
    // Run focused tests
    test_process_yield_basic();
    test_process_yield_error_handling();
    test_process_yield_reduction_reset();
    test_process_yield_state_transitions();
    
    printf("\n=== PROCESS_YIELD STANDALONE TEST COMPLETE ===\n");
    printf("If you see this message, the test completed without crashing.\n");
    printf("Check the output above for any assertion failures.\n");
    
    return 0;
}
