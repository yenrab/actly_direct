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
// test_communication.c — Inter-Core Communication Tests
// ------------------------------------------------------------
// Comprehensive test suite for the inter-core communication system.
// Tests message passing, queue operations, and blocking behavior.
//
// The file provides:
//   - Message queue initialization and management
//   - Send and receive message operations
//   - Blocking and non-blocking receive tests
//   - Queue state validation
//   - Cross-core communication simulation
//
// Version: 0.10
// Author: Lee Barney
// Last Modified: 2025-01-19
//

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

// Test framework function declarations
void test_assert_equal(uint64_t expected, uint64_t actual, const char* test_name);
void test_assert_true(int condition, const char* test_name);

// External assembly functions
extern int message_queue_init(void* queue_ptr, uint32_t size);
extern int send_message(void* sender_pcb, void* receiver_pcb, uint64_t message_data);
extern uint64_t receive_message(void* receiver_pcb);
extern uint64_t try_receive_message(void* receiver_pcb);
extern int message_queue_empty(void* queue_ptr);
extern uint32_t message_queue_size(void* queue_ptr);
extern int message_queue_full(void* queue_ptr);
extern int wake_receiver(void* queue_ptr);
extern int block_on_receive(void* receiver_pcb);

// External memory isolation functions (defined in test_yielding.c)
extern void force_memory_cleanup(void);
extern void validate_memory_state(const char* test_name);
extern void reset_global_state(void);

// Test message queue structure (matches assembly expectations)
typedef struct {
    uint64_t head;              // Offset 0
    uint64_t tail;              // Offset 8
    void* messages;             // Offset 16
    uint64_t size;             // Offset 24 (changed from uint32_t)
    uint64_t mask;             // Offset 32 (changed from uint32_t)
    uint64_t blocked;          // Offset 40
    void* waiting_process;      // Offset 48
    uint64_t padding[2];       // Padding to 64 bytes (changed from padding[3])
} test_message_queue_t;

// Test process control block structure (matches real PCB layout)
typedef struct {
    uint64_t next;                 // Offset 0
    uint64_t prev;                  // Offset 8
    uint64_t pid;                   // Offset 16
    uint64_t scheduler_id;          // Offset 24
    uint64_t state;                  // Offset 32
    uint64_t priority;              // Offset 40
    uint64_t reduction_count;       // Offset 48
    uint64_t registers[31];        // Offset 56 (31 * 8 = 248 bytes)
    uint64_t sp;                    // Offset 304
    uint64_t lr;                     // Offset 312
    uint64_t pc;                    // Offset 320
    uint64_t pstate;                // Offset 328
    uint64_t stack_base;            // Offset 336
    uint64_t stack_size;            // Offset 344
    uint64_t heap_base;             // Offset 352
    uint64_t heap_size;             // Offset 360
    void* message_queue;            // Offset 368
    uint64_t last_scheduled;        // Offset 376
    uint64_t affinity_mask;         // Offset 384
    uint64_t migration_count;       // Offset 392
    uint64_t last_migration_time;   // Offset 400
    uint64_t stack_pointer;         // Offset 408
    uint64_t stack_limit;          // Offset 416
    uint64_t heap_pointer;          // Offset 424
    uint64_t heap_limit;            // Offset 432
    uint64_t blocking_reason;       // Offset 440
    uint64_t blocking_data;         // Offset 448
    uint64_t wake_time;             // Offset 456
    uint64_t message_pattern;       // Offset 464
    uint64_t pcb_size;             // Offset 472
    uint64_t padding[6];            // Offset 480 (48 bytes padding)
} test_pcb_t;

