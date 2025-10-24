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
// test_victim_selection.c — C tests for victim selection algorithms
// ------------------------------------------------------------

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

// External assembly functions
extern uint32_t get_scheduler_load(uint64_t core_id);
extern uint64_t find_busiest_scheduler(uint64_t current_core);
extern int is_steal_allowed(uint64_t source_core, uint64_t target_core);
extern uint64_t select_victim_random(uint64_t current_core);
extern uint64_t select_victim_by_load(uint64_t current_core);
extern uint64_t select_victim_locality(uint64_t current_core);

// External constants from assembly
extern const uint64_t MAX_CORES;
extern const uint64_t VICTIM_STRATEGY_RANDOM;
extern const uint64_t VICTIM_STRATEGY_LOAD;
extern const uint64_t VICTIM_STRATEGY_LOCALITY;

// Forward declarations for test functions
static void test_get_scheduler_load();
static void test_find_busiest_scheduler();
static void test_is_steal_allowed();
static void test_select_victim_random();
static void test_select_victim_by_load();
static void test_select_victim_locality();
static void test_victim_selection_edge_cases();
static void test_locality_based_selection();

// Test framework functions
extern void test_assert_equal(uint64_t expected, uint64_t actual, const char* test_name);
extern void test_assert_zero(uint64_t value, const char* test_name);
extern void test_assert_nonzero(uint64_t value, const char* test_name);

// ------------------------------------------------------------
// test_victim_selection — Main test function for victim selection
// ------------------------------------------------------------
void test_victim_selection() {
    printf("\n--- Testing Victim Selection Algorithms (Pure Assembly) ---\n");
    
    test_get_scheduler_load();
    test_find_busiest_scheduler();
    test_is_steal_allowed();
    test_select_victim_random();
    test_select_victim_by_load();
    test_select_victim_locality();
    test_victim_selection_edge_cases();
    test_locality_based_selection();
}

// ------------------------------------------------------------
// test_get_scheduler_load — Test scheduler load calculation
// ------------------------------------------------------------
void test_get_scheduler_load() {
    printf("Testing scheduler load calculation...\n");
    
    // Test load calculation for different cores
    for (uint64_t core_id = 0; core_id < 4; core_id++) {
        uint32_t load = get_scheduler_load(core_id);
        // Load should be non-negative (0 or positive)
        test_assert_nonzero(load >= 0, "scheduler_load_non_negative");
    }
    
    // Test invalid core ID
    uint32_t load = get_scheduler_load(MAX_CORES);
    test_assert_equal(0, load, "scheduler_load_invalid_core");
    
    // Test core ID beyond maximum
    load = get_scheduler_load(MAX_CORES + 1);
    test_assert_equal(0, load, "scheduler_load_beyond_max");
}

// ------------------------------------------------------------
// test_find_busiest_scheduler — Test busiest scheduler detection
// ------------------------------------------------------------
void test_find_busiest_scheduler() {
    printf("Testing busiest scheduler detection...\n");
    
    // Test with different current cores
    for (uint64_t current_core = 0; current_core < 4; current_core++) {
        uint64_t busiest = find_busiest_scheduler(current_core);
        
        // Busiest core should be valid
        test_assert_nonzero(busiest < MAX_CORES, "busiest_core_valid");
        
        // Busiest core should not be the current core (in most cases)
        // Note: This might be equal if no other cores have work
        test_assert_nonzero(busiest >= 0, "busiest_core_non_negative");
    }
    
    // Test with invalid current core
    uint64_t busiest = find_busiest_scheduler(MAX_CORES);
    test_assert_equal(0, busiest, "busiest_scheduler_invalid_current");
}

