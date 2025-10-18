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
// test_process_state_management.c — Test process state management (Task 2.4)
// ------------------------------------------------------------

#include <stdio.h>
#include <stdint.h>
#include <string.h>

// External assembly functions
extern uint64_t process_get_state(void* process);
extern void process_set_state(void* process, uint64_t state);
extern uint64_t process_transition_to_ready(void* process);
extern uint64_t process_transition_to_running(void* process);
extern uint64_t process_transition_to_waiting(void* process);
extern uint64_t process_transition_to_suspended(void* process);
extern uint64_t process_transition_to_terminated(void* process);
extern uint64_t process_is_runnable(void* process);

// External constants
extern const uint64_t PROCESS_STATE_CREATED;
extern const uint64_t PROCESS_STATE_READY;
extern const uint64_t PROCESS_STATE_RUNNING;
extern const uint64_t PROCESS_STATE_WAITING;
extern const uint64_t PROCESS_STATE_SUSPENDED;
extern const uint64_t PROCESS_STATE_TERMINATED;

// External test framework functions
extern void test_assert_equal(uint64_t expected, uint64_t actual, const char* test_name);
extern void test_assert_not_equal(uint64_t value1, uint64_t value2, const char* test_name);
extern void test_assert_true(uint64_t value, const char* test_name);
extern void test_assert_false(uint64_t value, const char* test_name);

// Mock process structure for testing - matches real PCB layout
typedef struct {
    void* next;          // Offset 0: Next pointer in queue (8 bytes)
    void* prev;          // Offset 8: Previous pointer in queue (8 bytes)
    uint64_t pid;        // Offset 16: Process ID (8 bytes)
    uint64_t scheduler_id; // Offset 24: Scheduler ID (8 bytes)
    uint64_t state;      // Offset 32: Process state (8 bytes)
    uint64_t priority;   // Offset 40: Priority level (8 bytes)
    uint64_t reduction_count; // Offset 48: Reduction counter (8 bytes)
    uint64_t registers[31]; // Offset 56: x0-x30 register save area (31 * 8 = 248 bytes)
    uint64_t sp;         // Offset 304: Stack pointer (8 bytes)
    uint64_t lr;         // Offset 312: Link register (8 bytes)
    uint64_t pc;         // Offset 320: Program counter (8 bytes)
    uint64_t pstate;     // Offset 328: Processor state (8 bytes)
    uint64_t stack_base; // Offset 336: Stack base address (8 bytes)
    uint64_t stack_size; // Offset 344: Stack size (8 bytes)
    uint64_t heap_base;  // Offset 352: Heap base address (8 bytes)
    uint64_t heap_size;  // Offset 360: Heap size (8 bytes)
    void* message_queue; // Offset 368: Message queue pointer (8 bytes)
    uint64_t last_scheduled; // Offset 376: Last scheduled timestamp (8 bytes)
    uint64_t affinity_mask; // Offset 384: CPU affinity mask (8 bytes)
    uint64_t migration_count; // Offset 392: Migration count (8 bytes)
    // Total size: 400 bytes (matches PCB_SIZE)
} mock_process_t;

// ------------------------------------------------------------
// test_process_state_get_set — Test basic state get/set
// ------------------------------------------------------------
void test_process_state_get_set(void) {
    printf("\n--- Testing Process State Get/Set (Task 2.4) ---\n");
    
    mock_process_t process;
    memset(&process, 0, sizeof(process));
    
    // Test initial state
    process.state = PROCESS_STATE_CREATED;
    uint64_t state = process_get_state(&process);
    test_assert_equal(PROCESS_STATE_CREATED, state, "get_state_initial");
    
    // Test setting different states
    process_set_state(&process, PROCESS_STATE_READY);
    state = process_get_state(&process);
    test_assert_equal(PROCESS_STATE_READY, state, "set_state_ready");
    
    process_set_state(&process, PROCESS_STATE_RUNNING);
    state = process_get_state(&process);
    test_assert_equal(PROCESS_STATE_RUNNING, state, "set_state_running");
    
    process_set_state(&process, PROCESS_STATE_WAITING);
    state = process_get_state(&process);
    test_assert_equal(PROCESS_STATE_WAITING, state, "set_state_waiting");
    
    process_set_state(&process, PROCESS_STATE_SUSPENDED);
    state = process_get_state(&process);
    test_assert_equal(PROCESS_STATE_SUSPENDED, state, "set_state_suspended");
    
    process_set_state(&process, PROCESS_STATE_TERMINATED);
    state = process_get_state(&process);
    test_assert_equal(PROCESS_STATE_TERMINATED, state, "set_state_terminated");
    
    // Test NULL pointer handling
    state = process_get_state(NULL);
    test_assert_equal(PROCESS_STATE_TERMINATED, state, "get_state_null");
    
    printf("✓ Process state get/set tests passed\n");
}

