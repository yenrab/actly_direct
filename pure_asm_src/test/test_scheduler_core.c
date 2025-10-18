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
// test_scheduler_core.c — Test runner for scheduler core functionality
// ------------------------------------------------------------
// Standalone test runner for scheduler functionality.
// This file provides a minimal test harness to run tests
// in isolation for debugging and development purposes.
//
// Version: 0.10
// Author: Lee Barney
// Last Modified: 2025-01-19
//

#include <stdio.h>
#include <stdint.h>
#include <assert.h>

// Test framework functions
static uint64_t test_passed_count = 0;
static uint64_t test_failed_count = 0;

void test_assert_equal(uint64_t expected, uint64_t actual, const char* test_name) {
    if (expected == actual) {
        test_passed_count++;
        printf("  ✓ %s\n", test_name);
    } else {
        test_failed_count++;
        printf("  ✗ %s: Expected %llu, got %llu\n", test_name, expected, actual);
    }
}

void test_assert_not_null(void* ptr, const char* test_name) {
    if (ptr != NULL) {
        test_passed_count++;
        printf("  ✓ %s\n", test_name);
    } else {
        test_failed_count++;
        printf("  ✗ %s: Expected non-NULL pointer, got NULL\n", test_name);
    }
}

void test_assert_null(void* ptr, const char* test_name) {
    if (ptr == NULL) {
        test_passed_count++;
        printf("  ✓ %s\n", test_name);
    } else {
        test_failed_count++;
        printf("  ✗ %s: Expected NULL pointer, got %p\n", test_name, ptr);
    }
}

void test_assert_pointer_equal(void* expected, void* actual, const char* test_name) {
    if (expected == actual) {
        test_passed_count++;
        printf("  ✓ %s\n", test_name);
    } else {
        test_failed_count++;
        printf("  ✗ %s: Expected %p, got %p\n", test_name, expected, actual);
    }
}

// External scheduler functions
extern int scheduler_init(uint32_t core_id);
extern void* scheduler_schedule(void);
extern void* scheduler_idle(void);
extern int scheduler_enqueue_process(void* process, uint32_t priority);
extern int scheduler_dequeue_process(void* process);
extern void* scheduler_get_current_process(void);
extern int scheduler_set_current_process(void* process);
extern uint32_t scheduler_get_current_reductions(void);
extern int scheduler_set_current_reductions(uint32_t reductions);
extern uint32_t scheduler_decrement_reductions(void);
extern uint32_t scheduler_get_core_id(void);
extern uint32_t scheduler_get_queue_length(uint32_t priority);

// External process functions
extern void* process_create_fixed(uint64_t entry_point, uint32_t priority, uint64_t scheduler_id);

// Priority level constants
#define PRIORITY_MAX 0
#define PRIORITY_HIGH 1
#define PRIORITY_NORMAL 2
#define PRIORITY_LOW 3

// Process state constants
#define PROCESS_STATE_CREATED 0
#define PROCESS_STATE_READY 1
#define PROCESS_STATE_RUNNING 2
#define PROCESS_STATE_WAITING 3
#define PROCESS_STATE_SUSPENDED 4
#define PROCESS_STATE_TERMINATED 5

// Test scheduler initialization
void test_scheduler_init(void) {
    printf("Testing scheduler initialization...\n");
    
    // Test valid core ID
    int result = scheduler_init(0);
    test_assert_equal(result, 1, "scheduler_init(0) should succeed");
    
    // Test invalid core ID
    result = scheduler_init(8);  // MAX_CORES is 8, so 8 is invalid
    test_assert_equal(result, 0, "scheduler_init(8) should fail");
    
    // Test core ID retrieval
    uint32_t core_id = scheduler_get_core_id();
    test_assert_equal(core_id, 0, "get_core_id should return 0");
    
    // Test initial state
    void* current_process = scheduler_get_current_process();
    test_assert_null(current_process, "initial current process should be NULL");
    
    uint32_t reductions = scheduler_get_current_reductions();
    test_assert_equal(reductions, 2000, "initial reductions should be 2000");
    
    // Test initial queue lengths
    for (int priority = 0; priority < 4; priority++) {
        uint32_t length = scheduler_get_queue_length(priority);
        test_assert_equal(length, 0, "initial queue length should be 0");
    }
}

