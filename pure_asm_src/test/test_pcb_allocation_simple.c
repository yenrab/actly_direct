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
// test_pcb_allocation_simple.c — Simple test for PCB allocation without framework
// ------------------------------------------------------------

#include <stdint.h>
#include <stdio.h>

// External assembly functions
extern void* allocate_pcb(void);
extern uint64_t free_pcb(void* pcb);

// Simple test framework
static int test_count = 0;
static int test_passed = 0;

void simple_assert_equal(uint64_t expected, uint64_t actual, const char* test_name) {
    test_count++;
    if (expected == actual) {
        printf("✓ %s\n", test_name);
        test_passed++;
    } else {
        printf("✗ %s (expected: %llu, actual: %llu)\n", test_name, expected, actual);
    }
}

void simple_assert_not_null(void* ptr, const char* test_name) {
    test_count++;
    if (ptr != NULL) {
        printf("✓ %s\n", test_name);
        test_passed++;
    } else {
        printf("✗ %s (expected: non-null, actual: null)\n", test_name);
    }
}

void simple_assert_null(void* ptr, const char* test_name) {
    test_count++;
    if (ptr == NULL) {
        printf("✓ %s\n", test_name);
        test_passed++;
    } else {
        printf("✗ %s (expected: null, actual: %p)\n", test_name, ptr);
    }
}

void simple_assert_true(int condition, const char* test_name) {
    test_count++;
    if (condition) {
        printf("✓ %s\n", test_name);
        test_passed++;
    } else {
        printf("✗ %s (expected: true, actual: false)\n", test_name);
    }
}

// ------------------------------------------------------------
// test_allocate_pcb — Test PCB allocation function
// ------------------------------------------------------------
void test_allocate_pcb(void) {
    printf("\n--- Testing PCB allocation ---\n");
    
    // Test allocating a single PCB
    void* pcb1 = allocate_pcb();
    simple_assert_not_null(pcb1, "allocate_pcb_single_allocation");
    
    // Test allocating multiple PCBs
    void* pcb2 = allocate_pcb();
    simple_assert_not_null(pcb2, "allocate_pcb_second_allocation");
    
    void* pcb3 = allocate_pcb();
    simple_assert_not_null(pcb3, "allocate_pcb_third_allocation");
    
    // Verify all PCBs are different
    simple_assert_true(pcb1 != pcb2, "allocate_pcb_different_addresses_1_2");
    simple_assert_true(pcb1 != pcb3, "allocate_pcb_different_addresses_1_3");
    simple_assert_true(pcb2 != pcb3, "allocate_pcb_different_addresses_2_3");
    
    // Test that PCBs are aligned properly (should be 512-byte aligned)
    uint64_t addr1 = (uint64_t)pcb1;
    uint64_t addr2 = (uint64_t)pcb2;
    uint64_t addr3 = (uint64_t)pcb3;
    
    simple_assert_equal(0, addr1 % 512, "allocate_pcb_alignment_pcb1");
    simple_assert_equal(0, addr2 % 512, "allocate_pcb_alignment_pcb2");
    simple_assert_equal(0, addr3 % 512, "allocate_pcb_alignment_pcb3");
    
    // Clean up
    free_pcb(pcb1);
    free_pcb(pcb2);
    free_pcb(pcb3);
}

// ------------------------------------------------------------
// test_free_pcb — Test PCB deallocation function
// ------------------------------------------------------------
void test_free_pcb(void) {
    printf("\n--- Testing PCB deallocation ---\n");
    
    // Allocate a PCB first
    void* pcb = allocate_pcb();
    simple_assert_not_null(pcb, "free_pcb_allocate_first");
    
    // Free the PCB
    uint64_t result = free_pcb(pcb);
    simple_assert_equal(1, result, "free_pcb_success");
    
    // Test freeing NULL pointer
    result = free_pcb(NULL);
    simple_assert_equal(0, result, "free_pcb_null_pointer");
    
    // Test freeing invalid pointer
    void* invalid_pcb = (void*)0x12345678;
    result = free_pcb(invalid_pcb);
    simple_assert_equal(0, result, "free_pcb_invalid_pointer");
}

// ------------------------------------------------------------
// test_pcb_allocation_exhaustion — Test PCB pool exhaustion
// ------------------------------------------------------------
void test_pcb_allocation_exhaustion(void) {
    printf("\n--- Testing PCB pool exhaustion ---\n");
    
    void* pcbs[10];  // MAX_PROCESSES is 10 for testing
    
    // Allocate all available PCBs
    for (int i = 0; i < 10; i++) {
        pcbs[i] = allocate_pcb();
        simple_assert_not_null(pcbs[i], "allocate_pcb_exhaustion_allocate");
    }
    
    // Try to allocate one more PCB (should fail)
    void* pcb = allocate_pcb();
    simple_assert_null(pcb, "allocate_pcb_exhaustion_failure");
    
    // Free one PCB
    uint64_t result = free_pcb(pcbs[0]);
    simple_assert_equal(1, result, "allocate_pcb_exhaustion_free_one");
    
    // Now we should be able to allocate one more
    void* new_pcb = allocate_pcb();
    simple_assert_not_null(new_pcb, "allocate_pcb_exhaustion_allocate_after_free");
    
    // Clean up remaining PCBs
    for (int i = 1; i < 10; i++) {
        free_pcb(pcbs[i]);
    }
    free_pcb(new_pcb);
}

// ------------------------------------------------------------
// test_pcb_allocation_reuse — Test PCB reuse after deallocation
// ------------------------------------------------------------
void test_pcb_allocation_reuse(void) {
    printf("\n--- Testing PCB reuse after deallocation ---\n");
    
    // Allocate a PCB
    void* pcb1 = allocate_pcb();
    simple_assert_not_null(pcb1, "allocate_pcb_reuse_allocate_first");
    
    // Free it
    uint64_t result = free_pcb(pcb1);
    simple_assert_equal(1, result, "allocate_pcb_reuse_free_first");
    
    // Allocate again - should get the same PCB back
    void* pcb2 = allocate_pcb();
    simple_assert_not_null(pcb2, "allocate_pcb_reuse_allocate_second");
    simple_assert_equal((uint64_t)pcb1, (uint64_t)pcb2, "allocate_pcb_reuse_same_address");
    
    // Free it again
    result = free_pcb(pcb2);
    simple_assert_equal(1, result, "allocate_pcb_reuse_free_second");
}

// ------------------------------------------------------------
// test_pcb_allocation — Main test function
// ------------------------------------------------------------
void test_pcb_allocation(void) {
    printf("\n--- Testing PCB allocation and deallocation (Pure Assembly) ---\n");
    
    test_allocate_pcb();
    test_free_pcb();
    test_pcb_allocation_exhaustion();
    test_pcb_allocation_reuse();
    
    printf("\n=== Test Results ===\n");
    printf("Total Tests: %d\n", test_count);
    printf("Passed: %d\n", test_passed);
    printf("Failed: %d\n", test_count - test_passed);
    printf("========================\n");
}

int main(void) {
    printf("Starting PCB allocation tests...\n");
    test_pcb_allocation();
    printf("PCB allocation tests completed\n");
    return 0;
}
