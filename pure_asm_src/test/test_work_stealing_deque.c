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
// test_work_stealing_deque.c — C tests for work stealing deque operations
// ------------------------------------------------------------

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// External assembly functions
extern int ws_deque_init(void* deque_ptr, uint32_t size);
extern int ws_deque_push_bottom(void* deque_ptr, void* process);
extern void* ws_deque_pop_bottom(void* deque_ptr);
extern void* ws_deque_pop_top(void* deque_ptr);
extern int ws_deque_is_empty(void* deque_ptr);
extern uint32_t ws_deque_size(void* deque_ptr);

// Use constant from config.inc
#define WS_DEQUE_SIZE_BYTES 64

// Forward declarations for test functions
static void test_deque_init();
static void test_deque_push_bottom();
static void test_deque_pop_bottom();
static void test_deque_pop_top();
static void test_deque_is_empty();
static void test_deque_size();
static void test_deque_circular_buffer();
static void test_deque_concurrent_access();

// Test framework functions
extern void test_assert_equal(uint64_t expected, uint64_t actual, const char* test_name);
extern void test_assert_zero(uint64_t value, const char* test_name);
extern void test_assert_nonzero(uint64_t value, const char* test_name);

// ------------------------------------------------------------
// test_work_stealing_deque — Main test function for work stealing deque
// ------------------------------------------------------------
void test_work_stealing_deque() {
    printf("\n--- Testing Work Stealing Deque (Pure Assembly) ---\n");
    
    test_deque_init();
    test_deque_push_bottom();
    test_deque_pop_bottom();
    test_deque_pop_top();
    test_deque_is_empty();
    test_deque_size();
    test_deque_circular_buffer();
    test_deque_concurrent_access();
}

// ------------------------------------------------------------
// test_deque_init — Test deque initialization
// ------------------------------------------------------------
void test_deque_init() {
    printf("Testing deque initialization...\n");
    
    // Test initialization with valid size (power of 2)
    void* deque1 = malloc(WS_DEQUE_SIZE_BYTES);
    memset(deque1, 0, WS_DEQUE_SIZE_BYTES);  // Zero out to avoid garbage values
    test_assert_nonzero((uint64_t)deque1, "deque1_allocation");
    int result = ws_deque_init(deque1, 8);
    test_assert_equal(1, result, "deque_init_valid_size");
    free(deque1);
    
    // Test initialization with invalid size (not power of 2)
    void* deque2 = malloc(WS_DEQUE_SIZE_BYTES);
    memset(deque2, 0, WS_DEQUE_SIZE_BYTES);
    test_assert_nonzero((uint64_t)deque2, "deque2_allocation");
    result = ws_deque_init(deque2, 7);
    test_assert_equal(0, result, "deque_init_invalid_size");
    free(deque2);
    
    // Test initialization with size too small
    void* deque3 = malloc(WS_DEQUE_SIZE_BYTES);
    memset(deque3, 0, WS_DEQUE_SIZE_BYTES);
    test_assert_nonzero((uint64_t)deque3, "deque3_allocation");
    result = ws_deque_init(deque3, 1);
    test_assert_equal(0, result, "deque_init_size_too_small");
    free(deque3);
    
    // Test initialization with size too large
    void* deque4 = malloc(WS_DEQUE_SIZE_BYTES);
    memset(deque4, 0, WS_DEQUE_SIZE_BYTES);
    test_assert_nonzero((uint64_t)deque4, "deque4_allocation");
    result = ws_deque_init(deque4, 2048);
    test_assert_equal(0, result, "deque_init_size_too_large");
    free(deque4);
    
    // Test initialization with NULL pointer
    result = ws_deque_init(NULL, 8);
    test_assert_equal(0, result, "deque_init_null_pointer");
    
    // Test initialization with zero size
    void* deque5 = malloc(WS_DEQUE_SIZE_BYTES);
    memset(deque5, 0, WS_DEQUE_SIZE_BYTES);
    test_assert_nonzero((uint64_t)deque5, "deque5_allocation");
    result = ws_deque_init(deque5, 0);
    test_assert_equal(0, result, "deque_init_zero_size");
    free(deque5);
}

