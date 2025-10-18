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
// test_scheduler_queue_length_individual.c — Individual test runner for scheduler queue length
// ------------------------------------------------------------
// Standalone test runner for scheduler queue length functionality.
// This file provides a minimal test harness to run queue length tests
// in isolation for debugging and development purposes.
//
// Version: 0.10
// Author: Lee Barney
// Last Modified: 2025-01-19
//

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

// External assembly functions
extern void* get_scheduler_state(uint64_t core_id);
extern void* get_priority_queue(void* state, uint64_t priority);
extern uint64_t scheduler_get_queue_length_from_queue(void* queue);
extern uint64_t scheduler_get_queue_length_queue_ptr(void* queue);

// Test functions
extern void test_scheduler_queue_length(void);

int main(void) {
    printf("Running scheduler queue length individual test...\n");
    
    test_scheduler_queue_length();
    
    printf("Scheduler queue length individual test completed.\n");
    return 0;
}