// ------------------------------------------------------------
// test_process_state_transitions — Test state transition validation
// ------------------------------------------------------------
void test_process_state_transitions(void) {
    printf("\n--- Testing Process State Transitions (Task 2.4) ---\n");
    
    mock_process_t process;
    memset(&process, 0, sizeof(process));
    
    // Test CREATED -> READY transition (valid)
    process.state = PROCESS_STATE_CREATED;
    uint64_t result = process_transition_to_ready(&process);
    test_assert_equal(1, result, "transition_created_to_ready_success");
    test_assert_equal(PROCESS_STATE_READY, process.state, "transition_created_to_ready_state");
    
    // Test READY -> RUNNING transition (valid)
    result = process_transition_to_running(&process);
    test_assert_equal(1, result, "transition_ready_to_running_success");
    test_assert_equal(PROCESS_STATE_RUNNING, process.state, "transition_ready_to_running_state");
    
    // Test RUNNING -> WAITING transition (valid)
    result = process_transition_to_waiting(&process);
    test_assert_equal(1, result, "transition_running_to_waiting_success");
    test_assert_equal(PROCESS_STATE_WAITING, process.state, "transition_running_to_waiting_state");
    
    // Test WAITING -> READY transition (valid)
    result = process_transition_to_ready(&process);
    test_assert_equal(1, result, "transition_waiting_to_ready_success");
    test_assert_equal(PROCESS_STATE_READY, process.state, "transition_waiting_to_ready_state");
    
    // Test READY -> SUSPENDED transition (valid)
    result = process_transition_to_suspended(&process);
    test_assert_equal(1, result, "transition_ready_to_suspended_success");
    test_assert_equal(PROCESS_STATE_SUSPENDED, process.state, "transition_ready_to_suspended_state");
    
    // Test SUSPENDED -> READY transition (valid)
    result = process_transition_to_ready(&process);
    test_assert_equal(1, result, "transition_suspended_to_ready_success");
    test_assert_equal(PROCESS_STATE_READY, process.state, "transition_suspended_to_ready_state");
    
    // Test any state -> TERMINATED transition (valid)
    result = process_transition_to_terminated(&process);
    test_assert_equal(1, result, "transition_to_terminated_success");
    test_assert_equal(PROCESS_STATE_TERMINATED, process.state, "transition_to_terminated_state");
    
    printf("✓ Process state transition tests passed\n");
}

