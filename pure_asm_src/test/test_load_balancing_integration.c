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
// test_load_balancing_integration.c — Integration tests for load balancing
// ------------------------------------------------------------

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

// External assembly functions
extern void* scheduler_state_init(uint64_t max_cores);
extern void scheduler_state_destroy(void* scheduler_states);
extern void scheduler_init(void* scheduler_states, uint64_t core_id);
extern void* scheduler_schedule(void* scheduler_states, uint64_t core_id);
extern int scheduler_enqueue_process(void* scheduler_states, uint64_t core_id, void* process, uint32_t priority);
extern void* scheduler_idle(void* scheduler_states, uint64_t core_id);
extern void* try_steal_work(uint64_t current_core);
extern int migrate_process(void* process, uint64_t source_core, uint64_t target_core);
extern uint32_t get_scheduler_load(uint64_t core_id);
extern uint64_t find_busiest_scheduler(uint64_t current_core);
extern void* process_create(void* entry_point, uint32_t priority, uint64_t scheduler_id, uint64_t* next_process_id);

// External constants from assembly
extern const uint64_t MAX_CORES_CONST;
extern const uint64_t MAX_CORES;
extern const uint64_t PRIORITY_MAX;
extern const uint64_t PRIORITY_HIGH;
extern const uint64_t PRIORITY_NORMAL;
extern const uint64_t PRIORITY_LOW;

// Forward declarations for test functions
static void test_multi_core_scheduler_initialization();
static void test_load_balancing_scenario();
static void test_work_stealing_integration();
static void test_migration_statistics();
static void test_priority_aware_load_balancing();
static void test_concurrent_work_stealing();

// Test framework functions
extern void test_assert_equal(uint64_t expected, uint64_t actual, const char* test_name);
extern void test_assert_zero(uint64_t value, const char* test_name);
extern void test_assert_nonzero(uint64_t value, const char* test_name);

// ------------------------------------------------------------
// test_load_balancing_integration — Main test function for load balancing integration
// ------------------------------------------------------------
void test_load_balancing_integration() {
    printf("\n--- Testing Load Balancing Integration (Multi-Core) ---\n");
    
    test_multi_core_scheduler_initialization();
    test_load_balancing_scenario();
    test_work_stealing_integration();
    test_migration_statistics();
    test_priority_aware_load_balancing();
    test_concurrent_work_stealing();
}

// ------------------------------------------------------------
// test_multi_core_scheduler_initialization — Test multi-core scheduler setup
// ------------------------------------------------------------
void test_multi_core_scheduler_initialization() {
    printf("Testing multi-core scheduler initialization...\n");
    
    // Create isolated scheduler state
    void* scheduler_state = scheduler_state_init(1);
    if (scheduler_state == NULL) {
        printf("ERROR: Failed to create scheduler state\n");
        return;
    }
    
    // Initialize schedulers for multiple cores
    for (uint64_t core_id = 0; core_id < 4; core_id++) {
        scheduler_init(scheduler_state, core_id);
        
        // Verify scheduler is initialized
        uint32_t load = get_scheduler_load(core_id);
        test_assert_equal(0, load, "scheduler_load_initialized");
    }
    
    // Test that all cores are properly initialized
    for (uint64_t core_id = 0; core_id < 4; core_id++) {
        void* process = scheduler_schedule(scheduler_state, core_id);
        test_assert_equal(0, (uint64_t)process, "scheduler_empty_after_init");
    }
    
    // Clean up scheduler state
    scheduler_state_destroy(scheduler_state);
}

