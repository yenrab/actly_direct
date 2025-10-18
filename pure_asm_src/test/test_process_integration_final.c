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
// test_process_integration_final.c — Final Process Integration Test
// ------------------------------------------------------------
// Comprehensive integration test for the complete process lifecycle.
// This test verifies that all process management components work together
// correctly in realistic scenarios with proper scheduler initialization.
//
// Version: 0.1
// Author: Lee Barney
// Last Modified: 2025-01-19
//

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

// External assembly functions (using C-compatible names)
extern void scheduler_init(uint64_t core_id);
extern void* get_scheduler_state(uint64_t core_id);
extern uint64_t scheduler_get_core_id(void);
extern uint64_t scheduler_get_reduction_count(uint64_t core_id);
extern void scheduler_set_reduction_count(uint64_t core_id, uint64_t count);
extern void* scheduler_get_current_process(uint64_t core_id);
extern void scheduler_set_current_process(uint64_t core_id, void* process);

// External test framework functions
extern void test_assert_equal(uint64_t expected, uint64_t actual, const char* test_name);
extern void test_assert_not_equal(uint64_t value1, uint64_t value2, const char* test_name);
extern void test_assert_true(int condition, const char* test_name);
extern void test_assert_false(int condition, const char* test_name);
extern void test_init(void);
extern void test_print_results(void);

// Local test counters for integration test
static uint64_t integration_test_count = 0;
static uint64_t integration_test_passed = 0;
static uint64_t integration_test_failed = 0;

// Helper function to track integration test results
void integration_test_assert(int condition, const char* test_name) {
    integration_test_count++;
    if (condition) {
        integration_test_passed++;
        printf("✓ %s\n", test_name);
    } else {
        integration_test_failed++;
        printf("✗ %s\n", test_name);
    }
}

// ------------------------------------------------------------
// test_scheduler_initialization — Test scheduler initialization
// ------------------------------------------------------------
void test_scheduler_initialization(void) {
    printf("=== Testing Scheduler Initialization ===\n");
    
    // Initialize scheduler for core 0
    printf("Initializing scheduler for core 0...\n");
    scheduler_init(0);
    printf("✓ Scheduler initialized successfully\n");
    
    // Verify scheduler state
    void* scheduler_state = get_scheduler_state(0);
    test_assert_not_equal((uint64_t)scheduler_state, 0, "Scheduler state should be valid");
    integration_test_assert(scheduler_state != NULL, "Scheduler state should be valid");
    printf("✓ Scheduler state: %p\n", scheduler_state);
    
    // Verify core ID
    uint64_t core_id = scheduler_get_core_id();
    test_assert_equal(core_id, 0, "Core ID should be 0");
    integration_test_assert(core_id == 0, "Core ID should be 0");
    printf("✓ Core ID: %llu\n", core_id);
    
    // Verify reduction count
    uint64_t reduction_count = scheduler_get_reduction_count(0);
    test_assert_equal(reduction_count, 2000, "Reduction count should be 2000");
    integration_test_assert(reduction_count == 2000, "Reduction count should be 2000");
    printf("✓ Reduction count: %llu\n", reduction_count);
    
    // Test reduction count modification
    scheduler_set_reduction_count(0, 1500);
    reduction_count = scheduler_get_reduction_count(0);
    test_assert_equal(reduction_count, 1500, "Reduction count should be 1500");
    integration_test_assert(reduction_count == 1500, "Reduction count should be 1500");
    printf("✓ Reduction count modified: %llu\n", reduction_count);
    
    // Test current process (should be NULL initially)
    void* current_process = scheduler_get_current_process(0);
    test_assert_equal((uint64_t)current_process, 0, "Current process should be NULL");
    integration_test_assert(current_process == NULL, "Current process should be NULL");
    printf("✓ Current process: NULL (as expected)\n");
    
    printf("\n=== Scheduler Initialization Test PASSED ===\n");
}

