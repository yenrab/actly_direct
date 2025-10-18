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
// test_scheduler_scheduling.c — C tests for pure assembly scheduler scheduling
// ------------------------------------------------------------

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// External assembly functions
extern void scheduler_init(uint64_t core_id);
extern void* scheduler_schedule(uint64_t core_id);
extern void scheduler_enqueue_process(uint64_t core_id, void* process, uint64_t priority);
extern void* scheduler_dequeue_process(void* queue);
extern uint64_t scheduler_is_queue_empty(void* queue);
extern uint64_t scheduler_get_queue_length(void* queue);
extern void* get_scheduler_state(uint64_t core_id);
extern void* get_priority_queue(void* state, uint64_t priority);
extern void* scheduler_get_current_process(uint64_t core_id);
extern void test_assert_equal(uint64_t expected, uint64_t actual, const char* test_name);
extern void test_assert_zero(uint64_t value, const char* test_name);
extern void test_assert_not_zero(uint64_t value, const char* test_name);

// External constants from assembly
extern const uint64_t DEFAULT_REDUCTIONS;

// Priority levels (must match scheduler.s)
#define PRIORITY_MAX 0
#define PRIORITY_HIGH 1
#define PRIORITY_NORMAL 2
#define PRIORITY_LOW 3

// PCB structure offsets (must match process.s)
#define PCB_NEXT_OFFSET 0
#define PCB_PREV_OFFSET 8
#define PCB_PID_OFFSET 16
#define PCB_SCHEDULER_ID_OFFSET 24
#define PCB_STATE_OFFSET 32
#define PCB_PRIORITY_OFFSET 40
#define PCB_REDUCTION_COUNT_OFFSET 48
#define PCB_TOTAL_SIZE 512

// Process states (must match config.inc)
#define PROCESS_STATE_CREATED 0
#define PROCESS_STATE_READY 1
#define PROCESS_STATE_RUNNING 2
#define PROCESS_STATE_WAITING 3
#define PROCESS_STATE_SUSPENDED 4
#define PROCESS_STATE_TERMINATED 5

// Helper function to create a valid PCB structure for testing
void* create_test_pcb(uint64_t pid, uint64_t priority, uint64_t state) {
    void* pcb = malloc(PCB_TOTAL_SIZE);
    if (pcb == NULL) {
        return NULL;
    }
    
    // Initialize the entire PCB to zero
    memset(pcb, 0, PCB_TOTAL_SIZE);
    
    // Set up the PCB fields that the scheduler expects
    *(void**)((char*)pcb + PCB_NEXT_OFFSET) = NULL;           // next pointer
    *(void**)((char*)pcb + PCB_PREV_OFFSET) = NULL;           // prev pointer
    *(uint64_t*)((char*)pcb + PCB_PID_OFFSET) = pid;          // process ID
    *(uint64_t*)((char*)pcb + PCB_SCHEDULER_ID_OFFSET) = 0;   // scheduler ID
    *(uint64_t*)((char*)pcb + PCB_STATE_OFFSET) = state;      // process state
    *(uint64_t*)((char*)pcb + PCB_PRIORITY_OFFSET) = priority; // priority
    *(uint64_t*)((char*)pcb + PCB_REDUCTION_COUNT_OFFSET) = 2000; // reduction count
    
    return pcb;
}

// Forward declarations
void test_scheduler_schedule_empty(void);
void test_scheduler_enqueue_dequeue(void);
void test_scheduler_priority_ordering(void);
void test_scheduler_round_robin(void);
void test_scheduler_queue_operations(void);

// ------------------------------------------------------------
// test_scheduler_scheduling — Main test function for scheduler scheduling
// ------------------------------------------------------------
void test_scheduler_scheduling(void) {
    printf("\n--- Testing scheduler_scheduling (Pure Assembly) ---\n");
    
    // Initialize scheduler once for all tests
    scheduler_init(0);
    
    test_scheduler_schedule_empty();
    test_scheduler_enqueue_dequeue();
    test_scheduler_priority_ordering();
    test_scheduler_round_robin();
    test_scheduler_queue_operations();
}

// ------------------------------------------------------------
// test_scheduler_schedule_empty — Test scheduling with empty queues
// ------------------------------------------------------------
void test_scheduler_schedule_empty(void) {
    // Note: scheduler is already initialized by main test function
    
    // Try to schedule with empty queues
    void* process = scheduler_schedule(0);
    test_assert_zero((uint64_t)process, "scheduler_schedule_empty_queues");
    
    // Verify current process is still NULL
    void* current_process = scheduler_get_current_process(0);
    test_assert_zero((uint64_t)current_process, "scheduler_schedule_empty_current_process");
}

