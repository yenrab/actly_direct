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
// test_scheduler_reduction_count.c — C tests for pure assembly reduction count
// ------------------------------------------------------------

#include <stdint.h>
#include <stdio.h>

// External assembly functions
extern void scheduler_init(uint64_t core_id);
extern uint64_t scheduler_get_reduction_count(uint64_t core_id);
extern void scheduler_set_reduction_count(uint64_t core_id, uint64_t count);
extern void scheduler_decrement_reductions(void);
extern void test_assert_equal(uint64_t expected, uint64_t actual, const char* test_name);
extern void test_assert_zero(uint64_t value, const char* test_name);

// External constants from assembly
extern const uint64_t DEFAULT_REDUCTIONS;

// Forward declarations for test functions
void test_scheduler_get_reduction_count(void);
void test_scheduler_set_reduction_count(void);
void test_scheduler_reduction_count_cross_core_isolation(void);
void test_scheduler_reduction_count_boundary_values(void);
void test_scheduler_decrement_reductions(void);

// ------------------------------------------------------------
// test_scheduler_reduction_count — Main test function
// ------------------------------------------------------------
void test_scheduler_reduction_count(void) {
    printf("\n--- Testing scheduler reduction count (Pure Assembly) ---\n");
    
    test_scheduler_get_reduction_count();
    test_scheduler_set_reduction_count();
    test_scheduler_reduction_count_cross_core_isolation();
    test_scheduler_reduction_count_boundary_values();
    test_scheduler_decrement_reductions();
}

// ------------------------------------------------------------
// test_scheduler_get_reduction_count — Test get reduction count
// ------------------------------------------------------------
void test_scheduler_get_reduction_count(void) {
    // Initialize scheduler for core 0
    scheduler_init(0);
    
    // Test getting reduction count (should be 2000 initially)
    uint64_t count = scheduler_get_reduction_count(0);
    test_assert_equal(2000, count, "scheduler_get_reduction_count_default");
    
    // Test getting reduction count for core 1
    scheduler_init(1);
    count = scheduler_get_reduction_count(1);
    test_assert_equal(2000, count, "scheduler_get_reduction_count_default_core1");
    
    // Test getting reduction count for core 2
    scheduler_init(2);
    count = scheduler_get_reduction_count(2);
    test_assert_equal(2000, count, "scheduler_get_reduction_count_default_core2");
}

// ------------------------------------------------------------
// test_scheduler_set_reduction_count — Test set reduction count
// ------------------------------------------------------------
void test_scheduler_set_reduction_count(void) {
    // Initialize scheduler for core 0
    scheduler_init(0);
    
    // Test setting reduction count to a specific value
    scheduler_set_reduction_count(0, 1000);
    
    // Verify the reduction count was set correctly
    uint64_t count = scheduler_get_reduction_count(0);
    test_assert_equal(1000, count, "scheduler_set_get_reduction_count_1000");
    
    // Test setting reduction count to 0
    scheduler_set_reduction_count(0, 0);
    
    // Verify the reduction count was set to 0
    count = scheduler_get_reduction_count(0);
    test_assert_zero(count, "scheduler_set_get_reduction_count_zero");
    
    // Test setting reduction count to a large value
    scheduler_set_reduction_count(0, 0xFFFFFFFFULL);
    
    // Verify the reduction count was set correctly
    count = scheduler_get_reduction_count(0);
    test_assert_equal(0xFFFFFFFFULL, count, "scheduler_set_get_reduction_count_large");
    
    // Test multiple cores with different reduction counts
    scheduler_init(1);
    scheduler_set_reduction_count(1, 500);
    
    count = scheduler_get_reduction_count(1);
    test_assert_equal(500, count, "scheduler_set_get_reduction_count_core1");
    
    // Verify core 0 still has its reduction count
    count = scheduler_get_reduction_count(0);
    test_assert_equal(0xFFFFFFFFULL, count, "scheduler_set_get_reduction_count_core0_unchanged");
}

