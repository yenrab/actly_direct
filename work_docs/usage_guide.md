# Actly Direct Usage Guide

## Overview

This guide provides step-by-step instructions for building, testing, and using the Actly Direct BEAM implementation. The system is designed for macOS userspace testing on Apple Silicon.

## Prerequisites

### System Requirements
- **macOS** with ARM64 architecture (Apple Silicon)
- **Xcode Command Line Tools** installed
- **GCC** compiler with ARM64 support
- **GNU Assembler** (as)

### Installation
```bash
# Install Xcode Command Line Tools
xcode-select --install

# Verify installation
gcc --version
as --version
```

## Building the System

### Quick Start
```bash
# Clone or navigate to the project directory
cd /Users/yenrab/Documents/actly_direct/pure_asm_src

# Build the complete system
make

# Run comprehensive tests
make test
```

### Build Targets

#### Main Targets
```bash
# Build everything
make all

# Build and run tests
make test

# Run code coverage analysis
make coverage

# Clean generated files
make clean

# Show help
make help
```

#### Individual Component Targets
```bash
# Build specific components
make scheduler.o
make process.o
make loadbalancer.o
make affinity.o
make communication.o
make timer.o
make apple_silicon.o
```

## Running Tests

### Complete Test Suite
```bash
# Run all tests
make test

# Run tests and save output
make test > test_output.log 2>&1
```

### Individual Test Suites
```bash
# Run specific test suites
make test_scheduler
make test_process
make test_yielding
make test_load_balancing
make test_affinity
make test_communication
make test_timer
make test_apple_silicon
```

### Test Output
The test suite provides comprehensive output:
```
=========================================
COMPREHENSIVE TEST SUITE
=========================================

COMPREHENSIVE SCHEDULER TESTS:
  • Total Tests: 151
  • Total Assertions: 281
  • Assertions Passed: 265
  • Assertions Failed: 16
  • Success Rate: 94.3%

INDIVIDUAL BEAM TESTS:
  • Total Tests: 4
  • Total Assertions: 10
  • Assertions Passed: 10
  • Assertions Failed: 0
  • Success Rate: 100%

OVERALL SUMMARY:
  • Total Tests: 281
  • Total Assertions: 281
  • Assertions Passed: 265
  • Assertions Failed: 16
  • Success Rate: 100%
=========================================
```

## Using the API

### Basic Scheduler Usage

#### Initialize the Scheduler
```c
#include <stdio.h>
#include <stdint.h>

// Initialize scheduler for core 0
int result = scheduler_init(0);
if (result != 1) {
    printf("Failed to initialize scheduler\n");
    return -1;
}
```

#### Create and Manage Processes
```c
// Allocate a new process
void* pcb = process_alloc();
if (pcb == NULL) {
    printf("Failed to allocate process\n");
    return -1;
}

// Set process state
process_set_state(pcb, PROCESS_STATE_READY);

// Get process state
uint32_t state = process_get_state(pcb);
printf("Process state: %u\n", state);

// Free process when done
process_free(pcb);
```

#### Schedule Processes
```c
// Schedule next process on core 0
void* next_process = scheduler_schedule(0);
if (next_process != NULL) {
    printf("Scheduled process: %p\n", next_process);
} else {
    printf("No processes available\n");
}
```

### Load Balancing Usage

#### Work Stealing
```c
// Try to steal work from another core
void* stolen_process = try_steal_work(scheduler_states, 0);
if (stolen_process != NULL) {
    printf("Stolen process: %p\n", stolen_process);
} else {
    printf("No work to steal\n");
}

// Get current load
uint32_t load = get_scheduler_load(0);
printf("Core 0 load: %u\n", load);
```

#### Work Stealing Deque
```c
// Initialize deque
void* deque = malloc(WS_DEQUE_SIZE);
int result = ws_deque_init(deque, 16);
if (result != 1) {
    printf("Failed to initialize deque\n");
    return -1;
}

// Push process to bottom
void* process = process_alloc();
result = ws_deque_push_bottom(deque, process);
if (result != 1) {
    printf("Failed to push process\n");
}

// Pop process from bottom
void* popped = ws_deque_pop_bottom(deque);
if (popped != NULL) {
    printf("Popped process: %p\n", popped);
}

// Pop process from top (work stealing)
void* stolen = ws_deque_pop_top(deque);
if (stolen != NULL) {
    printf("Stolen process: %p\n", stolen);
}
```

### CPU Affinity Usage