// ------------------------------------------------------------
// test_deque_push_bottom — Test push bottom operation
// ------------------------------------------------------------
void test_deque_push_bottom() {
    printf("Testing deque push bottom...\n");
    
    // Allocate memory for deque structure
    void* deque = malloc(WS_DEQUE_SIZE_BYTES);
    memset(deque, 0, WS_DEQUE_SIZE_BYTES);
    test_assert_nonzero((uint64_t)deque, "deque_allocation");
    
    // Initialize deque
    int result = ws_deque_init(deque, 8);
    test_assert_equal(1, result, "deque_init");
    
    // Test pushing valid process
    void* process1 = (void*)0x12345678;
    result = ws_deque_push_bottom(deque, process1);
    test_assert_equal(1, result, "deque_push_bottom_valid");
    
    // Test pushing another process
    void* process2 = (void*)0x87654321;
    result = ws_deque_push_bottom(deque, process2);
    test_assert_equal(1, result, "deque_push_bottom_second");
    
    // Test pushing NULL process
    result = ws_deque_push_bottom(deque, NULL);
    test_assert_equal(0, result, "deque_push_bottom_null_process");
    
    // Test pushing to NULL deque
    result = ws_deque_push_bottom(NULL, process1);
    test_assert_equal(0, result, "deque_push_bottom_null_deque");
    
    // Verify deque is not empty
    int empty = ws_deque_is_empty(deque);
    test_assert_equal(0, empty, "deque_not_empty_after_push");
    
    // Verify deque size
    uint32_t size = ws_deque_size(deque);
    test_assert_equal(2, size, "deque_size_after_push");
    
    free(deque);
}

// ------------------------------------------------------------
// test_deque_pop_bottom — Test pop bottom operation
// ------------------------------------------------------------
void test_deque_pop_bottom() {
    printf("Testing deque pop bottom...\n");
    
    // Allocate memory for deque structure
    void* deque = malloc(WS_DEQUE_SIZE_BYTES);
    memset(deque, 0, WS_DEQUE_SIZE_BYTES);
    test_assert_nonzero((uint64_t)deque, "deque_allocation");
    
    // Initialize deque
    int result = ws_deque_init(deque, 8);
    test_assert_equal(1, result, "deque_init");
    
    // Test popping from empty deque
    void* process = ws_deque_pop_bottom(deque);
    test_assert_equal(0, (uint64_t)process, "deque_pop_bottom_empty");
    
    // Push some processes
    void* process1 = (void*)0x11111111;
    void* process2 = (void*)0x22222222;
    void* process3 = (void*)0x33333333;
    
    ws_deque_push_bottom(deque, process1);
    ws_deque_push_bottom(deque, process2);
    ws_deque_push_bottom(deque, process3);
    
    // Test popping processes (should get them in LIFO order)
    process = ws_deque_pop_bottom(deque);
    test_assert_equal((uint64_t)process3, (uint64_t)process, "deque_pop_bottom_first");
    
    process = ws_deque_pop_bottom(deque);
    test_assert_equal((uint64_t)process2, (uint64_t)process, "deque_pop_bottom_second");
    
    process = ws_deque_pop_bottom(deque);
    test_assert_equal((uint64_t)process1, (uint64_t)process, "deque_pop_bottom_third");
    
    // Test popping from empty deque again
    process = ws_deque_pop_bottom(deque);
    test_assert_equal(0, (uint64_t)process, "deque_pop_bottom_empty_again");
    
    // Test popping from NULL deque
    process = ws_deque_pop_bottom(NULL);
    test_assert_equal(0, (uint64_t)process, "deque_pop_bottom_null_deque");
    
    free(deque);
}

// ------------------------------------------------------------
// test_deque_pop_top — Test pop top operation (work stealing)
// ------------------------------------------------------------
void test_deque_pop_top() {
    printf("Testing deque pop top (work stealing)...\n");
    
    // Allocate memory for deque structure
    void* deque = malloc(WS_DEQUE_SIZE_BYTES);
    memset(deque, 0, WS_DEQUE_SIZE_BYTES);
    test_assert_nonzero((uint64_t)deque, "deque_allocation");
    
    // Initialize deque
    int result = ws_deque_init(deque, 8);
    test_assert_equal(1, result, "deque_init");
    
    // Test stealing from empty deque
    void* process = ws_deque_pop_top(deque);
    test_assert_equal(0, (uint64_t)process, "deque_pop_top_empty");
    
    // Push some processes
    void* process1 = (void*)0xAAAAAAAA;
    void* process2 = (void*)0xBBBBBBBB;
    void* process3 = (void*)0xCCCCCCCC;
    
    ws_deque_push_bottom(deque, process1);
    ws_deque_push_bottom(deque, process2);
    ws_deque_push_bottom(deque, process3);
    
    // Test stealing processes (should get them in FIFO order)
    process = ws_deque_pop_top(deque);
    test_assert_equal((uint64_t)process1, (uint64_t)process, "deque_pop_top_first");
    
    process = ws_deque_pop_top(deque);
    test_assert_equal((uint64_t)process2, (uint64_t)process, "deque_pop_top_second");
    
    process = ws_deque_pop_top(deque);
    test_assert_equal((uint64_t)process3, (uint64_t)process, "deque_pop_top_third");
    
    // Test stealing from empty deque again
    process = ws_deque_pop_top(deque);
    test_assert_equal(0, (uint64_t)process, "deque_pop_top_empty_again");
    
    // Test stealing from NULL deque
    process = ws_deque_pop_top(NULL);
    test_assert_equal(0, (uint64_t)process, "deque_pop_top_null_deque");
    
    free(deque);
}

