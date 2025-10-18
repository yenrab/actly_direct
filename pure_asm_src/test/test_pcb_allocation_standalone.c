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
// test_pcb_allocation_standalone.c — Standalone test for PCB allocation
// ------------------------------------------------------------

#include <stdio.h>

// External test function
extern void test_pcb_allocation(void);

// External test framework functions
extern void test_init(void);
extern void test_print_results(void);

int main(void) {
    printf("Starting individual test: pcb_allocation\n");
    
    // Initialize test framework
    test_init();
    printf("Test framework initialized\n");
    
    // Run the specific test
    printf("Running pcb_allocation...\n");
    test_pcb_allocation();
    printf("pcb_allocation completed\n");
    
    // Print test results
    test_print_results();
    
    printf("Individual test completed\n");
    return 0;
}