#### Set Process Affinity
```c
// Create process
void* pcb = process_alloc();

// Set affinity to cores 0-7 (P-cores)
uint64_t affinity_mask = 0xFF;  // Bits 0-7 set
int result = set_process_affinity(pcb, affinity_mask);
if (result != 1) {
    printf("Failed to set affinity\n");
}

// Check if process can run on core 0
int can_run = check_affinity(pcb, 0);
if (can_run) {
    printf("Process can run on core 0\n");
} else {
    printf("Process cannot run on core 0\n");
}
```

#### Core Type Detection
```c
// Get core type
uint64_t core_type = get_core_type(0);
if (core_type == CORE_TYPE_PERFORMANCE) {
    printf("Core 0 is a P-core\n");
} else if (core_type == CORE_TYPE_EFFICIENCY) {
    printf("Core 0 is an E-core\n");
}

// Check if core is P-core
int is_p_core = is_performance_core(0);
if (is_p_core) {
    printf("Core 0 is a performance core\n");
}

// Get optimal core for CPU-intensive process
uint64_t optimal_core = get_optimal_core(0);  // 0 = CPU_INTENSIVE
printf("Optimal core for CPU-intensive process: %lu\n", optimal_core);
```

### Inter-Core Communication Usage

#### Message Queues
```c
// Initialize message queue
void* queue = malloc(MESSAGE_QUEUE_SIZE);
int result = message_queue_init(queue, 16);
if (result != 1) {
    printf("Failed to initialize message queue\n");
    return -1;
}

// Send message
uint64_t message = 0x123456789ABCDEF0;
result = send_message(queue, message);
if (result != 1) {
    printf("Failed to send message\n");
}

// Try to receive message (non-blocking)
uint64_t received = try_receive_message(queue);
if (received != 0) {
    printf("Received message: 0x%lx\n", received);
} else {
    printf("No message available\n");
}

// Check queue status
int is_empty = message_queue_is_empty(queue);
uint32_t size = message_queue_size(queue);
printf("Queue empty: %d, size: %u\n", is_empty, size);
```

#### Blocking Receive
```c
// Create process for blocking receive
void* pcb = process_alloc();

// Blocking receive (will block until message available)
uint64_t message = receive_message_blocking(queue, pcb);
printf("Received message: 0x%lx\n", message);
```

### Timer System Usage

#### Timer Management
```c
// Initialize timer system
int result = timer_init();
if (result != 1) {
    printf("Failed to initialize timer system\n");
    return -1;
}

// Get current system time
uint64_t current_time = get_system_ticks();
printf("Current time: %lu ticks\n", current_time);

// Insert timer
uint64_t expiry_time = current_time + 1000;  // 1000 ticks from now
void* callback = (void*)my_callback_function;
uint64_t process_id = 123;
uint64_t timer_id = insert_timer(expiry_time, callback, process_id);
if (timer_id != 0) {
    printf("Timer inserted with ID: %lu\n", timer_id);
} else {
    printf("Failed to insert timer\n");
}

// Cancel timer
result = cancel_timer(timer_id);
if (result != 1) {
    printf("Failed to cancel timer\n");
}
```

#### Timeout Support
```c
// Schedule timeout for blocking operation
uint64_t timeout_ticks = 5000;  // 5000 ticks timeout
uint64_t process_id = 456;
uint64_t timeout_id = schedule_timeout(timeout_ticks, process_id);
if (timeout_id != 0) {
    printf("Timeout scheduled with ID: %lu\n", timeout_id);
}

// Cancel timeout
result = cancel_timeout(timeout_id);
if (result != 1) {
    printf("Failed to cancel timeout\n");
}
```

### Apple Silicon Optimization Usage

#### Core Detection
```c
// Detect Apple Silicon core types
uint8_t* core_type_map = malloc(128);  // MAX_CORES
int result = detect_apple_silicon_core_types(core_type_map);
if (result != 1) {
    printf("Failed to detect core types\n");
    return -1;
}

// Check core types
for (int i = 0; i < 16; i++) {
    if (core_type_map[i] == APPLE_SILICON_CORE_TYPE_PERFORMANCE) {
        printf("Core %d is a P-core\n", i);
    } else if (core_type_map[i] == APPLE_SILICON_CORE_TYPE_EFFICIENCY) {
        printf("Core %d is an E-core\n", i);
    }
}
```

