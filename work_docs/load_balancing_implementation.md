# Phase 4: Load Balancing Implementation Documentation

## Overview

This document describes the implementation of Phase 4: Load Balancing for the BEAM Multi-Core Threading Implementation. The load balancing system implements work stealing algorithms to enable efficient distribution of work across multiple CPU cores, following BEAM-style scheduling principles.

## Architecture

### Core Components

1. **Lock-Free Work Stealing Deque** (`loadbalancer.s`)
   - Implements a circular buffer-based deque with atomic operations
   - Supports both local (bottom) and remote (top) access patterns
   - Uses ARM64 LDXR/STXR for atomic compare-and-swap operations

2. **Victim Selection Strategies**
   - Random selection for load distribution
   - Load-based selection to find busiest cores
   - Locality-aware selection for NUMA optimization

3. **Work Stealing Logic**
   - Attempts to steal work from other schedulers when idle
   - Respects migration constraints and affinity settings
   - Tracks statistics for performance monitoring

4. **Scheduler Integration**
   - Modified `scheduler_idle` function to attempt work stealing
   - Seamless integration with existing scheduler infrastructure
   - Maintains compatibility with all existing functionality

## Implementation Details

### Lock-Free Deque Data Structure

The work stealing deque is implemented as a circular buffer with the following layout:

```assembly
// Work stealing deque structure offsets (64 bytes aligned)
.equ ws_deque_top, 0              // Top pointer (8 bytes) - remote access
.equ ws_deque_bottom, 8           // Bottom pointer (8 bytes) - local access
.equ ws_deque_processes, 16       // Process array pointer (8 bytes)
.equ ws_deque_size, 24            // Maximum size (4 bytes)
.equ ws_deque_mask, 28            // Size mask for circular buffer (4 bytes)
.equ ws_deque_steal_count, 32    // Successful steals (8 bytes)
.equ ws_deque_steal_attempts, 40 // Total steal attempts (8 bytes)
.equ ws_deque_local_pops, 48     // Local pop operations (8 bytes)
.equ ws_deque_size_bytes, 64     // Total structure size
```

### Key Functions

#### Deque Operations
- `_ws_deque_init(deque_ptr, size)` - Initialize deque with given size
- `_ws_deque_push_bottom(deque_ptr, process)` - Local scheduler adds work
- `_ws_deque_pop_bottom(deque_ptr)` - Local scheduler gets work
- `_ws_deque_pop_top(deque_ptr)` - Remote scheduler steals work
- `_ws_deque_is_empty(deque_ptr)` - Check if deque is empty
- `_ws_deque_size(deque_ptr)` - Get current size

#### Victim Selection
- `_get_scheduler_load(core_id)` - Calculate current load for a scheduler
- `_find_busiest_scheduler(current_core)` - Find scheduler with most work
- `_select_victim_random(current_core)` - Random victim selection
- `_select_victim_by_load(current_core)` - Load-based victim selection
- `_select_victim_locality(current_core)` - Locality-aware selection

#### Work Stealing
- `_try_steal_work(current_core)` - Attempt to steal work from other cores
- `_migrate_process(process, source_core, target_core)` - Migrate process between cores
- `_is_steal_allowed(source_core, target_core)` - Check migration constraints

### ARM64 Atomic Operations

The implementation uses ARM64 atomic operations for thread-safe deque operations:

- **LDXR/STXR**: Load/Store Exclusive for atomic compare-and-swap
- **DMB**: Data Memory Barrier for memory ordering
- **Retry Logic**: Handles failed atomic operations with exponential backoff
- **ABA Prevention**: Version counters prevent ABA problems

### Load Calculation

Load is calculated using a weighted priority system:

```
scheduler_load = MAX_priority_count * 4 + 
                 HIGH_priority_count * 3 + 
                 NORMAL_priority_count * 2 + 
                 LOW_priority_count * 1
```

This ensures that higher priority processes contribute more to the load calculation, making work stealing more effective for load balancing.

## Configuration

### Work Stealing Constants

The following constants are defined in `config.inc`:

```assembly
// Work stealing configuration
.equ WORK_STEAL_ENABLED, 1           // Enable work stealing
.equ MIN_STEAL_QUEUE_SIZE, 2         // Don't steal if queue has < 2 processes
.equ MAX_MIGRATIONS_PER_PROCESS, 100 // Max migrations per process
.equ STEAL_COOLDOWN_CYCLES, 10000    // Cycles between steals from same victim
.equ LOAD_IMBALANCE_THRESHOLD, 2     // Steal if load difference > 2
.equ STEAL_RETRY_LIMIT, 3            // Max retry attempts per steal

// Victim selection strategy
.equ VICTIM_STRATEGY_RANDOM, 0       // Random victim selection
.equ VICTIM_STRATEGY_LOAD, 1         // Load-based victim selection
.equ VICTIM_STRATEGY_LOCALITY, 2     // Locality-aware selection
.equ DEFAULT_VICTIM_STRATEGY, 1      // Use load-based by default
```

### Tuning Guidelines