// ------------------------------------------------------------
// test_deque_is_empty — Test empty check
// ------------------------------------------------------------
void test_deque_is_empty() {
    printf("Testing deque empty check...\n");
    
    // Allocate memory for deque structure
    void* deque = malloc(WS_DEQUE_SIZE_BYTES);
    memset(deque, 0, WS_DEQUE_SIZE_BYTES);
    test_assert_nonzero((uint64_t)deque, "deque_allocation");
    
    // Initialize deque
    int result = ws_deque_init(deque, 8);
    test_assert_equal(1, result, "deque_init");
    
    // Test empty deque
    int empty = ws_deque_is_empty(deque);
    test_assert_equal(1, empty, "deque_is_empty_initially");
    
    // Push a process
    void* process = (void*)0x12345678;
    ws_deque_push_bottom(deque, process);
    
    // Test non-empty deque
    empty = ws_deque_is_empty(deque);
    test_assert_equal(0, empty, "deque_is_not_empty_after_push");
    
    // Pop the process
    ws_deque_pop_bottom(deque);
    
    // Test empty deque again
    empty = ws_deque_is_empty(deque);
    test_assert_equal(1, empty, "deque_is_empty_after_pop");
    
    // Test NULL deque
    empty = ws_deque_is_empty(NULL);
    test_assert_equal(1, empty, "deque_is_empty_null_deque");
    
    free(deque);
}

// ------------------------------------------------------------
// test_deque_size — Test size calculation
// ------------------------------------------------------------
void test_deque_size() {
    printf("Testing deque size calculation...\n");
    
    // Allocate memory for deque structure
    void* deque = malloc(WS_DEQUE_SIZE_BYTES);
    memset(deque, 0, WS_DEQUE_SIZE_BYTES);
    test_assert_nonzero((uint64_t)deque, "deque_allocation");
    
    // Initialize deque
    int result = ws_deque_init(deque, 8);
    test_assert_equal(1, result, "deque_init");
    
    // Test empty deque size
    uint32_t size = ws_deque_size(deque);
    test_assert_equal(0, size, "deque_size_empty");
    
    // Push processes and test size
    void* process1 = (void*)0x11111111;
    void* process2 = (void*)0x22222222;
    void* process3 = (void*)0x33333333;
    
    ws_deque_push_bottom(deque, process1);
    size = ws_deque_size(deque);
    test_assert_equal(1, size, "deque_size_one");
    
    ws_deque_push_bottom(deque, process2);
    size = ws_deque_size(deque);
    test_assert_equal(2, size, "deque_size_two");
    
    ws_deque_push_bottom(deque, process3);
    size = ws_deque_size(deque);
    test_assert_equal(3, size, "deque_size_three");
    
    // Pop processes and test size
    ws_deque_pop_bottom(deque);
    size = ws_deque_size(deque);
    test_assert_equal(2, size, "deque_size_after_pop");
    
    ws_deque_pop_bottom(deque);
    size = ws_deque_size(deque);
    test_assert_equal(1, size, "deque_size_after_second_pop");
    
    ws_deque_pop_bottom(deque);
    size = ws_deque_size(deque);
    test_assert_equal(0, size, "deque_size_after_third_pop");
    
    // Test NULL deque size
    size = ws_deque_size(NULL);
    test_assert_equal(0, size, "deque_size_null_deque");
    
    free(deque);
}

