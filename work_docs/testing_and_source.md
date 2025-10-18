# Testing and Source Code Standards

## Pure Assembly Source with C-Based Testing Architecture

This document defines the standards and quality prompts for maintaining a clean separation between pure assembly source code and C-based testing infrastructure.

## 🎯 Core Principle

**Source files in the `src/` directory MUST be pure assembly with NO C library dependencies, while test files can use C libraries for convenience and maintainability.**

## 📋 Quality Prompt for Pure Assembly Source Files

When creating or modifying any `.s` file in the `src/` directory, ensure the following:

### ✅ **MANDATORY REQUIREMENTS:**

1. **No C Library Dependencies**
   - ❌ NO calls to `printf`, `malloc`, `free`, `strlen`, `strcpy`, `memcpy`, etc.
   - ❌ NO `#include` directives for C headers
   - ❌ NO `extern` declarations for C library functions
   - ✅ ONLY pure assembly instructions and system calls

2. **No External Function Calls**
   - ❌ NO `bl` (branch and link) to C functions
   - ❌ NO `call` instructions to external libraries
   - ✅ ONLY self-contained assembly code or system calls

3. **C-Callable Interface**
   - ✅ Export functions with `_` prefix for macOS calling convention
   - ✅ Use proper ARM64 calling conventions (x0-x7 for parameters, x0 for return)
   - ✅ Export constants as global symbols for C access

4. **Self-Contained Implementation**
   - ✅ All functionality implemented in pure assembly
   - ✅ Direct memory management without C library helpers
   - ✅ Custom string operations if needed (no `strlen`, `strcpy`, etc.)

### 🔍 **VERIFICATION CHECKLIST:**

Before committing any `.s` file, verify:

```bash
# Check for C library function calls
grep -E "printf|malloc|free|strlen|strcpy|memcpy|memset|stdlib|stdio" src/*.s

# Check for external function calls (should be empty or only system calls)
grep -E "bl [a-zA-Z]" src/*.s

# Check for C includes (should be empty)
grep -E "\.include.*\.h|#include" src/*.s

# Verify only system calls and self-contained functions
grep -E "\.extern" src/*.s
```

### 📝 **EXAMPLE: CORRECT Pure Assembly File**

```assembly
// ------------------------------------------------------------
// example.s — Pure assembly implementation
// ------------------------------------------------------------
// NO C library dependencies - pure assembly only

    .text
    .align 4

    // C-callable function exports
    .global _example_function
    .global _example_constant

    // Constants
    .equ EXAMPLE_VALUE, 42

    // Data section for constants
    .data
    .align 3
_example_constant:
    .quad EXAMPLE_VALUE

    // Code section
    .text
    .align 4

// ------------------------------------------------------------
// example_function — Pure assembly implementation
// ------------------------------------------------------------
// Input: x0 = parameter
// Output: x0 = result
_example_function:
    // Pure assembly implementation
    add x0, x0, #1
    ret
```

### ❌ **EXAMPLE: INCORRECT Assembly File**

```assembly
// ❌ WRONG - Contains C library dependencies
    .extern printf          // ❌ C library function
    .extern malloc          // ❌ C library function
    .include "stdio.h"      // ❌ C header

    .text
    .align 4

_wrong_function:
    // ❌ WRONG - Calls C library function
    bl printf               // ❌ C library call
    ret
```

## 🧪 C-Based Testing Standards

### ✅ **TESTING REQUIREMENTS:**

1. **C Library Usage Allowed**
   - ✅ Use `printf`, `assert`, `stdint.h`, etc. for convenience
   - ✅ Use standard C debugging tools
   - ✅ Use C string functions for test output

2. **Test Organization**
   - ✅ Place all test files in `test/` directory
   - ✅ Use descriptive test names
   - ✅ Include comprehensive test coverage

3. **Test Structure**
   - ✅ Initialize test framework
   - ✅ Test all public functions
   - ✅ Test edge cases and error conditions
   - ✅ Verify cross-core isolation (if applicable)

### 📝 **EXAMPLE: CORRECT C Test File**