#### Cache Optimization
```c
// Get cache line size
uint64_t cache_line_size = get_cache_line_size_apple_silicon();
printf("Cache line size: %lu bytes\n", cache_line_size);

// Apply Apple Silicon optimizations
result = optimize_for_apple_silicon();
if (result != 1) {
    printf("Failed to apply Apple Silicon optimizations\n");
}
```

## Advanced Usage

### Custom Test Development

#### Creating Custom Tests
```c
#include "test_framework.h"

void test_my_feature() {
    printf("--- Testing My Feature ---\n");
    
    // Test implementation
    int result = my_function();
    test_assert_equal(1, result, "my_function_success");
    
    // Test edge cases
    result = my_function_with_invalid_input();
    test_assert_equal(0, result, "my_function_invalid_input");
}

void test_my_feature_main() {
    printf("=== MY FEATURE TEST SUITE ===\n");
    test_my_feature();
    printf("=== MY FEATURE TEST SUITE COMPLETE ===\n");
}
```

#### Adding Tests to Test Runner
```c
// In test_runner.c
extern void test_my_feature_main();

// In test_runner_main()
test_my_feature_main();
```

### Performance Optimization

#### Memory Alignment
```c
// Ensure proper memory alignment for ARM64
void* aligned_memory = aligned_alloc(8, size);  // 8-byte alignment
if (aligned_memory == NULL) {
    printf("Failed to allocate aligned memory\n");
    return -1;
}
```

#### Cache Line Optimization
```c
// Use 128-byte cache lines for Apple Silicon
#define CACHE_LINE_SIZE 128
void* cache_aligned = aligned_alloc(CACHE_LINE_SIZE, size);
```

### Error Handling Best Practices

#### Check Return Values
```c
// Always check return values
int result = scheduler_init(0);
if (result != 1) {
    printf("Scheduler initialization failed\n");
    return -1;
}

// Check for NULL pointers
void* pcb = process_alloc();
if (pcb == NULL) {
    printf("Process allocation failed\n");
    return -1;
}
```

#### Resource Cleanup
```c
// Always free allocated resources
void* pcb = process_alloc();
if (pcb != NULL) {
    // Use process
    process_set_state(pcb, PROCESS_STATE_RUNNING);
    
    // Clean up
    process_free(pcb);
}
```

## Troubleshooting

### Common Issues

#### Build Errors
```bash
# If you get assembly errors
make clean
make

# If you get linker errors
gcc --version  # Check GCC version
xcode-select --install  # Reinstall command line tools
```

#### Test Failures
```bash
# Run individual tests to isolate issues
make test_scheduler
make test_process
make test_yielding

# Check test output for specific failures
make test 2>&1 | grep "Failed Tests"
```

#### Memory Issues
```bash
# Check for memory leaks
valgrind --tool=memcheck ./scheduler_tests_exe

# Use Address Sanitizer
gcc -fsanitize=address -g test.c -o test
```

### Debugging

#### Enable Debug Output
```c
// Add debug prints in your code
printf("DEBUG: Process allocated at %p\n", pcb);
printf("DEBUG: Scheduler state: %p\n", scheduler_state);
```

#### Use GDB
```bash
# Compile with debug symbols
gcc -g -O0 test.c -o test

# Run with GDB
gdb ./test
(gdb) break main
(gdb) run
(gdb) step
```

## Performance Tuning

### Apple Silicon Optimization
- Use P-cores for CPU-intensive processes
- Use E-cores for I/O-bound processes
- Align memory to 128-byte cache lines
- Use appropriate CPU affinity masks

### Memory Management
- Pre-allocate PCBs when possible
- Use memory pools for frequent allocations
- Implement proper garbage collection
- Monitor memory usage patterns

### Load Balancing
- Tune work stealing parameters
- Monitor load distribution across cores
- Adjust victim selection strategies
- Optimize migration thresholds

## Integration with Other Systems

### macOS Integration
```c
// Use macOS-specific features
#include <mach/mach.h>
#include <mach/mach_time.h>

// Get high-resolution time
uint64_t time = mach_absolute_time();
```

### Bare Metal Deployment
- Remove macOS-specific code
- Implement hardware-specific initialization
- Add interrupt handling
- Implement system call interface

## Support and Contributing

### Getting Help
- Check the API reference for function documentation
- Review test cases for usage examples
- Check the README for system requirements
- Review the source code for implementation details

### Contributing
- Follow the existing code style
- Add comprehensive tests for new features
- Update documentation for API changes
- Ensure all tests pass before submitting

### License
This project is licensed under the MIT License. See the LICENSE file for details.
