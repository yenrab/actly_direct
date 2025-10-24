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
// scheduler_functions.h — Shared scheduler function declarations
// ------------------------------------------------------------
// This header contains all scheduler function declarations for use by leaf node test files
// Only individual test files should include this header

#ifndef SCHEDULER_FUNCTIONS_H
#define SCHEDULER_FUNCTIONS_H

#include <stdint.h>

// Scheduler state management functions
extern void* scheduler_state_init(uint64_t max_cores);
extern void scheduler_state_destroy(void* scheduler_states);

// Core scheduler functions
extern void scheduler_init(void* scheduler_states, uint64_t core_id);
extern void* scheduler_get_current_process_with_state(void* scheduler_states, uint64_t core_id);
extern void scheduler_set_current_process_with_state(void* scheduler_states, uint64_t core_id, void* process);
extern uint64_t scheduler_get_reduction_count_with_state(void* scheduler_states, uint64_t core_id);
extern void scheduler_set_reduction_count_with_state(void* scheduler_states, uint64_t core_id, uint64_t count);
extern uint64_t scheduler_get_core_id_with_state(void* scheduler_states, uint64_t core_id);
extern void* get_scheduler_state(uint64_t core_id);
extern void* get_priority_queue(void* state, uint64_t priority);

// Scheduler queue operations
extern int scheduler_enqueue_process_with_state(void* scheduler_states, uint64_t core_id, void* process, uint64_t priority);
extern void* scheduler_dequeue_process_with_state(void* scheduler_states, uint64_t core_id);
extern uint64_t scheduler_get_queue_length_with_state(void* scheduler_states, uint64_t core_id, uint64_t priority);

// Scheduler scheduling functions
extern void* scheduler_schedule(void* scheduler_states, uint64_t core_id);
extern void scheduler_idle(void* scheduler_states, uint64_t core_id);

// Additional scheduler functions without _with_state suffix
extern void scheduler_set_current_process(void* scheduler_states, uint64_t core_id, void* process);
extern void scheduler_set_reduction_count_with_state(void* scheduler_states, uint64_t core_id, uint64_t count);
extern uint64_t scheduler_get_reduction_count_with_state(void* scheduler_states, uint64_t core_id);
extern int scheduler_enqueue_process(void* scheduler_states, uint64_t core_id, void* process, uint64_t priority);

// Process management functions
extern void* process_create(uint64_t entry_point, uint64_t priority, uint64_t stack_size, uint64_t heap_size);
extern void process_destroy(void* pcb);
extern uint64_t process_get_pid(void* pcb);
extern uint64_t process_get_priority(void* pcb);
extern uint64_t process_get_state(void* pcb);
extern void process_set_state(void* pcb, uint64_t state);

// Process control functions
extern void* process_create_fixed(uint64_t entry_point, uint32_t priority, uint64_t scheduler_id);
extern void* process_allocate_pcb(void);
extern void process_deallocate_pcb(void* pcb);

// Yielding functions
extern int process_yield_check(void* scheduler_states, uint64_t core_id, void* pcb);
extern void* process_preempt(void* scheduler_states, uint64_t core_id, void* pcb);
extern int process_decrement_reductions_with_check(void* scheduler_states, uint64_t core_id);
extern void* process_yield_with_state(void* scheduler_states, uint64_t core_id, void* pcb);
extern int process_yield_conditional_with_state(void* scheduler_states, uint64_t core_id, void* pcb);
extern int actly_yield(uint64_t core_id);

// Blocking functions
extern void* process_block(void* scheduler_states, uint64_t core_id, void* pcb, uint64_t reason);
extern int process_wake(void* scheduler_states, uint64_t core_id, void* pcb);
extern void* process_block_on_receive(void* scheduler_states, uint64_t core_id, void* pcb, uint64_t pattern);
extern int process_block_on_timer(void* scheduler_states, uint64_t core_id, void* pcb, uint64_t timeout_ticks);
extern int process_block_on_io(void* scheduler_states, uint64_t core_id, void* pcb, uint64_t io_descriptor);
extern uint64_t process_check_timer_wakeups(uint64_t core_id);

// Work stealing and load balancing functions
extern void* work_steal_process(void* scheduler_states, uint64_t core_id);
extern int select_victim_core(uint64_t core_id, uint64_t max_cores);
extern void* load_balance_processes(void* scheduler_states, uint64_t core_id);

// External constants from assembly
extern const uint64_t MAX_CORES_CONST;
extern const uint64_t NUM_PRIORITIES_CONST;
extern const uint64_t DEFAULT_REDUCTIONS;
extern const uint64_t PRIORITY_QUEUE_SIZE_CONST;
extern const uint64_t SCHEDULER_SIZE_CONST;

// Process state constants
extern const uint64_t PROCESS_STATE_READY;
extern const uint64_t PROCESS_STATE_RUNNING;
extern const uint64_t PROCESS_STATE_WAITING;
extern const uint64_t PRIORITY_NORMAL;
extern const uint64_t PRIORITY_HIGH;
extern const uint64_t REASON_RECEIVE;
extern const uint64_t REASON_TIMER;
extern const uint64_t REASON_IO;
extern const uint64_t MAX_BLOCKING_TIME;

#endif // SCHEDULER_FUNCTIONS_H
