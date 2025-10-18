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
// test_first_function.c — First function isolation test
// ------------------------------------------------------------
// Test to isolate the first test function and debug scheduler functionality.
// This file provides a minimal test harness to run basic scheduler tests
// in isolation for debugging and development purposes.
//
// Version: 0.10
// Author: Lee Barney
// Last Modified: 2025-01-19
//

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

// External assembly functions
extern void scheduler_init(uint64_t core_id);
extern void* scheduler_schedule(uint64_t core_id);
extern void* scheduler_get_current_process(uint64_t core_id);

// External test framework functions
extern void test_init(void);
extern void test_assert_zero(uint64_t value, const char* test_name);
extern void test_print_results(void);
extern uint64_t test_failed_count;

// Forward declaration
void test_scheduler_schedule_empty(void);

int main() {
    printf("Starting first function test...\n");
    
    printf("Calling test_init()...\n");
    test_init();
    printf("test_init() completed\n");
    
    printf("Calling scheduler_init(0)...\n");
    scheduler_init(0);
    printf("scheduler_init(0) completed\n");
    
    printf("Calling test_scheduler_schedule_empty()...\n");
    test_scheduler_schedule_empty();
    printf("test_scheduler_schedule_empty() completed\n");
    
    printf("Calling test_print_results()...\n");
    test_print_results();
    printf("test_print_results() completed\n");
    
    printf("First function test completed successfully\n");
    return 0;
}

// Copy the first test function
void test_scheduler_schedule_empty(void) {
    // Note: scheduler is already initialized by main test function
    
    // Try to schedule with empty queues
    void* process = scheduler_schedule(0);
    test_assert_zero((uint64_t)process, "scheduler_schedule_empty_queues");
    
    // Verify current process is still NULL
    void* current_process = scheduler_get_current_process(0);
    test_assert_zero((uint64_t)current_process, "scheduler_schedule_empty_current_process");
}
