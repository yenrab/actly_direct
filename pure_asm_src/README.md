# Pure Assembly Scheduler with C Tests

This directory contains a **pure assembly scheduler implementation** that can be tested using **C test code**. This approach provides the best of both worlds:

- **Core scheduler**: Pure assembly implementation with no C library dependencies
- **Test framework**: C-based tests using standard library for convenience

## 🏗️ Architecture

### Pure Assembly Components
- **`scheduler.s`** - Core scheduler implementation in pure ARM64 assembly
- **No C library dependencies** - Uses only assembly instructions and system calls
- **C-callable interface** - Exports functions that can be called from C code

### C Test Components
- **`test_framework.c`** - C-based test framework using standard library
- **`test_*.c`** - Individual test suites for different scheduler functions
- **`test_runner.c`** - Main test runner and entry point

## 🚀 Building and Running

### Prerequisites
- **macOS** with ARM64 architecture (Apple Silicon)
- **GCC** compiler
- **GNU Assembler** (as)

### Build Commands
```bash
# Build the scheduler tests
make

# Build and run tests
make test

# Clean generated files
make clean

# Show help
make help
```

### Run Tests
```bash
./scheduler_tests
```

## 📋 Test Coverage

The test suite covers all major scheduler functionality:

### 1. Scheduler Initialization (`test_scheduler_init.c`)
- Core ID initialization
- Priority queue initialization
- Current process initialization
- Reduction count initialization
- Statistics initialization

### 2. Process Management (`test_scheduler_get_set_process.c`)
- Get/set current process
- Cross-core isolation
- NULL process handling

### 3. Reduction Count (`test_scheduler_reduction_count.c`)
- Get/set reduction count
- Cross-core isolation
- Boundary value testing

### 4. Core ID (`test_scheduler_core_id.c`)
- Core ID retrieval
- Consistency testing

### 5. Helper Functions (`test_scheduler_helper_functions.c`)
- Scheduler state access
- Priority queue access
- Data structure layout validation

## 🔧 Key Features

### Pure Assembly Scheduler
- **No C library dependencies** - Core scheduler is pure assembly
- **C-callable interface** - Functions can be called from C code
- **BEAM-style design** - Implements Erlang/OTP scheduler concepts
- **Multi-core support** - Per-core scheduler state management
- **Priority queues** - 4-level priority system (max, high, normal, low)

### C Test Framework
- **Standard library usage** - Uses printf, stdint.h, etc. for convenience
- **Comprehensive testing** - Covers all scheduler functionality
- **Clear output** - Readable test results and error messages
- **Easy debugging** - C debugging tools work normally

## 📊 Expected Output

```
[test_boot] Starting test mode

========================================
    SCHEDULER UNIT TEST RUNNER
========================================

--- System Information ---
MAX_CORES: 32
DEFAULT_REDUCTIONS: 2000
NUM_PRIORITIES: 4
scheduler_size: 144
priority_queue_size: 24

--- Testing scheduler_init (Pure Assembly) ---
--- Testing scheduler get/set current process (Pure Assembly) ---
--- Testing scheduler reduction count (Pure Assembly) ---
--- Testing scheduler get_core_id (Pure Assembly) ---
--- Testing scheduler helper functions (Pure Assembly) ---

=== Test Results ===
Total Tests: 77
Passed: 77
Failed: 0
========================

*** ALL TESTS PASSED ***
[runtime_test_mode] All tests completed
```

## 🎯 Benefits of This Approach

### Pure Assembly Scheduler
- ✅ **No C library dependencies** - Core system is self-contained
- ✅ **Full control** - Complete control over memory layout and execution
- ✅ **Performance** - No C library overhead in critical paths
- ✅ **Educational** - Learn assembly programming and system design

### C Test Framework
- ✅ **Easy testing** - Standard C library makes testing convenient
- ✅ **Readable output** - Clear test results and error messages
- ✅ **Debugging** - Standard C debugging tools work
- ✅ **Maintainable** - Easy to add new tests and modify existing ones

## 🔍 File Structure

```
pure_asm_src/
├── scheduler.s                    # Pure assembly scheduler implementation
├── Makefile                       # Main build system
├── README.md                      # This file
└── test/                          # Test directory
    ├── test_framework.c           # C test framework
    ├── test_runner.c              # Main test runner
    ├── test_scheduler_init.c      # Scheduler initialization tests
    ├── test_scheduler_get_set_process.c # Process management tests
    ├── test_scheduler_reduction_count.c # Reduction count tests
    ├── test_scheduler_core_id.c   # Core ID tests
    ├── test_scheduler_helper_functions.c # Helper function tests
    └── Makefile                   # Test build system
```

## 🚀 Next Steps

This pure assembly scheduler can be extended with:
- **Process scheduling algorithms** - Implement actual process scheduling
- **Work stealing** - Add work stealing between cores
- **Process migration** - Implement process migration between cores
- **Real-time scheduling** - Add real-time scheduling capabilities
- **Integration with OS** - Integrate with actual operating system

The C test framework makes it easy to add new tests as the scheduler evolves!
