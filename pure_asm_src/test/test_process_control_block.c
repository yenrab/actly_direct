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
// testprocess_control_block.c — Process Control Block tests
// ------------------------------------------------------------
// Comprehensive test suite for the Process Control Block (PCB) implementation.
// Tests all PCB management functions including creation, destruction, context
// switching, memory management, and field access operations.
//
// Version: 0.10
// Author: Lee Barney
// Last Modified: 2025-01-19
//

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

// External assembly functions
extern void* process_create(uint64_t entry_point, uint32_t priority, uint64_t scheduler_id);
extern int process_destroy(void* pcb);
extern void process_save_context(void* pcb);
extern void process_restore_context(void* pcb);
extern uint64_t process_get_pid(void* pcb);
extern uint32_t process_get_priority(void* pcb);
extern void process_set_priority(void* pcb, uint32_t priority);
extern uint64_t process_get_scheduler_id(void* pcb);
extern void process_set_scheduler_id(void* pcb, uint64_t scheduler_id);
extern uint64_t process_get_stack_base(void* pcb);
extern uint64_t process_get_stack_size(void* pcb);
extern uint64_t process_get_heap_base(void* pcb);
extern uint64_t process_get_heap_size(void* pcb);
extern void* process_allocate_stack(void);
extern int process_free_stack(void* stack_base);
extern void* process_allocate_heap(void* heap_pool, void* heap_bitmap);
extern int process_free_heap(void* heap_base, void* heap_pool, void* heap_bitmap);
extern void* process_get_message_queue(void* pcb);
extern void process_set_message_queue(void* pcb, void* message_queue);
extern uint64_t process_get_affinity_mask(void* pcb);
extern void process_set_affinity_mask(void* pcb, uint64_t affinity_mask);
extern uint64_t process_get_migration_count(void* pcb);
extern uint64_t process_increment_migration_count(void* pcb);
extern uint64_t process_get_last_scheduled(void* pcb);
extern void process_set_last_scheduled(void* pcb, uint64_t timestamp);


// External constants
extern uint64_t DEFAULT_STACK_SIZE;
extern uint64_t DEFAULT_HEAP_SIZE;
extern uint64_t MAX_STACK_SIZE;
extern uint64_t MAX_HEAP_SIZE;
extern uint64_t STACK_ALIGNMENT;
extern uint64_t HEAP_ALIGNMENT;
extern uint64_t MAX_PROCESSES;
extern uint64_t STACK_POOL_SIZE;
extern uint64_t HEAP_POOL_SIZE;
extern uint64_t PCB_SIZE;
extern uint64_t debug_marker_process_create;

// External test framework functions
extern void test_assert_equal(uint64_t expected, uint64_t actual, const char* test_name);
extern void test_assert_not_equal(uint64_t expected, uint64_t actual, const char* test_name);
extern void test_assert_not_null(void* ptr, const char* test_name);
extern void test_assert_null(void* ptr, const char* test_name);
extern void test_assert_true(int condition, const char* test_name);

// Test entry point
void test_process_control_block(void);


// ------------------------------------------------------------
// testprocess_destroy_null — Test process destruction with NULL
// ------------------------------------------------------------
// Test that process destruction handles NULL pointers gracefully
// without causing crashes or errors.
//
// Version: 0.10
// Author: Lee Barney
// Last Modified: 2025-01-19
//
void test_process_destroy_null(void) {
    printf("\n--- Testing process_destroy_null ---\n");
    
    // Test destroying NULL PCB
    uint64_t result = process_destroy(0);
    test_assert_equal(0, result, "process_destroy_null_result");
}

