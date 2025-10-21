# Core Erlang Commands Reference

Core Erlang is an intermediate representation of the Erlang programming language, designed to simplify the compilation process by providing a more regular and simplified syntax. It serves as a bridge between high-level Erlang code and the lower-level BEAM bytecode executed by the Erlang virtual machine.

## Table of Contents
1. [Basic Expressions](#basic-expressions)
2. [Control Flow](#control-flow)
3. [Function Definitions](#function-definitions)
4. [Modules and Attributes](#modules-and-attributes)
5. [Pattern Matching](#pattern-matching)
6. [Data Structures](#data-structures)
7. [Operators](#operators)
8. [Exception Handling](#exception-handling)
9. [Process Communication](#process-communication)
10. [Built-in Functions (BIFs)](#built-in-functions-bifs)
11. [Annotations and Metadata](#annotations-and-metadata)

---

## Basic Expressions

### Variables
- **Syntax**: `Var`
- **Description**: Represents a variable identifier. Variables in Core Erlang start with uppercase letters or underscores.
- **Example**: `X`, `Result`, `_Temp`

### Literals
- **Syntax**: `Literal`
- **Description**: Represents constant values including integers, floats, atoms, strings, and booleans.
- **Examples**:
  - Integers: `42`, `-17`, `0`
  - Floats: `3.14`, `-2.5`, `0.0`
  - Atoms: `'ok'`, `'error'`, `'undefined'`
  - Strings: `"hello"`, `"world"`
  - Booleans: `true`, `false`

### Function Application
- **Syntax**: `apply(Expr, [Expr1, Expr2, ...])`
- **Description**: Applies a function to a list of arguments.
- **Parameters**:
  - `Expr`: Expression that evaluates to a function
  - `[Expr1, Expr2, ...]`: List of argument expressions
- **Example**: `apply(f, [X, Y])`

---

## Control Flow

### Case Expression
- **Syntax**: `case Expr of [Clause1, Clause2, ...] end`
- **Description**: Evaluates an expression and matches its result against a series of patterns.
- **Parameters**:
  - `Expr`: Expression to evaluate
  - `[Clause1, Clause2, ...]`: List of pattern clauses
- **Clause Format**: `Pattern when Guard -> Body`
- **Example**:
  ```erlang
  case X of
    0 -> zero;
    1 -> one;
    N when N > 0 -> positive;
    _ -> negative
  end
  ```

### Let Expression
- **Syntax**: `let [Var1 = Expr1, Var2 = Expr2, ...] in Expr`
- **Description**: Binds variables to expressions and evaluates a body expression within that context.
- **Parameters**:
  - `[Var1 = Expr1, Var2 = Expr2, ...]`: List of variable bindings
  - `Expr`: Body expression to evaluate
- **Example**: `let X = 5, Y = 10 in X + Y`

### If Expression
- **Syntax**: `if Guard1 -> Body1; Guard2 -> Body2; ... end`
- **Description**: Conditional branching based on guard expressions.
- **Parameters**:
  - `Guard1, Guard2, ...`: Guard expressions to evaluate
  - `Body1, Body2, ...`: Corresponding body expressions
- **Example**:
  ```erlang
  if
    X > 0 -> positive;
    X < 0 -> negative;
    true -> zero
  end
  ```

---

## Function Definitions

### Named Function Definition
- **Syntax**: `function Name/Arity = fun ([Clause1, Clause2, ...])`
- **Description**: Defines a named function with multiple clauses.
- **Parameters**:
  - `Name/Arity`: Function name and arity (number of arguments)
  - `[Clause1, Clause2, ...]`: List of function clauses
- **Clause Format**: `(Pattern1, Pattern2, ...) when Guard -> Body`
- **Example**:
  ```erlang
  function factorial/1 = fun
    (0) -> 1;
    (N) when N > 0 -> N * factorial(N - 1)
  end
  ```

### Anonymous Function
- **Syntax**: `fun ([Clause1, Clause2, ...])`
- **Description**: Defines an anonymous function with multiple clauses.
- **Parameters**:
  - `[Clause1, Clause2, ...]`: List of function clauses
- **Example**:
  ```erlang
  fun
    (X, Y) when X > Y -> X;
    (X, Y) -> Y
  end
  ```

---

## Modules and Attributes

### Module Declaration
- **Syntax**: `module Name [Attr1, Attr2, ...] [Func1, Func2, ...]`
- **Description**: Defines a module with attributes and function definitions.
- **Parameters**:
  - `Name`: Module name
  - `[Attr1, Attr2, ...]`: List of module attributes
  - `[Func1, Func2, ...]`: List of function definitions
- **Example**:
  ```erlang
  module math
    export [factorial/1, fibonacci/1]
    function factorial/1 = fun (N) -> ... end
    function fibonacci/1 = fun (N) -> ... end
  end
  ```

### Module Attributes
- **Syntax**: `attribute Name Value`
- **Description**: Defines an attribute for a module.
- **Parameters**:
  - `Name`: Attribute name
  - `Value`: Attribute value
- **Common Attributes**:
  - `export [Function1/Arity1, ...]`: Exports functions
  - `import Module [Function1/Arity1, ...]`: Imports functions
  - `include "File"`: Includes another file
- **Example**: `attribute export [add/2, subtract/2]`

---

## Pattern Matching

### Match Expression
- **Syntax**: `Expr1 = Expr2`
- **Description**: Matches the result of `Expr2` against the pattern `Expr1`.
- **Parameters**:
  - `Expr1`: Pattern to match against
  - `Expr2`: Expression to evaluate
- **Example**: `{ok, Result} = some_function()`

### Pattern Types

#### Variable Pattern
- **Syntax**: `Var`
- **Description**: Matches any value and binds it to the variable.
- **Example**: `X` (matches any value)

#### Literal Pattern
- **Syntax**: `Literal`
- **Description**: Matches the exact literal value.
- **Example**: `42`, `'ok'`, `"hello"`

#### Tuple Pattern
- **Syntax**: `{Pattern1, Pattern2, ...}`
- **Description**: Matches a tuple with elements matching each pattern.
- **Example**: `{ok, Result}`, `{error, Reason}`

#### List Pattern
- **Syntax**: `[Pattern1, Pattern2, ...]`
- **Description**: Matches a list with elements matching each pattern.
- **Example**: `[X, Y, Z]`, `[Head | Tail]`

#### Cons Pattern
- **Syntax**: `[Pattern1 | Pattern2]`
- **Description**: Matches a list where `Pattern1` is the head and `Pattern2` is the tail.
- **Example**: `[X | Rest]`, `[First, Second | Others]`

#### Binary Pattern
- **Syntax**: `<<Pattern1, Pattern2, ...>>`
- **Description**: Matches a binary with segments matching each pattern.
- **Example**: `<<Size:8, Data:Size/binary>>`

---

## Data Structures

### Tuples
- **Syntax**: `{Expr1, Expr2, ...}`
- **Description**: Fixed-size collections of elements.
- **Example**: `{ok, 42}`, `{error, "not_found"}`

### Lists
- **Syntax**: `[Expr1, Expr2, ...]`
- **Description**: Linked lists of elements.
- **Example**: `[1, 2, 3]`, `[a, b, c]`

### Maps
- **Syntax**: `#{Key1 => Value1, Key2 => Value2, ...}`
- **Description**: Key-value collections.
- **Example**: `#{name => "Alice", age => 30}`

### Binaries
- **Syntax**: `<<Expr1, Expr2, ...>>`
- **Description**: Binary data structures.
- **Example**: `<<1, 2, 3>>`, `<<Size:8, Data:Size/binary>>`

---

## Operators

### Arithmetic Operators
- **Operators**: `+`, `-`, `*`, `/`, `div`, `rem`
- **Description**: Perform arithmetic operations.
- **Examples**:
  - `X + Y`: Addition
  - `X - Y`: Subtraction
  - `X * Y`: Multiplication
  - `X / Y`: Division
  - `X div Y`: Integer division
  - `X rem Y`: Remainder

### Comparison Operators
- **Operators**: `==`, `/=`, `=<`, `<`, `>=`, `>`, `=:=`, `=/=`
- **Description**: Compare values.
- **Examples**:
  - `X == Y`: Equal (with type coercion)
  - `X =:= Y`: Exactly equal (no type coercion)
  - `X /= Y`: Not equal
  - `X =/= Y`: Not exactly equal
  - `X < Y`: Less than
  - `X =< Y`: Less than or equal
  - `X > Y`: Greater than
  - `X >= Y`: Greater than or equal

### Boolean Operators
- **Operators**: `and`, `or`, `not`, `xor`
- **Description**: Logical operations.
- **Examples**:
  - `X and Y`: Logical AND
  - `X or Y`: Logical OR
  - `not X`: Logical NOT
  - `X xor Y`: Exclusive OR

### Bitwise Operators
- **Operators**: `band`, `bor`, `bxor`, `bnot`, `bsl`, `bsr`
- **Description**: Bitwise operations.
- **Examples**:
  - `X band Y`: Bitwise AND
  - `X bor Y`: Bitwise OR
  - `X bxor Y`: Bitwise XOR
  - `bnot X`: Bitwise NOT
  - `X bsl Y`: Bit shift left
  - `X bsr Y`: Bit shift right

---

## Exception Handling

### Try-Catch Expression
- **Syntax**: `try Expr of [Clause1, Clause2, ...] catch [Clause3, Clause4, ...] end`
- **Description**: Evaluates an expression and handles exceptions using catch clauses.
- **Parameters**:
  - `Expr`: Expression to evaluate
  - `[Clause1, Clause2, ...]`: Clauses for successful evaluation
  - `[Clause3, Clause4, ...]`: Clauses for handling exceptions
- **Example**:
  ```erlang
  try risky_function() of
    {ok, Result} -> Result
  catch
    error:Reason -> handle_error(Reason);
    throw:Value -> handle_throw(Value)
  end
  ```

### Throw Expression
- **Syntax**: `throw Expr`
- **Description**: Throws an exception that can be caught.
- **Parameters**:
  - `Expr`: Expression to throw
- **Example**: `throw {error, "invalid_input"}`

---

## Process Communication

### Receive Expression
- **Syntax**: `receive [Clause1, Clause2, ...] after Expr -> Expr end`
- **Description**: Waits for messages matching specified patterns, with an optional timeout.
- **Parameters**:
  - `[Clause1, Clause2, ...]`: List of clauses for pattern matching messages
  - `Expr`: Timeout expression
  - `Expr`: Expression to evaluate if timeout occurs
- **Clause Format**: `Pattern when Guard -> Body`
- **Example**:
  ```erlang
  receive
    {msg, Content} -> handle_msg(Content);
    {stop, Reason} -> stop(Reason)
  after
    5000 -> timeout
  end
  ```

### Send Expression
- **Syntax**: `Expr1 ! Expr2`
- **Description**: Sends a message to a process.
- **Parameters**:
  - `Expr1`: Process identifier
  - `Expr2`: Message to send
- **Example**: `Pid ! {msg, "hello"}`

### Spawn Expression
- **Syntax**: `spawn(Module, Function, [Arg1, Arg2, ...])`
- **Description**: Creates a new process.
- **Parameters**:
  - `Module`: Module containing the function
  - `Function`: Function to execute
  - `[Arg1, Arg2, ...]`: Arguments for the function
- **Example**: `spawn(my_module, worker, [Config])`

---

## Built-in Functions (BIFs)

### Call to BIF
- **Syntax**: `call Module:Function([Arg1, Arg2, ...])`
- **Description**: Calls a built-in function from a specified module.
- **Parameters**:
  - `Module`: Module containing the function
  - `Function`: Function name
  - `[Arg1, Arg2, ...]`: List of arguments
- **Common BIFs**:
  - `call erlang:length([1, 2, 3])`: Get list length
  - `call erlang:element(2, {a, b, c})`: Get tuple element
  - `call erlang:setelement(2, {a, b, c}, new)`: Set tuple element
  - `call erlang:make_tuple(3, 0)`: Create tuple
  - `call erlang:list_to_atom("hello")`: Convert list to atom
  - `call erlang:atom_to_list('hello')`: Convert atom to list

### Primitive Operations
- **Syntax**: `primop Name([Arg1, Arg2, ...])`
- **Description**: Represents low-level operations.
- **Parameters**:
  - `Name`: Name of the primitive operation
  - `[Arg1, Arg2, ...]`: List of arguments
- **Example**: `primop make_fun([Module, Function, Arity])`

---

## Guards

### Guard Sequences
- **Syntax**: `Guard1; Guard2; ...; GuardN`
- **Description**: A sequence of guards; succeeds if at least one guard succeeds.
- **Example**: `X > 0; X < 0; X == 0`

### Guard Expressions
- **Syntax**: `Expr1, Expr2, ..., ExprN`
- **Description**: A conjunction of expressions; succeeds if all expressions evaluate to true.
- **Example**: `X > 0, X < 100`

### Guard Functions
- **Description**: Special functions that can be used in guards.
- **Common Guard Functions**:
  - `is_atom(X)`: Check if X is an atom
  - `is_integer(X)`: Check if X is an integer
  - `is_float(X)`: Check if X is a float
  - `is_list(X)`: Check if X is a list
  - `is_tuple(X)`: Check if X is a tuple
  - `is_binary(X)`: Check if X is a binary
  - `is_function(X)`: Check if X is a function
  - `is_pid(X)`: Check if X is a process identifier
  - `is_reference(X)`: Check if X is a reference
  - `is_port(X)`: Check if X is a port

---

## Annotations and Metadata

### Expression Annotations
- **Syntax**: `Expr : Annotation`
- **Description**: Attaches metadata to expressions.
- **Parameters**:
  - `Expr`: The expression to annotate
  - `Annotation`: The annotation metadata
- **Example**: `X : {type, integer}`

### Type Annotations
- **Common Types**:
  - `{type, integer}`: Integer type
  - `{type, float}`: Float type
  - `{type, atom}`: Atom type
  - `{type, list}`: List type
  - `{type, tuple}`: Tuple type
  - `{type, binary}`: Binary type
  - `{type, function}`: Function type

---

## Comments

### Single-Line Comment
- **Syntax**: `% Comment`
- **Description**: A comment extending to the end of the line.
- **Example**: `% This is a comment`

### Multi-Line Comment
- **Syntax**: `%{ Comment %}`
- **Description**: A comment that can span multiple lines.
- **Example**:
  ```erlang
  %{
  This is a
  multi-line comment
  %}
  ```

---

## File Operations

### Include Declaration
- **Syntax**: `include "File"`
- **Description**: Includes another Core Erlang source file.
- **Parameters**:
  - `File`: File name to include
- **Example**: `include "common.hrl"`

---

This reference provides a comprehensive overview of Core Erlang constructs, facilitating the development and analysis of Erlang programs at the intermediate representation level. Core Erlang serves as a crucial bridge between high-level Erlang code and the BEAM virtual machine's bytecode, enabling advanced optimizations and transformations during the compilation process.
