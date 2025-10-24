// C wrapper for memory pool expansion that uses C library functions
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

// C implementation of expand_memory_pool that uses C library functions
void expand_memory_pool_c_wrapper() {
    // Validate input parameters
    if (pool_base == NULL) {
        return 0;
    }
    if (block_size == 0) {
        return 0;
    }
    if (expansion_size == 0) {
        return 0;
    }
    
    // Check for reasonable limits
    if (expansion_size > 1024) {
        return 0;
    }
    
    // Calculate total memory needed for expansion
    uint32_t total_bytes = expansion_size * block_size;
    if (total_bytes > 1048576) { // 1MB limit
        return 0;
    }
    
    // Calculate new pool end address
    uint32_t current_pool_size = current_size * block_size;
    uintptr_t current_pool_end = (uintptr_t)pool_base + current_pool_size;
    
    // Use mmap to allocate memory
    void* new_memory = mmap(NULL, total_bytes, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);
    
    if (new_memory == MAP_FAILED) {
        return 0;
    }
    
    // Check if the allocated memory is contiguous with existing pool
    if ((uintptr_t)new_memory != current_pool_end) {
        // Not contiguous, unmap and return failure
        munmap(new_memory, total_bytes);
        return 0;
    }
    
    // Initialize newly allocated memory blocks as free (set to 0)
    memset(new_memory, 0, total_bytes);
    
    return 1; // Success
}

int main() {
    printf("=== Testing C Wrapper for Memory Pool Expansion ===\n");
    
    // Test 1: Basic functionality
    printf("\n--- Test 1: Basic Functionality ---\n");
    
    // Use static memory
    static uint8_t test_pool[8192]; // 8KB buffer
    uint32_t block_size = 64;
    uint32_t initial_blocks = 10;
    uint32_t expansion_blocks = 5;
    
    // Initialize pool with test data
    memset(test_pool, 0xAA, sizeof(test_pool));
    
    printf("Pool allocated at: %p\n", test_pool);
    printf("Pool size: %zu bytes\n", sizeof(test_pool));
    printf("Block size: %u bytes\n", block_size);
    printf("Initial blocks: %u\n", initial_blocks);
    printf("Expansion blocks: %u\n", expansion_blocks);
    
    // Test the function
    printf("Calling expand_memory_pool_c_wrapper...\n");
    int result = expand_memory_pool_c_wrapper(test_pool, initial_blocks, block_size, expansion_blocks);
    printf("Result: %d\n", result);
    
    if (result == 1) {
        printf("✓ SUCCESS: Pool expansion succeeded\n");
    } else {
        printf("✗ FAILED: Pool expansion failed\n");
    }
    
    // Test 2: Invalid parameters
    printf("\n--- Test 2: Invalid Parameters ---\n");
    
    // Test NULL pool_base
    printf("Testing NULL pool_base...\n");
    result = expand_memory_pool_c_wrapper(NULL, 10, 64, 5);
    printf("Result: %d (expected 0)\n", result);
    
    if (result == 0) {
        printf("✓ SUCCESS: NULL pool_base correctly rejected\n");
    } else {
        printf("✗ FAILED: NULL pool_base should be rejected\n");
    }
    
    printf("\n=== C Wrapper Test Complete ===\n");
    return 0;
}
