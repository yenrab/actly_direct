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
// test_scheduler_get_set_process.c — C tests for pure assembly process management
// ------------------------------------------------------------

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "scheduler_functions.h"
extern void test_assert_equal(uint64_t expected, uint64_t actual, const char* test_name);
extern void test_assert_zero(uint64_t value, const char* test_name);


// Forward declarations for test functions
static void test_scheduler_get_current_process();
static void test_scheduler_set_current_process();
static void test_scheduler_process_cross_core_isolation();

// ------------------------------------------------------------
// test_scheduler_get_set_process — Main test function
// ------------------------------------------------------------
void test_scheduler_get_set_process() {
    printf("\n--- Testing scheduler get/set current process (Pure Assembly) ---\n");
    
    test_scheduler_get_current_process();
    test_scheduler_set_current_process();
    
    test_scheduler_process_cross_core_isolation();
}

// ------------------------------------------------------------
// test_scheduler_get_current_process — Test get current process
// ------------------------------------------------------------
void test_scheduler_get_current_process() {
    printf("DEBUG: test_scheduler_get_current_process() starting\n");
    fflush(stdout);
    
    // Create multi-core scheduler state for 4 cores
    printf("DEBUG: About to call scheduler_state_init() for 4 cores\n");
    fflush(stdout);
    void* scheduler_state = scheduler_state_init(4);
    if (scheduler_state == NULL) {
        printf("ERROR: Failed to create scheduler state\n");
        return;
    }
    printf("DEBUG: scheduler_state_init() completed successfully\n");
    fflush(stdout);
    
    // Initialize schedulers for multiple cores (proper multi-core usage)
    printf("DEBUG: About to call scheduler_init() for core 0\n");
    fflush(stdout);
    scheduler_init(scheduler_state, 0);
    printf("DEBUG: scheduler_init() for core 0 completed\n");
    fflush(stdout);
    
    printf("DEBUG: About to call scheduler_init() for core 1\n");
    fflush(stdout);
    scheduler_init(scheduler_state, 1);
    printf("DEBUG: scheduler_init() for core 1 completed\n");
    fflush(stdout);
    
    printf("DEBUG: About to call scheduler_init() for core 2\n");
    fflush(stdout);
    scheduler_init(scheduler_state, 2);
    printf("DEBUG: scheduler_init() for core 2 completed\n");
    fflush(stdout);
    
    // Test getting current process for each core (should be NULL initially)
    printf("DEBUG: About to call scheduler_get_current_process_with_state() for core 0\n");
    fflush(stdout);
    void* process = scheduler_get_current_process_with_state(scheduler_state, 0);
    printf("DEBUG: scheduler_get_current_process_with_state() for core 0 completed, got process=%p\n", process);
    fflush(stdout);
    test_assert_zero((uint64_t)process, "scheduler_get_current_process_null");
    
    printf("DEBUG: About to call scheduler_get_current_process_with_state() for core 1\n");
    fflush(stdout);
    process = scheduler_get_current_process_with_state(scheduler_state, 1);
    printf("DEBUG: scheduler_get_current_process_with_state() for core 1 completed, got process=%p\n", process);
    fflush(stdout);
    test_assert_zero((uint64_t)process, "scheduler_get_current_process_null_core1");
    
    printf("DEBUG: About to call scheduler_get_current_process_with_state() for core 2\n");
    fflush(stdout);
    process = scheduler_get_current_process_with_state(scheduler_state, 2);
    printf("DEBUG: scheduler_get_current_process_with_state() for core 2 completed, got process=%p\n", process);
    fflush(stdout);
    test_assert_zero((uint64_t)process, "scheduler_get_current_process_null_core2");
    
    // Clean up scheduler state
    scheduler_state_destroy(scheduler_state);
}