```c
// ------------------------------------------------------------
// test_example.c — C tests for pure assembly example
// ------------------------------------------------------------

#include <stdint.h>
#include <stdio.h>

// External assembly functions
extern uint64_t example_function(uint64_t input);
extern uint64_t example_constant;

// External test framework functions
extern void test_assert_equal(uint64_t expected, uint64_t actual, const char* test_name);

// ------------------------------------------------------------
// test_example_function — Test the assembly function
// ------------------------------------------------------------
void test_example_function(void) {
    printf("\n--- Testing example_function (Pure Assembly) ---\n");
    
    // Test basic functionality
    uint64_t result = example_function(5);
    test_assert_equal(6, result, "example_function_basic");
    
    // Test edge cases
    result = example_function(0);
    test_assert_equal(1, result, "example_function_zero");
    
    // Test constant access
    test_assert_equal(42, example_constant, "example_constant_value");
}
```

## 🏗️ Build System Standards

### **Makefile Structure:**

```makefile
# Main Makefile - builds pure assembly + C tests
AS_SOURCES = src/example.s
C_SOURCES = test/test_example.c test/test_framework.c

# Assembly compilation
%.o: %.s
	as -arch arm64 $< -o $@

# C compilation  
%.o: %.c
	gcc -arch arm64 -Wall -Wextra -std=c99 -O2 -c $< -o $@

# Linking
$(TARGET): $(AS_OBJECTS) $(C_OBJECTS)
	gcc -arch arm64 $(AS_OBJECTS) $(C_OBJECTS) -o $(TARGET)
```

## 🔍 **Quality Assurance Process**

### **Pre-Commit Checklist:**

1. **Source Code Verification:**
   ```bash
   # Run verification checks
   make verify-pure-asm
   ```

2. **Test Coverage:**
   ```bash
   # Ensure all tests pass
   make test
   ```

3. **Architecture Compliance:**
   - ✅ Source files in `src/` are pure assembly
   - ✅ Test files in `test/` use C libraries
   - ✅ No C dependencies in source code
   - ✅ C-callable interface properly exported

### **Continuous Integration Checks:**

```bash
#!/bin/bash
# ci-check.sh - Continuous integration verification

echo "🔍 Verifying pure assembly source files..."

# Check for C library dependencies in source
if grep -r -E "printf|malloc|free|strlen|strcpy|memcpy|memset|stdlib|stdio" src/; then
    echo "❌ FAIL: C library dependencies found in source files"
    exit 1
fi

# Check for external function calls in source
if grep -r -E "bl [a-zA-Z]" src/; then
    echo "❌ FAIL: External function calls found in source files"
    exit 1
fi

# Verify tests compile and run
if ! make test; then
    echo "❌ FAIL: Tests failed to compile or run"
    exit 1
fi

echo "✅ PASS: All quality checks passed"
```

## 📊 **Success Metrics**

### **Quality Indicators:**
- ✅ **0 C library dependencies** in source files
- ✅ **100% test coverage** of public functions
- ✅ **All tests passing** (77/77 in current implementation)
- ✅ **Clean separation** between source and test code
- ✅ **C-callable interface** properly implemented

### **Performance Benefits:**
- 🚀 **No C library overhead** in critical paths
- 🚀 **Direct memory management** without malloc/free
- 🚀 **Full control** over execution and memory layout
- 🚀 **Native performance** without emulation

## 🎯 **Implementation Guidelines**

### **For New Source Files:**
1. Create pure assembly implementation in `src/`
2. Export C-callable functions with `_` prefix
3. Export constants as global symbols
4. Implement comprehensive C tests in `test/`
5. Verify no C library dependencies

### **For New Test Files:**
1. Use C libraries for convenience
2. Test all public assembly functions
3. Include edge cases and error conditions
4. Use descriptive test names
5. Ensure comprehensive coverage

## 🔧 **Tools and Commands**

### **Verification Commands:**
```bash
# Check for C dependencies
grep -r -E "printf|malloc|free|strlen" src/

# Check for external calls
grep -r -E "bl [a-zA-Z]" src/

# Run all tests
make test

# Clean build
make clean && make
```

### **Development Workflow:**
1. Implement pure assembly in `src/`
2. Create C tests in `test/`
3. Run verification checks
4. Ensure all tests pass
5. Commit with confidence

---

## 📝 **Summary**

This architecture provides the **best of both worlds**:
- **Pure assembly source code** with no external dependencies
- **C-based testing** for convenience and maintainability
- **Clean separation** of concerns
- **High performance** with easy testing

By following these standards, we maintain a robust, performant, and maintainable codebase that leverages the strengths of both assembly and C programming languages.