- **MIN_STEAL_QUEUE_SIZE**: Prevents stealing from cores with insufficient work
- **MAX_MIGRATIONS_PER_PROCESS**: Limits process bouncing between cores
- **STEAL_COOLDOWN_CYCLES**: Prevents excessive stealing from the same victim
- **LOAD_IMBALANCE_THRESHOLD**: Triggers work stealing when load difference exceeds threshold

## Testing

### Test Coverage

The implementation includes comprehensive test coverage:

1. **Unit Tests**
   - `test_work_stealing_deque.c` - Lock-free deque operations
   - `test_victim_selection.c` - Victim selection algorithms
   - `test_work_stealing.c` - Work stealing operations

2. **Integration Tests**
   - `test_load_balancing_integration.c` - Multi-core load balancing scenarios

### Test Scenarios

- **Deque Operations**: Push, pop, empty checks, circular buffer wraparound
- **Concurrent Access**: Multiple cores accessing deques simultaneously
- **Load Balancing**: Unbalanced load scenarios with work stealing
- **Migration**: Process migration between cores with statistics tracking
- **Edge Cases**: Empty queues, single processes, invalid parameters

### Running Tests

```bash
# Run individual test suites
make test_work_stealing_deque_standalone
make test_victim_selection_standalone
make test_work_stealing_standalone
make test_load_balancing_integration_standalone

# Run all work stealing tests
make test_standalone_all
```

## Performance Characteristics

### Time Complexity
- **Deque Operations**: O(1) for push/pop operations
- **Victim Selection**: O(n) where n is number of cores
- **Work Stealing**: O(n) for victim selection + O(1) for steal attempt
- **Load Calculation**: O(p) where p is number of priority levels (4)

### Space Complexity
- **Deque Structure**: O(s) where s is deque size (typically 16-64 processes)
- **Scheduler State**: O(1) per core (no additional space overhead)
- **Migration Tracking**: O(1) per process (migration count field)

### Memory Ordering
- **Acquire Semantics**: Load operations use acquire semantics
- **Release Semantics**: Store operations use release semantics
- **Sequential Consistency**: Critical sections maintain sequential consistency

## Integration Points

### Scheduler Integration

The work stealing system integrates with the existing scheduler through:

1. **Modified `scheduler_idle` Function**
   - Calls `_try_steal_work` when no local work is available
   - Maintains existing scheduler interface
   - Preserves all existing functionality

2. **Process Migration**
   - Updates process scheduler_id field
   - Increments migration count
   - Updates scheduler statistics

3. **Load Monitoring**
   - Integrates with existing priority queue system
   - Uses existing scheduler state structure
   - Maintains compatibility with all priority levels

### External Dependencies

The implementation depends on:
- `scheduler.s` - Scheduler state and priority queues
- `process.s` - Process control block structure
- `config.inc` - Configuration constants
- C library functions for memory management (`mmap`)

## Known Limitations

### Current Limitations
1. **Simplified Victim Selection**: Locality-aware selection falls back to load-based
2. **Basic Migration Constraints**: Limited affinity constraint checking
3. **No NUMA Awareness**: Does not detect NUMA topology
4. **Single Deque Per Core**: No multiple deque support

### Future Improvements
1. **NUMA Topology Detection**: Implement proper NUMA node awareness
2. **Advanced Migration Policies**: More sophisticated affinity constraints
3. **Multiple Deque Support**: Support for multiple work stealing queues per core
4. **Dynamic Load Balancing**: Adaptive load balancing based on system state

## Troubleshooting

### Common Issues

1. **Compilation Errors**
   - Ensure `config.inc` is included
   - Check ARM64 assembly syntax
   - Verify external function declarations

2. **Runtime Errors**
   - Check deque initialization
   - Verify process pointer validity
   - Ensure proper memory alignment

3. **Performance Issues**
   - Monitor steal success rates
   - Tune victim selection strategy
   - Adjust migration constraints

### Debugging

Enable debug output by setting debug flags in test framework:
```c
// Enable work stealing debug output
#define DEBUG_WORK_STEALING 1
#define DEBUG_VICTIM_SELECTION 1
#define DEBUG_MIGRATION 1
```

## Conclusion

The Phase 4 Load Balancing implementation provides a solid foundation for multi-core work distribution in the BEAM Multi-Core Threading Implementation. The lock-free deque implementation ensures thread-safe operations, while the victim selection strategies enable effective load balancing across cores.

The system is designed to be:
- **Efficient**: O(1) deque operations with minimal overhead
- **Scalable**: Supports up to MAX_CORES cores
- **Robust**: Comprehensive error handling and edge case coverage
- **Extensible**: Easy to add new victim selection strategies
- **Testable**: Complete test coverage with integration tests

This implementation successfully addresses the critical missing functionality identified in the undone analysis and provides the foundation for the remaining phases of the multi-core implementation.

## References

- [BEAM Multi-Core Threading Implementation Plan](../research_implementation_plan.md)
- [Undone Analysis](../undone_analysis.md)
- [ARM64 Instruction Reference](../arm64_rules.md)
- [Coding Rules](../coding%20rules.md)