// ------------------------------------------------------------
// test_scheduler_enqueue_dequeue — Test basic enqueue/dequeue operations
// ------------------------------------------------------------
void test_scheduler_enqueue_dequeue(void) {
    // Note: scheduler should already be initialized by the main test function
    
    // Create valid PCB structures for testing
    void* process1 = create_test_pcb(1, PRIORITY_NORMAL, PROCESS_STATE_READY);
    void* process2 = create_test_pcb(2, PRIORITY_NORMAL, PROCESS_STATE_READY);
    
    if (process1 == NULL || process2 == NULL) {
        test_assert_equal(0, 1, "Failed to create test PCB structures");
        return;
    }
    
    // Get priority queue for NORMAL priority
    void* state = get_scheduler_state(0);
    void* queue = get_priority_queue(state, PRIORITY_NORMAL);
    
    // Test empty queue
    uint64_t is_empty = scheduler_is_queue_empty(queue);
    test_assert_not_zero(is_empty, "scheduler_queue_initially_empty");
    
    uint64_t length = scheduler_get_queue_length(queue);
    test_assert_zero(length, "scheduler_queue_initially_zero_length");
    
    // Enqueue first process
    scheduler_enqueue_process(0, process1, PRIORITY_NORMAL);
    
    is_empty = scheduler_is_queue_empty(queue);
    test_assert_zero(is_empty, "scheduler_queue_not_empty_after_enqueue");
    
    length = scheduler_get_queue_length(queue);
    test_assert_equal(1, length, "scheduler_queue_length_after_first_enqueue");
    
    // Enqueue second process
    scheduler_enqueue_process(0, process2, PRIORITY_NORMAL);
    
    length = scheduler_get_queue_length(queue);
    test_assert_equal(2, length, "scheduler_queue_length_after_second_enqueue");
    
    // Dequeue first process
    void* dequeued = scheduler_dequeue_process(queue);
    test_assert_equal((uint64_t)process1, (uint64_t)dequeued, "scheduler_dequeue_first_process");
    
    length = scheduler_get_queue_length(queue);
    test_assert_equal(1, length, "scheduler_queue_length_after_first_dequeue");
    
    // Dequeue second process
    dequeued = scheduler_dequeue_process(queue);
    test_assert_equal((uint64_t)process2, (uint64_t)dequeued, "scheduler_dequeue_second_process");
    
    is_empty = scheduler_is_queue_empty(queue);
    test_assert_not_zero(is_empty, "scheduler_queue_empty_after_all_dequeues");
    
    length = scheduler_get_queue_length(queue);
    test_assert_zero(length, "scheduler_queue_zero_length_after_all_dequeues");
    
    // Clean up allocated memory
    free(process1);
    free(process2);
}

// ------------------------------------------------------------
// test_scheduler_priority_ordering — Test priority-based scheduling
// ------------------------------------------------------------
void test_scheduler_priority_ordering(void) {
    // Note: scheduler should already be initialized by the main test function
    
    // Create valid PCB structures for testing
    void* low_process = create_test_pcb(1, PRIORITY_LOW, PROCESS_STATE_READY);
    void* normal_process = create_test_pcb(2, PRIORITY_NORMAL, PROCESS_STATE_READY);
    void* high_process = create_test_pcb(3, PRIORITY_HIGH, PROCESS_STATE_READY);
    void* max_process = create_test_pcb(4, PRIORITY_MAX, PROCESS_STATE_READY);
    
    if (low_process == NULL || normal_process == NULL || high_process == NULL || max_process == NULL) {
        test_assert_equal(0, 1, "Failed to create test PCB structures for priority test");
        return;
    }
    
    // Enqueue processes in reverse priority order
    scheduler_enqueue_process(0, low_process, PRIORITY_LOW);
    scheduler_enqueue_process(0, normal_process, PRIORITY_NORMAL);
    scheduler_enqueue_process(0, high_process, PRIORITY_HIGH);
    scheduler_enqueue_process(0, max_process, PRIORITY_MAX);
    
    // Schedule should return MAX priority process first
    void* scheduled = scheduler_schedule(0);
    test_assert_equal((uint64_t)max_process, (uint64_t)scheduled, "scheduler_priority_max_first");
    
    // Schedule should return HIGH priority process next
    scheduled = scheduler_schedule(0);
    test_assert_equal((uint64_t)high_process, (uint64_t)scheduled, "scheduler_priority_high_second");
    
    // Schedule should return NORMAL priority process next
    scheduled = scheduler_schedule(0);
    test_assert_equal((uint64_t)normal_process, (uint64_t)scheduled, "scheduler_priority_normal_third");
    
    // Schedule should return LOW priority process last
    scheduled = scheduler_schedule(0);
    test_assert_equal((uint64_t)low_process, (uint64_t)scheduled, "scheduler_priority_low_last");
    
    // No more processes should be available
    scheduled = scheduler_schedule(0);
    test_assert_zero((uint64_t)scheduled, "scheduler_priority_no_more_processes");
    
    // Clean up allocated memory
    free(low_process);
    free(normal_process);
    free(high_process);
    free(max_process);
}