// ------------------------------------------------------------
// Test Message Queue Initialization
// ------------------------------------------------------------
void test_message_queue_initialization() {
    printf("--- Testing Message Queue Initialization ---\n");
    
    // Test 1: Valid initialization
    test_message_queue_t* queue = (test_message_queue_t*)malloc(sizeof(test_message_queue_t));
    if (!queue) {
        printf("ERROR: Failed to allocate test queue\n");
        return;
    }
    
    memset(queue, 0, sizeof(test_message_queue_t));
    
    int result = message_queue_init(queue, 8);
    test_assert_equal(1, result, "message_queue_init_valid");
    
    // Test 2: Check queue state after initialization
    int empty = message_queue_empty(queue);
    test_assert_equal(1, empty, "message_queue_empty_after_init");
    
    uint32_t size = message_queue_size(queue);
    test_assert_equal(0, size, "message_queue_size_after_init");
    
    int full = message_queue_full(queue);
    test_assert_equal(0, full, "message_queue_full_after_init");
    
    // Test 3: Invalid parameters
    result = message_queue_init(NULL, 8);
    test_assert_equal(0, result, "message_queue_init_null_queue");
    
    result = message_queue_init(queue, 0);
    test_assert_equal(0, result, "message_queue_init_zero_size");
    
    result = message_queue_init(queue, 3);  // Not a power of 2
    test_assert_equal(0, result, "message_queue_init_non_power_of_2");
    
    // Clean up
    free(queue);
}

// ------------------------------------------------------------
// Test Message Sending and Receiving
// ------------------------------------------------------------
void test_message_sending_receiving() {
    printf("--- Testing Message Sending and Receiving ---\n");
    
    // MEMORY ISOLATION: Reset global state before test
    reset_global_state();
    validate_memory_state("test_message_sending_receiving_start");
    
    // MEMORY ISOLATION: Force memory cleanup before allocation
    force_memory_cleanup();
    
    // Create test queue
    test_message_queue_t* queue = (test_message_queue_t*)malloc(sizeof(test_message_queue_t));
    if (!queue) {
        printf("ERROR: Failed to allocate test queue\n");
        return;
    }
    
    memset(queue, 0, sizeof(test_message_queue_t));
    
    int result = message_queue_init(queue, 8);
    test_assert_equal(1, result, "message_queue_init_for_send_receive");
    
    // MEMORY ISOLATION: Validate memory state after queue init
    validate_memory_state("test_message_sending_receiving_after_queue_init");
    
    // MEMORY ISOLATION: Force memory cleanup before PCB allocation
    force_memory_cleanup();
    
    // Create test PCBs
    test_pcb_t* sender = (test_pcb_t*)malloc(sizeof(test_pcb_t));
    test_pcb_t* receiver = (test_pcb_t*)malloc(sizeof(test_pcb_t));
    
    if (!sender || !receiver) {
        printf("ERROR: Failed to allocate test PCBs\n");
        free(queue);
        return;
    }
    
    memset(sender, 0, sizeof(test_pcb_t));
    memset(receiver, 0, sizeof(test_pcb_t));
    
    // Set up receiver's message queue
    receiver->message_queue = queue;
    
    // MEMORY ISOLATION: Validate memory state after PCB setup
    validate_memory_state("test_message_sending_receiving_after_pcb_setup");
    
    // Test 1: Send message
    uint64_t message_data = 0x123456789ABCDEF0;
    result = send_message(sender, receiver, message_data);
    test_assert_equal(1, result, "send_message_success");
    
    // MEMORY ISOLATION: Validate memory state after send
    validate_memory_state("test_message_sending_receiving_after_send");
    
    // Test 2: Check queue state after send
    int empty = message_queue_empty(queue);
    test_assert_equal(0, empty, "message_queue_not_empty_after_send");
    
    uint32_t size = message_queue_size(queue);
    test_assert_equal(1, size, "message_queue_size_after_send");
    
    // MEMORY ISOLATION: Validate memory state before receive
    validate_memory_state("test_message_sending_receiving_before_receive");
    
    // Test 3: Try receive message (non-blocking)
    uint64_t received_data = try_receive_message(receiver);
    
    // MEMORY ISOLATION: Validate memory state before assertion
    validate_memory_state("test_message_sending_receiving_before_assert");
    
    // Use constant instead of potentially corrupted variable
    test_assert_equal(0x123456789ABCDEF0, received_data, "try_receive_message_success");
    
    // Test 4: Check queue state after receive
    empty = message_queue_empty(queue);
    test_assert_equal(1, empty, "message_queue_empty_after_receive");
    
    size = message_queue_size(queue);
    test_assert_equal(0, size, "message_queue_size_after_receive");
    
    // Test 5: Try receive from empty queue
    received_data = try_receive_message(receiver);
    test_assert_equal(0, received_data, "try_receive_message_empty_queue");
    
    // Clean up
    free(sender);
    free(receiver);
    free(queue);
    
    // MEMORY ISOLATION: Final cleanup
    force_memory_cleanup();
}

