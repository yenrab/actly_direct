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
// test_scheduler_queue_length.c — C tests for scheduler queue length functions
// ------------------------------------------------------------
// Tests the _scheduler_get_queue_length_from_queue and _scheduler_get_queue_length_queue_ptr
// functions to ensure they work correctly and provide proper queue length information.

#include <stdint.h>
#include <stdio.h>

// External assembly functions
extern void* scheduler_state_init(uint64_t max_cores);
extern void scheduler_state_destroy(void* scheduler_states);
extern void* get_scheduler_state(void* scheduler_states, uint64_t core_id);
extern void* get_priority_queue(void* state, uint64_t priority);
extern uint64_t scheduler_get_queue_length_from_queue(void* queue);
extern uint64_t scheduler_get_queue_length_queue_ptr(void* queue);

// External test framework functions
extern const uint64_t MAX_CORES_CONST;
extern void test_assert_equal(uint64_t expected, uint64_t actual, const char* test_name);
extern void test_assert_not_null(void* ptr, const char* test_name);
extern void test_pass(const char* test_name);
extern void test_fail(uint64_t expected, uint64_t actual, const char* test_name);

// Forward declarations for test functions
void test_scheduler_get_queue_length_from_queue();
void test_scheduler_get_queue_length_queue_ptr();
void test_scheduler_queue_length_consistency();

// ------------------------------------------------------------
// test_scheduler_queue_length — Main test function
// ------------------------------------------------------------
void test_scheduler_queue_length() {
    printf("\n--- Testing scheduler queue length functions (Pure Assembly) ---\n");
    
    test_scheduler_get_queue_length_from_queue();
    test_scheduler_get_queue_length_queue_ptr();
    test_scheduler_queue_length_consistency();
}

// ------------------------------------------------------------
// test_scheduler_get_queue_length_from_queue — Test _scheduler_get_queue_length_from_queue
// ------------------------------------------------------------
void test_scheduler_get_queue_length_from_queue() {
    printf("\n--- Testing _scheduler_get_queue_length_from_queue ---\n");
    
    // Create isolated scheduler state
    void* scheduler_state = scheduler_state_init(1);
    if (scheduler_state == NULL) {
        printf("ERROR: Failed to create scheduler state\n");
        return;
    }
    
    // Get scheduler state for core 0
    void* state = get_scheduler_state(scheduler_state, 0);
    test_assert_not_null(state, "scheduler_get_queue_length_from_queue_state_not_null");
    
    if (state != NULL) {
        // Test all priority queues
        for (uint64_t priority = 0; priority < 4; priority++) {
            void* queue = get_priority_queue(state, priority);
            test_assert_not_null(queue, "scheduler_get_queue_length_from_queue_queue_not_null");
            
            if (queue != NULL) {
                uint64_t length = scheduler_get_queue_length_from_queue(queue);
                
                // Initially, all queues should be empty (length = 0)
                char test_name[64];
                snprintf(test_name, sizeof(test_name), "scheduler_get_queue_length_from_queue_empty_priority_%llu", priority);
                test_assert_equal(0, length, test_name);
            }
        }
    }
    
    // Clean up scheduler state
    scheduler_state_destroy(scheduler_state);
}

// ------------------------------------------------------------
// test_scheduler_get_queue_length_queue_ptr — Test _scheduler_get_queue_length_queue_ptr
// ------------------------------------------------------------
void test_scheduler_get_queue_length_queue_ptr() {
    printf("\n--- Testing _scheduler_get_queue_length_queue_ptr ---\n");
    
    // Create isolated scheduler state
    void* scheduler_state = scheduler_state_init(1);
    if (scheduler_state == NULL) {
        printf("ERROR: Failed to create scheduler state\n");
        return;
    }
    
    // Get scheduler state for core 0
    void* state = get_scheduler_state(scheduler_state, 0);
    test_assert_not_null(state, "scheduler_get_queue_length_queue_ptr_state_not_null");
    
    if (state != NULL) {
        // Test all priority queues
        for (uint64_t priority = 0; priority < 4; priority++) {
            void* queue = get_priority_queue(state, priority);
            test_assert_not_null(queue, "scheduler_get_queue_length_queue_ptr_queue_not_null");
            
            if (queue != NULL) {
                uint64_t length = scheduler_get_queue_length_queue_ptr(queue);
                
                // Initially, all queues should be empty (length = 0)
                char test_name[64];
                snprintf(test_name, sizeof(test_name), "scheduler_get_queue_length_queue_ptr_empty_priority_%llu", priority);
                test_assert_equal(0, length, test_name);
            }
        }
    }
    
    // Clean up scheduler state
    scheduler_state_destroy(scheduler_state);
}

// ------------------------------------------------------------
// test_scheduler_queue_length_consistency — Test consistency between the two functions
// ------------------------------------------------------------
void test_scheduler_queue_length_consistency() {
    printf("\n--- Testing scheduler queue length consistency ---\n");
    
    // Create isolated scheduler state
    void* scheduler_state = scheduler_state_init(1);
    if (scheduler_state == NULL) {
        printf("ERROR: Failed to create scheduler state\n");
        return;
    }
    
    // Get scheduler state for core 0
    void* state = get_scheduler_state(scheduler_state, 0);
    test_assert_not_null(state, "scheduler_queue_length_consistency_state_not_null");
    
    if (state != NULL) {
        // Test that both functions return the same result for the same queue
        for (uint64_t priority = 0; priority < 4; priority++) {
            void* queue = get_priority_queue(state, priority);
            test_assert_not_null(queue, "scheduler_queue_length_consistency_queue_not_null");
            
            if (queue != NULL) {
                uint64_t length1 = scheduler_get_queue_length_from_queue(queue);
                uint64_t length2 = scheduler_get_queue_length_queue_ptr(queue);
                
                // Both functions should return the same result
                char test_name[64];
                snprintf(test_name, sizeof(test_name), "scheduler_queue_length_consistency_priority_%llu", priority);
                test_assert_equal(length1, length2, test_name);
            }
        }
    }
    
    // Clean up scheduler state
    scheduler_state_destroy(scheduler_state);
}