// ------------------------------------------------------------
// test_scheduler_round_robin — Test round-robin within same priority
// ------------------------------------------------------------
void test_scheduler_round_robin(void) {
    // Note: scheduler should already be initialized by the main test function
    
    // Create valid PCB structures for testing
    void* process1 = create_test_pcb(1, PRIORITY_NORMAL, PROCESS_STATE_READY);
    void* process2 = create_test_pcb(2, PRIORITY_NORMAL, PROCESS_STATE_READY);
    void* process3 = create_test_pcb(3, PRIORITY_NORMAL, PROCESS_STATE_READY);
    
    if (process1 == NULL || process2 == NULL || process3 == NULL) {
        test_assert_equal(0, 1, "Failed to create test PCB structures for round-robin test");
        return;
    }
    
    // Enqueue multiple processes at same priority
    scheduler_enqueue_process(0, process1, PRIORITY_NORMAL);
    scheduler_enqueue_process(0, process2, PRIORITY_NORMAL);
    scheduler_enqueue_process(0, process3, PRIORITY_NORMAL);
    
    // Schedule should return processes in FIFO order
    void* scheduled = scheduler_schedule(0);
    test_assert_equal((uint64_t)process1, (uint64_t)scheduled, "scheduler_round_robin_first");
    
    scheduled = scheduler_schedule(0);
    test_assert_equal((uint64_t)process2, (uint64_t)scheduled, "scheduler_round_robin_second");
    
    scheduled = scheduler_schedule(0);
    test_assert_equal((uint64_t)process3, (uint64_t)scheduled, "scheduler_round_robin_third");
    
    // No more processes should be available
    scheduled = scheduler_schedule(0);
    test_assert_zero((uint64_t)scheduled, "scheduler_round_robin_no_more_processes");
    
    // Clean up allocated memory
    free(process1);
    free(process2);
    free(process3);
}

// ------------------------------------------------------------
// test_scheduler_queue_operations — Test queue state after operations
// ------------------------------------------------------------
void test_scheduler_queue_operations(void) {
    // Note: scheduler should already be initialized by the main test function
    
    // Create valid PCB structures for testing
    void* process1 = create_test_pcb(1, PRIORITY_NORMAL, PROCESS_STATE_READY);
    void* process2 = create_test_pcb(2, PRIORITY_HIGH, PROCESS_STATE_READY);
    
    if (process1 == NULL || process2 == NULL) {
        test_assert_equal(0, 1, "Failed to create test PCB structures for queue operations test");
        return;
    }
    
    // Get all priority queues
    void* state = get_scheduler_state(0);
    void* max_queue = get_priority_queue(state, PRIORITY_MAX);
    void* high_queue = get_priority_queue(state, PRIORITY_HIGH);
    void* normal_queue = get_priority_queue(state, PRIORITY_NORMAL);
    void* low_queue = get_priority_queue(state, PRIORITY_LOW);
    
    // All queues should be empty initially
    test_assert_not_zero(scheduler_is_queue_empty(max_queue), "scheduler_max_queue_initially_empty");
    test_assert_not_zero(scheduler_is_queue_empty(high_queue), "scheduler_high_queue_initially_empty");
    test_assert_not_zero(scheduler_is_queue_empty(normal_queue), "scheduler_normal_queue_initially_empty");
    test_assert_not_zero(scheduler_is_queue_empty(low_queue), "scheduler_low_queue_initially_empty");
    
    // Add processes to different queues
    scheduler_enqueue_process(0, process1, PRIORITY_HIGH);
    scheduler_enqueue_process(0, process2, PRIORITY_LOW);
    
    // Verify queue states
    test_assert_not_zero(scheduler_is_queue_empty(max_queue), "scheduler_max_queue_still_empty");
    test_assert_zero(scheduler_is_queue_empty(high_queue), "scheduler_high_queue_not_empty");
    test_assert_not_zero(scheduler_is_queue_empty(normal_queue), "scheduler_normal_queue_still_empty");
    test_assert_zero(scheduler_is_queue_empty(low_queue), "scheduler_low_queue_not_empty");
    
    // Verify queue lengths
    test_assert_equal(0, scheduler_get_queue_length(max_queue), "scheduler_max_queue_length_zero");
    test_assert_equal(1, scheduler_get_queue_length(high_queue), "scheduler_high_queue_length_one");
    test_assert_equal(0, scheduler_get_queue_length(normal_queue), "scheduler_normal_queue_length_zero");
    test_assert_equal(1, scheduler_get_queue_length(low_queue), "scheduler_low_queue_length_one");
    
    // Clean up allocated memory
    free(process1);
    free(process2);
}