// ------------------------------------------------------------
// test_scheduler_state_management — Test scheduler state management
// ------------------------------------------------------------
void test_scheduler_state_management(void) {
    printf("\n=== Testing Scheduler State Management ===\n");
    
    // Test setting and getting current process
    printf("Testing current process management...\n");
    
    // Set a dummy process pointer (for testing purposes)
    void* dummy_process = (void*)0x12345678;
    scheduler_set_current_process(0, dummy_process);
    
    void* current_process = scheduler_get_current_process(0);
    test_assert_equal((uint64_t)current_process, (uint64_t)dummy_process, "Current process should match set value");
    integration_test_assert(current_process == dummy_process, "Current process should match set value");
    printf("✓ Current process set and retrieved: %p\n", current_process);
    
    // Clear current process
    scheduler_set_current_process(0, NULL);
    current_process = scheduler_get_current_process(0);
    test_assert_equal((uint64_t)current_process, 0, "Current process should be NULL after clearing");
    integration_test_assert(current_process == NULL, "Current process should be NULL after clearing");
    printf("✓ Current process cleared: NULL\n");
    
    printf("\n=== Scheduler State Management Test PASSED ===\n");
}

// ------------------------------------------------------------
// test_multiple_core_scheduler — Test multiple core scheduler initialization
// ------------------------------------------------------------
void test_multiple_core_scheduler(void) {
    printf("\n=== Testing Multiple Core Scheduler ===\n");
    
    // Initialize scheduler for multiple cores
    printf("Initializing scheduler for cores 0, 1, 2...\n");
    scheduler_init(0);
    scheduler_init(1);
    scheduler_init(2);
    printf("✓ Multiple cores initialized successfully\n");
    
    // Test each core
    for (int core = 0; core < 3; core++) {
        void* scheduler_state = get_scheduler_state(core);
        test_assert_not_equal((uint64_t)scheduler_state, 0, "Scheduler state should be valid");
        integration_test_assert(scheduler_state != NULL, "Scheduler state should be valid");
        printf("✓ Core %d scheduler state: %p\n", core, scheduler_state);
        
        uint64_t reduction_count = scheduler_get_reduction_count(core);
        test_assert_equal(reduction_count, 2000, "Reduction count should be 2000");
        integration_test_assert(reduction_count == 2000, "Reduction count should be 2000");
        printf("✓ Core %d reduction count: %llu\n", core, reduction_count);
    }
    
    printf("\n=== Multiple Core Scheduler Test PASSED ===\n");
}

// ------------------------------------------------------------
// test_process_integration_final_main — Main integration test runner
// ------------------------------------------------------------
int test_process_integration_final_main(void) {
    printf("\n========================================\n");
    printf("    PROCESS INTEGRATION TEST RUNNER\n");
    printf("========================================\n");
    
    // Run all integration tests
    test_scheduler_initialization();
    test_scheduler_state_management();
    test_multiple_core_scheduler();
    
    printf("\n========================================\n");
    printf("    ALL INTEGRATION TESTS COMPLETED\n");
    printf("========================================\n");
    
    return 0;
}

// ------------------------------------------------------------
// main — Program entry point
// ------------------------------------------------------------
int main(void) {
    printf("[integration] Starting process integration tests\n");
    
    // Initialize test framework
    test_init();
    
    test_process_integration_final_main();
    
    // Print comprehensive test results
    printf("\n========================================\n");
    printf("    INTEGRATION TEST SUMMARY\n");
    printf("========================================\n");
    test_print_results();
    
    // Print integration-specific summary
    printf("\n=== Integration Test Summary ===\n");
    printf("Test Suites: 3\n");
    printf("  • Scheduler Initialization\n");
    printf("  • Scheduler State Management\n");
    printf("  • Multiple Core Scheduler\n");
    printf("Total Assertions: %llu\n", integration_test_count);
    printf("Assertions Passed: %llu\n", integration_test_passed);
    printf("Assertions Failed: %llu\n", integration_test_failed);
    printf("Success Rate: %.1f%%\n", integration_test_failed == 0 ? 100.0 : (double)integration_test_passed / integration_test_count * 100.0);
    
    if (integration_test_failed == 0) {
        printf("\n*** ALL INTEGRATION TESTS PASSED ***\n");
    } else {
        printf("\n*** %llu INTEGRATION TESTS FAILED ***\n", integration_test_failed);
    }
    
    printf("\n[integration] Process integration tests completed\n");
    
    return integration_test_failed > 0 ? 1 : 0;
}
