#!/bin/bash

# MIT License
#
# Copyright (c) 2025 Lee Barney
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

# Code Statistics Script for Pure Assembly Scheduler
# Counts ship-ready code, testing code, and comments
# Run from within the pure_asm_src directory

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Get the directory where this script is located
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

echo -e "${BLUE}=== Pure Assembly Scheduler Code Statistics ===${NC}"
echo "Analyzing pure assembly scheduler code and tests..."
echo "Working directory: $(pwd)"
echo

# Initialize counters
ship_ready_lines=0
test_lines=0
comment_lines=0
total_files=0

# Function to count non-empty, non-comment lines in a file
count_code_lines() {
    local file="$1"
    
    if [[ ! -f "$file" ]]; then
        echo 0
        return 0
    fi
    
    # Get file extension
    local ext="${file##*.}"
    
    case "$ext" in
        "s"|"S")
            # Assembly files - count non-empty lines that don't start with comment characters
            grep -v '^\s*$' "$file" | grep -v '^\s*//' | grep -v '^\s*#' | grep -v '^\s*;' | wc -l | tr -d ' '
            ;;
        "c"|"h")
            # C files - count non-empty lines that don't start with comment characters
            grep -v '^\s*$' "$file" | grep -v '^\s*//' | grep -v '^\s*/\*' | grep -v '^\s*\*' | wc -l | tr -d ' '
            ;;
        "sh"|"bash")
            # Bash files - count non-empty lines that don't start with #
            grep -v '^\s*$' "$file" | grep -v '^\s*#' | wc -l | tr -d ' '
            ;;
        "md"|"txt")
            # Documentation files - count all non-empty lines
            grep -v '^\s*$' "$file" | wc -l | tr -d ' '
            ;;
        *)
            # Other files - count all non-empty lines
            grep -v '^\s*$' "$file" | wc -l | tr -d ' '
            ;;
    esac
}

# Function to count comment lines in a file
count_comment_lines() {
    local file="$1"
    
    if [[ ! -f "$file" ]]; then
        echo 0
        return 0
    fi
    
    # Get file extension
    local ext="${file##*.}"
    local total=0
    
    case "$ext" in
        "s"|"S")
            # Assembly files - count lines starting with comment characters
            local c1=$(grep -c '^\s*//' "$file" 2>/dev/null)
            local c2=$(grep -c '^\s*#' "$file" 2>/dev/null)
            local c3=$(grep -c '^\s*;' "$file" 2>/dev/null)
            # Convert to numbers, defaulting to 0 if empty
            c1=$((c1 + 0))
            c2=$((c2 + 0))
            c3=$((c3 + 0))
            total=$((c1 + c2 + c3))
            ;;
        "c"|"h")
            # C files - count lines starting with // or /* or *
            local c1=$(grep -c '^\s*//' "$file" 2>/dev/null)
            local c2=$(grep -c '^\s*/\*' "$file" 2>/dev/null)
            local c3=$(grep -c '^\s*\*' "$file" 2>/dev/null)
            # Convert to numbers, defaulting to 0 if empty
            c1=$((c1 + 0))
            c2=$((c2 + 0))
            c3=$((c3 + 0))
            total=$((c1 + c2 + c3))
            ;;
        "sh"|"bash")
            # Bash files - count lines starting with #
            local c=$(grep -c '^\s*#' "$file" 2>/dev/null)
            total=$((c + 0))
            ;;
        *)
            # Other files - no comment lines counted
            total=0
            ;;
    esac
    
    echo $total
}

# Analyze ship-ready code (pure assembly scheduler)
echo -e "${GREEN}=== SHIP-READY CODE ===${NC}"
echo -e "${YELLOW}Analyzing Ship-Ready Code: scheduler.s${NC}"
if [[ -f "scheduler.s" ]]; then
    code_lines=$(count_code_lines "scheduler.s")
    comments=$(count_comment_lines "scheduler.s")
    echo "  scheduler.s: $code_lines code lines, $comments comment lines"
    ship_ready_lines=$code_lines
    comment_lines=$((comment_lines + comments))
    total_files=1
    echo -e "  ${GREEN}Total: $code_lines code lines, $comments comment lines in 1 file${NC}"
    echo
