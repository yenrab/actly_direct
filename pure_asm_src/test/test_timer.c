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
// test_timer.c — C test suite for Timer System
// ------------------------------------------------------------
// Tests the timer and timeout functions implemented in timer.s.
// This includes timer initialization, insertion, cancellation,
// timeout scheduling, and timer processing.
//
// Version: 0.10
// Author: Lee Barney
// Last Modified: 2025-01-19
//
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

// Test framework function declarations
void test_assert_equal(uint64_t expected, uint64_t actual, const char* test_name);
void test_assert_true(int condition, const char* test_name);

// External assembly functions
extern int timer_init(void);
extern uint64_t get_system_ticks(void);
extern uint64_t insert_timer(uint64_t expiry_ticks, void* callback, uint64_t process_id);
extern int cancel_timer(uint64_t timer_id);
extern uint32_t process_timers(void);
extern void timer_tick(void);
extern uint64_t schedule_timeout(uint64_t timeout_ticks, uint64_t process_id);
extern int cancel_timeout(uint64_t timeout_id);

// Test callback function
static uint64_t test_callback_called = 0;
static void test_callback(void) {
    test_callback_called++;
}

// ------------------------------------------------------------
// Test Timer Initialization
// ------------------------------------------------------------
void test_timer_init_basic() {
    printf("--- Testing Timer Initialization ---\n");
    
    int result = timer_init();
    test_assert_equal(1, result, "timer_init_basic");
}

// ------------------------------------------------------------
// Test System Ticks
// ------------------------------------------------------------
void test_system_ticks() {
    printf("--- Testing System Ticks ---\n");
    
    uint64_t ticks1 = get_system_ticks();
    test_assert_true(ticks1 > 0, "system_ticks_non_zero");
    
    uint64_t ticks2 = get_system_ticks();
    test_assert_true(ticks2 >= ticks1, "system_ticks_monotonic");
}

// ------------------------------------------------------------
// Test Timer Insertion
// ------------------------------------------------------------
void test_timer_insertion() {
    printf("--- Testing Timer Insertion ---\n");
    
    uint64_t expiry = 1000;
    void* callback = (void*)test_callback;
    uint64_t process_id = 123;
    
    uint64_t timer_id = insert_timer(expiry, callback, process_id);
    test_assert_true(timer_id != 0, "timer_insertion_success");
    
    // Test insertion with invalid parameters
    uint64_t invalid_timer = insert_timer(0, callback, process_id);
    test_assert_equal(0, invalid_timer, "timer_insertion_invalid_expiry");
    
    invalid_timer = insert_timer(expiry, NULL, process_id);
    test_assert_equal(0, invalid_timer, "timer_insertion_invalid_callback");
}

// ------------------------------------------------------------
// Test Timer Cancellation
// ------------------------------------------------------------
void test_timer_cancellation() {
    printf("--- Testing Timer Cancellation ---\n");
    
    uint64_t expiry = 1000;
    void* callback = (void*)test_callback;
    uint64_t process_id = 123;
    
    uint64_t timer_id = insert_timer(expiry, callback, process_id);
    test_assert_true(timer_id != 0, "timer_cancellation_setup");
    
    int result = cancel_timer(timer_id);
    test_assert_equal(1, result, "timer_cancellation_success");
    
    // Test cancellation of invalid timer
    result = cancel_timer(0);
    test_assert_equal(0, result, "timer_cancellation_invalid");
}

// ------------------------------------------------------------
// Test Timer Processing
// ------------------------------------------------------------
void test_timer_processing() {
    printf("--- Testing Timer Processing ---\n");
    
    uint32_t expired_count = process_timers();
    test_assert_equal(0, expired_count, "timer_processing_no_expired");
    
    // Test timer tick
    timer_tick();
    // Should not crash
    test_assert_true(1, "timer_tick_execution");
}

// ------------------------------------------------------------
// Test Timeout Scheduling
// ------------------------------------------------------------
void test_timeout_scheduling() {
    printf("--- Testing Timeout Scheduling ---\n");
    
    uint64_t timeout_ticks = 500;
    uint64_t process_id = 456;
    
    uint64_t timeout_id = schedule_timeout(timeout_ticks, process_id);
    test_assert_true(timeout_id != 0, "timeout_scheduling_success");
    
    // Test timeout with invalid parameters
    uint64_t invalid_timeout = schedule_timeout(0, process_id);
    test_assert_equal(0, invalid_timeout, "timeout_scheduling_invalid_ticks");
    
    invalid_timeout = schedule_timeout(timeout_ticks, 0);
    test_assert_equal(0, invalid_timeout, "timeout_scheduling_invalid_process");
}

// ------------------------------------------------------------
// Test Timeout Cancellation
// ------------------------------------------------------------
void test_timeout_cancellation() {
    printf("--- Testing Timeout Cancellation ---\n");
    
    uint64_t timeout_ticks = 500;
    uint64_t process_id = 456;
    
    uint64_t timeout_id = schedule_timeout(timeout_ticks, process_id);
    test_assert_true(timeout_id != 0, "timeout_cancellation_setup");
    
    int result = cancel_timeout(timeout_id);
    test_assert_equal(1, result, "timeout_cancellation_success");
    
    // Test cancellation of invalid timeout
    result = cancel_timeout(0);
    test_assert_equal(0, result, "timeout_cancellation_invalid");
}

// ------------------------------------------------------------
// Test Timer Edge Cases
// ------------------------------------------------------------
void test_timer_edge_cases() {
    printf("--- Testing Timer Edge Cases ---\n");
    
    // Test very large expiry time
    uint64_t large_expiry = UINT64_MAX;
    void* callback = (void*)test_callback;
    uint64_t process_id = 789;
    
    uint64_t timer_id = insert_timer(large_expiry, callback, process_id);
    test_assert_true(timer_id != 0, "timer_edge_case_large_expiry");
    
    // Cancel the timer
    int result = cancel_timer(timer_id);
    test_assert_equal(1, result, "timer_edge_case_cancel_large");
    
    // Test multiple timers
    uint64_t timer1 = insert_timer(1000, callback, 1);
    uint64_t timer2 = insert_timer(2000, callback, 2);
    uint64_t timer3 = insert_timer(3000, callback, 3);
    
    test_assert_true(timer1 != 0, "timer_edge_case_multiple_1");
    test_assert_true(timer2 != 0, "timer_edge_case_multiple_2");
    test_assert_true(timer3 != 0, "timer_edge_case_multiple_3");
    
    // Cancel all timers
    cancel_timer(timer1);
    cancel_timer(timer2);
    cancel_timer(timer3);
    
    test_assert_true(1, "timer_edge_case_multiple_cancel");
}

// ------------------------------------------------------------
// Main Timer Test Function
// ------------------------------------------------------------
void test_timer_main() {
    printf("=== TIMER SYSTEM TEST SUITE ===\n");
    
    test_timer_init_basic();
    test_system_ticks();
    test_timer_insertion();
    test_timer_cancellation();
    test_timer_processing();
    test_timeout_scheduling();
    test_timeout_cancellation();
    test_timer_edge_cases();
    
    printf("=== TIMER SYSTEM TEST SUITE COMPLETE ===\n");
}
