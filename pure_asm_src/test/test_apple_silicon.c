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
// test_apple_silicon.c — C test suite for Apple Silicon Optimizations
// ------------------------------------------------------------
// Tests the Apple Silicon specific optimization functions implemented in apple_silicon.s.
// This includes core type detection, cluster mapping, optimal core selection,
// and cache line size detection.
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
extern int detect_apple_silicon_core_types(void* core_type_map);
extern uint64_t get_core_type_apple_silicon(uint64_t core_id);
extern uint64_t get_core_cluster_apple_silicon(uint64_t core_id);
extern int is_performance_core_apple_silicon(uint64_t core_id);
extern uint64_t get_optimal_core_apple_silicon(uint64_t process_type);
extern uint64_t get_cache_line_size_apple_silicon(void);
extern int optimize_for_apple_silicon(void);

// Apple Silicon constants
#define APPLE_SILICON_CORE_TYPE_PERFORMANCE 0
#define APPLE_SILICON_CORE_TYPE_EFFICIENCY 1
#define APPLE_SILICON_CORE_TYPE_UNKNOWN 2

// ------------------------------------------------------------
// Test Core Type Detection (Apple Silicon)
// ------------------------------------------------------------
void test_core_type_detection_apple_silicon() {
    printf("--- Testing Core Type Detection ---\n");
    
    // Test P-core detection (cores 0-7)
    uint64_t core_type = get_core_type_apple_silicon(0);
    test_assert_equal(APPLE_SILICON_CORE_TYPE_PERFORMANCE, core_type, "core_type_p_core_0");
    
    core_type = get_core_type_apple_silicon(7);
    test_assert_equal(APPLE_SILICON_CORE_TYPE_PERFORMANCE, core_type, "core_type_p_core_7");
    
    // Test E-core detection (cores 8+)
    core_type = get_core_type_apple_silicon(8);
    test_assert_equal(APPLE_SILICON_CORE_TYPE_EFFICIENCY, core_type, "core_type_e_core_8");
    
    core_type = get_core_type_apple_silicon(15);
    test_assert_equal(APPLE_SILICON_CORE_TYPE_EFFICIENCY, core_type, "core_type_e_core_15");
    
    // Test invalid core ID
    core_type = get_core_type_apple_silicon(128);
    test_assert_equal(APPLE_SILICON_CORE_TYPE_UNKNOWN, core_type, "core_type_invalid");
}

// ------------------------------------------------------------
// Test Core Cluster Detection
// ------------------------------------------------------------
void test_core_cluster_detection() {
    printf("--- Testing Core Cluster Detection ---\n");
    
    // Test P-core cluster (cores 0-7)
    uint64_t cluster = get_core_cluster_apple_silicon(0);
    test_assert_equal(0, cluster, "cluster_p_core_0");
    
    cluster = get_core_cluster_apple_silicon(7);
    test_assert_equal(0, cluster, "cluster_p_core_7");
    
    // Test E-core cluster (cores 8+)
    cluster = get_core_cluster_apple_silicon(8);
    test_assert_equal(1, cluster, "cluster_e_core_8");
    
    cluster = get_core_cluster_apple_silicon(15);
    test_assert_equal(1, cluster, "cluster_e_core_15");
    
    // Test invalid core ID
    cluster = get_core_cluster_apple_silicon(128);
    test_assert_equal(0, cluster, "cluster_invalid");
}

// ------------------------------------------------------------
// Test Performance Core Detection
// ------------------------------------------------------------
void test_performance_core_detection() {
    printf("--- Testing Performance Core Detection ---\n");
    
    // Test P-core detection (cores 0-7)
    int is_p_core = is_performance_core_apple_silicon(0);
    test_assert_equal(1, is_p_core, "is_p_core_0");
    
    is_p_core = is_performance_core_apple_silicon(7);
    test_assert_equal(1, is_p_core, "is_p_core_7");
    
    // Test E-core detection (cores 8+)
    is_p_core = is_performance_core_apple_silicon(8);
    test_assert_equal(0, is_p_core, "is_e_core_8");
    
    is_p_core = is_performance_core_apple_silicon(15);
    test_assert_equal(0, is_p_core, "is_e_core_15");
    
    // Test invalid core ID
    is_p_core = is_performance_core_apple_silicon(128);
    test_assert_equal(0, is_p_core, "is_p_core_invalid");
}

