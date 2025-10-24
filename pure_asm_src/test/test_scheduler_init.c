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
// test_scheduler_init.c — C tests for pure assembly scheduler initialization
// ------------------------------------------------------------

#include <stdint.h>
#include <stdio.h>

// External assembly functions
extern void* scheduler_state_init(uint64_t max_cores);
extern void scheduler_state_destroy(void* scheduler_states);
extern void scheduler_init(void* scheduler_states, uint64_t core_id);
extern void* get_scheduler_state(void* scheduler_states, uint64_t core_id);
extern void test_assert_equal(uint64_t expected, uint64_t actual, const char* test_name);
extern void test_assert_zero(uint64_t value, const char* test_name);

// External constants from assembly
extern const uint64_t MAX_CORES_CONST;
extern const uint64_t DEFAULT_REDUCTIONS;
extern const uint64_t SCHEDULER_SIZE_CONST;
extern const uint64_t PRIORITY_QUEUE_SIZE_CONST;

// Forward declarations for test functions
static void test_scheduler_init_core_id();
static void test_scheduler_init_queues();
static void test_scheduler_init_current_process();
static void test_scheduler_init_reduction_count();
static void test_scheduler_init_statistics();

// ------------------------------------------------------------
// test_scheduler_init — Main test function for scheduler_init
// ------------------------------------------------------------
void test_scheduler_init() {
    printf("\n--- Testing scheduler_init (Pure Assembly) ---\n");
    
    test_scheduler_init_core_id();
    test_scheduler_init_queues();
    test_scheduler_init_current_process();
    test_scheduler_init_reduction_count();
    test_scheduler_init_statistics();
}

// ------------------------------------------------------------
// test_scheduler_init_core_id — Test core ID initialization
// ------------------------------------------------------------
void test_scheduler_init_core_id() {
    // Create multi-core scheduler state for 3 cores
    void* scheduler_state = scheduler_state_init(3);
    if (scheduler_state == NULL) {
        printf("ERROR: Failed to create scheduler state\n");
        return;
    }
    
    // Test core 0
    scheduler_init(scheduler_state, 0);
    void* state = get_scheduler_state(scheduler_state, 0);
    uint64_t core_id = *(uint64_t*)state; // First field is core_id
    
    test_assert_equal(0, core_id, "scheduler_init_core_id_0");
    
    // Test core 1
    scheduler_init(scheduler_state, 1);
    state = get_scheduler_state(scheduler_state, 1);
    core_id = *(uint64_t*)state;
    
    test_assert_equal(1, core_id, "scheduler_init_core_id_1");
    
    // Test core 2
    scheduler_init(scheduler_state, 2);
    state = get_scheduler_state(scheduler_state, 2);
    core_id = *(uint64_t*)state;
    
    test_assert_equal(2, core_id, "scheduler_init_core_id_2");
    
    // Clean up scheduler state
    scheduler_state_destroy(scheduler_state);
}

// ------------------------------------------------------------
// test_scheduler_init_queues — Test priority queue initialization
// ------------------------------------------------------------
void test_scheduler_init_queues() {
    // Create single-core scheduler state
    void* scheduler_state = scheduler_state_init(1);
    if (scheduler_state == NULL) {
        printf("ERROR: Failed to create scheduler state\n");
        return;
    }
    
    // Initialize scheduler for core 0
    scheduler_init(scheduler_state, 0);
    void* state = get_scheduler_state(scheduler_state, 0);
    
    // Test all priority queues are empty
    for (int i = 0; i < 4; i++) {
        uint64_t offset = 8 + (i * PRIORITY_QUEUE_SIZE_CONST); // Skip core_id, then queue offset
        
        // Check head is NULL
        void* head = *(void**)((char*)state + offset);
        char test_name[64];
        snprintf(test_name, sizeof(test_name), "scheduler_init_queue_%d_head", i);
        test_assert_zero((uint64_t)head, test_name);
        
        // Check tail is NULL
        void* tail = *(void**)((char*)state + offset + 8);
        snprintf(test_name, sizeof(test_name), "scheduler_init_queue_%d_tail", i);
        test_assert_zero((uint64_t)tail, test_name);
        
        // Check count is 0
        uint64_t count = *(uint64_t*)((char*)state + offset + 16);
        snprintf(test_name, sizeof(test_name), "scheduler_init_queue_%d_count", i);
        test_assert_zero(count, test_name);
    }
    
    // Clean up scheduler state
    scheduler_state_destroy(scheduler_state);
}

