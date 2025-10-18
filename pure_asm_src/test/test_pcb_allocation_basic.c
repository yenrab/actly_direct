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
// test_pcb_allocation_basic.c — Basic test for PCB allocation and deallocation
// ------------------------------------------------------------

#include <stdint.h>
#include <stdio.h>

// External assembly functions
extern void* allocate_pcb(void);
extern uint64_t free_pcb(void* pcb);

int main(void) {
    printf("Testing basic PCB allocation and deallocation...\n");
    
    // Test allocating a single PCB
    printf("Calling allocate_pcb()...\n");
    void* pcb = allocate_pcb();
    printf("allocate_pcb() returned: %p\n", pcb);
    
    if (pcb != NULL) {
        printf("PCB allocation successful!\n");
        
        // Test freeing the PCB
        printf("Calling free_pcb()...\n");
        uint64_t result = free_pcb(pcb);
        printf("free_pcb() returned: %llu\n", result);
        
        if (result == 1) {
            printf("PCB deallocation successful!\n");
        } else {
            printf("PCB deallocation failed!\n");
        }
    } else {
        printf("PCB allocation failed!\n");
    }
    
    printf("Test completed.\n");
    return 0;
}
