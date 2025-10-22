# Actly Language Development Prompt

### **Template Usage Requirements:**
- **Use ONLY defined behavior templates**: Map, Filter, Reduce, For Each, Conditional Match/Comparison/Default Branching, Tag, Peak, Message, Keep, Write to Physical Storage, FindIn, Zip, Unzip, Tuple Decomposition
- **NO custom functions**: Do not create external helper functions or undefined operations
- **Template syntax**: Follow exact syntax patterns (e.g., `changeAll <data> in <result> by <calculation>`, `pickFrom <data> with <condition>`)

### **Tuple Decomposition Requirements:**
- **Single match statement**: All tuple decomposition must happen in one `match` statement
- **Type annotations**: Every decomposed variable must have explicit static typing
- **Pattern syntax**: Use `match <data> and tag <tuple_pattern>` with proper type annotations
- **Example**: `match Message and tag {:vote, ParticipantId: String, {VoteChoice: Atom, VoteWeight: Number, VoteMetadata: Dictionary[String, String]}, TimeStamp: Number}`

### **Static Typing Requirements:**
- **All variables typed**: Every variable must have explicit type annotations
- **Tag template variables**: Variables created by `Tag` template must have type declarations (e.g., `Tag VoteWeight: Number with Timestamp`)
- **For Each template variables**: Variables created by `For Each` template must have type declarations (e.g., `With Each of the participants Send @timeout to Participant: String`)
- **Atoms vs Variables**: Atoms start with `@` (e.g., `@vote`), variables cannot start with `@`
- **Type consistency**: Use only defined types (String, Number, Dictionary, List[Type], Set, etc.)
- **No undefined types**: Do not use types not defined in the language specification

### **Conditional Branching Requirements:**
- **Three branching types**: Use Conditional Match Branching (`match-pattern <value> <pattern>`), Conditional Comparison Branching (`match-condition <value> <condition>`), and Conditional Default Branching (`match-default <value>`)
- **Mutual exclusion**: Only one match executes per sequence
- **Complete coverage**: Always include a default case with `match-default`
- **Clear conditions**: Use explicit comparisons (e.g., `voteCount >= threshold`, `voteCount < threshold`)

### **Functional Programming Requirements:**
- **Immutability**: Never modify existing state variables directly
- **New results**: Use `changeAll <data> in <new_result> by <calculation>` to create new data structures
- **State preservation**: Original data must remain unchanged
- **Pure transformations**: All operations must create new results, not modify existing ones

### **Message Passing Requirements:**
- **Atom-based messaging**: Use atoms for message tags (e.g., `@vote`, `@register`, `@timeout`)
- **Type-safe patterns**: All message patterns must include type annotations
- **Distributed coordination**: Use proper actor-to-actor communication patterns

### **Algorithm Implementation Requirements:**
- **Professional examples**: Implement real-world distributed computing scenarios
- **Complete logic**: Handle all cases (success, pending, error, timeout)
- **Standard algorithms**: Follow established consensus protocols (Raft, PBFT elements)
- **Error handling**: Include proper timeout and edge case management

### **Code Quality Requirements:**
- **No undefined variables**: Every variable must be defined or decomposed from messages
- **No undefined operations**: Use only operations defined in the Ops section
- **Consistent syntax**: Follow Actly's syntax patterns exactly
- **Professional documentation**: Include clear comments explaining the purpose and logic

### **Example Structure:**
```actly
// Type aliases for clarity
type ParticipantId = String
type VoteData = Dictionary[String, Any]

// Message protocol declarations
protocol ActorName:
    messages:
        messageName: {InputTypes} -> outputName: {OutputTypes}
        anotherMessage: {InputTypes} -> anotherOutput: {OutputTypes}

Actor: ActorName
    State: {variable1: Type, variable2: Type, ...}
    waitFor Message
        match-pattern Message and tag {@atom, variable: Type, {tuple: Type, components: Type}, timestamp: Type}
            // Store data using decomposed components
            Keep {state: state + {variable: {tuple, components}}}
            // Transform data functionally
            changeAll data in newResult: List[Type] by
                calculation
            // Conditional logic
            match-condition newResult
                condition >= threshold
                // Handle success case
            match-condition newResult
                condition < threshold
                // Handle pending case
            match-default newResult
                // Handle error case
```

## LLM-Friendly Patterns

### Template Syntax Clarity
- **Use explicit keywords**: `match-pattern`, `match-condition`, `match-default` for maximum LLM clarity
- **Clear intent**: Each match type should be immediately obvious to LLMs
- **Consistent patterns**: Use the same syntax structure across all match statements

### Type System Optimization
- **Use type aliases**: Define complex types with semantic names for better LLM understanding
- **Example**: `type ParticipantId = String` instead of `String` throughout code
- **Nested types**: Use aliases for complex nested structures like `type VoteData = Dictionary[String, Any]`
- **Atom clarity**: Maintain clear distinction between atoms (`@atom`) and variables (`variable`)

### Actor Model Clarity
- **Protocol declarations**: Define message protocols before actor implementations for clear LLM understanding
- **Message contracts**: Explicit input/output type definitions for all actor communication
- **Actor interfaces**: Clear actor API definitions for distributed coordination
- **Message flow documentation**: Include explicit message flow comments for complex interactions
- **State transition clarity**: Document state changes and actor lifecycle clearly

### LLM Code Generation Best Practices
```actly
// Define types for clarity
type ParticipantId = String
type VoteData = Dictionary[String, Any]
type VoteMetadata = Dictionary[String, String]

// Define message protocols for clear actor communication
protocol ConsensusVoter:
    messages:
        register: {ParticipantId: String, NodeId: String} -> registered: {ParticipantId: String}
        vote: {ParticipantId: String, VoteData: Dictionary} -> consensus: {Result: Any} | pending: {Count: Number, Threshold: Number}
        timeout: {VoteId: String} -> timeout: {VoteId: String}

// Use explicit match keywords
Responder: ConsensusVoter
    State: {votes: Dictionary[ParticipantId, VoteData], participants: Set[ParticipantId]}
    waitFor Message
        match-pattern Message and tag {@register, ParticipantId, NodeId: String}
            // Clear message flow: register -> registered
            Keep {participants: participants + ParticipantId, votes: votes}
            Send {@registered, ParticipantId} to NodeId
        
        match-condition processedVotes voteCount >= threshold
            // Handle consensus case
        
        match-default processedVotes
            // Handle error cases
```

**CRITICAL**: Follow these requirements exactly. Any deviation from these patterns will result in non-functional code. Use only the defined templates, maintain static typing, follow functional programming principles, and implement professional distributed computing algorithms.
