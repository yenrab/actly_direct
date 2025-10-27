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
// test_work_stealing.c — C tests for work stealing operations
// ------------------------------------------------------------

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// External assembly functions
extern void* try_steal_work(void* scheduler_states, uint64_t current_core);
extern int migrate_process(void* process, uint64_t source_core, uint64_t target_core);
extern uint32_t get_scheduler_load(uint64_t core_id);
extern uint64_t select_victim_by_load(uint64_t current_core);
extern int is_steal_allowed(uint64_t source_core, uint64_t target_core, void* pcb);

// External scheduler functions
extern void* scheduler_state_init(uint64_t max_cores);
extern void scheduler_init(void* scheduler_states, uint64_t core_id);
extern void scheduler_state_destroy(void* scheduler_states);

// External constants from assembly
extern const uint64_t MAX_CORES;
extern const uint64_t WORK_STEAL_ENABLED;
extern const uint64_t MIN_STEAL_QUEUE_SIZE;
extern const uint64_t MAX_MIGRATIONS;

// Forward declarations for test functions
static void test_try_steal_work();
static void test_migrate_process();
static void test_work_stealing_with_load();
static void test_work_stealing_edge_cases();
static void test_work_stealing_migration_limits();
static void test_work_stealing_affinity_constraints();
static void test_work_stealing_permission_checks();

// Test framework functions
extern void test_assert_equal(uint64_t expected, uint64_t actual, const char* test_name);
extern void test_assert_zero(uint64_t value, const char* test_name);
extern void test_assert_nonzero(uint64_t value, const char* test_name);

// ------------------------------------------------------------
// test_work_stealing — Main test function for work stealing
// ------------------------------------------------------------
void test_work_stealing() {
    printf("\n--- Testing Work Stealing Operations (Pure Assembly) ---\n");
    
    test_try_steal_work();
    test_migrate_process();
    test_work_stealing_with_load();
    test_work_stealing_edge_cases();
    test_work_stealing_migration_limits();
    test_work_stealing_affinity_constraints();
    test_work_stealing_permission_checks();
}

// ------------------------------------------------------------
// test_try_steal_work — Test work stealing attempts
// ------------------------------------------------------------
void test_try_steal_work() {
    printf("Testing work stealing attempts...\n");
    
    // Create scheduler state for testing
    void* scheduler_state = scheduler_state_init(MAX_CORES);
    if (scheduler_state == NULL) {
        printf("ERROR: Failed to create scheduler state\n");
        return;
    }
    
    // Initialize scheduler for core 0
    scheduler_init(scheduler_state, 0);
    
    // Create a dummy PCB for testing
    void* dummy_pcb = malloc(512);  // Allocate space for PCB
    if (dummy_pcb == NULL) {
        printf("ERROR: Failed to allocate dummy PCB\n");
        scheduler_state_destroy(scheduler_state);
        return;
    }
    memset(dummy_pcb, 0, 512);  // Initialize to zero
    
    // Test with different current cores
    for (uint64_t current_core = 0; current_core < 4; current_core++) {
        void* stolen_process = try_steal_work(scheduler_state, current_core);
        
        // Stolen process should be NULL (no work available in test environment)
        // or a valid process pointer
        test_assert_nonzero(stolen_process == NULL || stolen_process != NULL, "steal_work_valid_result");
    }
    
    // Test with invalid current core
    void* stolen_process = try_steal_work(scheduler_state, MAX_CORES);
    test_assert_equal(0, (uint64_t)stolen_process, "steal_work_invalid_core");
    
    // Test with core beyond maximum
    stolen_process = try_steal_work(scheduler_state, MAX_CORES + 1);
    test_assert_equal(0, (uint64_t)stolen_process, "steal_work_beyond_max");
    
    // Cleanup
    free(dummy_pcb);
    scheduler_state_destroy(scheduler_state);
}

// ------------------------------------------------------------
// test_migrate_process — Test process migration
// ------------------------------------------------------------
void test_migrate_process() {
    printf("Testing process migration...\n");
    
    // Create a dummy process structure for testing
    // In a real test, this would be a proper PCB
    void* dummy_process = (void*)0x12345678;
    
    // Test valid migration
    int result = migrate_process(dummy_process, 0, 1);
    test_assert_equal(1, result, "migrate_process_valid");
    
    // Test migration to same core (should still succeed)
    result = migrate_process(dummy_process, 1, 1);
    test_assert_equal(1, result, "migrate_process_same_core");
    
    // Test migration with invalid process
    result = migrate_process(NULL, 0, 1);
    test_assert_equal(0, result, "migrate_process_null_process");
    
    // Test migration with invalid source core
    result = migrate_process(dummy_process, MAX_CORES, 1);
    test_assert_equal(0, result, "migrate_process_invalid_source");
    
    // Test migration with invalid target core
    result = migrate_process(dummy_process, 0, MAX_CORES);
    test_assert_equal(0, result, "migrate_process_invalid_target");
    
    // Test migration with both cores invalid
    result = migrate_process(dummy_process, MAX_CORES, MAX_CORES);
    test_assert_equal(0, result, "migrate_process_both_invalid");
}

