#Actly Language Description
Actly is a declarative, general purpose language, that follows the actor and template patterns. It is Turing Complete and has one set of declarative patterns for creating the actors and another set of declarative patterns for describing the behaviors of the actors when they receive a message defined as Behavior Templates in this document. The language will execute on both computers and boards. This enables computer and board-to-board communication to be a foundational part of the language, not a seperate library.

##Process types
**Responder**: a a stateful process that receives only call-type requests. the state can be ignored
**Doer**: a stateful process that receives only cast-type requests. the state can be ignored
**Morpher**: a process that is a state machine, used to represent physical or imaginary machines
**Rememberer**: a stateless process that provides physical storage IO
**Watcher**: a process that watches other processes and restarts them when they fail
**Bridger**: a process that sends messages to external hardware
**Trigger**: a proactive process that responds to non-hardware external events (user input, system signals, timers, external APIs) and initiates execution cascades by sending the first messages that start application workflows

##Atoms
Atoms are immutable, unique identifiers used for message tagging, pattern matching, and state identification. They are fundamental to the language's message passing and pattern matching system.
**Examples**: @cutIn, @joining, @givesUp, @positive, @negative, @calculate, @result, @true, @false
**Usage**: Atoms serve as message tags, state identifiers, and pattern matching keys
**Syntax**: Atoms use the @ prefix to distinguish them from variables

##Type Aliases
Type aliases provide semantic names for complex types, improving code readability and LLM understanding.
**Syntax**: `type AliasName = TypeDefinition`
**Examples**: 
- `type ParticipantId = String`
- `type VoteData = Dictionary[String, Any]`
- `type VoteMetadata = Dictionary[String, String]`
**Usage**: Use aliases for complex nested types and semantic clarity

##Language-Available Data Structures
These data structures are not part of a library. They are an integral part of the language and are intended to be more complete than the set of those found in most langauges.
**List**
**Finger Tree**
**Deque**
**B+ Tree**
**Splay Tree**
**AVL Tree**
**Dictionary**
**Tuple**
**Set**

##Variable and Atom Naming Rules
**Variables**: Cannot start with '@' indicator. Any other character is valid for variable names.
**Atoms**: Must start with '@' indicator to distinguish them from variables.
**Tuples**: Regular data structures, no special prefix required.
**Examples**: 
- Variables: taskId, userData, processName, result
- Atoms: @taskId, @userData, @processName, @result
- Tuples: (taskId, userData), (processName, result) (tuples can have any combination of variables and atoms)

##Message Protocol Declarations
Define message protocols before actor implementations for clear LLM understanding of actor communication.
**Syntax**: `protocol ActorName: messages: messageName: {InputTypes} -> {OutputTypes}`
**Examples**:
```actly
protocol ConsensusVoter:
    messages:
        register: {ParticipantId: String, NodeId: String} -> registered: {ParticipantId: String}
        vote: {ParticipantId: String, VoteData: Dictionary} -> consensus: {Result: Any} | pending: {Count: Number, Threshold: Number}
        timeout: {VoteId: String} -> timeout: {VoteId: String}
```
**Usage**: Declare all message types and their input/output contracts before actor definitions

##Behavior Templates
Used by the programmer to implement all behavior in the system
**Map** Change All <data structure> by <calculation>
**Filter** Pick From <data structure> with <calculation>
**Reduce** Combine <data structure> into <result> using <calculation>
**For Each** With Each of the <data structure> <calculation>
**Conditional Match Branching** match-pattern <some value> <other value>
**Conditional Comparison Branching** match-condition <some value> <comparison>
**Conditional Default Branching** match-default <some value>
**Variable Declaration** Tag <some_value> with <some_label>
**Peak** Peak into <data structure> and tag<some tag or tags> 
**Message** Send <message> to <name>
**State Update** Keep <value>
**Write to Physical Storage** store <value> in <name>
**FindIn** FindIn <data structure> using <operation>
**Zip** Zip <data structure1> with <data structure2> using <operation>
**Unzip** Unzip <data structure> into <structure1> and <structure2>
**Tuple Decomposition** match <data> and tag <tuple pattern>

##Match Template Rules and Examples

###Conditional Match Branching
**Purpose**: Pattern matching for exact values and message routing
**Syntax**: `match-pattern <value> <pattern>`
**Rules**: 
- Use for exact value matching
- Patterns can include atoms, variables, and data structures
- First match wins - only one match executes per sequence

**Examples**:
```actly
match-pattern Message @vote
    // Handle vote messages

match-pattern Message @register  
    // Handle registration messages

match-pattern Message @timeout
    // Handle timeout messages
```

###Conditional Comparison Branching
**Purpose**: Conditional logic with comparisons and calculations
**Syntax**: `match-condition <value> <condition>`
**Rules**:
- Use for conditional logic based on comparisons
- Conditions can include arithmetic, logical, and comparison operations
- Multiple conditions can be chained with logical operators

**Examples**:
```actly
match-condition processedVotes
    voteCount >= threshold
    // Handle consensus reached

match-condition processedVotes
    voteCount < threshold
    // Handle consensus pending

match-condition userAge
    age >= 18
    // Handle adult users
```

###Conditional Default Branching
**Purpose**: Catch-all for unmatched conditions and error handling
**Syntax**: `match-default <value>`
**Rules**:
- Use as the final match in a sequence
- Handles any unmatched conditions
- Always executes if no other matches succeed
- Essential for robust error handling

**Examples**:
```actly
match-default processedVotes
    // Handle unexpected vote states

match-default userInput
    // Handle invalid input

match-default systemState
    // Handle error conditions
```

###Complete Match Sequence Example
```actly
match-condition processedVotes
    voteCount >= threshold
    // Handle consensus reached
    Send {@consensus, Result} to participants

match-condition processedVotes
    voteCount < threshold
    // Handle consensus pending
    Send {@pending, voteCount, threshold} to ParticipantId

match-default processedVotes
    // Handle errors and edge cases
    Send {@error, @unexpectedState} to ParticipantId
```

### Ops
    **addition**
    **subtraction**
    **multiplication**
    **division**
    **equality** (==)
    **inequality** (!=)
    **less than** (<)
    **less than or equal** (<=)
    **greater than** (>)
    **greater than or equal** (>=)
    **logical and** (and)
    **logical or** (or)
    **logical not** (not)
    **modulo** (%)
    **exponentiation** (**)
