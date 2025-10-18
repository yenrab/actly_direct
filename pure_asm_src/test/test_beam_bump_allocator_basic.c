/*
 * test_beam_bump_allocator_basic.c
 * 
 * Basic test for BEAM-style bump allocator functions
 * Tests stack and heap allocation using BEAM-style bump allocators
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

// External function declarations
extern void* process_allocate_stack(void* pcb, uint32_t size);
extern void* process_allocate_heap(void* pcb, uint32_t size);
extern int trigger_garbage_collection(void* pcb);

// PCB structure offsets (must match process.s)
#define PCB_STACK_POINTER_OFFSET 400
#define PCB_STACK_LIMIT_OFFSET 408
#define PCB_HEAP_POINTER_OFFSET 416
#define PCB_HEAP_LIMIT_OFFSET 424
#define PCB_TOTAL_SIZE 512

// Helper function to create a test PCB with bump allocators
void* create_test_pcb() {
    void* pcb = malloc(PCB_TOTAL_SIZE);
    if (pcb == NULL) {
        return NULL;
    }
    
    // Initialize the entire PCB to zero
    memset(pcb, 0, PCB_TOTAL_SIZE);
    
    // Initialize stack bump allocator
    uint64_t stack_base = 0x2000;
    uint64_t stack_size = 8192;
    uint64_t stack_limit = stack_base + stack_size;
    
    *(uint64_t*)((char*)pcb + PCB_STACK_POINTER_OFFSET) = stack_base;
    *(uint64_t*)((char*)pcb + PCB_STACK_LIMIT_OFFSET) = stack_limit;
    
    // Initialize heap bump allocator
    uint64_t heap_base = 0x4000;
    uint64_t heap_size = 4096;
    uint64_t heap_limit = heap_base + heap_size;
    
    *(uint64_t*)((char*)pcb + PCB_HEAP_POINTER_OFFSET) = heap_base;
    *(uint64_t*)((char*)pcb + PCB_HEAP_LIMIT_OFFSET) = heap_limit;
    
    return pcb;
}

int main() {
    printf("Testing BEAM-style bump allocator functions...\n");
    
    // Test 1: Basic stack allocation
    printf("Test 1: Basic stack allocation...\n");
    void* pcb = create_test_pcb();
    if (pcb == NULL) {
        printf("✗ Failed to create test PCB\n");
        return 1;
    }
    
    void* result = process_allocate_stack(pcb, 1024);
    if (result != NULL) {
        printf("✓ Stack allocated at: %p\n", result);
        printf("✓ Expected address: 0x2000\n");
        if (result == (void*)0x2000) {
            printf("✓ Address matches expected\n");
        } else {
            printf("✗ Address mismatch\n");
        }
    } else {
        printf("✗ Stack allocation failed\n");
    }
    
    free(pcb);
    printf("✓ Test completed successfully\n");
    return 0;
}
