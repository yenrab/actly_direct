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
// test_integration_yielding_standalone.c — Standalone test runner for integration yielding
// ------------------------------------------------------------
// Standalone test runner for integration yielding functionality.
// This file provides a minimal test harness to run integration yielding tests
// in isolation for debugging and development purposes.
//
// Version: 0.10
// Author: Lee Barney
// Last Modified: 2025-01-19
//

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

// External test functions
extern void test_integration_yielding_main(void);

// External test framework functions
extern void test_init(void);
extern void test_print_results(void);
extern uint64_t test_failed_count;

// External scheduler functions
extern void scheduler_init(uint64_t core_id);

// External constants
extern const uint64_t MAX_CORES_CONST;
extern const uint64_t DEFAULT_REDUCTIONS;
extern const uint64_t NUM_PRIORITIES_CONST;
extern const uint64_t SCHEDULER_SIZE_CONST;
extern const uint64_t PRIORITY_QUEUE_SIZE_CONST;

int main(void) {
    printf("=== Integration Yielding Test (Standalone) ===\n");
    
    // Initialize test framework
    test_init();
    printf("Test framework initialized\n");
    
    // Print system information
    printf("\n--- System Information ---\n");
    printf("MAX_CORES: %llu\n", MAX_CORES_CONST);
    printf("DEFAULT_REDUCTIONS: %llu\n", DEFAULT_REDUCTIONS);
    printf("NUM_PRIORITIES: %llu\n", NUM_PRIORITIES_CONST);
    printf("scheduler_size: %llu\n", SCHEDULER_SIZE_CONST);
    printf("priority_queue_size: %llu\n", PRIORITY_QUEUE_SIZE_CONST);
    
    // Initialize scheduler for core 0 (required for integration tests)
    printf("Initializing scheduler for core 0...\n");
    scheduler_init(0);
    printf("Scheduler initialized\n");
    
    // Run the integration yielding tests
    printf("Running integration yielding tests...\n");
    test_integration_yielding_main();
    printf("Integration yielding tests completed\n");
    
    // Print test results
    test_print_results();
    
    // Return exit code based on test results
    if (test_failed_count == 0) {
        printf("\n*** INTEGRATION YIELDING TESTS PASSED ***\n");
        return 0;
    } else {
        printf("\n*** INTEGRATION YIELDING TESTS FAILED ***\n");
        return 1;
    }
}