// ------------------------------------------------------------
// test_scheduler_set_current_process — Test set current process
// ------------------------------------------------------------
void test_scheduler_set_current_process() {
    printf("DEBUG: test_scheduler_set_current_process() starting\n");
    fflush(stdout);
    
    // Test 1: Setting current process to a valid memory address
    {
        // Create single-core scheduler state for this test
        printf("DEBUG: About to call scheduler_state_init() for test 1\n");
        fflush(stdout);
        void* scheduler_state = scheduler_state_init(1);
        if (scheduler_state == NULL) {
            printf("ERROR: Failed to create scheduler state for test 1\n");
            return;
        }
        printf("DEBUG: scheduler_state_init() completed successfully for test 1\n");
        fflush(stdout);
        
        // Initialize scheduler for core 0
        printf("DEBUG: About to call scheduler_init() for core 0 in test 1\n");
        fflush(stdout);
        scheduler_init(scheduler_state, 0);
        printf("DEBUG: scheduler_init() for core 0 completed in test 1\n");
        fflush(stdout);
        
        // Test setting current process to a valid memory address
        printf("DEBUG: About to allocate memory for dummy process 1\n");
        fflush(stdout);
        void* dummy_process = malloc(64);
        if (dummy_process == NULL) {
            printf("ERROR: Failed to allocate memory for dummy process 1\n");
            scheduler_state_destroy(scheduler_state);
            return;
        }
        printf("DEBUG: Memory allocated for dummy process 1 at %p\n", dummy_process);
        fflush(stdout);
        
        printf("DEBUG: About to call scheduler_set_current_process_with_state() for test 1\n");
        fflush(stdout);
        scheduler_set_current_process_with_state(scheduler_state, 0, dummy_process);
        printf("DEBUG: scheduler_set_current_process_with_state() completed for test 1\n");
        fflush(stdout);
        
        // Verify the process was set correctly
        printf("DEBUG: About to call scheduler_get_current_process_with_state() to verify test 1\n");
        fflush(stdout);
        void* process = scheduler_get_current_process_with_state(scheduler_state, 0);
        printf("DEBUG: scheduler_get_current_process_with_state() returned process=%p, expected=%p\n", process, dummy_process);
        fflush(stdout);
        test_assert_equal((uint64_t)dummy_process, (uint64_t)process, "scheduler_set_get_current_process");
        
        // Clean up for this test
        free(dummy_process);
        scheduler_state_destroy(scheduler_state);
        printf("DEBUG: Test 1 cleanup completed\n");
        fflush(stdout);
    }
    
    // Test 2: Setting current process to NULL
    {
        // Create single-core scheduler state for this test
        printf("DEBUG: About to call scheduler_state_init() for test 2\n");
        fflush(stdout);
        void* scheduler_state = scheduler_state_init(1);
        if (scheduler_state == NULL) {
            printf("ERROR: Failed to create scheduler state for test 2\n");
            return;
        }
        printf("DEBUG: scheduler_state_init() completed successfully for test 2\n");
        fflush(stdout);
        
        // Initialize scheduler for core 0
        printf("DEBUG: About to call scheduler_init() for core 0 in test 2\n");
        fflush(stdout);
        scheduler_init(scheduler_state, 0);
        printf("DEBUG: scheduler_init() for core 0 completed in test 2\n");
        fflush(stdout);
        
        // Test setting current process to NULL
        printf("DEBUG: About to call scheduler_set_current_process_with_state() for test 2\n");
        fflush(stdout);
        scheduler_set_current_process_with_state(scheduler_state, 0, NULL);
        printf("DEBUG: scheduler_set_current_process_with_state() completed for test 2\n");
        fflush(stdout);
        
        // Verify the process was set to NULL
        printf("DEBUG: About to call scheduler_get_current_process_with_state() to verify test 2\n");
        fflush(stdout);
        void* process = scheduler_get_current_process_with_state(scheduler_state, 0);
        printf("DEBUG: scheduler_get_current_process_with_state() returned process=%p, expected=NULL\n", process);
        fflush(stdout);
        test_assert_zero((uint64_t)process, "scheduler_set_get_current_process_null");
        
        // Clean up for this test
        scheduler_state_destroy(scheduler_state);
        printf("DEBUG: Test 2 cleanup completed\n");
        fflush(stdout);
    }
    
    // Test 3: Setting current process to another valid memory address
    {
        // Create single-core scheduler state for this test
        printf("DEBUG: About to call scheduler_state_init() for test 3\n");
        fflush(stdout);
        void* scheduler_state = scheduler_state_init(1);
        if (scheduler_state == NULL) {
            printf("ERROR: Failed to create scheduler state for test 3\n");
            return;
        }
        printf("DEBUG: scheduler_state_init() completed successfully for test 3\n");
        fflush(stdout);
        
        // Initialize scheduler for core 0
        printf("DEBUG: About to call scheduler_init() for core 0 in test 3\n");
        fflush(stdout);
        scheduler_init(scheduler_state, 0);
        printf("DEBUG: scheduler_init() for core 0 completed in test 3\n");
        fflush(stdout);
        
        // Test setting current process to another valid memory address
        printf("DEBUG: About to allocate memory for dummy process 3\n");
        fflush(stdout);
        void* dummy_process2 = malloc(64);
        if (dummy_process2 == NULL) {
            printf("ERROR: Failed to allocate memory for dummy process 3\n");
            scheduler_state_destroy(scheduler_state);
            return;
        }
        printf("DEBUG: Memory allocated for dummy process 3 at %p\n", dummy_process2);
        fflush(stdout);
        
        printf("DEBUG: About to call scheduler_set_current_process_with_state() for test 3\n");
        fflush(stdout);
        scheduler_set_current_process_with_state(scheduler_state, 0, dummy_process2);
        printf("DEBUG: scheduler_set_current_process_with_state() completed for test 3\n");
        fflush(stdout);
        
        // Verify the process was set correctly
        printf("DEBUG: About to call scheduler_get_current_process_with_state() to verify test 3\n");
        fflush(stdout);
        void* process = scheduler_get_current_process_with_state(scheduler_state, 0);
        printf("DEBUG: scheduler_get_current_process_with_state() returned process=%p, expected=%p\n", process, dummy_process2);
        fflush(stdout);
        test_assert_equal((uint64_t)dummy_process2, (uint64_t)process, "scheduler_set_get_current_process_2");
        
        // Clean up for this test
        free(dummy_process2);
        scheduler_state_destroy(scheduler_state);
        printf("DEBUG: Test 3 cleanup completed\n");
        fflush(stdout);
    }
    
    // Test 4: Multiple cores with different processes
    {
        // Create multi-core scheduler state for this test (2 cores)
        printf("DEBUG: About to call scheduler_state_init() for test 4\n");
        fflush(stdout);
        void* scheduler_state = scheduler_state_init(2);
        if (scheduler_state == NULL) {
            printf("ERROR: Failed to create scheduler state for test 4\n");
            return;
        }
        printf("DEBUG: scheduler_state_init() completed successfully for test 4\n");
        fflush(stdout);
        
        // Initialize scheduler for core 0
        printf("DEBUG: About to call scheduler_init() for core 0 in test 4\n");
        fflush(stdout);
        scheduler_init(scheduler_state, 0);
        printf("DEBUG: scheduler_init() for core 0 completed in test 4\n");
        fflush(stdout);
        
        // Initialize scheduler for core 1
        printf("DEBUG: About to call scheduler_init() for core 1 in test 4\n");
        fflush(stdout);
        scheduler_init(scheduler_state, 1);
        printf("DEBUG: scheduler_init() for core 1 completed in test 4\n");
        fflush(stdout);
        
        // Allocate memory for dummy processes
        printf("DEBUG: About to allocate memory for dummy processes in test 4\n");
        fflush(stdout);
        void* dummy_process2 = malloc(64);
        void* dummy_process3 = malloc(64);
        if (dummy_process2 == NULL || dummy_process3 == NULL) {
            printf("ERROR: Failed to allocate memory for dummy processes in test 4\n");
            if (dummy_process2) free(dummy_process2);
            if (dummy_process3) free(dummy_process3);
            scheduler_state_destroy(scheduler_state);
            return;
        }
        printf("DEBUG: Memory allocated for dummy processes in test 4: %p, %p\n", dummy_process2, dummy_process3);
        fflush(stdout);
        
        // Set processes for both cores
        printf("DEBUG: About to call scheduler_set_current_process_with_state() for core 0 in test 4\n");
        fflush(stdout);
        scheduler_set_current_process_with_state(scheduler_state, 0, dummy_process2);
        printf("DEBUG: scheduler_set_current_process_with_state() completed for core 0 in test 4\n");
        fflush(stdout);
        
        printf("DEBUG: About to call scheduler_set_current_process_with_state() for core 1 in test 4\n");
        fflush(stdout);
        scheduler_set_current_process_with_state(scheduler_state, 1, dummy_process3);
        printf("DEBUG: scheduler_set_current_process_with_state() completed for core 1 in test 4\n");
        fflush(stdout);
        
        // Verify core 1 has its process
        printf("DEBUG: About to call scheduler_get_current_process_with_state() for core 1 in test 4\n");
        fflush(stdout);
        void* process = scheduler_get_current_process_with_state(scheduler_state, 1);
        printf("DEBUG: scheduler_get_current_process_with_state() returned process=%p, expected=%p for core 1\n", process, dummy_process3);
        fflush(stdout);
        test_assert_equal((uint64_t)dummy_process3, (uint64_t)process, "scheduler_set_get_current_process_core1");
        
        // Verify core 0 still has its process
        printf("DEBUG: About to call scheduler_get_current_process_with_state() for core 0 in test 4\n");
        fflush(stdout);
        process = scheduler_get_current_process_with_state(scheduler_state, 0);
        printf("DEBUG: scheduler_get_current_process_with_state() returned process=%p, expected=%p for core 0\n", process, dummy_process2);
        fflush(stdout);
        if ((uint64_t)process == (uint64_t)dummy_process2) {
            printf("✓ Core 0 process isolation test passed\n");
        } else {
            printf("✗ Core 0 process isolation test failed: got %llu, expected %llu\n", (uint64_t)process, (uint64_t)dummy_process2);
        }
        
        // Clean up for this test
        free(dummy_process2);
        free(dummy_process3);
        scheduler_state_destroy(scheduler_state);
        printf("DEBUG: Test 4 cleanup completed\n");
        fflush(stdout);
    }
}