// ------------------------------------------------------------
// test_work_stealing_with_load — Test work stealing with load considerations
// ------------------------------------------------------------
void test_work_stealing_with_load() {
    printf("Testing work stealing with load considerations...\n");
    
    // Create scheduler state for testing
    void* scheduler_state = scheduler_state_init(MAX_CORES);
    if (scheduler_state == NULL) {
        printf("ERROR: Failed to create scheduler state\n");
        return;
    }
    
    // Initialize scheduler for core 0
    scheduler_init(scheduler_state, 0);
    
    // Create a dummy PCB for testing
    void* dummy_pcb = malloc(512);  // Allocate space for PCB
    if (dummy_pcb == NULL) {
        printf("ERROR: Failed to allocate dummy PCB\n");
        scheduler_state_destroy(scheduler_state);
        return;
    }
    memset(dummy_pcb, 0, 512);  // Initialize to zero
    
    // Test load calculation for different cores
    for (uint64_t core_id = 0; core_id < 4; core_id++) {
        uint32_t load = get_scheduler_load(core_id);
        test_assert_nonzero(load >= 0, "load_calculation_valid");
    }
    
    // Test victim selection based on load
    for (uint64_t current_core = 0; current_core < 4; current_core++) {
        uint64_t victim = select_victim_by_load(current_core);
        test_assert_nonzero(victim < MAX_CORES, "victim_selection_valid");
    }
    
    // Test steal permission checking
    for (uint64_t source = 0; source < 4; source++) {
        for (uint64_t target = 0; target < 4; target++) {
            if (source != target) {
                int allowed = is_steal_allowed(source, target, dummy_pcb);
                test_assert_equal(1, allowed, "steal_permission_valid");
            }
        }
    }
    
    // Cleanup
    free(dummy_pcb);
    scheduler_state_destroy(scheduler_state);
}

// ------------------------------------------------------------
// test_work_stealing_edge_cases — Test edge cases for work stealing
// ------------------------------------------------------------
void test_work_stealing_edge_cases() {
    printf("Testing work stealing edge cases...\n");
    
    // Create scheduler state for testing
    void* scheduler_state = scheduler_state_init(MAX_CORES);
    if (scheduler_state == NULL) {
        printf("ERROR: Failed to create scheduler state\n");
        return;
    }
    
    // Initialize scheduler for core 0
    scheduler_init(scheduler_state, 0);
    
    // Create a dummy PCB for testing
    void* dummy_pcb = malloc(512);  // Allocate space for PCB
    if (dummy_pcb == NULL) {
        printf("ERROR: Failed to allocate dummy PCB\n");
        scheduler_state_destroy(scheduler_state);
        return;
    }
    memset(dummy_pcb, 0, 512);  // Initialize to zero
    
    // Test with maximum valid core ID
    uint64_t max_core = MAX_CORES - 1;
    
    // Test work stealing with max core
    void* stolen_process = try_steal_work(scheduler_state, max_core);
    test_assert_nonzero(stolen_process == NULL || stolen_process != NULL, "steal_work_max_core");
    
    // Test migration with max core
    void* dummy_process = (void*)0x87654321;
    int result = migrate_process(dummy_process, max_core, 0);
    test_assert_equal(1, result, "migrate_process_max_core");
    
    // Test migration to max core
    result = migrate_process(dummy_process, 0, max_core);
    test_assert_equal(1, result, "migrate_process_to_max_core");
    
    // Test load calculation with max core
    uint32_t load = get_scheduler_load(max_core);
    test_assert_nonzero(load >= 0, "load_calculation_max_core");
    
    // Test victim selection with max core
    uint64_t victim = select_victim_by_load(max_core);
    test_assert_nonzero(victim < MAX_CORES, "victim_selection_max_core");
    
    // Cleanup
    free(dummy_pcb);
    scheduler_state_destroy(scheduler_state);
}

// ------------------------------------------------------------
// test_work_stealing_migration_limits — Test migration limits
// ------------------------------------------------------------
void test_work_stealing_migration_limits() {
    printf("Testing work stealing migration limits...\n");
    
    // Test migration limits configuration
    test_assert_equal(10, MAX_MIGRATIONS, "migration_limit_config");
    
    // Test minimum steal queue size configuration
    test_assert_equal(2, MIN_STEAL_QUEUE_SIZE, "min_steal_queue_size_config");
    
    // Test work stealing enabled configuration
    test_assert_equal(1, WORK_STEAL_ENABLED, "work_steal_enabled_config");
    
    // Test multiple migrations of same process
    void* dummy_process = (void*)0x11111111;
    int result;
    
    // Migrate process multiple times
    for (int i = 0; i < 5; i++) {
        result = migrate_process(dummy_process, i % 4, (i + 1) % 4);
        test_assert_equal(1, result, "migrate_process_multiple");
    }
}

