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
// test_scheduler_runner.c — Test runner for scheduler functionality
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

// External test functions
extern void test_scheduler_init(void);
extern void test_scheduler_get_set_process(void);
extern void test_scheduler_reduction_count(void);
extern void test_scheduler_core_id(void);
extern void test_scheduler_helper_functions(void);
extern void test_scheduler_scheduling(void);
extern void test_scheduler_edge_cases_simple(void);
// extern void test_scheduler_core(void);  // Removed - conflicts with other tests

// External test framework functions
extern void test_init(void);
extern void test_print_results(void);
extern uint64_t test_failed_count;

// ------------------------------------------------------------
// test_scheduler_runner_main — Scheduler-only test runner entry point
// ------------------------------------------------------------
int test_scheduler_runner_main(void) {
    printf("\n========================================\n");
    printf("    SCHEDULER UNIT TEST RUNNER\n");
    printf("========================================\n");
    
    // Initialize test framework
    test_init();
    
    // Run scheduler tests
    test_scheduler_init();
    test_scheduler_get_set_process();
    test_scheduler_reduction_count();
    test_scheduler_core_id();
    test_scheduler_helper_functions();
    test_scheduler_scheduling();
    test_scheduler_edge_cases_simple();
    
    // Print test results
    test_print_results();
    
    // Return exit code based on test results
    if (test_failed_count == 0) {
        printf("\n*** ALL SCHEDULER TESTS PASSED ***\n");
        return 0;
    } else {
        printf("\n*** SOME SCHEDULER TESTS FAILED ***\n");
        return 1;
    }
}

// ------------------------------------------------------------
// main — Program entry point
// ------------------------------------------------------------
int main(void) {
    printf("[test_scheduler] Starting scheduler test mode\n");
    
    int result = test_scheduler_runner_main();
    
    printf("[test_scheduler] Scheduler tests completed\n");
    
    return result;
}