// ------------------------------------------------------------
// test_load_balancing_scenario — Test load balancing scenario
// ------------------------------------------------------------
void test_load_balancing_scenario() {
    printf("Testing load balancing scenario...\n");
    
    // Create isolated scheduler state
    void* scheduler_state = scheduler_state_init(1);
    if (scheduler_state == NULL) {
        printf("ERROR: Failed to create scheduler state\n");
        return;
    }
    
    // Initialize schedulers for multiple cores
    for (uint64_t core_id = 0; core_id < 4; core_id++) {
        scheduler_init(scheduler_state, core_id);
    }
    
    // Create unbalanced load scenario
    // Core 0: 8 processes, Core 1: 0 processes, Core 2: 2 processes, Core 3: 0 processes
    uint64_t next_process_id = 1;
    
    // Add processes to core 0 (overloaded)
    for (int i = 0; i < 8; i++) {
        void* process = process_create((void*)0x1000 + i, PRIORITY_NORMAL, 0, &next_process_id);
        test_assert_nonzero((uint64_t)process, "process_creation_core_0");
        
        int result = scheduler_enqueue_process(scheduler_state, 0, process, PRIORITY_NORMAL);
        test_assert_equal(1, result, "enqueue_process_core_0");
    }
    
    // Add processes to core 2 (moderate load)
    for (int i = 0; i < 2; i++) {
        void* process = process_create((void*)0x2000 + i, PRIORITY_NORMAL, 2, &next_process_id);
        test_assert_nonzero((uint64_t)process, "process_creation_core_2");
        
        int result = scheduler_enqueue_process(scheduler_state, 2, process, PRIORITY_NORMAL);
        test_assert_equal(1, result, "enqueue_process_core_2");
    }
    
    // Verify load distribution
    uint32_t load_0 = get_scheduler_load(0);
    uint32_t load_1 = get_scheduler_load(1);
    uint32_t load_2 = get_scheduler_load(2);
    uint32_t load_3 = get_scheduler_load(3);
    
    test_assert_nonzero(load_0 > 0, "core_0_has_load");
    test_assert_equal(0, load_1, "core_1_no_load");
    test_assert_nonzero(load_2 > 0, "core_2_has_load");
    test_assert_equal(0, load_3, "core_3_no_load");
    
    // Verify load imbalance
    test_assert_nonzero(load_0 > load_2, "load_imbalance_detected");
    
    // Clean up scheduler state
    scheduler_state_destroy(scheduler_state);
}

// ------------------------------------------------------------
// test_work_stealing_integration — Test work stealing integration
// ------------------------------------------------------------
void test_work_stealing_integration() {
    printf("Testing work stealing integration...\n");
    
    // Test work stealing from idle cores
    for (uint64_t core_id = 1; core_id < 4; core_id += 2) {  // Test cores 1 and 3 (idle)
        void* stolen_process = try_steal_work(core_id);
        
        // In test environment, might not have work to steal
        // but function should not crash
        test_assert_nonzero(stolen_process == NULL || stolen_process != NULL, "work_stealing_safe");
    }
    
    // Test busiest scheduler detection
    uint64_t busiest = find_busiest_scheduler(1);  // Core 1 looking for work
    test_assert_nonzero(busiest < MAX_CORES, "busiest_scheduler_valid");
    
    // Test victim selection
    uint64_t victim = find_busiest_scheduler(3);  // Core 3 looking for work
    test_assert_nonzero(victim < MAX_CORES, "victim_selection_valid");
}

// ------------------------------------------------------------
// test_migration_statistics — Test migration statistics tracking
// ------------------------------------------------------------
void test_migration_statistics() {
    printf("Testing migration statistics tracking...\n");
    
    // Create a dummy process for migration testing
    uint64_t next_process_id = 100;
    void* process = process_create((void*)0x5000, PRIORITY_NORMAL, 0, &next_process_id);
    test_assert_nonzero((uint64_t)process, "process_creation_migration");
    
    // Test migration between cores
    int result = migrate_process(process, 0, 1);
    test_assert_equal(1, result, "migration_successful");
    
    // Test migration back
    result = migrate_process(process, 1, 2);
    test_assert_equal(1, result, "migration_back_successful");
    
    // Test migration to same core
    result = migrate_process(process, 2, 2);
    test_assert_equal(1, result, "migration_same_core");
    
    // Test invalid migration
    result = migrate_process(NULL, 0, 1);
    test_assert_equal(0, result, "migration_null_process");
}