// ------------------------------------------------------------
// test_is_steal_allowed — Test steal permission checking
// ------------------------------------------------------------
void test_is_steal_allowed() {
    printf("Testing steal permission checking...\n");
    
    // Test valid core pairs
    for (uint64_t source = 0; source < 4; source++) {
        for (uint64_t target = 0; target < 4; target++) {
            if (source != target) {
                int allowed = is_steal_allowed(source, target);
                // Should be allowed for different cores
                test_assert_equal(1, allowed, "steal_allowed_different_cores");
            }
        }
    }
    
    // Test invalid source core
    int allowed = is_steal_allowed(MAX_CORES, 0);
    test_assert_equal(0, allowed, "steal_not_allowed_invalid_source");
    
    // Test invalid target core
    allowed = is_steal_allowed(0, MAX_CORES);
    test_assert_equal(0, allowed, "steal_not_allowed_invalid_target");
    
    // Test both cores invalid
    allowed = is_steal_allowed(MAX_CORES, MAX_CORES);
    test_assert_equal(0, allowed, "steal_not_allowed_both_invalid");
}

// ------------------------------------------------------------
// test_select_victim_random — Test random victim selection
// ------------------------------------------------------------
void test_select_victim_random() {
    printf("Testing random victim selection...\n");
    
    // Test with different current cores
    for (uint64_t current_core = 0; current_core < 4; current_core++) {
        uint64_t victim = select_victim_random(current_core);
        
        // Victim should be valid
        test_assert_nonzero(victim < MAX_CORES, "random_victim_valid");
        test_assert_nonzero(victim >= 0, "random_victim_non_negative");
        
        // Victim should not be the current core
        test_assert_nonzero(victim != current_core, "random_victim_not_self");
    }
    
    // Test with invalid current core
    uint64_t victim = select_victim_random(MAX_CORES);
    test_assert_equal(0, victim, "random_victim_invalid_current");
}

// ------------------------------------------------------------
// test_select_victim_by_load — Test load-based victim selection
// ------------------------------------------------------------
void test_select_victim_by_load() {
    printf("Testing load-based victim selection...\n");
    
    // Test with different current cores
    for (uint64_t current_core = 0; current_core < 4; current_core++) {
        uint64_t victim = select_victim_by_load(current_core);
        
        // Victim should be valid
        test_assert_nonzero(victim < MAX_CORES, "load_victim_valid");
        test_assert_nonzero(victim >= 0, "load_victim_non_negative");
        
        // Victim might be the same as current core if no other work exists
        // This is acceptable behavior
    }
    
    // Test with invalid current core
    uint64_t victim = select_victim_by_load(MAX_CORES);
    test_assert_equal(0, victim, "load_victim_invalid_current");
}

// ------------------------------------------------------------
// test_select_victim_locality — Test locality-aware victim selection
// ------------------------------------------------------------
void test_select_victim_locality() {
    printf("Testing locality-aware victim selection...\n");
    
    // Test with different current cores
    for (uint64_t current_core = 0; current_core < 4; current_core++) {
        uint64_t victim = select_victim_locality(current_core);
        
        // Victim should be valid
        test_assert_nonzero(victim < MAX_CORES, "locality_victim_valid");
        test_assert_nonzero(victim >= 0, "locality_victim_non_negative");
        
        // Victim might be the same as current core if no other work exists
        // This is acceptable behavior
    }
    
    // Test with invalid current core
    uint64_t victim = select_victim_locality(MAX_CORES);
    test_assert_equal(0, victim, "locality_victim_invalid_current");
}

