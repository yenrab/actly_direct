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
// test_runner.c — C test runner for pure assembly scheduler
// ------------------------------------------------------------

#include <stdio.h>
#include <stdint.h>

// External assembly functions
extern uint64_t scheduler_get_core_id(void);

// External constants from assembly
extern const uint64_t MAX_CORES_CONST;
extern const uint64_t DEFAULT_REDUCTIONS;
extern const uint64_t NUM_PRIORITIES_CONST;
extern const uint64_t SCHEDULER_SIZE_CONST;
extern const uint64_t PRIORITY_QUEUE_SIZE_CONST;

// External test functions
extern void test_scheduler_init(void);
extern void test_scheduler_get_set_process(void);
extern void test_scheduler_reduction_count(void);
extern void test_scheduler_core_id(void);
extern void test_scheduler_helper_functions(void);
extern void test_scheduler_edge_cases_simple(void);
extern void test_process_state_management(void);
extern void test_process_control_block(void);
extern void test_scheduler_queue_length(void);
extern void test_expand_memory_pool(void);

// External Phase 6 test functions (now working!)
extern void test_yielding_main(void);
extern void test_blocking_main(void);
extern void test_actly_bifs_main(void);
extern void test_integration_yielding_main(void);

// External Phase 6 individual test functions
extern void test_process_yield_basic(void);
extern void test_process_yield_with_pcb(void);
extern void test_scheduler_enqueue_basic(void);
extern void test_scheduler_enqueue_edge_cases(void);
extern void test_process_save_context_basic(void);
extern void test_process_restore_context_basic(void);
extern void test_context_functions_edge_cases(void);
extern void test_integration_yield_scheduling(void);
extern void test_integration_multiple_processes(void);

// External test framework functions
extern void test_init(void);
extern void test_print_results(void);
extern uint64_t test_failed_count;

// ------------------------------------------------------------
// test_runner_main — Main test runner entry point
// ------------------------------------------------------------
int test_runner_main(void) {
    printf("\n========================================\n");
    printf("    SCHEDULER UNIT TEST RUNNER\n");
    printf("========================================\n");
    
    // Initialize test framework
    test_init();
    
    // Print system information
    printf("\n--- System Information ---\n");
    printf("MAX_CORES: %llu\n", MAX_CORES_CONST);
    printf("DEFAULT_REDUCTIONS: %llu\n", DEFAULT_REDUCTIONS);
    printf("NUM_PRIORITIES: %llu\n", NUM_PRIORITIES_CONST);
    printf("scheduler_size: %llu\n", SCHEDULER_SIZE_CONST);
    printf("priority_queue_size: %llu\n", PRIORITY_QUEUE_SIZE_CONST);
    
    // Run all test suites
    test_scheduler_init();
    test_scheduler_get_set_process();
    test_scheduler_reduction_count();
    test_scheduler_core_id();
    test_scheduler_helper_functions();
    test_scheduler_edge_cases_simple();
    test_process_state_management();
    test_scheduler_queue_length();
    test_expand_memory_pool();
    
    // Run new Phase 6 yielding and blocking tests
    // NOTE: These tests have been fixed and are now working!
    // The illegal instruction errors have been resolved.
    test_yielding_main();
    test_blocking_main();
    test_actly_bifs_main();
    test_integration_yielding_main();
    
    // Run new Phase 6 individual test suites
    test_process_yield_basic();
    test_process_yield_with_pcb();
    test_scheduler_enqueue_basic();
    test_scheduler_enqueue_edge_cases();
    test_process_save_context_basic();
    test_process_restore_context_basic();
    test_context_functions_edge_cases();
    test_integration_yield_scheduling();
    test_integration_multiple_processes();
    
    // Print test results
    test_print_results();
    
    // Return exit code based on test results
    if (test_failed_count == 0) {
        printf("\n*** ALL TESTS PASSED ***\n");
        return 0;
    } else {
        printf("\n*** SOME TESTS FAILED ***\n");
        return 1;
    }
}

// ------------------------------------------------------------
// main — Program entry point
// ------------------------------------------------------------
int main(void) {
    printf("[test_boot] Starting test mode\n");
    
    int result = test_runner_main();
    
    printf("[runtime_test_mode] All tests completed\n");
    
    return result;
}