// Test process enqueue/dequeue
void test_scheduler_enqueue_dequeue(void) {
    printf("Testing scheduler enqueue/dequeue...\n");
    
    // Create test processes
    void* process1 = process_create_fixed(0x1000, PRIORITY_NORMAL, 0);
    test_assert_not_null(process1, "process1 creation should succeed");
    
    void* process2 = process_create_fixed(0x2000, PRIORITY_HIGH, 0);
    test_assert_not_null(process2, "process2 creation should succeed");
    
    void* process3 = process_create_fixed(0x3000, PRIORITY_LOW, 0);
    test_assert_not_null(process3, "process3 creation should succeed");
    
    // Test enqueue to different priorities
    int result = scheduler_enqueue_process(process1, PRIORITY_NORMAL);
    test_assert_equal(result, 1, "enqueue process1 should succeed");
    
    result = scheduler_enqueue_process(process2, PRIORITY_HIGH);
    test_assert_equal(result, 1, "enqueue process2 should succeed");
    
    result = scheduler_enqueue_process(process3, PRIORITY_LOW);
    test_assert_equal(result, 1, "enqueue process3 should succeed");
    
    // Test queue lengths
    uint32_t length = scheduler_get_queue_length(PRIORITY_NORMAL);
    test_assert_equal(length, 1, "NORMAL queue should have 1 process");
    
    length = scheduler_get_queue_length(PRIORITY_HIGH);
    test_assert_equal(length, 1, "HIGH queue should have 1 process");
    
    length = scheduler_get_queue_length(PRIORITY_LOW);
    test_assert_equal(length, 1, "LOW queue should have 1 process");
    
    length = scheduler_get_queue_length(PRIORITY_MAX);
    test_assert_equal(length, 0, "MAX queue should be empty");
    
    // Test dequeue
    result = scheduler_dequeue_process(process1);
    test_assert_equal(result, 1, "dequeue process1 should succeed");
    
    length = scheduler_get_queue_length(PRIORITY_NORMAL);
    test_assert_equal(length, 0, "NORMAL queue should be empty after dequeue");
    
    // Test invalid enqueue
    result = scheduler_enqueue_process(NULL, PRIORITY_NORMAL);
    test_assert_equal(result, 0, "enqueue NULL should fail");
    
    result = scheduler_enqueue_process(process2, 4);  // Invalid priority
    test_assert_equal(result, 0, "enqueue with invalid priority should fail");
    
    // Test invalid dequeue
    result = scheduler_dequeue_process(NULL);
    test_assert_equal(result, 0, "dequeue NULL should fail");
}