// ------------------------------------------------------------
// test_deque_circular_buffer — Test circular buffer wraparound
// ------------------------------------------------------------
void test_deque_circular_buffer() {
    printf("Testing deque circular buffer wraparound...\n");
    
    // Allocate memory for deque structure
    void* deque = malloc(WS_DEQUE_SIZE_BYTES);
    memset(deque, 0, WS_DEQUE_SIZE_BYTES);
    test_assert_nonzero((uint64_t)deque, "deque_allocation");
    
    // Initialize deque with small size to test wraparound
    int result = ws_deque_init(deque, 4);
    test_assert_equal(1, result, "deque_init");
    
    // Fill deque to capacity
    void* process1 = (void*)0x11111111;
    void* process2 = (void*)0x22222222;
    void* process3 = (void*)0x33333333;
    void* process4 = (void*)0x44444444;
    
    ws_deque_push_bottom(deque, process1);
    ws_deque_push_bottom(deque, process2);
    ws_deque_push_bottom(deque, process3);
    ws_deque_push_bottom(deque, process4);
    
    // Verify size is correct
    uint32_t size = ws_deque_size(deque);
    test_assert_equal(4, size, "deque_size_full");
    
    // Pop all processes
    void* popped1 = ws_deque_pop_bottom(deque);
    void* popped2 = ws_deque_pop_bottom(deque);
    void* popped3 = ws_deque_pop_bottom(deque);
    void* popped4 = ws_deque_pop_bottom(deque);
    
    // Verify we got the processes in correct order
    test_assert_equal((uint64_t)process4, (uint64_t)popped1, "deque_wraparound_first");
    test_assert_equal((uint64_t)process3, (uint64_t)popped2, "deque_wraparound_second");
    test_assert_equal((uint64_t)process2, (uint64_t)popped3, "deque_wraparound_third");
    test_assert_equal((uint64_t)process1, (uint64_t)popped4, "deque_wraparound_fourth");
    
    // Verify deque is empty
    int empty = ws_deque_is_empty(deque);
    test_assert_equal(1, empty, "deque_empty_after_wraparound");
    
    free(deque);
}

// ------------------------------------------------------------
// test_deque_concurrent_access — Test concurrent access patterns
// ------------------------------------------------------------
void test_deque_concurrent_access() {
    printf("Testing deque concurrent access patterns...\n");
    
    // Allocate memory for deque structure
    void* deque = malloc(WS_DEQUE_SIZE_BYTES);
    memset(deque, 0, WS_DEQUE_SIZE_BYTES);
    test_assert_nonzero((uint64_t)deque, "deque_allocation");
    
    // Initialize deque
    int result = ws_deque_init(deque, 16);
    test_assert_equal(1, result, "deque_init");
    
    // Test mixed push/pop operations
    void* process1 = (void*)0x11111111;
    void* process2 = (void*)0x22222222;
    void* process3 = (void*)0x33333333;
    void* process4 = (void*)0x44444444;
    void* process5 = (void*)0x55555555;
    
    // Push some processes
    ws_deque_push_bottom(deque, process1);
    ws_deque_push_bottom(deque, process2);
    
    // Steal one process
    void* stolen = ws_deque_pop_top(deque);
    test_assert_equal((uint64_t)process1, (uint64_t)stolen, "deque_concurrent_steal");
    
    // Push more processes
    ws_deque_push_bottom(deque, process3);
    ws_deque_push_bottom(deque, process4);
    
    // Pop locally
    void* local = ws_deque_pop_bottom(deque);
    test_assert_equal((uint64_t)process4, (uint64_t)local, "deque_concurrent_local_pop");
    
    // Steal another process
    stolen = ws_deque_pop_top(deque);
    test_assert_equal((uint64_t)process2, (uint64_t)stolen, "deque_concurrent_steal_second");
    
    // Push and pop to test consistency
    ws_deque_push_bottom(deque, process5);
    local = ws_deque_pop_bottom(deque);
    test_assert_equal((uint64_t)process5, (uint64_t)local, "deque_concurrent_push_pop");
    
    // Verify final state
    uint32_t size = ws_deque_size(deque);
    test_assert_equal(1, size, "deque_concurrent_final_size");
    
    // Pop remaining process
    local = ws_deque_pop_bottom(deque);
    test_assert_equal((uint64_t)process3, (uint64_t)local, "deque_concurrent_final_pop");
    
    // Verify empty
    int empty = ws_deque_is_empty(deque);
    test_assert_equal(1, empty, "deque_concurrent_final_empty");
    
    free(deque);
}
