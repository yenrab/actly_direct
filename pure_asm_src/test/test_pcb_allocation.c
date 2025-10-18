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
// test_pcb_allocation.c — Test PCB allocation and deallocation functions
// ------------------------------------------------------------

#include <stdint.h>
#include <stdio.h>

// External assembly functions
extern void* allocate_pcb(void);
extern uint64_t free_pcb(void* pcb);
extern void test_assert_equal(uint64_t expected, uint64_t actual, const char* test_name);
extern void test_assert_not_null(void* ptr, const char* test_name);
extern void test_assert_null(void* ptr, const char* test_name);
extern void test_assert_true(int condition, const char* test_name);

// Forward declarations for test functions
void testallocate_pcb(void);
void testfree_pcb(void);
void test_pcb_allocation_exhaustion(void);
void test_pcb_allocation_reuse(void);

// ------------------------------------------------------------
// test_pcb_allocation — Main test function
// ------------------------------------------------------------
void test_pcb_allocation(void) {
    printf("\n--- Testing PCB allocation and deallocation (Pure Assembly) ---\n");
    
    testallocate_pcb();
    testfree_pcb();
    test_pcb_allocation_exhaustion();
    test_pcb_allocation_reuse();
}

// ------------------------------------------------------------
// testallocate_pcb — Test PCB allocation function
// ------------------------------------------------------------
void testallocate_pcb(void) {
    // Test allocating a single PCB
    void* pcb1 = allocate_pcb();
    test_assert_not_null(pcb1, "allocate_pcb_single_allocation");
    
    // Test allocating multiple PCBs
    void* pcb2 = allocate_pcb();
    test_assert_not_null(pcb2, "allocate_pcb_second_allocation");
    
    void* pcb3 = allocate_pcb();
    test_assert_not_null(pcb3, "allocate_pcb_third_allocation");
    
    // Verify all PCBs are different
    test_assert_true(pcb1 != pcb2, "allocate_pcb_different_addresses_1_2");
    test_assert_true(pcb1 != pcb3, "allocate_pcb_different_addresses_1_3");
    test_assert_true(pcb2 != pcb3, "allocate_pcb_different_addresses_2_3");
    
    // Test that PCBs are aligned properly (should be 512-byte aligned)
    uint64_t addr1 = (uint64_t)pcb1;
    uint64_t addr2 = (uint64_t)pcb2;
    uint64_t addr3 = (uint64_t)pcb3;
    
    test_assert_equal(0, addr1 % 512, "allocate_pcb_alignment_pcb1");
    test_assert_equal(0, addr2 % 512, "allocate_pcb_alignment_pcb2");
    test_assert_equal(0, addr3 % 512, "allocate_pcb_alignment_pcb3");
}

// ------------------------------------------------------------
// testfree_pcb — Test PCB deallocation function
// ------------------------------------------------------------
void testfree_pcb(void) {
    // Allocate a PCB first
    void* pcb = allocate_pcb();
    test_assert_not_null(pcb, "free_pcb_allocate_first");
    
    // Free the PCB
    uint64_t result = free_pcb(pcb);
    test_assert_equal(1, result, "free_pcb_success");
    
    // Test freeing NULL pointer
    result = free_pcb(NULL);
    test_assert_equal(0, result, "free_pcb_null_pointer");
    
    // Test freeing invalid pointer
    void* invalid_pcb = (void*)0x12345678;
    result = free_pcb(invalid_pcb);
    test_assert_equal(0, result, "free_pcb_invalid_pointer");
}

// ------------------------------------------------------------
// test_pcb_allocation_exhaustion — Test PCB pool exhaustion
// ------------------------------------------------------------
void test_pcb_allocation_exhaustion(void) {
    void* pcbs[10];  // MAX_PROCESSES is 10 for testing
    
    // Allocate all available PCBs
    for (int i = 0; i < 10; i++) {
        pcbs[i] = allocate_pcb();
        test_assert_not_null(pcbs[i], "allocate_pcb_exhaustion_allocate");
    }
    
    // Try to allocate one more PCB (should fail)
    void* pcb = allocate_pcb();
    test_assert_null(pcb, "allocate_pcb_exhaustion_failure");
    
    // Free one PCB
    uint64_t result = free_pcb(pcbs[0]);
    test_assert_equal(1, result, "allocate_pcb_exhaustion_free_one");
    
    // Now we should be able to allocate one more
    void* new_pcb = allocate_pcb();
    test_assert_not_null(new_pcb, "allocate_pcb_exhaustion_allocate_after_free");
    
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
    // Allocate a PCB
    void* pcb1 = allocate_pcb();
    test_assert_not_null(pcb1, "allocate_pcb_reuse_allocate_first");
    
    // Free it
    uint64_t result = free_pcb(pcb1);
    test_assert_equal(1, result, "allocate_pcb_reuse_free_first");
    
    // Allocate again - should get the same PCB back
    void* pcb2 = allocate_pcb();
    test_assert_not_null(pcb2, "allocate_pcb_reuse_allocate_second");
    test_assert_equal((uint64_t)pcb1, (uint64_t)pcb2, "allocate_pcb_reuse_same_address");
    
    // Free it again
    result = free_pcb(pcb2);
    test_assert_equal(1, result, "allocate_pcb_reuse_free_second");
    
    // Test multiple allocations and deallocations
    void* pcbs[5];
    for (int i = 0; i < 5; i++) {
        pcbs[i] = allocate_pcb();
        test_assert_not_null(pcbs[i], "allocate_pcb_reuse_multiple_allocate");
    }
    
    // Free all of them
    for (int i = 0; i < 5; i++) {
        result = free_pcb(pcbs[i]);
        test_assert_equal(1, result, "allocate_pcb_reuse_multiple_free");
    }
    
    // Allocate again - should get the same PCBs back
    for (int i = 0; i < 5; i++) {
        void* pcb = allocate_pcb();
        test_assert_not_null(pcb, "allocate_pcb_reuse_multiple_reallocate");
        
        // Check if this is one of the previously allocated PCBs
        int found = 0;
        for (int j = 0; j < 5; j++) {
            if ((uint64_t)pcb == (uint64_t)pcbs[j]) {
                found = 1;
                break;
            }
        }
        test_assert_true(found, "allocate_pcb_reuse_multiple_same_address");
        
        // Free it immediately
        free_pcb(pcb);
    }
}
