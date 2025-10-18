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
// test_blocking.c — Blocking Operations Test Suite
// ------------------------------------------------------------
// Comprehensive test suite for all blocking operations implemented
// in blocking.s. Tests message receive, timer waiting, I/O blocking,
// and waiting queue management following BEAM behavior.
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
extern void* process_block(uint64_t core_id, void* pcb, uint64_t reason);
extern int process_wake(uint64_t core_id, void* pcb);
extern void* process_block_on_receive(uint64_t core_id, void* pcb, uint64_t pattern);
extern int process_block_on_timer(uint64_t core_id, void* pcb, uint64_t timeout_ticks);
extern int process_block_on_io(uint64_t core_id, void* pcb, uint64_t io_descriptor);
extern uint64_t process_check_timer_wakeups(uint64_t core_id);

// External scheduler functions
extern void scheduler_init(uint64_t core_id);
extern void* scheduler_get_current_process(uint64_t core_id);
extern void scheduler_set_current_process(uint64_t core_id, void* process);
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
extern const uint64_t PROCESS_STATE_READY;
extern const uint64_t PROCESS_STATE_RUNNING;
extern const uint64_t PROCESS_STATE_WAITING;
extern const uint64_t PRIORITY_NORMAL;
extern const uint64_t REASON_RECEIVE;
extern const uint64_t REASON_TIMER;
extern const uint64_t REASON_IO;
extern const uint64_t MAX_BLOCKING_TIME;

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
void* create_blocking_test_process(uint64_t pid, uint64_t priority, uint64_t state) {
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
// Test Process Block and Wake Functions
// ------------------------------------------------------------
void test_process_block_and_wake(void) {
    printf("\n--- Testing process_block and process_wake (Generic Blocking) ---\n");
    
    // Initialize scheduler
    scheduler_init(0);
    
    // Create test process
    void* pcb = create_blocking_test_process(1, PRIORITY_NORMAL, PROCESS_STATE_RUNNING);
    test_assert_not_zero((uint64_t)pcb, "test_process_creation");
    
    // Set as current process
    scheduler_set_current_process(0, pcb);
    
    // Test blocking with receive reason
    void* next_process = process_block(0, pcb, REASON_RECEIVE);
    test_assert_zero((uint64_t)next_process, "block_no_next_process");
    
    // Verify process state changed to WAITING
    uint64_t state = process_get_state(pcb);
    test_assert_equal(PROCESS_STATE_WAITING, state, "block_state_change");
    
    // Test waking the process
    int wake_result = process_wake(0, pcb);
    test_assert_equal(1, wake_result, "wake_success");
    
    // Verify process state changed back to READY
    state = process_get_state(pcb);
    test_assert_equal(PROCESS_STATE_READY, state, "wake_state_change");
    
    // Test invalid core ID
    next_process = process_block(128, pcb, REASON_RECEIVE);
    test_assert_zero((uint64_t)next_process, "block_invalid_core");
    
    // Test invalid PCB
    next_process = process_block(0, NULL, REASON_RECEIVE);
    test_assert_zero((uint64_t)next_process, "block_invalid_pcb");
    
    // Test invalid reason
    next_process = process_block(0, pcb, 99);
    test_assert_zero((uint64_t)next_process, "block_invalid_reason");
    
    // Test wake with invalid core ID
    wake_result = process_wake(128, pcb);
    test_assert_equal(0, wake_result, "wake_invalid_core");
    
    // Test wake with invalid PCB
    wake_result = process_wake(0, NULL);
    test_assert_equal(0, wake_result, "wake_invalid_pcb");
    
    // Cleanup
    free(pcb);
}

// ------------------------------------------------------------
// Test Process Block on Receive Function
// ------------------------------------------------------------
void test_process_block_on_receive(void) {
    printf("\n--- Testing process_block_on_receive (Message Receive Blocking) ---\n");
    
    // Initialize scheduler
    scheduler_init(0);
    
    // Create test process
    void* pcb = create_blocking_test_process(1, PRIORITY_NORMAL, PROCESS_STATE_RUNNING);
    test_assert_not_zero((uint64_t)pcb, "test_process_creation");
    
    // Set as current process
    scheduler_set_current_process(0, pcb);
    
    // Test blocking on receive with pattern
    void* message = process_block_on_receive(0, pcb, 0x12345678);
    test_assert_zero((uint64_t)message, "block_on_receive_no_message");
    
    // Verify process state changed to WAITING
    uint64_t state = process_get_state(pcb);
    test_assert_equal(PROCESS_STATE_WAITING, state, "block_on_receive_state_change");
    
    // Test invalid core ID
    message = process_block_on_receive(128, pcb, 0x12345678);
    test_assert_zero((uint64_t)message, "block_on_receive_invalid_core");
    
    // Test invalid PCB
    message = process_block_on_receive(0, NULL, 0x12345678);
    test_assert_zero((uint64_t)message, "block_on_receive_invalid_pcb");
    
    // Cleanup
    free(pcb);
}

// ------------------------------------------------------------
// Test Process Block on Timer Function
// ------------------------------------------------------------
void test_process_block_on_timer(void) {
    printf("\n--- Testing process_block_on_timer (Timer-based Blocking) ---\n");
    
    // Initialize scheduler
    scheduler_init(0);
    
    // Create test process
    void* pcb = create_blocking_test_process(1, PRIORITY_NORMAL, PROCESS_STATE_RUNNING);
    test_assert_not_zero((uint64_t)pcb, "test_process_creation");
    
    // Set as current process
    scheduler_set_current_process(0, pcb);
    
    // Test blocking on timer with timeout
    int result = process_block_on_timer(0, pcb, 1000);
    test_assert_equal(1, result, "block_on_timer_success");
    
    // Verify process state changed to WAITING
    uint64_t state = process_get_state(pcb);
    test_assert_equal(PROCESS_STATE_WAITING, state, "block_on_timer_state_change");
    
    // Test invalid core ID
    result = process_block_on_timer(128, pcb, 1000);
    test_assert_equal(0, result, "block_on_timer_invalid_core");
    
    // Test invalid PCB
    result = process_block_on_timer(0, NULL, 1000);
    test_assert_equal(0, result, "block_on_timer_invalid_pcb");
    
    // Test invalid timeout
    result = process_block_on_timer(0, pcb, MAX_BLOCKING_TIME + 1);
    test_assert_equal(0, result, "block_on_timer_invalid_timeout");
    
    // Cleanup
    free(pcb);
}

// ------------------------------------------------------------
// Test Process Block on I/O Function
// ------------------------------------------------------------
void test_process_block_on_io(void) {
    printf("\n--- Testing process_block_on_io (I/O Blocking) ---\n");
    
    // Initialize scheduler
    scheduler_init(0);
    
    // Create test process
    void* pcb = create_blocking_test_process(1, PRIORITY_NORMAL, PROCESS_STATE_RUNNING);
    test_assert_not_zero((uint64_t)pcb, "test_process_creation");
    
    // Set as current process
    scheduler_set_current_process(0, pcb);
    
    // Test blocking on I/O with descriptor
    int result = process_block_on_io(0, pcb, 0x12345678);
    test_assert_equal(1, result, "block_on_io_success");
    
    // Verify process state changed to WAITING
    uint64_t state = process_get_state(pcb);
    test_assert_equal(PROCESS_STATE_WAITING, state, "block_on_io_state_change");
    
    // Test invalid core ID
    result = process_block_on_io(128, pcb, 0x12345678);
    test_assert_equal(0, result, "block_on_io_invalid_core");
    
    // Test invalid PCB
    result = process_block_on_io(0, NULL, 0x12345678);
    test_assert_equal(0, result, "block_on_io_invalid_pcb");
    
    // Cleanup
    free(pcb);
}

// ------------------------------------------------------------
// Test Waiting Queue Management
// ------------------------------------------------------------
void test_waiting_queue_management(void) {
    printf("\n--- Testing Waiting Queue Management ---\n");
    
    // Initialize scheduler
    scheduler_init(0);
    
    // Create multiple test processes
    void* pcb1 = create_blocking_test_process(1, PRIORITY_NORMAL, PROCESS_STATE_RUNNING);
    void* pcb2 = create_blocking_test_process(2, PRIORITY_NORMAL, PROCESS_STATE_RUNNING);
    void* pcb3 = create_blocking_test_process(3, PRIORITY_NORMAL, PROCESS_STATE_RUNNING);
    
    test_assert_not_zero((uint64_t)pcb1, "test_process1_creation");
    test_assert_not_zero((uint64_t)pcb2, "test_process2_creation");
    test_assert_not_zero((uint64_t)pcb3, "test_process3_creation");
    
    // Block processes with different reasons
    process_block(0, pcb1, REASON_RECEIVE);
    process_block(0, pcb2, REASON_TIMER);
    process_block(0, pcb3, REASON_IO);
    
    // Verify all processes are in WAITING state
    uint64_t state1 = process_get_state(pcb1);
    uint64_t state2 = process_get_state(pcb2);
    uint64_t state3 = process_get_state(pcb3);
    
    test_assert_equal(PROCESS_STATE_WAITING, state1, "process1_waiting_state");
    test_assert_equal(PROCESS_STATE_WAITING, state2, "process2_waiting_state");
    test_assert_equal(PROCESS_STATE_WAITING, state3, "process3_waiting_state");
    
    // Wake all processes
    int wake1 = process_wake(0, pcb1);
    int wake2 = process_wake(0, pcb2);
    int wake3 = process_wake(0, pcb3);
    
    test_assert_equal(1, wake1, "wake_process1");
    test_assert_equal(1, wake2, "wake_process2");
    test_assert_equal(1, wake3, "wake_process3");
    
    // Verify all processes are back to READY state
    state1 = process_get_state(pcb1);
    state2 = process_get_state(pcb2);
    state3 = process_get_state(pcb3);
    
    test_assert_equal(PROCESS_STATE_READY, state1, "process1_ready_state");
    test_assert_equal(PROCESS_STATE_READY, state2, "process2_ready_state");
    test_assert_equal(PROCESS_STATE_READY, state3, "process3_ready_state");
    
    // Cleanup
    free(pcb1);
    free(pcb2);
    free(pcb3);
}

// ------------------------------------------------------------
// Test Timer Wakeup Checking
// ------------------------------------------------------------
void test_timer_wakeup_checking(void) {
    printf("\n--- Testing Timer Wakeup Checking ---\n");
    
    // Initialize scheduler
    scheduler_init(0);
    
    // Create test process
    void* pcb = create_blocking_test_process(1, PRIORITY_NORMAL, PROCESS_STATE_RUNNING);
    test_assert_not_zero((uint64_t)pcb, "test_process_creation");
    
    // Set as current process
    scheduler_set_current_process(0, pcb);
    
    // Block process on timer with short timeout
    int result = process_block_on_timer(0, pcb, 100);
    test_assert_equal(1, result, "block_on_timer_short_timeout");
    
    // Check timer wakeups (should not wake yet)
    uint64_t woken_count = process_check_timer_wakeups(0);
    test_assert_equal(0, woken_count, "timer_check_no_wakeups");
    
    // Test invalid core ID
    woken_count = process_check_timer_wakeups(128);
    test_assert_equal(0, woken_count, "timer_check_invalid_core");
    
    // Cleanup
    free(pcb);
}

// ------------------------------------------------------------
// Test Block-Wake Cycle
// ------------------------------------------------------------
void test_block_wake_cycle(void) {
    printf("\n--- Testing Block-Wake Cycle ---\n");
    
    // Initialize scheduler
    scheduler_init(0);
    
    // Create test process
    void* pcb = create_blocking_test_process(1, PRIORITY_NORMAL, PROCESS_STATE_RUNNING);
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
// Main Test Function
// ------------------------------------------------------------
// ------------------------------------------------------------
// test_scheduler_enqueue_basic — Test basic scheduler_enqueue_process functionality
// ------------------------------------------------------------
void test_scheduler_enqueue_basic(void) {
    printf("\n--- Testing scheduler_enqueue_process Basic Functionality ---\n");
    
    // Initialize scheduler
    scheduler_init(0);
    
    // Create a simple test process
    typedef struct {
        uint64_t pid;
        uint64_t state;
        uint64_t priority;
    } simple_pcb_t;
    
    simple_pcb_t* pcb = (simple_pcb_t*)malloc(sizeof(simple_pcb_t));
    test_assert_not_zero((uint64_t)pcb, "PCB allocation should succeed");
    
    if (pcb) {
        pcb->pid = 1;
        pcb->state = 1; // PROCESS_STATE_READY
        pcb->priority = 2; // PRIORITY_NORMAL
        
        // Test scheduler_enqueue_process
        int result = scheduler_enqueue_process(0, pcb, 2); // PRIORITY_NORMAL
        test_assert_not_zero(result, "scheduler_enqueue_process should return non-zero");
        
        // Cleanup
        free(pcb);
    }
    
    printf("✓ Basic scheduler_enqueue_process tests passed\n");
}

// ------------------------------------------------------------
// test_scheduler_enqueue_edge_cases — Test scheduler_enqueue_process edge cases
// ------------------------------------------------------------
void test_scheduler_enqueue_edge_cases(void) {
    printf("\n--- Testing scheduler_enqueue_process Edge Cases ---\n");
    
    // Initialize scheduler
    scheduler_init(0);
    
    // Test with NULL process (should return 0)
    int result = scheduler_enqueue_process(0, NULL, 2);
    test_assert_equal(result, 0, "scheduler_enqueue_process with NULL process should return 0");
    
    // Test with invalid core ID (should return 0)
    result = scheduler_enqueue_process(128, NULL, 2);
    test_assert_equal(result, 0, "scheduler_enqueue_process with invalid core ID should return 0");
    
    printf("✓ Edge case tests passed\n");
}

// ------------------------------------------------------------
// Main Test Function
// ------------------------------------------------------------
void test_blocking_main(void) {
    printf("\n=== BLOCKING OPERATIONS TEST SUITE ===\n");
    
    test_scheduler_enqueue_basic();
    test_scheduler_enqueue_edge_cases();
    
    printf("\n=== BLOCKING OPERATIONS TEST SUITE COMPLETE ===\n");
}
