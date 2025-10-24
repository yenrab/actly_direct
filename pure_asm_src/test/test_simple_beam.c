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
    // Test 1: Call with NULL PCB (should return NULL)
    void* result = process_allocate_stack(NULL, 1024);
    if (result != NULL) {
        printf("✗ NULL PCB not handled correctly\n");
        return 1;
    }
    
    // Test 2: Call garbage collection
    int gc_result = trigger_garbage_collection(NULL);
    if (gc_result != 0) {
        printf("✗ GC returned unexpected result: %d\n", gc_result);
        return 1;
    }
    
    printf("✓ Simple test completed successfully\n");
    return 0;
}