// Test scheduling algorithm
void test_scheduler_scheduling(void) {
    printf("Testing scheduler scheduling algorithm...\n");
    
    // Create processes with different priorities
    void* process_low = process_create_fixed(0x1000, PRIORITY_LOW, 0);
    void* process_normal = process_create_fixed(0x2000, PRIORITY_NORMAL, 0);
    void* process_high = process_create_fixed(0x3000, PRIORITY_HIGH, 0);
    void* process_max = process_create_fixed(0x4000, PRIORITY_MAX, 0);
    
    test_assert_not_null(process_low, "process_low creation should succeed");
    test_assert_not_null(process_normal, "process_normal creation should succeed");
    test_assert_not_null(process_high, "process_high creation should succeed");
    test_assert_not_null(process_max, "process_max creation should succeed");
    
    // Enqueue all processes
    scheduler_enqueue_process(process_low, PRIORITY_LOW);
    scheduler_enqueue_process(process_normal, PRIORITY_NORMAL);
    scheduler_enqueue_process(process_high, PRIORITY_HIGH);
    scheduler_enqueue_process(process_max, PRIORITY_MAX);
    
    // Test priority scheduling - MAX should be scheduled first
    void* scheduled = scheduler_schedule();
    test_assert_pointer_equal(scheduled, process_max, "MAX priority process should be scheduled first");
    
    // Test current process is set
    void* current = scheduler_get_current_process();
    test_assert_pointer_equal(current, process_max, "current process should be set");
    
    // Test reductions are set
    uint32_t reductions = scheduler_get_current_reductions();
    test_assert_equal(reductions, 2000, "reductions should be set to 2000");
    
    // Re-enqueue the MAX process and schedule again
    scheduler_enqueue_process(process_max, PRIORITY_MAX);
    scheduled = scheduler_schedule();
    test_assert_pointer_equal(scheduled, process_max, "MAX priority process should be scheduled again");
    
    // Test HIGH priority scheduling
    scheduler_dequeue_process(process_max);  // Remove MAX process
    scheduled = scheduler_schedule();
    test_assert_pointer_equal(scheduled, process_high, "HIGH priority process should be scheduled");
    
    // Test NORMAL priority scheduling
    scheduler_dequeue_process(process_high);  // Remove HIGH process
    scheduled = scheduler_schedule();
    test_assert_pointer_equal(scheduled, process_normal, "NORMAL priority process should be scheduled");
    
    // Test LOW priority scheduling
    scheduler_dequeue_process(process_normal);  // Remove NORMAL process
    scheduled = scheduler_schedule();
    test_assert_pointer_equal(scheduled, process_low, "LOW priority process should be scheduled");
    
    // Test no processes available
    scheduler_dequeue_process(process_low);  // Remove LOW process
    scheduled = scheduler_schedule();
    test_assert_null(scheduled, "scheduler should return NULL when no processes available");
}

// Test round-robin within priority
void test_scheduler_round_robin(void) {
    printf("Testing scheduler round-robin within priority...\n");
    
    // Create multiple processes with same priority
    void* process1 = process_create_fixed(0x1000, PRIORITY_NORMAL, 0);
    void* process2 = process_create_fixed(0x2000, PRIORITY_NORMAL, 0);
    void* process3 = process_create_fixed(0x3000, PRIORITY_NORMAL, 0);
    
    test_assert_not_null(process1, "process1 creation should succeed");
    test_assert_not_null(process2, "process2 creation should succeed");
    test_assert_not_null(process3, "process3 creation should succeed");
    
    // Enqueue processes in order
    scheduler_enqueue_process(process1, PRIORITY_NORMAL);
    scheduler_enqueue_process(process2, PRIORITY_NORMAL);
    scheduler_enqueue_process(process3, PRIORITY_NORMAL);
    
    // Test queue length
    uint32_t length = scheduler_get_queue_length(PRIORITY_NORMAL);
    test_assert_equal(length, 3, "NORMAL queue should have 3 processes");
    
    // Test round-robin scheduling
    void* scheduled = scheduler_schedule();
    test_assert_pointer_equal(scheduled, process1, "first process should be scheduled");
    
    // Re-enqueue and schedule again
    scheduler_enqueue_process(process1, PRIORITY_NORMAL);
    scheduled = scheduler_schedule();
    test_assert_pointer_equal(scheduled, process2, "second process should be scheduled");
    
    // Re-enqueue and schedule again
    scheduler_enqueue_process(process2, PRIORITY_NORMAL);
    scheduled = scheduler_schedule();
    test_assert_pointer_equal(scheduled, process3, "third process should be scheduled");
    
    // Re-enqueue and schedule again (should cycle back to first)
    scheduler_enqueue_process(process3, PRIORITY_NORMAL);
    scheduled = scheduler_schedule();
    test_assert_pointer_equal(scheduled, process1, "should cycle back to first process");
}

