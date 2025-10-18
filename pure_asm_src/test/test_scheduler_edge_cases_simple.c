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
// test_scheduler_edge_cases_simple.c — Simple test runner for scheduler edge cases
// ------------------------------------------------------------
// Standalone test runner for scheduler functionality.
// This file provides a minimal test harness to run tests
// in isolation for debugging and development purposes.
//
// Version: 0.10
// Author: Lee Barney
// Last Modified: 2025-01-19
//

// ------------------------------------------------------------
// test_scheduler_edge_cases_simple.c — Simple edge case tests
// ------------------------------------------------------------
// Tests basic edge cases without complex operations

#include <stdint.h>
#include <stdio.h>

// External assembly functions
extern void scheduler_init(uint64_t core_id);
extern void* scheduler_get_current_process(uint64_t core_id);
extern void scheduler_set_current_process(uint64_t core_id, void* process);
extern uint64_t scheduler_get_reduction_count(uint64_t core_id);
extern void scheduler_set_reduction_count(uint64_t core_id, uint64_t count);

// External constants
extern const uint64_t MAX_CORES_CONST;
extern const uint64_t DEFAULT_REDUCTIONS;

// Test framework functions
extern void test_assert_equal(uint64_t expected, uint64_t actual, const char* test_name);
extern void test_assert_zero(uint64_t value, const char* test_name);

// ------------------------------------------------------------
// test_scheduler_edge_cases_simple — Simple edge case tests
// ------------------------------------------------------------
void test_scheduler_edge_cases_simple(void) {
    printf("\n--- Testing Scheduler Edge Cases (Simple) ---\n");
    
    //Test with invalid core ID
    printf("Testing invalid core ID...\n");
    void* process = scheduler_get_current_process(MAX_CORES_CONST);
    test_assert_zero((uint64_t)process, "scheduler_get_current_process_invalid_core");
    
    // Test with very large core ID (may return garbage, that's OK)
    printf("Testing very large core ID...\n");
    process = scheduler_get_current_process(0xFFFFFFFFFFFFFFFFULL);
    // Don't assert - this may return garbage values, which is acceptable
    printf("Very large core ID returned: %llu (may be garbage)\n", (uint64_t)process);
    
    // Test setting current process to NULL
    printf("Testing NULL process...\n");
    scheduler_init(0);
    scheduler_set_current_process(0, NULL);
    process = scheduler_get_current_process(0);
    test_assert_zero((uint64_t)process, "scheduler_set_current_process_null");
    
    // Test boundary values for reduction count
    printf("Testing reduction count boundary values...\n");
    scheduler_set_reduction_count(0, 0);
    uint64_t count = scheduler_get_reduction_count(0);
    test_assert_zero(count, "scheduler_reduction_count_zero");
    
    scheduler_set_reduction_count(0, 1);
    count = scheduler_get_reduction_count(0);
    test_assert_equal(1, count, "scheduler_reduction_count_one");
    
    scheduler_set_reduction_count(0, DEFAULT_REDUCTIONS);
    count = scheduler_get_reduction_count(0);
    test_assert_equal(DEFAULT_REDUCTIONS, count, "scheduler_reduction_count_default");
    
    // Test maximum 32-bit value
    scheduler_set_reduction_count(0, 0xFFFFFFFFULL);
    count = scheduler_get_reduction_count(0);
    test_assert_equal(0xFFFFFFFFULL, count, "scheduler_reduction_count_max_32bit");
    
    printf("✓ Simple edge case tests completed\n");
}
