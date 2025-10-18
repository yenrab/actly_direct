# BEAM CPU Affinity and Migration Research

## Overview

This document provides detailed research and analysis of BEAM's CPU affinity and migration mechanisms, which are crucial for optimizing performance in multi-core systems. CPU affinity allows processes to be bound to specific cores, while migration enables dynamic load balancing while respecting affinity constraints.

## 1. Process Affinity to Specific Schedulers

### Affinity Concept
- **Core binding**: Processes can be bound to specific CPU cores
- **Scheduler affinity**: Processes prefer to run on specific schedulers
- **Flexible binding**: Affinity can be set, changed, or removed dynamically
- **Performance optimization**: Improves cache locality and reduces migration overhead

### Affinity Representation
```
Process Affinity Structure:
struct process_affinity {
    uint64_t affinity_mask;      // Bitmask of allowed cores (64-bit)
    uint32_t preferred_core;     // Preferred core for this process
    uint32_t current_core;       // Core where process is currently running
    uint32_t migration_count;    // Number of times migrated
    uint32_t max_migrations;     // Maximum allowed migrations
    uint64_t last_migration;     // Timestamp of last migration
};
```

### Affinity Types
- **Strict affinity**: Process can only run on specified cores
- **Preferred affinity**: Process prefers specified cores but can migrate
- **No affinity**: Process can run on any core (default)
- **Dynamic affinity**: Affinity can be changed at runtime

### Affinity Benefits
- **Cache locality**: Processes stay on same core, improving cache hit rates
- **NUMA optimization**: Keep processes on same NUMA node
- **Reduced migration**: Minimize expensive cross-core migrations
- **Predictable performance**: Consistent performance characteristics

### Affinity Implementation
```
Affinity Management Functions:
- set_affinity(pid, core_mask): Set allowed cores for process
- get_affinity(pid): Get current affinity mask
- check_affinity(pid, core_id): Check if process can run on core
- update_affinity(pid, new_mask): Change affinity at runtime
- clear_affinity(pid): Remove affinity constraints
```

## 2. Migration Cost Considerations

### Migration Overhead Components
- **Context switching**: Save/restore process context
- **Cache effects**: Loss of cache locality and cache misses
- **Memory access**: Remote memory access latency
- **NUMA effects**: Cross-NUMA node migration costs
- **Synchronization**: Atomic operations for migration

### Cost Analysis
```
Migration Cost Breakdown:
- Context switch: ~100-200 CPU cycles
- Cache miss penalty: ~100-300 cycles per miss
- Memory access: ~200-500 cycles for remote access
- NUMA penalty: ~1000+ cycles for cross-NUMA migration
- Total cost: ~500-2000 cycles per migration
```

### Cost-Benefit Analysis
- **Migration benefit**: Load balancing and CPU utilization
- **Migration cost**: Performance overhead and cache effects
- **Decision criteria**: Only migrate if benefit > cost
- **Threshold calculation**: Dynamic thresholds based on system state

### Cost Factors
```
Migration Cost Factors:
1. Process characteristics:
   - CPU-bound vs I/O-bound
   - Memory access patterns
   - Cache working set size
   - Execution time remaining

2. System characteristics:
   - Cache hierarchy
   - Memory bandwidth
   - NUMA topology
   - Core types (P-core vs E-core)

3. Migration distance:
   - Same core: 0 cost
   - Same cluster: Low cost
   - Different cluster: Medium cost
   - Different NUMA node: High cost
```

### Migration Decision Algorithm
```
Migration Decision Process:
1. Calculate migration cost based on:
   - Process characteristics
   - System topology
   - Migration distance
   
2. Calculate migration benefit:
   - Load balancing improvement
   - CPU utilization gain
   - Response time improvement
   
3. Make migration decision:
   if (benefit > cost * threshold_factor):
       allow_migration()
   else:
       deny_migration()
```

## 3. NUMA-Aware Scheduling Principles

### NUMA Architecture
- **Non-Uniform Memory Access**: Memory access time varies by location
- **NUMA nodes**: Groups of cores with shared memory
- **Local memory**: Fast access to memory on same NUMA node
- **Remote memory**: Slower access to memory on different NUMA node

