/*
 * test_beam_bump_allocator.c
 * 
 * Test BEAM-style bump allocator implementation
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

// Test 1: Basic stack allocation
int test_basic_stack_allocation() {
    printf("Test 1: Basic stack allocation...\n");
    
    void* pcb = create_test_pcb();
    if (pcb == NULL) {
        printf("✗ Failed to create test PCB\n");
        return 0;
    }
    
    // Allocate 1024 bytes from stack
    void* result = process_allocate_stack(pcb, 1024);
    
    if (result != NULL) {
        printf("✓ Stack allocated at: %p\n", result);
        printf("✓ Expected address: 0x2000\n");
        
        if (result == (void*)0x2000) {
            printf("✓ Address matches expected\n");
            free(pcb);
            return 1;
        } else {
            printf("✗ Address mismatch\n");
            free(pcb);
            return 0;
        }
    } else {
        printf("✗ Stack allocation failed\n");
        free(pcb);
        return 0;
    }
}

// Test 2: Multiple stack allocations
int test_multiple_stack_allocations() {
    printf("Test 2: Multiple stack allocations...\n");
    
    void* pcb = create_test_pcb();
    if (pcb == NULL) {
        printf("✗ Failed to create test PCB\n");
        return 0;
    }
    
    // Allocate multiple chunks
    void* result1 = process_allocate_stack(pcb, 512);
    void* result2 = process_allocate_stack(pcb, 1024);
    void* result3 = process_allocate_stack(pcb, 256);
    
    if (result1 != NULL && result2 != NULL && result3 != NULL) {
        printf("✓ All allocations successful\n");
        printf("✓ First allocation: %p\n", result1);
        printf("✓ Second allocation: %p\n", result2);
        printf("✓ Third allocation: %p\n", result3);
        
        // Check that allocations are sequential
        if (result1 == (void*)0x2000 && 
            result2 == (void*)0x2200 && 
            result3 == (void*)0x2600) {
            printf("✓ Allocations are sequential as expected\n");
            free(pcb);
            return 1;
        } else {
            printf("✗ Allocations are not sequential\n");
            free(pcb);
            return 0;
        }
    } else {
        printf("✗ Some allocations failed\n");
        free(pcb);
        return 0;
    }
}

// Test 3: Basic heap allocation
int test_basic_heap_allocation() {
    printf("Test 3: Basic heap allocation...\n");
    
    void* pcb = create_test_pcb();
    if (pcb == NULL) {
        printf("✗ Failed to create test PCB\n");
        return 0;
    }
    
    // Allocate 1024 bytes from heap
    void* result = process_allocate_heap(pcb, 1024);
    
    if (result != NULL) {
        printf("✓ Heap allocated at: %p\n", result);
        printf("✓ Expected address: 0x4000\n");
        
        if (result == (void*)0x4000) {
            printf("✓ Address matches expected\n");
            free(pcb);
            return 1;
        } else {
            printf("✗ Address mismatch\n");
            free(pcb);
            return 0;
        }
    } else {
        printf("✗ Heap allocation failed\n");
        free(pcb);
        return 0;
    }
}

// Test 4: Stack exhaustion
int test_stack_exhaustion() {
    printf("Test 4: Stack exhaustion...\n");
    
    void* pcb = create_test_pcb();
    if (pcb == NULL) {
        printf("✗ Failed to create test PCB\n");
        return 0;
    }
    
    // Try to allocate more than available stack space
    void* result = process_allocate_stack(pcb, 10000);  // More than 8192 bytes
    
    if (result == NULL) {
        printf("✓ Stack exhaustion handled correctly (returned NULL)\n");
        free(pcb);
        return 1;
    } else {
        printf("✗ Stack exhaustion not handled correctly\n");
        free(pcb);
        return 0;
    }
}

// Test 5: Heap exhaustion
int test_heap_exhaustion() {
    printf("Test 5: Heap exhaustion...\n");
    
    void* pcb = create_test_pcb();
    if (pcb == NULL) {
        printf("✗ Failed to create test PCB\n");
        return 0;
    }
    
    // Try to allocate more than available heap space
    void* result = process_allocate_heap(pcb, 5000);  // More than 4096 bytes
    
    if (result == NULL) {
        printf("✓ Heap exhaustion handled correctly (returned NULL)\n");
        free(pcb);
        return 1;
    } else {
        printf("✗ Heap exhaustion not handled correctly\n");
        free(pcb);
        return 0;
    }
}

// Test 6: Null PCB handling
int test_null_pcb_handling() {
    printf("Test 6: Null PCB handling...\n");
    
    // Try to allocate with NULL PCB
    void* result = process_allocate_stack(NULL, 1024);
    
    if (result == NULL) {
        printf("✓ Null PCB handled correctly (returned NULL)\n");
        return 1;
    } else {
        printf("✗ Null PCB not handled correctly\n");
        return 0;
    }
}

int main() {
    printf("Testing BEAM-style bump allocator implementation...\n\n");
    
    int passed = 0;
    int total = 6;
    
    passed += test_basic_stack_allocation();
    printf("\n");
    
    passed += test_multiple_stack_allocations();
    printf("\n");
    
    passed += test_basic_heap_allocation();
    printf("\n");
    
    passed += test_stack_exhaustion();
    printf("\n");
    
    passed += test_heap_exhaustion();
    printf("\n");
    
    passed += test_null_pcb_handling();
    printf("\n");
    
    printf("Results: %d/%d tests passed\n", passed, total);
    
    if (passed == total) {
        printf("✓ All tests passed!\n");
        return 0;
    } else {
        printf("✗ Some tests failed\n");
        return 1;
    }
}