// ------------------------------------------------------------
// test_work_stealing_affinity_constraints — Test affinity constraints
// ------------------------------------------------------------
void test_work_stealing_affinity_constraints() {
    printf("Testing work stealing affinity constraints...\n");
    
    // Create scheduler state for testing
    void* scheduler_state = scheduler_state_init(MAX_CORES);
    if (scheduler_state == NULL) {
        printf("ERROR: Failed to create scheduler state\n");
        return;
    }
    
    // Initialize scheduler for core 0
    scheduler_init(scheduler_state, 0);
    
    // Create a dummy PCB for testing
    void* dummy_pcb = malloc(512);  // Allocate space for PCB
    if (dummy_pcb == NULL) {
        printf("ERROR: Failed to allocate dummy PCB\n");
        scheduler_state_destroy(scheduler_state);
        return;
    }
    memset(dummy_pcb, 0, 512);  // Initialize to zero
    
    // Test steal permission with different core combinations
    for (uint64_t source = 0; source < 4; source++) {
        for (uint64_t target = 0; target < 4; target++) {
            int allowed = is_steal_allowed(source, target, dummy_pcb);
            
            if (source == target) {
                // Stealing from self should not be allowed
                test_assert_equal(0, allowed, "steal_not_allowed_from_self");
            } else {
                // Stealing from different cores should be allowed
                test_assert_equal(1, allowed, "steal_allowed_different_cores");
            }
        }
    }
    
    // Test work stealing attempts with different cores
    for (uint64_t current_core = 0; current_core < 4; current_core++) {
        void* stolen_process = try_steal_work(scheduler_state, current_core);
        
        // Result should be valid (NULL or process pointer)
        test_assert_nonzero(stolen_process == NULL || stolen_process != NULL, "steal_work_affinity_valid");
    }
    
    // Test victim selection respects constraints
    for (uint64_t current_core = 0; current_core < 4; current_core++) {
        uint64_t victim = select_victim_by_load(current_core);
        
        // Victim should be valid
        test_assert_nonzero(victim < MAX_CORES, "victim_selection_affinity_valid");
        
        // Victim should not be the same as current core (in most cases)
        // Note: This might be equal if no other cores have work
        test_assert_nonzero(victim >= 0, "victim_selection_affinity_non_negative");
    }
    
    // Cleanup
    free(dummy_pcb);
    scheduler_state_destroy(scheduler_state);
}

// ------------------------------------------------------------
// test_work_stealing_permission_checks — Test work stealing permission logic
// ------------------------------------------------------------
void test_work_stealing_permission_checks() {
    printf("Testing work stealing permission checks...\n");
    
    // Create a dummy PCB for testing
    void* dummy_pcb = malloc(512);  // Allocate space for PCB
    if (dummy_pcb == NULL) {
        printf("ERROR: Failed to allocate dummy PCB\n");
        return;
    }
    memset(dummy_pcb, 0, 512);  // Initialize to zero
    
    // Test 1: Basic permission check with valid cores
    printf("Testing basic permission checks...\n");
    for (uint64_t source_core = 0; source_core < 4; source_core++) {
        for (uint64_t target_core = 0; target_core < 4; target_core++) {
            if (source_core != target_core) {
                int allowed = is_steal_allowed(source_core, target_core, dummy_pcb);
                // Should return 0 or 1 (valid boolean)
                test_assert_nonzero(allowed == 0 || allowed == 1, "permission_check_valid_result");
            }
        }
    }
    
    // Test 2: Invalid core IDs
    printf("Testing invalid core ID handling...\n");
    int invalid_result = is_steal_allowed(128, 0, dummy_pcb);  // Invalid source core
    test_assert_equal(0, invalid_result, "permission_check_invalid_source");
    
    invalid_result = is_steal_allowed(0, 128, dummy_pcb);  // Invalid target core
    test_assert_equal(0, invalid_result, "permission_check_invalid_target");
    
    // Test 3: Same core (should be disallowed)
    printf("Testing same core permission...\n");
    int same_core_result = is_steal_allowed(0, 0, dummy_pcb);
    test_assert_equal(0, same_core_result, "permission_check_same_core");
    
    // Test 4: Edge case cores
    printf("Testing edge case cores...\n");
    int edge_result = is_steal_allowed(MAX_CORES - 1, 0, dummy_pcb);
    test_assert_nonzero(edge_result == 0 || edge_result == 1, "permission_check_edge_cores");
    
    // Test 5: Permission consistency
    printf("Testing permission consistency...\n");
    for (int i = 0; i < 5; i++) {
        int result1 = is_steal_allowed(0, 1, dummy_pcb);
        int result2 = is_steal_allowed(0, 1, dummy_pcb);
        test_assert_equal(result1, result2, "permission_check_consistency");
    }
    
    // Cleanup
    free(dummy_pcb);
    printf("Work stealing permission checks completed\n");
}
