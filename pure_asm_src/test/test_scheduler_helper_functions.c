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
// test_scheduler_helper_functions.c — C tests for pure assembly helper functions
// ------------------------------------------------------------

#include <stdint.h>
#include <stdio.h>

// External assembly functions
extern void* get_scheduler_state(uint64_t core_id);
extern void* get_priority_queue(void* state, uint64_t priority);
extern void test_assert_equal(uint64_t expected, uint64_t actual, const char* test_name);
extern void test_assert_not_zero(uint64_t value, const char* test_name);
extern void test_pass(const char* test_name);
extern void test_fail(uint64_t expected, uint64_t actual, const char* test_name);

// External constants from assembly
extern const uint64_t MAX_CORES_CONST;
extern const uint64_t SCHEDULER_SIZE_CONST;
extern const uint64_t PRIORITY_QUEUE_SIZE_CONST;
extern const uint64_t NUM_PRIORITIES_CONST;
extern const uint64_t DEFAULT_REDUCTIONS;

// Forward declarations for test functions
void test_scheduler_get_scheduler_state(void);
void test_scheduler_get_priority_queue(void);
void test_scheduler_data_structure_layout(void);

// ------------------------------------------------------------
// test_scheduler_helper_functions — Main test function
// ------------------------------------------------------------
void test_scheduler_helper_functions(void) {
    printf("\n--- Testing scheduler helper functions (Pure Assembly) ---\n");
    
    test_scheduler_get_scheduler_state();
    test_scheduler_get_priority_queue();
    test_scheduler_data_structure_layout();
}

// ------------------------------------------------------------
// test_scheduler_get_scheduler_state — Test get_scheduler_state helper
// ------------------------------------------------------------
void test_scheduler_get_scheduler_state(void) {
    // Test get_scheduler_state for core 0
    void* state = get_scheduler_state(0);
    
    // Verify the returned pointer is not NULL
    test_assert_not_zero((uint64_t)state, "scheduler_get_scheduler_state_not_null_core0");
    
    // Test get_scheduler_state for core 1
    void* state1 = get_scheduler_state(1);
    
    // Verify the returned pointer is not NULL
    test_assert_not_zero((uint64_t)state1, "scheduler_get_scheduler_state_not_null_core1");
    
    // Test that different cores return different pointers
    if (state != state1) {
        test_pass("scheduler_get_scheduler_state_different_pointers");
    } else {
        test_fail((uint64_t)state, (uint64_t)state1, "scheduler_get_scheduler_state_different_pointers");
    }
}

// ------------------------------------------------------------
// test_scheduler_get_priority_queue — Test get_priority_queue helper
// ------------------------------------------------------------
void test_scheduler_get_priority_queue(void) {
    // Get scheduler state for core 0
    void* state = get_scheduler_state(0);
    
    // Test get_priority_queue for all priority levels
    for (uint64_t i = 0; i < NUM_PRIORITIES_CONST; i++) {
        void* queue = get_priority_queue(state, i);
        
        // Verify the returned pointer is not NULL
        char test_name[64];
        snprintf(test_name, sizeof(test_name), "scheduler_get_priority_queue_not_null_%llu", i);
        test_assert_not_zero((uint64_t)queue, test_name);
    }
    
    // Test that different priority levels return different pointers
    void* queue0 = get_priority_queue(state, 0);
    void* queue1 = get_priority_queue(state, 1);
    
    if (queue0 != queue1) {
        test_pass("scheduler_get_priority_queue_different_pointers");
    } else {
        test_fail((uint64_t)queue0, (uint64_t)queue1, "scheduler_get_priority_queue_different_pointers");
    }
}

// ------------------------------------------------------------
// test_scheduler_data_structure_layout — Test data structure layout
// ------------------------------------------------------------
void test_scheduler_data_structure_layout(void) {
    // Test that priority_queue_size is correct
    // Each priority queue should have 3 quad words (head, tail, count)
    test_assert_equal(24, PRIORITY_QUEUE_SIZE_CONST, "scheduler_priority_queue_size");
    
    // Test that scheduler_size is correct
    // Should be: core_id + queues + current_process + reduction_count + 3 statistics + waiting queues + yield statistics
    // = 1 + (4 * 3) + 1 + 1 + 3 + (3 * 3) + 2 = 30 quad words = 240 bytes
    test_assert_equal(240, SCHEDULER_SIZE_CONST, "scheduler_scheduler_size");
    
    // Test that NUM_PRIORITIES is 4
    test_assert_equal(4, NUM_PRIORITIES_CONST, "scheduler_num_priorities");
    
    // Test that DEFAULT_REDUCTIONS is 2000
    test_assert_equal(2000, DEFAULT_REDUCTIONS, "scheduler_default_reductions");
    
    // Test that MAX_CORES is 128
    test_assert_equal(128, MAX_CORES_CONST, "scheduler_max_cores");
}