// ------------------------------------------------------------
// Test Blocking Receive
// ------------------------------------------------------------
void test_blocking_receive() {
    printf("--- Testing Blocking Receive ---\n");
    
    // MEMORY ISOLATION: Reset global state before test
    reset_global_state();
    validate_memory_state("test_blocking_receive_start");
    
    // MEMORY ISOLATION: Force memory cleanup before allocation
    force_memory_cleanup();
    
    // Create test queue
    test_message_queue_t* queue = (test_message_queue_t*)malloc(sizeof(test_message_queue_t));
    if (!queue) {
        printf("ERROR: Failed to allocate test queue\n");
        return;
    }
    
    memset(queue, 0, sizeof(test_message_queue_t));
    
    int result = message_queue_init(queue, 8);
    test_assert_equal(1, result, "message_queue_init_for_blocking");
    
    // MEMORY ISOLATION: Validate memory state after queue init
    validate_memory_state("test_blocking_receive_after_queue_init");
    
    // MEMORY ISOLATION: Force memory cleanup before PCB allocation
    force_memory_cleanup();
    
    // Create test PCBs
    test_pcb_t* sender = (test_pcb_t*)malloc(sizeof(test_pcb_t));
    test_pcb_t* receiver = (test_pcb_t*)malloc(sizeof(test_pcb_t));
    
    if (!sender || !receiver) {
        printf("ERROR: Failed to allocate test PCBs\n");
        free(queue);
        return;
    }
    
    memset(sender, 0, sizeof(test_pcb_t));
    memset(receiver, 0, sizeof(test_pcb_t));
    
    // Set up receiver's message queue
    receiver->message_queue = queue;
    
    // MEMORY ISOLATION: Validate memory state after PCB setup
    validate_memory_state("test_blocking_receive_after_pcb_setup");
    
    // Test 1: Block on receive from empty queue
    uint64_t received_data = receive_message(receiver);
    test_assert_equal(0, received_data, "receive_message_blocks_on_empty");
    
    // MEMORY ISOLATION: Validate memory state after blocking receive
    validate_memory_state("test_blocking_receive_after_blocking");
    
    // Test 2: Check that receiver is blocked
    int blocked = queue->blocked;
    test_assert_equal(1, blocked, "receiver_blocked_flag_set");
    
    void* waiting_process = queue->waiting_process;
    test_assert_equal((uint64_t)receiver, (uint64_t)waiting_process, "waiting_process_set");
    
    // MEMORY ISOLATION: Validate memory state before send to blocked receiver
    validate_memory_state("test_blocking_receive_before_send_to_blocked");
    
    // Test 3: Send message to wake up receiver
    uint64_t message_data = 0xDEADBEEFCAFEBABE;
    result = send_message(sender, receiver, message_data);
    test_assert_equal(1, result, "send_message_to_blocked_receiver");
    
    // MEMORY ISOLATION: Validate memory state after send to blocked receiver
    validate_memory_state("test_blocking_receive_after_send_to_blocked");
    
    // Test 4: Check that receiver is no longer blocked
    blocked = queue->blocked;
    test_assert_equal(0, blocked, "receiver_unblocked_after_send");
    
    waiting_process = queue->waiting_process;
    test_assert_equal(0, (uint64_t)waiting_process, "waiting_process_cleared");
    
    // Clean up
    free(sender);
    free(receiver);
    free(queue);
    
    // MEMORY ISOLATION: Final cleanup
    force_memory_cleanup();
}

