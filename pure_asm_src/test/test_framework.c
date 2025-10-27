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
// test_framework.c — C test framework for pure assembly scheduler
// ------------------------------------------------------------
// This provides a C-based test framework to test the pure assembly scheduler
// Uses standard C library for convenience in testing

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>

// External constants from assembly
extern const uint64_t MAX_CORES_CONST;
extern const uint64_t NUM_PRIORITIES_CONST;
extern const uint64_t DEFAULT_REDUCTIONS;
extern const uint64_t PRIORITY_QUEUE_SIZE_CONST;
extern const uint64_t SCHEDULER_SIZE_CONST;

// Forward declarations
void test_pass(const char* test_name);
void test_fail(uint64_t expected, uint64_t actual, const char* test_name);
void test_cleanup(void);

// External memory isolation functions (defined in test_yielding.c)
extern void force_memory_cleanup(void);
extern void validate_memory_state(const char* test_name);
extern void reset_global_state(void);

// Test framework constants
#define MAX_TESTS 1000  // Increased to handle more tests
#define TEST_NAME_LENGTH 128  // Increased for longer test names

// Test result structure
typedef struct {
    char name[TEST_NAME_LENGTH];
    bool passed;
    uint64_t expected;
    uint64_t actual;
} test_result_t;

// Test framework state - use dynamic allocation to avoid stack issues
static test_result_t* test_results = NULL;
static uint64_t test_count = 0;
static uint64_t test_passed_count = 0;
uint64_t test_failed_count = 0;  // Global for test_runner access
static uint64_t current_test_index = 0;
static uint64_t max_tests_allocated = 0;

// ------------------------------------------------------------
// test_init — Initialize test framework
// ------------------------------------------------------------
void test_init(void) {
    // MEMORY ISOLATION: Reset global state before initialization
    reset_global_state();
    validate_memory_state("test_init_start");
    
    test_count = 0;
    test_passed_count = 0;
    test_failed_count = 0;
    current_test_index = 0;
    
    // MEMORY ISOLATION: Force memory cleanup before allocation
    force_memory_cleanup();
    
    // Allocate memory for test results if not already allocated
    if (test_results == NULL) {
        test_results = malloc(MAX_TESTS * sizeof(test_result_t));
        if (test_results == NULL) {
            printf("ERROR: Failed to allocate memory for test results\n");
            return;
        }
        max_tests_allocated = MAX_TESTS;
    }
    
    // MEMORY ISOLATION: Validate memory state after allocation
    validate_memory_state("test_init_after_allocation");
}

// ------------------------------------------------------------
// test_assert_equal — Assert two values are equal
// ------------------------------------------------------------
void test_assert_equal(uint64_t expected, uint64_t actual, const char* test_name) {
    if (expected == actual) {
        test_pass(test_name);
    } else {
        test_fail(expected, actual, test_name);
    }
}

// ------------------------------------------------------------
// test_assert_not_equal — Assert two values are not equal
// ------------------------------------------------------------
void test_assert_not_equal(uint64_t value1, uint64_t value2, const char* test_name) {
    if (value1 != value2) {
        test_pass(test_name);
    } else {
        test_fail(value1, value2, test_name);
    }
}

// ------------------------------------------------------------
// test_assert_zero — Assert value is zero
// ------------------------------------------------------------
void test_assert_zero(uint64_t value, const char* test_name) {
    if (value == 0) {
        test_pass(test_name);
    } else {
        test_fail(0, value, test_name);
    }
}

// ------------------------------------------------------------
// test_assert_not_zero — Assert value is not zero
// ------------------------------------------------------------
void test_assert_not_zero(uint64_t value, const char* test_name) {
    if (value != 0) {
        test_pass(test_name);
    } else {
        test_fail(1, value, test_name);
    }
}

// Alias for compatibility
void test_assert_nonzero(uint64_t value, const char* test_name) {
    test_assert_not_zero(value, test_name);
}

// ------------------------------------------------------------
// test_assert_true — Assert value is non-zero (true)
// ------------------------------------------------------------
void test_assert_true(uint64_t value, const char* test_name) {
    test_assert_not_zero(value, test_name);
}

// ------------------------------------------------------------
// test_assert_false — Assert value is zero (false)
// ------------------------------------------------------------
void test_assert_false(uint64_t value, const char* test_name) {
    test_assert_zero(value, test_name);
}

// ------------------------------------------------------------
// test_assert_not_null — Assert pointer is not NULL
// ------------------------------------------------------------
void test_assert_not_null(void* ptr, const char* test_name) {
    if (ptr != NULL) {
        test_passed_count++;
        printf("  ✓ %s\n", test_name);
    } else {
        test_failed_count++;
        printf("  ✗ %s: Expected non-NULL pointer, got NULL\n", test_name);
    }
}