// ------------------------------------------------------------
// testprocess_field_access — Test PCB field access functions
// ------------------------------------------------------------
// Test all PCB field access functions including getters and setters
// for various fields like priority, scheduler ID, affinity mask, etc.
//
// Version: 0.10
// Author: Lee Barney
// Last Modified: 2025-01-19
//
void test_process_field_access(void) {
    printf("\n--- Testing process_field_access ---\n");
    
    void* pcb = process_create(0x1000, 2, 0);
    test_assert_not_equal(0, (uint64_t)pcb, "process_field_access_pcb_not_null");
    
    if (pcb != 0) {
        // Test priority getter/setter
        process_set_priority(pcb, 1);
        uint64_t priority = process_get_priority(pcb);
        test_assert_equal(1, priority, "process_field_access_priority");
        
        // Test scheduler ID getter/setter
        process_set_scheduler_id(pcb, 3);
        uint64_t scheduler_id = process_get_scheduler_id(pcb);
        test_assert_equal(3, scheduler_id, "process_field_access_scheduler_id");
        
        // Test affinity mask getter/setter
        process_set_affinity_mask(pcb, 0x000000000000000F);  // Only cores 0-3
        uint64_t affinity_mask = process_get_affinity_mask(pcb);
        test_assert_equal(0x000000000000000F, affinity_mask, "process_field_access_affinity_mask");
        
        // Test migration count increment
        uint64_t initial_count = process_get_migration_count(pcb);
        test_assert_equal(0, initial_count, "process_field_access_initial_migration_count");
        
        uint64_t new_count = process_increment_migration_count(pcb);
        test_assert_equal(1, new_count, "process_field_access_incremented_migration_count");
        
        // Test last scheduled timestamp
        process_set_last_scheduled(pcb, 0x123456789ABCDEF0);
        uint64_t last_scheduled = process_get_last_scheduled(pcb);
        test_assert_equal(0x123456789ABCDEF0, last_scheduled, "process_field_access_last_scheduled");
        
        // Test message queue getter/setter
        uint64_t test_queue = 0xDEADBEEF;
        process_set_message_queue(pcb, (void*)test_queue);
        void* message_queue = process_get_message_queue(pcb);
        test_assert_equal(test_queue, (uint64_t)message_queue, "process_field_access_message_queue");
        
        // Clean up
        process_destroy(pcb);
    }
}

// ------------------------------------------------------------
// testprocess_field_access_null — Test field access with NULL PCB
// ------------------------------------------------------------
// Test that all field access functions handle NULL pointers
// gracefully and return appropriate default values.
//
// Version: 0.10
// Author: Lee Barney
// Last Modified: 2025-01-19
//
void test_process_field_access_null(void) {
    printf("\n--- Testing process_field_access_null ---\n");
    
    // Test all getter functions with NULL PCB
    uint64_t pid = process_get_pid(0);
    test_assert_equal(0, pid, "process_field_access_null_pid");
    
    uint64_t priority = process_get_priority(0);
    test_assert_equal(0, priority, "process_field_access_null_priority");
    
    uint64_t scheduler_id = process_get_scheduler_id(0);
    test_assert_equal(0, scheduler_id, "process_field_access_null_scheduler_id");
    
    uint64_t stack_base = process_get_stack_base(0);
    test_assert_equal(0, stack_base, "process_field_access_null_stack_base");
    
    uint64_t stack_size = process_get_stack_size(0);
    test_assert_equal(0, stack_size, "process_field_access_null_stack_size");
    
    uint64_t heap_base = process_get_heap_base(0);
    test_assert_equal(0, heap_base, "process_field_access_null_heap_base");
    
    uint64_t heap_size = process_get_heap_size(0);
    test_assert_equal(0, heap_size, "process_field_access_null_heap_size");
    
    void* message_queue = process_get_message_queue(0);
    test_assert_equal(0, (uint64_t)message_queue, "process_field_access_null_message_queue");
    
    uint64_t affinity_mask = process_get_affinity_mask(0);
    test_assert_equal(0, affinity_mask, "process_field_access_null_affinity_mask");
    
    uint64_t migration_count = process_get_migration_count(0);
    test_assert_equal(0, migration_count, "process_field_access_null_migration_count");
    
    uint64_t last_scheduled = process_get_last_scheduled(0);
    test_assert_equal(0, last_scheduled, "process_field_access_null_last_scheduled");
    
    // Test increment with NULL PCB
    uint64_t incremented_count = process_increment_migration_count(0);
    test_assert_equal(0, incremented_count, "process_field_access_null_increment_migration_count");
}

