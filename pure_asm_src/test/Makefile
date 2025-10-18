# ------------------------------------------------------------
# Makefile for Test Directory
# ------------------------------------------------------------
# This Makefile builds the C test files in the test directory

# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -O2 -arch arm64

# Test source files
TEST_SOURCES = test_framework.c \
               test_runner.c \
               test_scheduler_init.c \
               test_scheduler_get_set_process.c \
               test_scheduler_reduction_count.c \
               test_scheduler_core_id.c \
               test_scheduler_helper_functions.c \
               test_scheduler_edge_cases_simple.c \
               test_process_state_management.c

# Test object files
TEST_OBJECTS = $(TEST_SOURCES:.c=.o)

# Default target
all: $(TEST_OBJECTS)

# Compile C source files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean target
clean:
	rm -f $(TEST_OBJECTS)

# Help target
help:
	@echo "Available targets:"
	@echo "  all     - Build all test object files"
	@echo "  clean   - Remove all generated test files"
	@echo "  help    - Show this help message"

# Phony targets
.PHONY: all clean help