// ------------------------------------------------------------
// test_scheduler_reduction_count_cross_core_isolation — Test cross-core isolation
// ------------------------------------------------------------
void test_scheduler_reduction_count_cross_core_isolation(void) {
    // Initialize schedulers for multiple cores
    scheduler_init(0);
    scheduler_init(1);
    scheduler_init(2);
    scheduler_init(3);
    
    // Set different reduction counts for each core
    scheduler_set_reduction_count(0, 100);
    scheduler_set_reduction_count(1, 200);
    scheduler_set_reduction_count(2, 300);
    scheduler_set_reduction_count(3, 400);
    
    // Verify each core has its own reduction count
    uint64_t count = scheduler_get_reduction_count(0);
    test_assert_equal(100, count, "scheduler_reduction_count_cross_core_isolation_core0");
    
    count = scheduler_get_reduction_count(1);
    test_assert_equal(200, count, "scheduler_reduction_count_cross_core_isolation_core1");
    
    count = scheduler_get_reduction_count(2);
    test_assert_equal(300, count, "scheduler_reduction_count_cross_core_isolation_core2");
    
    count = scheduler_get_reduction_count(3);
    test_assert_equal(400, count, "scheduler_reduction_count_cross_core_isolation_core3");
    
    // Test that changing one core doesn't affect others
    scheduler_set_reduction_count(1, 250);
    
    // Verify core 1 changed
    count = scheduler_get_reduction_count(1);
    test_assert_equal(250, count, "scheduler_reduction_count_cross_core_isolation_core1_changed");
    
    // Verify other cores unchanged
    count = scheduler_get_reduction_count(0);
    test_assert_equal(100, count, "scheduler_reduction_count_cross_core_isolation_core0_unchanged");
    
    count = scheduler_get_reduction_count(2);
    test_assert_equal(300, count, "scheduler_reduction_count_cross_core_isolation_core2_unchanged");
    
    count = scheduler_get_reduction_count(3);
    test_assert_equal(400, count, "scheduler_reduction_count_cross_core_isolation_core3_unchanged");
}

// ------------------------------------------------------------
// test_scheduler_reduction_count_boundary_values — Test boundary values
// ------------------------------------------------------------
void test_scheduler_reduction_count_boundary_values(void) {
    // Initialize scheduler for core 0
    scheduler_init(0);
    
    // Test minimum value (0)
    scheduler_set_reduction_count(0, 0);
    uint64_t count = scheduler_get_reduction_count(0);
    test_assert_zero(count, "scheduler_reduction_count_boundary_minimum");
    
    // Test maximum 32-bit value
    scheduler_set_reduction_count(0, 0xFFFFFFFFULL);
    count = scheduler_get_reduction_count(0);
    test_assert_equal(0xFFFFFFFFULL, count, "scheduler_reduction_count_boundary_maximum_32bit");
    
    // Test maximum 64-bit value
    scheduler_set_reduction_count(0, 0xFFFFFFFFFFFFFFFFULL);
    count = scheduler_get_reduction_count(0);
    test_assert_equal(0xFFFFFFFFFFFFFFFFULL, count, "scheduler_reduction_count_boundary_maximum_64bit");
    
    // Test some intermediate values
    scheduler_set_reduction_count(0, 1);
    count = scheduler_get_reduction_count(0);
    test_assert_equal(1, count, "scheduler_reduction_count_boundary_one");
    
    // Test a large but not maximum value
    scheduler_set_reduction_count(0, 0x7FFFFFFFULL);
    count = scheduler_get_reduction_count(0);
    test_assert_equal(0x7FFFFFFFULL, count, "scheduler_reduction_count_boundary_large_positive");
}

// ------------------------------------------------------------
// test_scheduler_decrement_reductions — Test decrement reductions function
// ------------------------------------------------------------
void test_scheduler_decrement_reductions(void) {
    // Initialize scheduler for core 0 (default core for decrement function)
    scheduler_init(0);
    
    // Test decrementing from default value (2000)
    uint64_t count = scheduler_get_reduction_count(0);
    test_assert_equal(2000, count, "scheduler_decrement_reductions_initial_default");
    
    // Decrement once
    scheduler_decrement_reductions();
    count = scheduler_get_reduction_count(0);
    test_assert_equal(1999, count, "scheduler_decrement_reductions_once");
    
    // Decrement multiple times
    scheduler_decrement_reductions();
    scheduler_decrement_reductions();
    scheduler_decrement_reductions();
    count = scheduler_get_reduction_count(0);
    test_assert_equal(1996, count, "scheduler_decrement_reductions_multiple");
    
    // Test decrementing from a specific value
    scheduler_set_reduction_count(0, 10);
    count = scheduler_get_reduction_count(0);
    test_assert_equal(10, count, "scheduler_decrement_reductions_set_to_10");
    
    // Decrement down to 1
    for (int i = 0; i < 9; i++) {
        scheduler_decrement_reductions();
    }
    count = scheduler_get_reduction_count(0);
    test_assert_equal(1, count, "scheduler_decrement_reductions_down_to_1");
    
    // Decrement to 0
    scheduler_decrement_reductions();
    count = scheduler_get_reduction_count(0);
    test_assert_zero(count, "scheduler_decrement_reductions_down_to_0");
    
    // Test that decrementing from 0 stays at 0 (no underflow)
    scheduler_decrement_reductions();
    count = scheduler_get_reduction_count(0);
    test_assert_zero(count, "scheduler_decrement_reductions_stays_at_0");
    
    // Test with large value
    scheduler_set_reduction_count(0, 0xFFFFFFFFULL);
    count = scheduler_get_reduction_count(0);
    test_assert_equal(0xFFFFFFFFULL, count, "scheduler_decrement_reductions_large_value");
    
    scheduler_decrement_reductions();
    count = scheduler_get_reduction_count(0);
    test_assert_equal(0xFFFFFFFEULL, count, "scheduler_decrement_reductions_large_value_decremented");
}
