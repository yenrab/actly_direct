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
    // Test 1: Call with NULL PCB (should return NULL)
    void* result = process_allocate_stack(NULL, 1024);
    if (result != NULL) {
        printf("✗ NULL PCB not handled correctly\n");
        return 1;
    }
    
    printf("✓ Test completed successfully\n");
    return 0;
}