### NUMA Topology Detection
```
NUMA Topology Structure:
struct numa_node {
    uint32_t node_id;           // NUMA node identifier
    uint32_t core_count;        // Number of cores in node
    uint32_t cores[MAX_CORES];  // Core IDs in this node
    uint64_t memory_size;       // Total memory in node
    uint32_t memory_bandwidth;  // Memory bandwidth
};

struct numa_topology {
    uint32_t node_count;        // Number of NUMA nodes
    struct numa_node nodes[MAX_NUMA_NODES];
    uint32_t core_to_node[MAX_CORES];  // Core to node mapping
};
```

### NUMA-Aware Scheduling
- **Local preference**: Prefer scheduling processes on local NUMA node
- **Memory locality**: Keep processes close to their memory
- **Load balancing**: Balance load within and across NUMA nodes
- **Migration policies**: Consider NUMA distance in migration decisions

### NUMA Optimization Strategies
```
NUMA Optimization:
1. Process placement:
   - Allocate memory on same NUMA node as process
   - Schedule process on cores in same NUMA node
   - Minimize cross-NUMA memory access

2. Load balancing:
   - Balance load within NUMA nodes first
   - Only migrate across NUMA nodes when necessary
   - Consider NUMA distance in migration decisions

3. Memory management:
   - Allocate stacks/heaps on local NUMA node
   - Use NUMA-aware memory allocators
   - Track memory access patterns
```

### Apple Silicon NUMA Considerations
- **Unified memory**: Apple Silicon has unified memory architecture
- **Memory controllers**: Multiple memory controllers for bandwidth
- **Cache hierarchy**: Shared L2 cache per cluster
- **Core types**: P-cores and E-cores in different clusters

## 4. Scheduler Binding Strategies

### Scheduler Binding Types
- **Static binding**: Schedulers bound to specific cores at boot time
- **Dynamic binding**: Schedulers can be moved between cores
- **Hybrid binding**: Mix of static and dynamic binding
- **No binding**: Schedulers can run on any available core

### Static Binding
```
Static Binding Benefits:
- Predictable performance
- Simple implementation
- No binding overhead
- Clear resource allocation

Static Binding Drawbacks:
- Inflexible load balancing
- Poor utilization of heterogeneous cores
- No adaptation to workload changes
```

### Dynamic Binding
```
Dynamic Binding Benefits:
- Flexible load balancing
- Optimal core utilization
- Adaptation to workload changes
- Better performance for heterogeneous cores

Dynamic Binding Drawbacks:
- Complex implementation
- Binding overhead
- Potential performance variability
- Cache effects from migration
```

### Hybrid Binding Strategy
```
Hybrid Binding Approach:
1. Core 0: Always bound to primary scheduler
2. P-cores: Bound to high-performance schedulers
3. E-cores: Bound to efficiency schedulers
4. Remaining cores: Dynamic binding based on load
```

### Binding Implementation
```
Scheduler Binding Functions:
- bind_scheduler_to_core(scheduler_id, core_id): Bind scheduler to core
- unbind_scheduler(scheduler_id): Remove scheduler binding
- get_scheduler_core(scheduler_id): Get current core binding
- migrate_scheduler(scheduler_id, new_core): Move scheduler to new core
```

## Affinity Management and Enforcement

### Affinity Enforcement Points
- **Process creation**: Set initial affinity for new processes
- **Process scheduling**: Check affinity before scheduling
- **Work stealing**: Respect affinity when stealing work
- **Migration**: Validate affinity before migration
- **Load balancing**: Consider affinity in balancing decisions

### Affinity Validation
```
Affinity Validation Process:
1. Check if target core is in affinity mask
2. Verify migration limits not exceeded
3. Calculate migration cost vs benefit
4. Consider NUMA distance and cache effects
5. Make final migration decision
```

### Affinity Change Handling
- **Runtime changes**: Allow affinity changes while process is running
- **Migration triggers**: Affinity changes may trigger immediate migration
- **Graceful handling**: Handle affinity changes without process interruption
- **Validation**: Validate new affinity before applying

### Affinity Inheritance
- **Process spawning**: Child processes inherit parent affinity
- **Thread creation**: Threads inherit process affinity
- **Resource sharing**: Shared resources respect affinity constraints
- **Message passing**: Consider affinity in message routing

## Migration Policies and Constraints

### Migration Policies
- **Conservative**: Only migrate when absolutely necessary
- **Aggressive**: Migrate frequently for optimal load balancing
- **Adaptive**: Adjust migration frequency based on system state
- **NUMA-aware**: Consider NUMA distance in migration decisions