// ------------------------------------------------------------
// test_invalid_state_transitions — Test invalid state transitions
// ------------------------------------------------------------
void test_invalid_state_transitions(void) {
    printf("\n--- Testing Invalid State Transitions (Task 2.4) ---\n");
    
    mock_process_t process;
    memset(&process, 0, sizeof(process));
    
    // Test CREATED -> RUNNING transition (invalid)
    process.state = PROCESS_STATE_CREATED;
    uint64_t result = process_transition_to_running(&process);
    test_assert_equal(0, result, "invalid_created_to_running");
    test_assert_equal(PROCESS_STATE_CREATED, process.state, "invalid_created_to_running_state_unchanged");
    
    // Test CREATED -> WAITING transition (invalid)
    result = process_transition_to_waiting(&process);
    test_assert_equal(0, result, "invalid_created_to_waiting");
    test_assert_equal(PROCESS_STATE_CREATED, process.state, "invalid_created_to_waiting_state_unchanged");
    
    // Test READY -> WAITING transition (invalid)
    process.state = PROCESS_STATE_READY;
    result = process_transition_to_waiting(&process);
    test_assert_equal(0, result, "invalid_ready_to_waiting");
    test_assert_equal(PROCESS_STATE_READY, process.state, "invalid_ready_to_waiting_state_unchanged");
    
    // Test WAITING -> RUNNING transition (invalid)
    process.state = PROCESS_STATE_WAITING;
    result = process_transition_to_running(&process);
    test_assert_equal(0, result, "invalid_waiting_to_running");
    test_assert_equal(PROCESS_STATE_WAITING, process.state, "invalid_waiting_to_running_state_unchanged");
    
    // Test TERMINATED -> READY transition (invalid)
    process.state = PROCESS_STATE_TERMINATED;
    result = process_transition_to_ready(&process);
    test_assert_equal(0, result, "invalid_terminated_to_ready");
    test_assert_equal(PROCESS_STATE_TERMINATED, process.state, "invalid_terminated_to_ready_state_unchanged");
    
    // Test NULL pointer transitions
    result = process_transition_to_ready(NULL);
    test_assert_equal(0, result, "null_transition_to_ready");
    
    result = process_transition_to_running(NULL);
    test_assert_equal(0, result, "null_transition_to_running");
    
    result = process_transition_to_waiting(NULL);
    test_assert_equal(0, result, "null_transition_to_waiting");
    
    result = process_transition_to_suspended(NULL);
    test_assert_equal(0, result, "null_transition_to_suspended");
    
    result = process_transition_to_terminated(NULL);
    test_assert_equal(0, result, "null_transition_to_terminated");
    
    printf("✓ Invalid state transition tests passed\n");
}

// ------------------------------------------------------------
// test_process_runnable_check — Test process runnable check
// ------------------------------------------------------------
void test_process_runnable_check(void) {
    printf("\n--- Testing Process Runnable Check (Task 2.4) ---\n");
    
    mock_process_t process;
    memset(&process, 0, sizeof(process));
    
    // Test READY state is runnable
    process.state = PROCESS_STATE_READY;
    uint64_t runnable = process_is_runnable(&process);
    test_assert_equal(1, runnable, "ready_is_runnable");
    
    // Test CREATED state is not runnable
    process.state = PROCESS_STATE_CREATED;
    runnable = process_is_runnable(&process);
    test_assert_equal(0, runnable, "created_not_runnable");
    
    // Test RUNNING state is not runnable
    process.state = PROCESS_STATE_RUNNING;
    runnable = process_is_runnable(&process);
    test_assert_equal(0, runnable, "running_not_runnable");
    
    // Test WAITING state is not runnable
    process.state = PROCESS_STATE_WAITING;
    runnable = process_is_runnable(&process);
    test_assert_equal(0, runnable, "waiting_not_runnable");
    
    // Test SUSPENDED state is not runnable
    process.state = PROCESS_STATE_SUSPENDED;
    runnable = process_is_runnable(&process);
    test_assert_equal(0, runnable, "suspended_not_runnable");
    
    // Test TERMINATED state is not runnable
    process.state = PROCESS_STATE_TERMINATED;
    runnable = process_is_runnable(&process);
    test_assert_equal(0, runnable, "terminated_not_runnable");
    
    // Test NULL pointer is not runnable
    runnable = process_is_runnable(NULL);
    test_assert_equal(0, runnable, "null_not_runnable");
    
    printf("✓ Process runnable check tests passed\n");
}

// ------------------------------------------------------------
// test_process_state_management — Main test function
// ------------------------------------------------------------
void test_process_state_management(void) {
    printf("\n========================================\n");
    printf("Testing Process State Management (Task 2.4)\n");
    printf("========================================\n");
    
    test_process_state_get_set();
    test_process_state_transitions();
    test_invalid_state_transitions();
    test_process_runnable_check();
    
    printf("\n========================================\n");
    printf("✓ All Process State Management Tests Passed!\n");
    printf("========================================\n");
}

