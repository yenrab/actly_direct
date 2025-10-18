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
// test_pcb_runner.c — Test runner for process control block functionality
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
extern void test_process_control_block(void);

// External test framework functions
extern void test_init(void);
extern void test_print_results(void);
extern uint64_t test_failed_count;

// ------------------------------------------------------------
// test_pcb_runner_main — PCB-only test runner entry point
// ------------------------------------------------------------
int test_pcb_runner_main(void) {
    printf("\n========================================\n");
    printf("    PCB UNIT TEST RUNNER\n");
    printf("========================================\n");
    
    // Initialize test framework
    test_init();
    
    // Run PCB tests
    test_process_control_block();
    
    // Print test results
    test_print_results();
    
    // Return exit code based on test results
    if (test_failed_count == 0) {
        printf("\n*** ALL PCB TESTS PASSED ***\n");
        return 0;
    } else {
        printf("\n*** SOME PCB TESTS FAILED ***\n");
        return 1;
    }
}

// ------------------------------------------------------------
// main — Program entry point
// ------------------------------------------------------------
int main(void) {
    printf("[test_pcb] Starting PCB test mode\n");
    
    int result = test_pcb_runner_main();
    
    printf("[test_pcb] PCB tests completed\n");
    
    return result;
}
