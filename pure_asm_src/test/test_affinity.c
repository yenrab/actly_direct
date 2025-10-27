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
// test_affinity.c — CPU Affinity System Tests
// ------------------------------------------------------------
// Comprehensive test suite for the CPU affinity management system.
// Tests affinity mask operations, core type detection, and migration constraints.
//
// The file provides:
//   - Affinity mask setting and getting
//   - Core type detection (P-core vs E-core)
//   - Migration constraint checking
//   - NUMA node detection
//   - Optimal core selection
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
extern int set_process_affinity(void* pcb, uint64_t core_mask);
extern uint64_t get_process_affinity(void* pcb);
extern int check_affinity(void* pcb, uint64_t core_id);
extern int is_migration_allowed(void* pcb, uint64_t source_core, uint64_t target_core);
extern int detect_core_types(void);
extern uint32_t get_core_type(uint64_t core_id);
extern uint64_t get_core_cluster(uint64_t core_id);
extern int is_performance_core(uint64_t core_id);
extern uint64_t get_optimal_core(uint32_t process_type);
extern uint64_t get_numa_node(uint64_t core_id);

// Test process control block structure (simplified)
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
    uint64_t message_queue;         // Offset 368
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
// Test Affinity Mask Operations
// ------------------------------------------------------------
void test_affinity_mask_operations() {
    printf("--- Testing Affinity Mask Operations ---\n");
    
    // Create a test PCB
    test_pcb_t* pcb = (test_pcb_t*)malloc(sizeof(test_pcb_t));
    if (!pcb) {
        printf("ERROR: Failed to allocate test PCB\n");
        return;
    }
    
    // Initialize PCB
    memset(pcb, 0, sizeof(test_pcb_t));
    
    // Test 1: Set affinity mask
    uint64_t core_mask = 0x0F;  // Allow cores 0-3
    int result = set_process_affinity(pcb, core_mask);
    test_assert_equal(1, result, "set_process_affinity_success");
    
    // Test 2: Get affinity mask
    uint64_t retrieved_mask = get_process_affinity(pcb);
    test_assert_equal(core_mask, retrieved_mask, "get_process_affinity_correct");
    
    // Test 3: Check affinity for allowed cores
    for (int i = 0; i < 4; i++) {
        int allowed = check_affinity(pcb, i);
        test_assert_equal(1, allowed, "check_affinity_allowed_core");
    }
    
    // Test 4: Check affinity for disallowed cores
    for (int i = 4; i < 8; i++) {
        int allowed = check_affinity(pcb, i);
        test_assert_equal(0, allowed, "check_affinity_disallowed_core");
    }
    
    // Test 5: Set invalid affinity mask (all zeros)
    result = set_process_affinity(pcb, 0);
    test_assert_equal(0, result, "set_process_affinity_invalid_mask");
    
    // Test 6: Set affinity mask with invalid core IDs
    result = set_process_affinity(pcb, 0xFFFFFFFFFFFFFFFF);
    test_assert_equal(1, result, "set_process_affinity_large_mask");
    
    // Clean up
    free(pcb);
}

// ------------------------------------------------------------
// Test Core Type Detection
// ------------------------------------------------------------
void test_core_type_detection() {
    printf("--- Testing Core Type Detection ---\n");
    
    // Test 1: Detect core types
    int result = detect_core_types();
    test_assert_equal(1, result, "detect_core_types_success");
    
    // Test 2: Check P-cores (cores 0-7)
    for (int i = 0; i < 8; i++) {
        uint32_t core_type = get_core_type(i);
        test_assert_equal(0, core_type, "get_core_type_p_core");
        
        int is_p_core = is_performance_core(i);
        test_assert_equal(1, is_p_core, "is_performance_core_p_core");
    }
    
    // Test 3: Check E-cores (cores 8+)
    for (int i = 8; i < 16; i++) {
        uint32_t core_type = get_core_type(i);
        test_assert_equal(1, core_type, "get_core_type_e_core");
        
        int is_p_core = is_performance_core(i);
        test_assert_equal(0, is_p_core, "is_performance_core_e_core");
    }
    
    // Test 4: Check cluster assignment
    for (int i = 0; i < 8; i++) {
        uint64_t cluster = get_core_cluster(i);
        test_assert_equal(0, cluster, "get_core_cluster_p_cores");
    }
    
    for (int i = 8; i < 16; i++) {
        uint64_t cluster = get_core_cluster(i);
        test_assert_equal(1, cluster, "get_core_cluster_e_cores");
    }
    
    // Test 5: Invalid core ID
    uint32_t core_type = get_core_type(128);
    test_assert_equal(1, core_type, "get_core_type_invalid_core");
}

