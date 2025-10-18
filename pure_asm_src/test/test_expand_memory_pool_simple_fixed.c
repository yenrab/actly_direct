// Simple test for memory pool expansion that doesn't access allocated memory
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// External assembly functions
extern int expand_memory_pool(void* pool_base, uint32_t current_size, uint32_t block_size, uint32_t expansion_size);

int main(void) {
    printf("=== Testing expand_memory_pool Function (Simple Fixed) ===\n");
    
    // Test 1: Basic functionality with larger static buffer
    printf("\n--- Test 1: Basic Functionality ---\n");
    
    // Use a larger static buffer to avoid accessing invalid memory
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
    printf("Calling expand_memory_pool...\n");
    int result = expand_memory_pool(test_pool, initial_blocks, block_size, expansion_blocks);
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
    result = expand_memory_pool(NULL, 10, 64, 5);
    printf("Result: %d (expected 0)\n", result);
    
    if (result == 0) {
        printf("✓ SUCCESS: NULL pool_base correctly rejected\n");
    } else {
        printf("✗ FAILED: NULL pool_base should be rejected\n");
    }
    
    // Test zero block_size
    printf("\nTesting zero block_size...\n");
    result = expand_memory_pool(test_pool, 10, 0, 5);
    printf("Result: %d (expected 0)\n", result);
    
    if (result == 0) {
        printf("✓ SUCCESS: Zero block_size correctly rejected\n");
    } else {
        printf("✗ FAILED: Zero block_size should be rejected\n");
    }
    
    // Test zero expansion_size
    printf("\nTesting zero expansion_size...\n");
    result = expand_memory_pool(test_pool, 10, 64, 0);
    printf("Result: %d (expected 0)\n", result);
    
    if (result == 0) {
        printf("✓ SUCCESS: Zero expansion_size correctly rejected\n");
    } else {
        printf("✗ FAILED: Zero expansion_size should be rejected\n");
    }
    
    printf("\n=== Simple Test Complete ===\n");
    return 0;
}
