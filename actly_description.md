#Actly Language Description
Actly is a declarative, general purpose language, that follows the actor and template patterns. It is Turing Complete and has one set of declarative patterns for creating the actors and another set of declarative patterns for describing the behaviors of the actors when they receive a message defined as Behavior Templates in this document. The language will execute on both computers and boards. This enables computer and board-to-board communication to be a foundational part of the language, not a seperate library.

##Process types
**Responder**: a a stateful process that receives only call-type requests. the state can be ignored
**Doer**: a stateful process that receives only cast-type requests. the state can be ignored
**Morpher**: a process that is a state machine, used to represent physical or imaginary machines
**Rememberer**: a stateless process that provides physical storage IO
**Watcher**: a process that watches other processes and restarts them when they fail
**Bridger**: a process that sends messages to external hardware

##Atoms
Atoms are immutable, unique identifiers used for message tagging, pattern matching, and state identification. They are fundamental to the language's message passing and pattern matching system.
**Examples**: cutIn, joining, givesUp, positive, negative, calculate, result, tr
**Usage**: Atoms serve as message tags, state identifiers, and pattern matching keys

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

##Behavior Templates
Used by the programmer to implement all behavior in the system
**Map** Change All <data structure> by <calculation>
**Filter** Pick From <data structure> with <calculation>
**Reduce** Combine <data structure> into <result> using <calculation>
**For Each** With Each of the <dictionary> <calculation>
**Conditional Branching** Match? <some value> <other value>
**Variable Declaration** Tag <some_value> with <some_label>
**Peak** Peak into <data structure> and tag<some tag or tags> 
**Message** Send <message> to <name>
**State Update** Keep <value>
**Write to Physical Storage** store <value> in <name>
**FindIn** FindIn <data structure> using <operation>
**Zip** Zip <data structure1> with <data structure2> using <operation>
**Unzip** Unzip <data structure> into <structure1> and <structure2>


##Examples

### A
Rememberer: MovieLine Type:Finger Tree Remembered:[sue,bob,sally]
    waitFor Message
        match Message and tag {cutIn,Person,Spot}
            Person joinAt Spot
        match Message and tag {joining,Person}
            Person join
        match Message and tag {givesUp,PersonAt}
            PersonAt leaves
        match Message next
            firstLeaves

### B
Responder: DoubleSummer
    waitFor Numbers
        changeAll Numbers by
            Number * 2
        pickFrom Numbers with
            Number > 5
        combine Numbers in Result using
    Bucket + Number

### Ops
    **addition**
    **subtraction**
    **multiplication**
    **division**