// Test reduction counting
void test_scheduler_reductions(void) {
    printf("Testing scheduler reduction counting...\n");
    
    // Test initial reductions
    uint32_t reductions = scheduler_get_current_reductions();
    test_assert_equal(reductions, 2000, "initial reductions should be 2000");
    
    // Test setting reductions
    int result = scheduler_set_current_reductions(1000);
    test_assert_equal(result, 1, "set_reductions(1000) should succeed");
    
    reductions = scheduler_get_current_reductions();
    test_assert_equal(reductions, 1000, "reductions should be 1000");
    
    // Test invalid reductions
    result = scheduler_set_current_reductions(10001);  // > MAX_REDUCTIONS
    test_assert_equal(result, 0, "set_reductions(10001) should fail");
    
    result = scheduler_set_current_reductions(99);  // < MIN_REDUCTIONS
    test_assert_equal(result, 0, "set_reductions(99) should fail");
    
    // Test decrement reductions
    scheduler_set_current_reductions(5);
    reductions = scheduler_decrement_reductions();
    test_assert_equal(reductions, 4, "decrement should return 4");
    
    reductions = scheduler_decrement_reductions();
    test_assert_equal(reductions, 3, "decrement should return 3");
    
    // Test decrement to zero
    scheduler_set_current_reductions(1);
    reductions = scheduler_decrement_reductions();
    test_assert_equal(reductions, 0, "decrement should return 0");
    
    // Test decrement when already zero
    reductions = scheduler_decrement_reductions();
    test_assert_equal(reductions, 0, "decrement when zero should return 0");
}

// Test scheduler idle
void test_scheduler_idle(void) {
    printf("Testing scheduler idle...\n");
    
    // Test idle when no processes
    void* result = scheduler_idle();
    test_assert_null(result, "idle should return NULL when no processes");
    
    // Test idle count increment (we can't directly test this, but we can verify it doesn't crash)
    for (int i = 0; i < 10; i++) {
        result = scheduler_idle();
        test_assert_null(result, "idle should consistently return NULL");
    }
}

// Test current process management
void test_scheduler_current_process(void) {
    printf("Testing scheduler current process management...\n");
    
    // Test initial state
    void* current = scheduler_get_current_process();
    test_assert_null(current, "initial current process should be NULL");
    
    // Create a test process
    void* process = process_create_fixed(0x1000, PRIORITY_NORMAL, 0);
    test_assert_not_null(process, "process creation should succeed");
    
    // Test setting current process
    int result = scheduler_set_current_process(process);
    test_assert_equal(result, 1, "set_current_process should succeed");
    
    current = scheduler_get_current_process();
    test_assert_pointer_equal(current, process, "current process should be set");
    
    // Test clearing current process
    result = scheduler_set_current_process(NULL);
    test_assert_equal(result, 1, "set_current_process(NULL) should succeed");
    
    current = scheduler_get_current_process();
    test_assert_null(current, "current process should be cleared");
}

// Test invalid parameters
void test_scheduler_invalid_params(void) {
    printf("Testing scheduler invalid parameters...\n");
    
    // Test invalid priority for get_queue_length
    uint32_t length = scheduler_get_queue_length(4);  // Invalid priority
    test_assert_equal(length, 0, "get_queue_length with invalid priority should return 0");
    
    // Test invalid priority for enqueue
    void* process = process_create_fixed(0x1000, PRIORITY_NORMAL, 0);
    int result = scheduler_enqueue_process(process, 4);  // Invalid priority
    test_assert_equal(result, 0, "enqueue with invalid priority should fail");
    
    // Test NULL process for enqueue
    result = scheduler_enqueue_process(NULL, PRIORITY_NORMAL);
    test_assert_equal(result, 0, "enqueue NULL process should fail");
    
    // Test NULL process for dequeue
    result = scheduler_dequeue_process(NULL);
    test_assert_equal(result, 0, "dequeue NULL process should fail");
}

// Main test function
void test_scheduler_core(void) {
    printf("=== Scheduler Core Tests ===\n");
    
    test_scheduler_init();
    test_scheduler_enqueue_dequeue();
    test_scheduler_scheduling();
    test_scheduler_round_robin();
    test_scheduler_reductions();
    test_scheduler_idle();
    test_scheduler_current_process();
    test_scheduler_invalid_params();
    
    printf("=== Scheduler Core Tests Complete ===\n");
}