// ------------------------------------------------------------
// test_scheduler_process_cross_core_isolation — Test cross-core isolation
// ------------------------------------------------------------
void test_scheduler_process_cross_core_isolation() {
    printf("DEBUG: test_scheduler_process_cross_core_isolation() starting\n");
    fflush(stdout);
    
    // Test 1: Basic cross-core isolation with 4 cores
    {
        printf("DEBUG: Starting cross-core isolation test 1\n");
        fflush(stdout);
        
        // Create multi-core scheduler state for this test (4 cores)
        void* scheduler_state = scheduler_state_init(4);
        if (scheduler_state == NULL) {
            printf("ERROR: Failed to create scheduler state for cross-core test 1\n");
            return;
        }
        printf("DEBUG: scheduler_state_init() completed for cross-core test 1\n");
        fflush(stdout);
        
        // Initialize schedulers for multiple cores
        printf("DEBUG: Initializing schedulers for cores 0-3 in test 1\n");
        fflush(stdout);
        scheduler_init(scheduler_state, 0);
        scheduler_init(scheduler_state, 1);
        scheduler_init(scheduler_state, 2);
        scheduler_init(scheduler_state, 3);
        printf("DEBUG: Scheduler initialization completed for test 1\n");
        fflush(stdout);
        
        // Allocate valid memory for different processes for each core
        printf("DEBUG: Allocating memory for processes in test 1\n");
        fflush(stdout);
        void* process0 = malloc(64);
        void* process1 = malloc(64);
        void* process2 = malloc(64);
        void* process3 = malloc(64);
        
        if (process0 == NULL || process1 == NULL || process2 == NULL || process3 == NULL) {
            printf("ERROR: Failed to allocate memory for cross-core test processes\n");
            if (process0) free(process0);
            if (process1) free(process1);
            if (process2) free(process2);
            if (process3) free(process3);
            scheduler_state_destroy(scheduler_state);
            return;
        }
        printf("DEBUG: Memory allocated for processes: %p, %p, %p, %p\n", process0, process1, process2, process3);
        fflush(stdout);
        
        // Set processes for each core
        printf("DEBUG: Setting processes for cores 0-3 in test 1\n");
        fflush(stdout);
        scheduler_set_current_process_with_state(scheduler_state, 0, process0);
        scheduler_set_current_process_with_state(scheduler_state, 1, process1);
        scheduler_set_current_process_with_state(scheduler_state, 2, process2);
        scheduler_set_current_process_with_state(scheduler_state, 3, process3);
        printf("DEBUG: Process setting completed for test 1\n");
        fflush(stdout);
        
        // Verify each core has its own process
        printf("DEBUG: Verifying core isolation in test 1\n");
        fflush(stdout);
        void* retrieved_process = scheduler_get_current_process_with_state(scheduler_state, 0);
        test_assert_equal((uint64_t)process0, (uint64_t)retrieved_process, "scheduler_cross_core_isolation_core0");
        
        retrieved_process = scheduler_get_current_process_with_state(scheduler_state, 1);
        test_assert_equal((uint64_t)process1, (uint64_t)retrieved_process, "scheduler_cross_core_isolation_core1");
        
        retrieved_process = scheduler_get_current_process_with_state(scheduler_state, 2);
        test_assert_equal((uint64_t)process2, (uint64_t)retrieved_process, "scheduler_cross_core_isolation_core2");
        
        retrieved_process = scheduler_get_current_process_with_state(scheduler_state, 3);
        test_assert_equal((uint64_t)process3, (uint64_t)retrieved_process, "scheduler_cross_core_isolation_core3");
        
        // Clean up for this test
        free(process0);
        free(process1);
        free(process2);
        free(process3);
        scheduler_state_destroy(scheduler_state);
        printf("DEBUG: Cross-core test 1 cleanup completed\n");
        fflush(stdout);
    }
    
    // Test 2: Changing one core doesn't affect others
    {
        printf("DEBUG: Starting cross-core isolation test 2\n");
        fflush(stdout);
        
        // Create multi-core scheduler state for this test (4 cores)
        void* scheduler_state = scheduler_state_init(4);
        if (scheduler_state == NULL) {
            printf("ERROR: Failed to create scheduler state for cross-core test 2\n");
            return;
        }
        printf("DEBUG: scheduler_state_init() completed for cross-core test 2\n");
        fflush(stdout);
        
        // Initialize schedulers for multiple cores
        printf("DEBUG: Initializing schedulers for cores 0-3 in test 2\n");
        fflush(stdout);
        scheduler_init(scheduler_state, 0);
        scheduler_init(scheduler_state, 1);
        scheduler_init(scheduler_state, 2);
        scheduler_init(scheduler_state, 3);
        printf("DEBUG: Scheduler initialization completed for test 2\n");
        fflush(stdout);
        
        // Allocate memory for initial processes
        printf("DEBUG: Allocating memory for initial processes in test 2\n");
        fflush(stdout);
        void* process0 = malloc(64);
        void* process1 = malloc(64);
        void* process2 = malloc(64);
        void* process3 = malloc(64);
        
        if (process0 == NULL || process1 == NULL || process2 == NULL || process3 == NULL) {
            printf("ERROR: Failed to allocate memory for initial processes in test 2\n");
            if (process0) free(process0);
            if (process1) free(process1);
            if (process2) free(process2);
            if (process3) free(process3);
            scheduler_state_destroy(scheduler_state);
            return;
        }
        printf("DEBUG: Initial processes allocated: %p, %p, %p, %p\n", process0, process1, process2, process3);
        fflush(stdout);
        
        // Set initial processes
        printf("DEBUG: Setting initial processes for cores 0-3 in test 2\n");
        fflush(stdout);
        scheduler_set_current_process_with_state(scheduler_state, 0, process0);
        scheduler_set_current_process_with_state(scheduler_state, 1, process1);
        scheduler_set_current_process_with_state(scheduler_state, 2, process2);
        scheduler_set_current_process_with_state(scheduler_state, 3, process3);
        printf("DEBUG: Initial process setting completed for test 2\n");
        fflush(stdout);
        
        // Allocate memory for new process for core 1
        printf("DEBUG: Allocating memory for new process 1 in test 2\n");
        fflush(stdout);
        void* new_process1 = malloc(64);
        if (new_process1 == NULL) {
            printf("ERROR: Failed to allocate memory for new process 1 in test 2\n");
            free(process0);
            free(process1);
            free(process2);
            free(process3);
            scheduler_state_destroy(scheduler_state);
            return;
        }
        printf("DEBUG: New process 1 allocated at %p\n", new_process1);
        fflush(stdout);
        
        // Change process for core 1
        printf("DEBUG: Changing process for core 1 in test 2\n");
        fflush(stdout);
        scheduler_set_current_process_with_state(scheduler_state, 1, new_process1);
        printf("DEBUG: Process change completed for core 1 in test 2\n");
        fflush(stdout);
        
        // Verify core 1 changed
        printf("DEBUG: Verifying core 1 changed in test 2\n");
        fflush(stdout);
        void* retrieved_process = scheduler_get_current_process_with_state(scheduler_state, 1);
        test_assert_equal((uint64_t)new_process1, (uint64_t)retrieved_process, "scheduler_cross_core_isolation_core1_changed");
        
        // Verify other cores unchanged
        printf("DEBUG: Verifying other cores unchanged in test 2\n");
        fflush(stdout);
        retrieved_process = scheduler_get_current_process_with_state(scheduler_state, 0);
        test_assert_equal((uint64_t)process0, (uint64_t)retrieved_process, "scheduler_cross_core_isolation_core0_unchanged");
        
        retrieved_process = scheduler_get_current_process_with_state(scheduler_state, 2);
        test_assert_equal((uint64_t)process2, (uint64_t)retrieved_process, "scheduler_cross_core_isolation_core2_unchanged");
        
        retrieved_process = scheduler_get_current_process_with_state(scheduler_state, 3);
        test_assert_equal((uint64_t)process3, (uint64_t)retrieved_process, "scheduler_cross_core_isolation_core3_unchanged");
        
        // Clean up for this test
        free(process0);
        free(process1);
        free(process2);
        free(process3);
        free(new_process1);
        scheduler_state_destroy(scheduler_state);
        printf("DEBUG: Cross-core test 2 cleanup completed\n");
        fflush(stdout);
    }
    
    printf("DEBUG: test_scheduler_process_cross_core_isolation() completed\n");
    fflush(stdout);
}
