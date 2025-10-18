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
// test_scheduler_core_id.c — C tests for pure assembly core ID functionality
// ------------------------------------------------------------

#include <stdint.h>
#include <stdio.h>

// External assembly functions
extern uint64_t scheduler_get_core_id(void);
extern void test_assert_equal(uint64_t expected, uint64_t actual, const char* test_name);
extern void test_assert_not_zero(uint64_t value, const char* test_name);
extern void test_pass(const char* test_name);
extern void test_fail(uint64_t expected, uint64_t actual, const char* test_name);

// External constants from assembly
extern const uint64_t MAX_CORES_CONST;

// Forward declarations for test functions
void test_scheduler_get_core_id_basic(void);
void test_scheduler_get_core_id_consistency(void);

// ------------------------------------------------------------
// test_scheduler_core_id — Main test function
// ------------------------------------------------------------
void test_scheduler_core_id(void) {
    printf("\n--- Testing scheduler get_core_id (Pure Assembly) ---\n");
    
    test_scheduler_get_core_id_basic();
    test_scheduler_get_core_id_consistency();
}

// ------------------------------------------------------------
// test_scheduler_get_core_id_basic — Test basic core ID functionality
// ------------------------------------------------------------
void test_scheduler_get_core_id_basic(void) {
    // Test that get_core_id returns a value (not necessarily 0)
    uint64_t core_id = scheduler_get_core_id();
    
    // The core ID should be a valid value (0 to MAX_CORES-1)
    // For native execution, we simulate core 0
    if (core_id < MAX_CORES_CONST) {
        test_pass("scheduler_get_core_id_range_check");
    } else {
        test_fail(MAX_CORES_CONST - 1, core_id, "scheduler_get_core_id_range_check");
    }
}

// ------------------------------------------------------------
// test_scheduler_get_core_id_consistency — Test core ID consistency
// ------------------------------------------------------------
void test_scheduler_get_core_id_consistency(void) {
    // Test that get_core_id returns consistent values
    uint64_t core_id1 = scheduler_get_core_id();
    uint64_t core_id2 = scheduler_get_core_id();
    
    // Compare the two results
    if (core_id1 == core_id2) {
        test_pass("scheduler_get_core_id_consistency");
    } else {
        test_fail(core_id1, core_id2, "scheduler_get_core_id_consistency");
    }
}