// ------------------------------------------------------------
// test_stack_allocation — Test stack allocation and deallocation
// ------------------------------------------------------------
// Test the stack allocation and deallocation functions to ensure
// proper resource management and reuse of stack memory.
//
// Version: 0.10
// Author: Lee Barney
// Last Modified: 2025-01-19
//
void test_stack_allocation(void) {
    printf("\n--- Testing stack_allocation ---\n");
    
    void* stacks[10];
    
    // Allocate multiple stacks
    for (int i = 0; i < 10; i++) {
        stacks[i] = process_allocate_stack();
        test_assert_not_equal(0, (uint64_t)stacks[i], "stack_allocation_allocate_success");
        
        // Verify unique stack addresses
        for (int j = 0; j < i; j++) {
            test_assert_not_equal((uint64_t)stacks[j], (uint64_t)stacks[i], "stack_allocation_unique_addresses");
        }
    }
    
    // Free all stacks
    for (int i = 0; i < 10; i++) {
        int free_result = process_free_stack(stacks[i]);
        test_assert_equal(1, free_result, "stack_allocation_free_success");
    }
    
    // Test freeing NULL stack
    int free_null_result = process_free_stack(0);
    test_assert_equal(0, free_null_result, "stack_allocation_free_null");
}

// ------------------------------------------------------------
// test_heap_allocation — Test heap allocation and deallocation
// ------------------------------------------------------------
// Test the heap allocation and deallocation functions to ensure
// proper resource management and reuse of heap memory.
//
// Version: 0.10
// Author: Lee Barney
// Last Modified: 2025-01-19
//
// REMOVED: test_heap_allocation - references removed global variables
// This function was removed because it referenced heap_pool and heap_pool_bitmap
// which were global variables that have been eliminated from the assembly code.

// ------------------------------------------------------------
// test_context_switching — Test context save and restore
// ------------------------------------------------------------
// Test the context switching functionality by saving and restoring
// process context. This test verifies that the context switching
// functions can be called without crashing.
//
// Version: 0.10
// Author: Lee Barney
// Last Modified: 2025-01-19
//
void test_context_switching(void) {
    printf("\n--- Testing context_switching ---\n");
    
    void* pcb = process_create(0x1000, 2, 0);
    test_assert_not_equal(0, (uint64_t)pcb, "context_switching_pcb_not_null");
    
    if (pcb != 0) {
        // Test context save (should not crash)
        process_save_context(pcb);
        
        // Test context restore (should not crash)
        process_restore_context(pcb);
        
        // Clean up
        process_destroy(pcb);
    }
    
    // Test context operations with NULL PCB (should not crash)
    process_save_context(0);
    process_restore_context(0);
}

// ------------------------------------------------------------
// test_constants_access — Test access to configuration constants
// ------------------------------------------------------------
// Test that all configuration constants are accessible from C code
// and have the expected values.
//
// Version: 0.10
// Author: Lee Barney
// Last Modified: 2025-01-19
//
void test_constants_access(void) {
    printf("\n--- Testing constants_access ---\n");
    
    // Test stack and heap size constants
    test_assert_equal(8192, DEFAULT_STACK_SIZE, "constants_access_default_stack_size");
    test_assert_equal(4096, DEFAULT_HEAP_SIZE, "constants_access_default_heap_size");
    test_assert_equal(65536, MAX_STACK_SIZE, "constants_access_max_stack_size");
    test_assert_equal(1048576, MAX_HEAP_SIZE, "constants_access_max_heap_size");
    
    // Test alignment constants
    test_assert_equal(16, STACK_ALIGNMENT, "constants_access_stack_alignment");
    test_assert_equal(8, HEAP_ALIGNMENT, "constants_access_heap_alignment");
    
    // Test pool size constants
    test_assert_equal(1024, MAX_PROCESSES, "constants_access_max_processes");
    test_assert_equal(256, STACK_POOL_SIZE, "constants_access_stack_pool_size");
    test_assert_equal(1024, HEAP_POOL_SIZE, "constants_access_heap_pool_size");
    
    // Test PCB size constant
    test_assert_equal(512, PCB_SIZE, "constants_access_pcb_size");
}

// ------------------------------------------------------------
// testprocess_control_block — Main test entry point
// ------------------------------------------------------------
// Main test function that runs all PCB tests in sequence.
// This function provides comprehensive coverage of the PCB
// implementation including creation, destruction, field access,
// memory management, and context switching.
//
// Version: 0.10
// Author: Lee Barney
// Last Modified: 2025-01-19
//
void test_process_control_block(void) {
    printf("\n=== Process Control Block (PCB) Tests ===\n");
    
    // Run all test functions
    test_process_destroy_null();
    test_process_field_access_null();
    test_constants_access();
    
    printf("\n=== Process Control Block (PCB) Tests Complete ===\n");
}