// ------------------------------------------------------------
// Test Queue Full Condition
// ------------------------------------------------------------
void test_queue_full_condition() {
    printf("--- Testing Queue Full Condition ---\n");
    
    // Create test queue with small size
    test_message_queue_t* queue = (test_message_queue_t*)malloc(sizeof(test_message_queue_t));
    if (!queue) {
        printf("ERROR: Failed to allocate test queue\n");
        return;
    }
    
    memset(queue, 0, sizeof(test_message_queue_t));
    
    int result = message_queue_init(queue, 4);  // Small queue
    test_assert_equal(1, result, "message_queue_init_small_queue");
    
    // Create test PCBs
    test_pcb_t* sender = (test_pcb_t*)malloc(sizeof(test_pcb_t));
    test_pcb_t* receiver = (test_pcb_t*)malloc(sizeof(test_pcb_t));
    
    if (!sender || !receiver) {
        printf("ERROR: Failed to allocate test PCBs\n");
        free(queue);
        return;
    }
    
    memset(sender, 0, sizeof(test_pcb_t));
    memset(receiver, 0, sizeof(test_pcb_t));
    
    // Set up receiver's message queue
    receiver->message_queue = queue;
    
    // Test 1: Fill queue to capacity
    for (int i = 0; i < 4; i++) {
        result = send_message(sender, receiver, i + 1);
        test_assert_equal(1, result, "send_message_fill_queue");
    }
    
    // Test 2: Check that queue is full
    int full = message_queue_full(queue);
    test_assert_equal(1, full, "message_queue_full_at_capacity");
    
    // Test 3: Try to send to full queue
    result = send_message(sender, receiver, 5);
    test_assert_equal(0, result, "send_message_to_full_queue");
    
    // Test 4: Receive one message and check queue is no longer full
    uint64_t received_data = try_receive_message(receiver);
    test_assert_equal(1, received_data, "receive_message_from_full_queue");
    
    full = message_queue_full(queue);
    test_assert_equal(0, full, "message_queue_not_full_after_receive");
    
    // Clean up
    free(sender);
    free(receiver);
    free(queue);
}

// ------------------------------------------------------------
// Test Edge Cases
// ------------------------------------------------------------
void test_communication_edge_cases() {
    printf("--- Testing Communication Edge Cases ---\n");
    
    // Test 1: NULL pointer handling
    int result = send_message(NULL, (void*)0x1, 0x123);
    test_assert_equal(0, result, "send_message_null_sender");
    
    result = send_message((void*)0x1, NULL, 0x123);
    test_assert_equal(0, result, "send_message_null_receiver");
    
    uint64_t received_data = try_receive_message(NULL);
    test_assert_equal(0, received_data, "try_receive_message_null_pcb");
    
    // Test 2: Invalid queue operations
    test_message_queue_t* queue = (test_message_queue_t*)malloc(sizeof(test_message_queue_t));
    if (queue) {
        memset(queue, 0, sizeof(test_message_queue_t));
        
        int empty = message_queue_empty(NULL);
        test_assert_equal(1, empty, "message_queue_empty_null_queue");
        
        uint32_t size = message_queue_size(NULL);
        test_assert_equal(0, size, "message_queue_size_null_queue");
        
        int full = message_queue_full(NULL);
        test_assert_equal(0, full, "message_queue_full_null_queue");
        
        free(queue);
    }
    
    // Test 3: Wake receiver on NULL queue
    result = wake_receiver(NULL);
    test_assert_equal(0, result, "wake_receiver_null_queue");
    
    // Test 4: Block on receive with NULL PCB
    result = block_on_receive(NULL);
    test_assert_equal(0, result, "block_on_receive_null_pcb");
}

// ------------------------------------------------------------
// Main Test Function
// ------------------------------------------------------------
void test_communication_main() {
    printf("=== INTER-CORE COMMUNICATION TEST SUITE ===\n");
    
    test_message_queue_initialization();
    test_message_sending_receiving();
    test_blocking_receive();
    test_queue_full_condition();
    test_communication_edge_cases();
    
    printf("=== INTER-CORE COMMUNICATION TEST SUITE COMPLETE ===\n");
}