// ------------------------------------------------------------
// test_scheduler_init_current_process — Test current process initialization
// ------------------------------------------------------------
void test_scheduler_init_current_process() {
    // Create multi-core scheduler state for 2 cores
    void* scheduler_state = scheduler_state_init(2);
    if (scheduler_state == NULL) {
        printf("ERROR: Failed to create scheduler state\n");
        return;
    }
    
    // Initialize scheduler for core 0
    scheduler_init(scheduler_state, 0);
    void* state = get_scheduler_state(scheduler_state, 0);
    
    // Verify current process is NULL (offset: core_id + queues = 8 + 4*24 = 104)
    void* current_process = *(void**)((char*)state + 104);
    test_assert_zero((uint64_t)current_process, "scheduler_init_current_process_null");
    
    // Test multiple cores
    scheduler_init(scheduler_state, 1);
    state = get_scheduler_state(scheduler_state, 1);
    current_process = *(void**)((char*)state + 104);
    test_assert_zero((uint64_t)current_process, "scheduler_init_current_process_null_core1");
    
    // Clean up scheduler state
    scheduler_state_destroy(scheduler_state);
}

// ------------------------------------------------------------
// test_scheduler_init_reduction_count — Test reduction count initialization
// ------------------------------------------------------------
void test_scheduler_init_reduction_count() {
    // Create multi-core scheduler state for 2 cores
    void* scheduler_state = scheduler_state_init(2);
    if (scheduler_state == NULL) {
        printf("ERROR: Failed to create scheduler state\n");
        return;
    }
    
    // Initialize scheduler for core 0
    scheduler_init(scheduler_state, 0);
    void* state = get_scheduler_state(scheduler_state, 0);
    
    // Verify defaultreduction count is 2000 (offset: 104 + 8 = 112)
    uint64_t reduction_count = *(uint64_t*)((char*)state + 112);
    test_assert_equal(2000, reduction_count, "scheduler_init_reduction_count_default");
    
    // Test multiple cores
    scheduler_init(scheduler_state, 1);
    state = get_scheduler_state(scheduler_state, 1);
    reduction_count = *(uint64_t*)((char*)state + 112);
    test_assert_equal(2000, reduction_count, "scheduler_init_reduction_count_default_core1");
    
    // Clean up scheduler state
    scheduler_state_destroy(scheduler_state);
}

// ------------------------------------------------------------
// test_scheduler_init_statistics — Test statistics initialization
// ------------------------------------------------------------
void test_scheduler_init_statistics() {
    // Create multi-core scheduler state for 2 cores
    void* scheduler_state = scheduler_state_init(2);
    if (scheduler_state == NULL) {
        printf("ERROR: Failed to create scheduler state\n");
        return;
    }
    
    // Initialize scheduler for core 0
    scheduler_init(scheduler_state, 0);
    void* state = get_scheduler_state(scheduler_state, 0);
    
    // Check total_scheduled is 0 (offset: 112 + 8 = 120)
    uint64_t total_scheduled = *(uint64_t*)((char*)state + 120);
    test_assert_zero(total_scheduled, "scheduler_init_stats_scheduled");
    
    // Check total_yields is 0 (offset: 120 + 8 = 128)
    uint64_t total_yields = *(uint64_t*)((char*)state + 128);
    test_assert_zero(total_yields, "scheduler_init_stats_yields");
    
    // Check total_migrations is 0 (offset: 128 + 8 = 136)
    uint64_t total_migrations = *(uint64_t*)((char*)state + 136);
    test_assert_zero(total_migrations, "scheduler_init_stats_migrations");
    
    // Test multiple cores
    scheduler_init(scheduler_state, 1);
    state = get_scheduler_state(scheduler_state, 1);
    
    total_scheduled = *(uint64_t*)((char*)state + 120);
    test_assert_zero(total_scheduled, "scheduler_init_stats_scheduled_core1");
    
    total_yields = *(uint64_t*)((char*)state + 128);
    test_assert_zero(total_yields, "scheduler_init_stats_yields_core1");
    
    total_migrations = *(uint64_t*)((char*)state + 136);
    test_assert_zero(total_migrations, "scheduler_init_stats_migrations_core1");
    
    // Clean up scheduler state
    scheduler_state_destroy(scheduler_state);
}
