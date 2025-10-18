/*
 * test_simple_beam.c
 * 
 * Very simple test for BEAM-style functions
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

// External function declarations
extern void* process_allocate_stack(void* pcb, uint32_t size);
extern int trigger_garbage_collection(void* pcb);

int main() {
    printf("Testing simple BEAM functions...\n");
    
    // Test 1: Call with NULL PCB (should return NULL)
    printf("Test 1: NULL PCB test...\n");
    void* result = process_allocate_stack(NULL, 1024);
    if (result == NULL) {
        printf("✓ NULL PCB handled correctly\n");
    } else {
        printf("✗ NULL PCB not handled correctly\n");
    }
    
    // Test 2: Call garbage collection
    printf("Test 2: Garbage collection test...\n");
    int gc_result = trigger_garbage_collection(NULL);
    printf("✓ GC returned: %d\n", gc_result);
    
    printf("✓ Simple test completed successfully\n");
    return 0;
}
