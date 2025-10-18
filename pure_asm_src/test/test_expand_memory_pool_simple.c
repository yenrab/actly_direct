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
// test_expand_memory_pool_simple.c — Simple test for memory pool expansion
// ------------------------------------------------------------
// Simple test to verify the _expand_memory_pool function works correctly.
//
// Version: 0.16
// Author: Lee Barney
// Last Modified: 2025-01-19
//

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// External assembly functions
extern int expand_memory_pool(void* pool_base, uint32_t current_size, uint32_t block_size, uint32_t expansion_size);

// ------------------------------------------------------------
// main — Simple test of expand_memory_pool function
// ------------------------------------------------------------
int main(void) {
    printf("=== Testing expand_memory_pool Function (Simple) ===\n");
    
    // Test 1: Basic functionality
    printf("\n--- Test 1: Basic Functionality ---\n");
    
    // Allocate a small test pool
    uint32_t block_size = 64;
    uint32_t initial_blocks = 2;
    uint32_t expansion_blocks = 1;
    size_t pool_size = initial_blocks * block_size;
    
    void* pool = malloc(pool_size);
    if (!pool) {
        printf("ERROR: Failed to allocate test pool\n");
        return 1;
    }
    
    printf("Pool allocated at: %p\n", pool);
    printf("Pool size: %zu bytes\n", pool_size);
    printf("Block size: %u bytes\n", block_size);
    printf("Initial blocks: %u\n", initial_blocks);
    printf("Expansion blocks: %u\n", expansion_blocks);
    
    // Initialize pool with test data
    memset(pool, 0xAA, pool_size);
    printf("Pool initialized with 0xAA\n");
    
    // Test the function
    printf("Calling expand_memory_pool...\n");
    int result = expand_memory_pool(pool, initial_blocks, block_size, expansion_blocks);
    printf("Result: %d\n", result);
    
    if (result == 1) {
        printf("✓ SUCCESS: Pool expansion succeeded\n");
    } else {
        printf("✗ FAILED: Pool expansion failed\n");
    }
    
    free(pool);
    
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
    printf("Testing zero block_size...\n");
    pool = malloc(640);
    if (pool) {
        result = expand_memory_pool(pool, 10, 0, 5);
        printf("Result: %d (expected 0)\n", result);
        
        if (result == 0) {
            printf("✓ SUCCESS: Zero block_size correctly rejected\n");
        } else {
            printf("✗ FAILED: Zero block_size should be rejected\n");
        }
        free(pool);
    }
    
    // Test zero expansion_size
    printf("Testing zero expansion_size...\n");
    pool = malloc(640);
    if (pool) {
        result = expand_memory_pool(pool, 10, 64, 0);
        printf("Result: %d (expected 0)\n", result);
        
        if (result == 0) {
            printf("✓ SUCCESS: Zero expansion_size correctly rejected\n");
        } else {
            printf("✗ FAILED: Zero expansion_size should be rejected\n");
        }
        free(pool);
    }
    
    printf("\n=== Simple Test Complete ===\n");
    return 0;
}