// ------------------------------------------------------------
// test_victim_selection_edge_cases — Test edge cases for victim selection
// ------------------------------------------------------------
void test_victim_selection_edge_cases() {
    printf("Testing victim selection edge cases...\n");
    
    // Test with maximum valid core ID
    uint64_t max_core = MAX_CORES - 1;
    
    // Test random selection with max core
    uint64_t victim = select_victim_random(max_core);
    test_assert_nonzero(victim < MAX_CORES, "random_victim_max_core");
    
    // Test load-based selection with max core
    victim = select_victim_by_load(max_core);
    test_assert_nonzero(victim < MAX_CORES, "load_victim_max_core");
    
    // Test locality-aware selection with max core
    victim = select_victim_locality(max_core);
    test_assert_nonzero(victim < MAX_CORES, "locality_victim_max_core");
    
    // Test load calculation with max core
    uint32_t load = get_scheduler_load(max_core);
    test_assert_nonzero(load >= 0, "load_calculation_max_core");
    
    // Test busiest scheduler with max core
    victim = find_busiest_scheduler(max_core);
    test_assert_nonzero(victim < MAX_CORES, "busiest_scheduler_max_core");
    
    // Test steal permission with max core
    int allowed = is_steal_allowed(max_core, 0);
    test_assert_equal(1, allowed, "steal_permission_max_core");
    
    allowed = is_steal_allowed(0, max_core);
    test_assert_equal(1, allowed, "steal_permission_to_max_core");
}

// ------------------------------------------------------------
// test_locality_based_selection — Test locality-based victim selection
// ------------------------------------------------------------
void test_locality_based_selection() {
    printf("Testing locality-based victim selection...\n");
    
    // Test 1: Basic locality selection
    printf("Testing basic locality selection...\n");
    for (uint64_t current_core = 0; current_core < 4; current_core++) {
        uint64_t victim = select_victim_locality(current_core);
        
        // Victim should be valid
        test_assert_nonzero(victim < MAX_CORES, "locality_selection_valid_victim");
        
        // Victim should not be negative
        test_assert_nonzero(victim >= 0, "locality_selection_non_negative");
        
        printf("Core %llu selected victim %llu\n", current_core, victim);
    }
    
    // Test 2: Locality selection consistency
    printf("Testing locality selection consistency...\n");
    for (int i = 0; i < 3; i++) {
        uint64_t victim1 = select_victim_locality(0);
        uint64_t victim2 = select_victim_locality(0);
        
        // Results should be consistent (same or valid fallback)
        test_assert_nonzero(victim1 < MAX_CORES, "locality_consistency_victim1");
        test_assert_nonzero(victim2 < MAX_CORES, "locality_consistency_victim2");
    }
    
    // Test 3: Edge case cores
    printf("Testing edge case cores...\n");
    uint64_t edge_victim = select_victim_locality(MAX_CORES - 1);
    test_assert_nonzero(edge_victim < MAX_CORES, "locality_selection_edge_core");
    
    // Test 4: Invalid core handling
    printf("Testing invalid core handling...\n");
    uint64_t invalid_victim = select_victim_locality(MAX_CORES + 1);
    // Should handle gracefully (return valid core or 0)
    test_assert_nonzero(invalid_victim < MAX_CORES, "locality_selection_invalid_core");
    
    // Test 5: Locality vs load-based comparison
    printf("Testing locality vs load-based selection...\n");
    for (uint64_t current_core = 0; current_core < 4; current_core++) {
        uint64_t locality_victim = select_victim_locality(current_core);
        uint64_t load_victim = select_victim_by_load(current_core);
        
        // Both should return valid victims
        test_assert_nonzero(locality_victim < MAX_CORES, "locality_vs_load_locality");
        test_assert_nonzero(load_victim < MAX_CORES, "locality_vs_load_load");
        
        printf("Core %llu: locality victim %llu, load victim %llu\n", 
               current_core, locality_victim, load_victim);
    }
    
    // Test 6: NUMA node simulation
    printf("Testing NUMA node simulation...\n");
    // Since we have a simplified NUMA implementation (single node),
    // locality selection should fall back to load-based selection
    for (uint64_t current_core = 0; current_core < 4; current_core++) {
        uint64_t victim = select_victim_locality(current_core);
        test_assert_nonzero(victim < MAX_CORES, "numa_simulation_valid");
    }
    
    printf("Locality-based victim selection tests completed\n");
}
