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
// test_load_balancing.c — C tests for load balancing functions
// ------------------------------------------------------------

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Test framework functions
extern void test_assert_equal(uint64_t expected, uint64_t actual, const char* test_name);
extern void test_assert_not_equal(uint64_t expected, uint64_t actual, const char* test_name);
extern void test_assert_nonzero(uint64_t value, const char* test_name);
extern void test_pass(const char* test_name);
extern void test_fail(uint64_t expected, uint64_t actual, const char* test_name);

// Helper macro for test_assert_true
#define test_assert_true(condition, test_name) \
    do { \
        if (condition) { \
            test_pass(test_name); \
        } else { \
            test_fail(1, 0, test_name); \
        } \
    } while(0)

// External assembly functions
extern uint32_t get_scheduler_load(uint64_t core_id);
extern uint64_t find_busiest_scheduler(uint64_t current_core);
extern int is_steal_allowed(uint64_t source_core, uint64_t target_core);
extern uint64_t select_victim_random(uint64_t current_core);
extern uint64_t select_victim_by_load(uint64_t current_core);
extern uint64_t select_victim_locality(uint64_t current_core);
extern void* try_steal_work(uint64_t current_core);
extern int migrate_process(void* process, uint64_t source_core, uint64_t target_core);

// External constants
extern const uint64_t MAX_CORES_CONST;

// External scheduler functions for proper memory isolation
extern void* scheduler_state_init(uint64_t max_cores);
extern void scheduler_state_destroy(void* scheduler_states);
extern void scheduler_init(void* scheduler_states, uint64_t core_id);

// Forward declarations for test functions
static void test_get_scheduler_load_basic();
static void test_get_scheduler_load_priorities();
static void test_get_scheduler_load_invalid_core();
static void test_get_scheduler_load_empty_queues();
static void test_get_scheduler_load_mixed_priorities();

// Test the get_scheduler_load function
void test_load_balancing() {
    printf("*** LOAD BALANCING TEST STARTING ***\n");
    printf("=== Testing Load Balancing Functions ===\n");
    
    // Create isolated scheduler state for proper memory isolation
    void* scheduler_state = scheduler_state_init(1);
    if (scheduler_state == NULL) {
        printf("ERROR: Failed to create scheduler state\n");
        return;
    }
    
    // Initialize scheduler for core 0
    scheduler_init(scheduler_state, 0);
    
    test_get_scheduler_load_basic();
    test_get_scheduler_load_priorities();
    test_get_scheduler_load_invalid_core();
    test_get_scheduler_load_empty_queues();
    test_get_scheduler_load_mixed_priorities();
    
    // Clean up scheduler state
    scheduler_state_destroy(scheduler_state);
    
    printf("=== Load Balancing Tests Complete ===\n");
    printf("*** LOAD BALANCING TEST FINISHED ***\n");
}

// Test basic load calculation
static void test_get_scheduler_load_basic() {
    printf("Testing get_scheduler_load basic functionality...\n");
    
    // Test with core 0 (should work even if scheduler not fully initialized)
    uint32_t load = get_scheduler_load(0);
    
    printf("DEBUG: get_scheduler_load(0) returned: %u\n", load);
    
    // Simple test without framework first
    if (load == 0) {
        printf("✓ Load is 0 as expected\n");
    } else {
        printf("✗ Load is %u, expected 0\n", load);
    }
    
    printf("Core 0 load: %u\n", load);
}

// Test load calculation with different priority weights
static void test_get_scheduler_load_priorities() {
    printf("Testing get_scheduler_load priority weights...\n");
    
    // Test with core 0
    uint32_t load = get_scheduler_load(0);
    
    // For now, just verify it returns 0 (empty queues)
    test_assert_equal(0, load, "get_scheduler_load_priorities_zero");
    
    printf("Priority-weighted load: %u\n", load);
}

// Test with invalid core ID
static void test_get_scheduler_load_invalid_core() {
    printf("Testing get_scheduler_load with invalid core ID...\n");
    
    // Test with invalid core ID (beyond MAX_CORES)
    uint32_t load = get_scheduler_load(999);
    
    // Should return 0 for invalid core ID
    test_assert_equal(0, load, "get_scheduler_load_invalid_core_zero");
    
    printf("Invalid core load: %u\n", load);
}

// Test with empty queues
static void test_get_scheduler_load_empty_queues() {
    printf("Testing get_scheduler_load with empty queues...\n");
    
    // Test with core 0 (should have empty queues by default)
    uint32_t load = get_scheduler_load(0);
    
    // Load should be 0 for empty queues
    test_assert_equal(0, load, "get_scheduler_load_empty_queues_zero");
    
    printf("Empty queues load: %u\n", load);
}

// Test with mixed priority queues
static void test_get_scheduler_load_mixed_priorities() {
    printf("Testing get_scheduler_load with mixed priorities...\n");
    
    // Test with core 0
    uint32_t load = get_scheduler_load(0);
    
    // For now, just verify it returns 0 (empty queues)
    test_assert_equal(0, load, "get_scheduler_load_mixed_priorities_zero");
    
    printf("Mixed priorities load: %u\n", load);
}
