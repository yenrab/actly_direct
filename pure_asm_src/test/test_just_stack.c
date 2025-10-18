/*
 * test_just_stack.c
 * 
 * Test just the stack allocation function
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

// External function declarations
extern void* process_allocate_stack(void* pcb, uint32_t size);

int main() {
    printf("Testing just stack allocation...\n");
    
    // Test 1: Call with NULL PCB (should return NULL)
    printf("Test 1: NULL PCB test...\n");
    void* result = process_allocate_stack(NULL, 1024);
    if (result == NULL) {
        printf("✓ NULL PCB handled correctly\n");
    } else {
        printf("✗ NULL PCB not handled correctly\n");
    }
    
    printf("✓ Test completed successfully\n");
    return 0;
}