// ------------------------------------------------------------
// Test Migration Constraints
// ------------------------------------------------------------
void test_migration_constraints() {
    printf("--- Testing Migration Constraints ---\n");
    
    // Create a test PCB
    test_pcb_t* pcb = (test_pcb_t*)malloc(sizeof(test_pcb_t));
    if (!pcb) {
        printf("ERROR: Failed to allocate test PCB\n");
        return;
    }
    
    // Initialize PCB
    memset(pcb, 0, sizeof(test_pcb_t));
    pcb->affinity_mask = 0x0F;  // Allow cores 0-3
    pcb->migration_count = 0;
    pcb->last_migration_time = 0;
    
    // Test 1: Migration to allowed core
    int allowed = is_migration_allowed(pcb, 0, 1);
    test_assert_equal(1, allowed, "is_migration_allowed_valid");
    
    // Test 2: Migration to disallowed core
    allowed = is_migration_allowed(pcb, 0, 8);
    test_assert_equal(0, allowed, "is_migration_allowed_disallowed");
    
    // Test 3: Migration to same core
    allowed = is_migration_allowed(pcb, 1, 1);
    test_assert_equal(1, allowed, "is_migration_allowed_same_core");
    
    // Test 4: Migration with high migration count
    pcb->migration_count = 11;   // Exceed limit (10 + 1)
    allowed = is_migration_allowed(pcb, 0, 1);
    test_assert_equal(0, allowed, "is_migration_allowed_high_count");
    
    // Test 5: Invalid core IDs
    allowed = is_migration_allowed(pcb, 128, 1);
    test_assert_equal(0, allowed, "is_migration_allowed_invalid_source");
    
    allowed = is_migration_allowed(pcb, 0, 128);
    test_assert_equal(0, allowed, "is_migration_allowed_invalid_target");
    
    // Clean up
    free(pcb);
}

// ------------------------------------------------------------
// Test Optimal Core Selection
// ------------------------------------------------------------
void test_optimal_core_selection() {
    printf("--- Testing Optimal Core Selection ---\n");
    
    // Test 1: CPU-intensive process (should prefer P-cores)
    uint64_t optimal_core = get_optimal_core(0);
    test_assert_true(optimal_core < 8, "get_optimal_core_cpu_intensive");
    
    // Test 2: I/O-bound process (should prefer E-cores)
    optimal_core = get_optimal_core(1);
    test_assert_true(optimal_core >= 8, "get_optimal_core_io_bound");
    
    // Test 3: Invalid process type
    optimal_core = get_optimal_core(2);
    test_assert_equal(0, optimal_core, "get_optimal_core_invalid_type");
}

// ------------------------------------------------------------
// Test NUMA Node Detection
// ------------------------------------------------------------
void test_numa_node_detection() {
    printf("--- Testing NUMA Node Detection ---\n");
    
    // Test NUMA node detection for various cores
    for (int i = 0; i < 16; i++) {
        uint64_t numa_node = get_numa_node(i);
        test_assert_equal(0, numa_node, "get_numa_node_single_node");
    }
    
    // Test invalid core ID
    uint64_t numa_node = get_numa_node(128);
    test_assert_equal(0, numa_node, "get_numa_node_invalid_core");
}

// ------------------------------------------------------------
// Test Edge Cases
// ------------------------------------------------------------
void test_affinity_edge_cases() {
    printf("--- Testing Affinity Edge Cases ---\n");
    
    // Test 1: NULL PCB pointer
    int result = set_process_affinity(NULL, 0x0F);
    test_assert_equal(0, result, "set_process_affinity_null_pcb");
    
    uint64_t mask = get_process_affinity(NULL);
    test_assert_equal(0, mask, "get_process_affinity_null_pcb");
    
    int allowed = check_affinity(NULL, 0);
    test_assert_equal(0, allowed, "check_affinity_null_pcb");
    
    // Test 2: Invalid core IDs
    allowed = check_affinity((void*)0x1, 128);
    test_assert_equal(0, allowed, "check_affinity_invalid_core");
    
    // Test 3: Single core affinity
    test_pcb_t* pcb = (test_pcb_t*)malloc(sizeof(test_pcb_t));
    if (pcb) {
        memset(pcb, 0, sizeof(test_pcb_t));
        
        result = set_process_affinity(pcb, 0x01);  // Only core 0
        test_assert_equal(1, result, "set_process_affinity_single_core");
        
        allowed = check_affinity(pcb, 0);
        test_assert_equal(1, allowed, "check_affinity_single_core_allowed");
        
        allowed = check_affinity(pcb, 1);
        test_assert_equal(0, allowed, "check_affinity_single_core_disallowed");
        
        free(pcb);
    }
}

// ------------------------------------------------------------
// Main Test Function
// ------------------------------------------------------------
void test_affinity_main() {
    printf("=== CPU AFFINITY SYSTEM TEST SUITE ===\n");
    
    test_affinity_mask_operations();
    test_core_type_detection();
    test_migration_constraints();
    test_optimal_core_selection();
    test_numa_node_detection();
    test_affinity_edge_cases();
    
    printf("=== CPU AFFINITY SYSTEM TEST SUITE COMPLETE ===\n");
}