### Migration Constraints
```
Migration Constraints:
1. Affinity constraints:
   - Process must be allowed on target core
   - Respect strict vs preferred affinity
   - Consider affinity change requests

2. Resource constraints:
   - Target core must have available resources
   - Memory must be available on target NUMA node
   - Scheduler must have capacity

3. Performance constraints:
   - Migration cost must be acceptable
   - Performance benefit must be significant
   - System stability must be maintained
```

### Migration Triggers
- **Load imbalance**: Significant load difference between cores
- **Process blocking**: Process blocks and can be migrated
- **Affinity change**: Process affinity is modified
- **Core failure**: Core becomes unavailable
- **Power management**: Core is powered down

### Migration Rate Limiting
```
Migration Rate Limiting:
- Per-process limit: Maximum migrations per process
- Per-core limit: Maximum migrations per core per second
- System-wide limit: Global migration rate limit
- Burst protection: Prevent migration storms
```

## Performance Impact Analysis

### Positive Performance Impacts
- **Load balancing**: Better CPU utilization across cores
- **Cache optimization**: Improved cache hit rates with affinity
- **NUMA optimization**: Reduced remote memory access
- **Responsiveness**: Faster process scheduling

### Negative Performance Impacts
- **Migration overhead**: Cost of moving processes between cores
- **Cache effects**: Cache misses from migration
- **Memory access**: Remote memory access penalties
- **Synchronization**: Atomic operation overhead

### Performance Metrics
```
Performance Metrics:
- Migration rate: Migrations per second
- Migration success rate: Percentage of successful migrations
- Average migration cost: Cycles per migration
- Load balance quality: Distribution of work across cores
- Cache hit rate: Cache performance with affinity
- Memory access latency: Local vs remote access times
```

### Performance Optimization
```
Performance Optimization Strategies:
1. Minimize migration frequency:
   - Use affinity to reduce unnecessary migrations
   - Implement smart migration triggers
   - Use migration rate limiting

2. Optimize migration cost:
   - Batch multiple migrations
   - Use efficient migration algorithms
   - Minimize cache effects

3. Improve cache locality:
   - Use affinity to keep processes on same core
   - Optimize memory allocation
   - Use cache-friendly data structures
```

## Implementation Considerations

### Data Structures
```
Affinity and Migration Data Structures:
struct process_affinity {
    uint64_t affinity_mask;
    uint32_t preferred_core;
    uint32_t current_core;
    uint32_t migration_count;
    uint32_t max_migrations;
    uint64_t last_migration;
};

struct migration_info {
    uint32_t source_core;
    uint32_t target_core;
    uint64_t migration_cost;
    uint64_t migration_benefit;
    uint64_t timestamp;
};

struct numa_info {
    uint32_t node_id;
    uint32_t core_count;
    uint32_t cores[MAX_CORES];
    uint64_t memory_size;
    uint32_t memory_bandwidth;
};
```

### ARM64 Implementation
```assembly
// Check affinity mask
check_affinity:
    ldr x0, [x1, #AFFINITY_MASK_OFFSET]  // Load affinity mask
    mov x2, #1
    lsl x2, x2, x3                       // Create core bit mask
    and x0, x0, x2                       // Check if core is allowed
    cbnz x0, affinity_ok
    // Affinity check failed
    ret

// Calculate migration cost
calculate_migration_cost:
    // Load source and target core IDs
    ldr x0, [x1, #CURRENT_CORE_OFFSET]
    ldr x1, [x1, #TARGET_CORE_OFFSET]
    
    // Calculate NUMA distance
    bl get_numa_distance
    
    // Calculate cache effects
    bl estimate_cache_effects
    
    // Calculate total cost
    add x0, x0, x1
    ret
```

### Error Handling
- **Invalid affinity**: Handle invalid affinity masks
- **Migration failures**: Handle failed migration attempts
- **Resource exhaustion**: Handle out-of-resources conditions
- **Core failures**: Handle core unavailability

### Testing and Validation
- **Unit tests**: Test individual affinity and migration functions
- **Integration tests**: Test complete migration scenarios
- **Performance tests**: Measure migration overhead and benefits
- **Stress tests**: Test under heavy load conditions

## Next Steps

This research provides the foundation for implementing BEAM's CPU affinity and migration mechanisms. The next phase should focus on:

1. **Affinity data structures** implementation in ARM64 assembly
2. **Migration cost calculation** algorithms
3. **NUMA topology detection** and management
4. **Scheduler binding** strategies
5. **Migration policies** and constraints
6. **Performance monitoring** and optimization

The implementation should start with basic affinity management and gradually add advanced features like NUMA awareness and adaptive migration policies.
