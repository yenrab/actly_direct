/*
 * test_gc.c
 * 
 * Test _trigger_garbage_collection with a valid PCB
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

// External function declarations
extern int trigger_garbage_collection(void* pcb);

// PCB structure offsets (must match process.s)
#define PCB_STACK_BASE_OFFSET 336
#define PCB_STACK_POINTER_OFFSET 400
#define PCB_STACK_LIMIT_OFFSET 408
#define PCB_HEAP_BASE_OFFSET 352
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
    
    *(uint64_t*)((char*)pcb + PCB_STACK_BASE_OFFSET) = stack_base;
    *(uint64_t*)((char*)pcb + PCB_STACK_POINTER_OFFSET) = stack_base + 1024;  // Allocated some
    *(uint64_t*)((char*)pcb + PCB_STACK_LIMIT_OFFSET) = stack_limit;
    
    // Initialize heap bump allocator
    uint64_t heap_base = 0x4000;
    uint64_t heap_size = 4096;
    uint64_t heap_limit = heap_base + heap_size;
    
    *(uint64_t*)((char*)pcb + PCB_HEAP_BASE_OFFSET) = heap_base;
    *(uint64_t*)((char*)pcb + PCB_HEAP_POINTER_OFFSET) = heap_base + 512;  // Allocated some
    *(uint64_t*)((char*)pcb + PCB_HEAP_LIMIT_OFFSET) = heap_limit;
    
    return pcb;
}

int main() {
    // Test 1: NULL PCB (should return 0)
    int result = trigger_garbage_collection(NULL);
    if (result != 0) {
        printf("✗ NULL PCB returned unexpected result: %d\n", result);
        return 1;
    }
    
    // Test 2: Valid PCB (should return 1)
    void* pcb = create_test_pcb();
    if (pcb == NULL) {
        printf("✗ Failed to create test PCB\n");
        return 1;
    }
    
    // Call garbage collection
    result = trigger_garbage_collection(pcb);
    if (result != 1) {
        printf("✗ GC returned %d (expected 1)\n", result);
        free(pcb);
        return 1;
    }
    
    // Check if pointers were reset
    uint64_t final_stack_ptr = *(uint64_t*)((char*)pcb + PCB_STACK_POINTER_OFFSET);
    uint64_t final_heap_ptr = *(uint64_t*)((char*)pcb + PCB_HEAP_POINTER_OFFSET);
    uint64_t stack_base = *(uint64_t*)((char*)pcb + PCB_STACK_BASE_OFFSET);
    uint64_t heap_base = *(uint64_t*)((char*)pcb + PCB_HEAP_BASE_OFFSET);
    
    if (final_stack_ptr != stack_base || final_heap_ptr != heap_base) {
        printf("✗ Pointers were not reset correctly\n");
        free(pcb);
        return 1;
    }
    
    free(pcb);
    printf("✓ Test completed successfully\n");
    return 0;
}