// ------------------------------------------------------------
// Test Optimal Core Selection (Apple Silicon)
// ------------------------------------------------------------
void test_optimal_core_selection_apple_silicon() {
    printf("--- Testing Optimal Core Selection ---\n");
    
    // Test CPU-intensive process (should get P-core)
    uint64_t optimal_core = get_optimal_core_apple_silicon(0);  // CPU_INTENSIVE
    test_assert_equal(0, optimal_core, "optimal_core_cpu_intensive");
    
    // Test I/O-bound process (should get E-core)
    optimal_core = get_optimal_core_apple_silicon(1);  // I/O_BOUND
    test_assert_equal(8, optimal_core, "optimal_core_io_bound");
    
    // Test mixed process (should get P-core)
    optimal_core = get_optimal_core_apple_silicon(2);  // MIXED
    test_assert_equal(0, optimal_core, "optimal_core_mixed");
    
    // Test invalid process type
    optimal_core = get_optimal_core_apple_silicon(3);  // Invalid
    test_assert_equal(0, optimal_core, "optimal_core_invalid");
}

// ------------------------------------------------------------
// Test Cache Line Size Detection
// ------------------------------------------------------------
void test_cache_line_size_detection() {
    printf("--- Testing Cache Line Size Detection ---\n");
    
    uint64_t cache_line_size = get_cache_line_size_apple_silicon();
    test_assert_equal(128, cache_line_size, "cache_line_size_128_bytes");
}

// ------------------------------------------------------------
// Test Apple Silicon Optimization
// ------------------------------------------------------------
void test_apple_silicon_optimization() {
    printf("--- Testing Apple Silicon Optimization ---\n");
    
    int result = optimize_for_apple_silicon();
    test_assert_equal(1, result, "optimize_for_apple_silicon_success");
}

// ------------------------------------------------------------
// Test Core Type Map Detection
// ------------------------------------------------------------
void test_core_type_map_detection() {
    printf("--- Testing Core Type Map Detection ---\n");
    
    // Allocate core type map
    uint8_t* core_type_map = (uint8_t*)malloc(128);  // MAX_CORES
    test_assert_true(core_type_map != NULL, "core_type_map_allocation");
    
    // Detect core types
    int result = detect_apple_silicon_core_types(core_type_map);
    test_assert_equal(1, result, "detect_core_types_success");
    
    // Verify P-cores (cores 0-7)
    for (int i = 0; i < 8; i++) {
        test_assert_equal(APPLE_SILICON_CORE_TYPE_PERFORMANCE, core_type_map[i], "core_type_map_p_core");
    }
    
    // Verify E-cores (cores 8-15)
    for (int i = 8; i < 16; i++) {
        test_assert_equal(APPLE_SILICON_CORE_TYPE_EFFICIENCY, core_type_map[i], "core_type_map_e_core");
    }
    
    // Test with NULL pointer
    result = detect_apple_silicon_core_types(NULL);
    test_assert_equal(0, result, "detect_core_types_null_pointer");
    
    free(core_type_map);
}

// ------------------------------------------------------------
// Test Apple Silicon Edge Cases
// ------------------------------------------------------------
void test_apple_silicon_edge_cases() {
    printf("--- Testing Apple Silicon Edge Cases ---\n");
    
    // Test boundary core IDs
    uint64_t core_type = get_core_type_apple_silicon(7);  // Last P-core
    test_assert_equal(APPLE_SILICON_CORE_TYPE_PERFORMANCE, core_type, "edge_case_last_p_core");
    
    core_type = get_core_type_apple_silicon(8);  // First E-core
    test_assert_equal(APPLE_SILICON_CORE_TYPE_EFFICIENCY, core_type, "edge_case_first_e_core");
    
    // Test cluster boundaries
    uint64_t cluster = get_core_cluster_apple_silicon(7);  // Last P-core cluster
    test_assert_equal(0, cluster, "edge_case_last_p_core_cluster");
    
    cluster = get_core_cluster_apple_silicon(8);  // First E-core cluster
    test_assert_equal(1, cluster, "edge_case_first_e_core_cluster");
    
    // Test performance core boundaries
    int is_p_core = is_performance_core_apple_silicon(7);  // Last P-core
    test_assert_equal(1, is_p_core, "edge_case_last_p_core_performance");
    
    is_p_core = is_performance_core_apple_silicon(8);  // First E-core
    test_assert_equal(0, is_p_core, "edge_case_first_e_core_performance");
}

// ------------------------------------------------------------
// Main Apple Silicon Test Function
// ------------------------------------------------------------
void test_apple_silicon_main() {
    printf("=== APPLE SILICON OPTIMIZATION TEST SUITE ===\n");
    
    test_core_type_detection_apple_silicon();
    test_core_cluster_detection();
    test_performance_core_detection();
    test_optimal_core_selection_apple_silicon();
    test_cache_line_size_detection();
    test_apple_silicon_optimization();
    test_core_type_map_detection();
    test_apple_silicon_edge_cases();
    
    printf("=== APPLE SILICON OPTIMIZATION TEST SUITE COMPLETE ===\n");
}