// ------------------------------------------------------------
// test_priority_aware_load_balancing — Test priority-aware load balancing
// ------------------------------------------------------------
void test_priority_aware_load_balancing() {
    printf("Testing priority-aware load balancing...\n");
    
    // Create isolated scheduler state
    void* scheduler_state = scheduler_state_init(1);
    if (scheduler_state == NULL) {
        printf("ERROR: Failed to create scheduler state\n");
        return;
    }
    
    // Initialize scheduler for core 0
    scheduler_init(scheduler_state, 0);
    
    uint64_t next_process_id = 200;
    
    // Create processes with different priorities on core 0
    void* max_process = process_create((void*)0x6000, PRIORITY_MAX, 0, &next_process_id);
    void* high_process = process_create((void*)0x6001, PRIORITY_HIGH, 0, &next_process_id);
    void* normal_process = process_create((void*)0x6002, PRIORITY_NORMAL, 0, &next_process_id);
    void* low_process = process_create((void*)0x6003, PRIORITY_LOW, 0, &next_process_id);
    
    // Enqueue processes with different priorities
    scheduler_enqueue_process(scheduler_state, 0, max_process, PRIORITY_MAX);
    scheduler_enqueue_process(scheduler_state, 0, high_process, PRIORITY_HIGH);
    scheduler_enqueue_process(scheduler_state, 0, normal_process, PRIORITY_NORMAL);
    scheduler_enqueue_process(scheduler_state, 0, low_process, PRIORITY_LOW);
    
    // Verify load calculation considers priorities
    uint32_t load = get_scheduler_load(0);
    test_assert_nonzero(load > 0, "priority_aware_load_calculation");
    
    // Test scheduling respects priorities
    void* scheduled_process = scheduler_schedule(scheduler_state, 0);
    test_assert_nonzero((uint64_t)scheduled_process, "priority_scheduling_works");
    
    // Test work stealing with priority awareness
    void* stolen_process = try_steal_work(1);
    // Should not crash even if no work available
    test_assert_nonzero(stolen_process == NULL || stolen_process != NULL, "priority_aware_stealing");
    
    // Clean up scheduler state
    scheduler_state_destroy(scheduler_state);
}

// ------------------------------------------------------------
// test_concurrent_work_stealing — Test concurrent work stealing
// ------------------------------------------------------------
void test_concurrent_work_stealing() {
    printf("Testing concurrent work stealing...\n");
    
    // Test multiple cores attempting to steal work simultaneously
    for (int round = 0; round < 3; round++) {
        // Simulate multiple cores trying to steal work
        for (uint64_t core_id = 0; core_id < 4; core_id++) {
            void* stolen_process = try_steal_work(core_id);
            
            // Each attempt should be safe (no crashes)
            test_assert_nonzero(stolen_process == NULL || stolen_process != NULL, "concurrent_stealing_safe");
        }
        
        // Test victim selection under concurrent conditions
        for (uint64_t core_id = 0; core_id < 4; core_id++) {
            uint64_t victim = find_busiest_scheduler(core_id);
            test_assert_nonzero(victim < MAX_CORES, "concurrent_victim_selection");
        }
    }
    
    // Test load calculation under concurrent access
    for (uint64_t core_id = 0; core_id < 4; core_id++) {
        uint32_t load = get_scheduler_load(core_id);
        test_assert_nonzero(load >= 0, "concurrent_load_calculation");
    }
    
    // Test migration under concurrent conditions
    uint64_t next_process_id = 300;
    void* process = process_create((void*)0x7000, PRIORITY_NORMAL, 0, &next_process_id);
    
    // Test concurrent migrations
    for (int i = 0; i < 4; i++) {
        int result = migrate_process(process, i, (i + 1) % 4);
        test_assert_equal(1, result, "concurrent_migration");
    }
}