else
    echo "  File not found: scheduler.s"
    echo
fi

# Analyze testing code
echo -e "${GREEN}=== TESTING CODE ===${NC}"

# Pure assembly tests
echo -e "${YELLOW}Analyzing Testing Code: test/${NC}"
if [[ -d "test" ]]; then
    dir_code=0
    dir_comments=0
    dir_files=0
    
    for file in test/*.{s,S,c,h}; do
        if [[ -f "$file" ]]; then
            code=$(count_code_lines "$file")
            comments=$(count_comment_lines "$file")
            echo "  $(basename "$file"): $code code lines, $comments comment lines"
            dir_code=$((dir_code + code))
            dir_comments=$((dir_comments + comments))
            dir_files=$((dir_files + 1))
        fi
    done
    
    echo -e "  ${GREEN}Total: $dir_code code lines, $dir_comments comment lines in $dir_files files${NC}"
    test_lines=$((test_lines + dir_code))
    comment_lines=$((comment_lines + dir_comments))
    total_files=$((total_files + dir_files))
    echo
else
    echo "  Directory not found: test/"
    echo
fi

# Note: Only analyzing .s, .h, and .c files - excluding Makefiles and documentation
echo -e "${GREEN}=== NOTE ===${NC}"
echo -e "${YELLOW}Only analyzing .s, .h, and .c files - excluding Makefiles and documentation${NC}"
echo

# Print summary
echo -e "${BLUE}=== SUMMARY ===${NC}"
echo -e "${GREEN}Ship-Ready Code Lines:${NC} $ship_ready_lines"
echo -e "${GREEN}Testing Code Lines:${NC} $test_lines"
echo -e "${GREEN}Total Comment Lines:${NC} $comment_lines"
echo -e "${GREEN}Total Files Analyzed:${NC} $total_files"
echo

# Calculate ratios
if [[ $ship_ready_lines -gt 0 ]]; then
    test_ratio=$(echo "scale=2; $test_lines / $ship_ready_lines" | bc -l 2>/dev/null || echo "N/A")
    echo -e "${YELLOW}Test-to-Code Ratio:${NC} $test_ratio (testing lines per ship-ready line)"
fi

if [[ $((ship_ready_lines + test_lines)) -gt 0 ]]; then
    comment_ratio=$(echo "scale=2; $comment_lines / ($ship_ready_lines + test_lines)" | bc -l 2>/dev/null || echo "N/A")
    echo -e "${YELLOW}Comment-to-Code Ratio:${NC} $comment_ratio (comment lines per code line)"
fi

echo
echo -e "${BLUE}=== BREAKDOWN BY FILE TYPE ===${NC}"

# Count by file type (only .s, .h, and .c files in current directory and test subdirectory)
echo "Assembly files (.s/.S):"
asm_files=$(find . -maxdepth 2 -name "*.s" -o -name "*.S" 2>/dev/null | wc -l | tr -d ' ')
echo "  Files: $asm_files"
if [[ $asm_files -gt 0 ]]; then
    asm_lines=0
    while IFS= read -r file; do
        if [[ -f "$file" ]]; then
            lines=$(wc -l < "$file" 2>/dev/null || echo "0")
            asm_lines=$((asm_lines + lines))
        fi
    done < <(find . -maxdepth 2 -name "*.s" -o -name "*.S" 2>/dev/null)
    echo "  Total lines: $asm_lines"
else
    echo "  Total lines: 0"
fi

echo "C files (.c/.h):"
c_files=$(find . -maxdepth 2 -name "*.c" -o -name "*.h" 2>/dev/null | wc -l | tr -d ' ')
echo "  Files: $c_files"
if [[ $c_files -gt 0 ]]; then
    c_lines=0
    while IFS= read -r file; do
        if [[ -f "$file" ]]; then
            lines=$(wc -l < "$file" 2>/dev/null || echo "0")
            c_lines=$((c_lines + lines))
        fi
    done < <(find . -maxdepth 2 -name "*.c" -o -name "*.h" 2>/dev/null)
    echo "  Total lines: $c_lines"
else
    echo "  Total lines: 0"
fi

echo
echo -e "${GREEN}Analysis complete!${NC}"