// ------------------------------------------------------------
// test_assert_null — Assert pointer is NULL
// ------------------------------------------------------------
void test_assert_null(void* ptr, const char* test_name) {
    if (ptr == NULL) {
        test_passed_count++;
        printf("  ✓ %s\n", test_name);
    } else {
        test_failed_count++;
        printf("  ✗ %s: Expected NULL pointer, got %p\n", test_name, ptr);
    }
}

// ------------------------------------------------------------
// test_pass — Record a passing test
// ------------------------------------------------------------
void test_pass(const char* test_name) {
    // MEMORY ISOLATION: Validate parameters before processing
    validate_memory_state("test_pass_start");
    
    if (test_results == NULL) {
        printf("ERROR: Test framework not initialized\n");
        return;
    }
    
    if (current_test_index >= max_tests_allocated) {
        printf("WARNING: Maximum number of tests reached, skipping test: %s\n", test_name);
        return;
    }
    
    // MEMORY ISOLATION: Force memory cleanup before storing
    force_memory_cleanup();
    
    test_result_t* result = &test_results[current_test_index];
    
    // Store values immediately to prevent corruption
    result->passed = true;
    result->expected = 0;
    result->actual = 0;
    
    // Copy test name safely
    strncpy(result->name, test_name, TEST_NAME_LENGTH - 1);
    result->name[TEST_NAME_LENGTH - 1] = '\0';
    
    // MEMORY ISOLATION: Validate after storing
    validate_memory_state("test_pass_after_store");
    
    current_test_index++;
    test_passed_count++;
    test_count++;
}

// ------------------------------------------------------------
// test_fail — Record a failing test
// ------------------------------------------------------------
void test_fail(uint64_t expected, uint64_t actual, const char* test_name) {
    // MEMORY ISOLATION: Validate parameters before processing
    validate_memory_state("test_fail_start");
    
    if (test_results == NULL) {
        printf("ERROR: Test framework not initialized\n");
        return;
    }
    
    if (current_test_index >= max_tests_allocated) {
        printf("WARNING: Maximum number of tests reached, skipping test: %s\n", test_name);
        return;
    }
    
    // MEMORY ISOLATION: Force memory cleanup before storing
    force_memory_cleanup();
    
    test_result_t* result = &test_results[current_test_index];
    
    // Store values immediately to prevent corruption
    result->passed = false;
    result->expected = expected;  // Store immediately
    result->actual = actual;      // Store immediately
    
    // Copy test name safely
    strncpy(result->name, test_name, TEST_NAME_LENGTH - 1);
    result->name[TEST_NAME_LENGTH - 1] = '\0';
    
    // MEMORY ISOLATION: Validate after storing
    validate_memory_state("test_fail_after_store");
    
    // Verify stored values haven't been corrupted
    if (result->expected != expected) {
        printf("ERROR: Expected value corrupted in test_fail: stored=0x%llx, original=0x%llx\n", 
               result->expected, expected);
    }
    if (result->actual != actual) {
        printf("ERROR: Actual value corrupted in test_fail: stored=0x%llx, original=0x%llx\n", 
               result->actual, actual);
    }
    
    current_test_index++;
    test_failed_count++;
    test_count++;
}

// ------------------------------------------------------------
// test_print_results — Print test results summary
// ------------------------------------------------------------
void test_print_results(void) {
    
    printf("\n=== Test Results ===\n");
    
    printf("Total Assertions: %llu\n", test_passed_count + test_failed_count);
    printf("Assertions Passed: %llu\n", test_passed_count);
    printf("Assertions Failed: %llu\n", test_failed_count);
    printf("Success Rate: %.1f%%\n", test_failed_count == 0 ? 100.0 : (double)test_passed_count / (test_passed_count + test_failed_count) * 100.0);
    printf("========================\n");
    
    // Print failed test details
    if (test_failed_count > 0 && test_results != NULL) {
        printf("\nFailed Tests:\n");
        for (uint64_t i = 0; i < current_test_index && i < max_tests_allocated; i++) {
            if (!test_results[i].passed) {
                printf("  - %s (expected: %llu, actual: %llu)\n", 
                       test_results[i].name, 
                       test_results[i].expected, 
                       test_results[i].actual);
            }
        }
    }
    
}

// ------------------------------------------------------------
// test_cleanup — Clean up test framework memory
// ------------------------------------------------------------
void test_cleanup(void) {
    if (test_results != NULL) {
        free(test_results);
        test_results = NULL;
        max_tests_allocated = 0;
    }
    
    test_count = 0;
    test_passed_count = 0;
    test_failed_count = 0;
    current_test_index = 0;
}
