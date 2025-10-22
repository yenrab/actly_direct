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
- Atoms: @taskId, @user, @processID, @true, @false
- Tuples: {@user, "bob"}, {@user,@bob}, {@done, computation}, {@ok,PhoneDictionary,AddressDictionary,@idaho} (tuples of with any number of elements can have any combination of variables and atoms in any order)

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
**Variable Declaration** Tag <some_value> with <some_label>
**Peak** Peak into <data structure> and tag<some tag or tags> 
**Message** Send <message> to <name>
**State Update** Keep <value>
**Write to Physical Storage** store <value> in <name>
**FindIn** FindIn <data structure> using <operation>
**Zip** Zip <data structure1> with <data structure2> using <operation>
**Unzip** Unzip <data structure> into <structure1> and <structure2>
**Tuple Decomposition** match <data> and tag <tuple pattern>

##Match Template and Translation

###Conditional Branching
**Purpose**: Pattern matching to decide in a case-like manner what is to be done when a match occures. Matches must be makeable between types, values, and have a catch-all available as well.

**Programmer Input**

####Exact Value Matching
**Actly Form**: `match <value> with <exact_value>`
**Examples**:
- `match status with @active`
- `match count with 0`
- `match message with @timeout`

####Pattern Matching with Guards
**Actly Form**: `match <pattern> with <value> when <condition>`
**Examples**:
- `match {status, count} with {Status, Count} when Count > 0`
- `match message with {@vote, Data} when Data /= null`
- `match state with @processing when timeout < 5000`

####Type-Based Matching
**Actly Form**: `match <value> as <type>`
**Examples**:
- `match data as String`
- `match result as Number`
- `match payload as Dictionary`

####Catch-All Matching
**Actly Form**: `otherwise`
**Examples**:
- `otherwise`

**Core Erlang Translation**

####Exact Value Matching
**Actly Form**: `check if <value> is <exact_value> then <action> otherwise <default_action>`
**Core Erlang Translation**:
```erlang
case <value> of
  <exact_value> ->
    % Handle exact match
    Result;
  _ ->
    % Handle no match
    DefaultResult
end
```

####Pattern Matching with Guards
**Actly Form**: `check if <value> is <pattern> and <condition> then <action> otherwise <default_action>`
**Core Erlang Translation**:
```erlang
case <value> of
  <pattern> when <condition> ->
    % Handle pattern match with guard
    Result;
  _ ->
    % Handle no match
    DefaultResult
end
```

####Type-Based Matching
**Actly Form**: `check if <value> is a <type> then <action> otherwise <default_action>`
**Core Erlang Translation**:
```erlang
case <value> of
  <type> ->
    % Handle type match
    Result;
  _ ->
    % Handle type mismatch
    DefaultResult
end
```

####Complex Multi-Pattern Matching
**Actly Form**: 
```
check if <value> is <pattern1> then <action1>
check if <value> is <pattern2> then <action2>
check if <value> is <pattern3> then <action3>
otherwise <default_action>
```
**Core Erlang Translation**:
```erlang
case <value> of
  <pattern1> ->
    % Handle pattern1
    Result1;
  <pattern2> ->
    % Handle pattern2
    Result2;
  <pattern3> ->
    % Handle pattern3
    Result3;
  _ ->
    % Handle no match
    DefaultResult
end
```

####Nested Pattern Matching
**Actly Form**: `check if <value> is {<pattern1>, <pattern2>} and <condition> then <action> otherwise <default_action>`
**Core Erlang Translation**:
```erlang
case <value> of
  {<pattern1>, <pattern2>} when <condition> ->
    % Handle nested pattern match
    Result;
  _ ->
    % Handle no match
    DefaultResult
end
```







###Process Type Scaffolding Templates

Each Actly process type maps to specific OTP behaviors and Core Erlang patterns. When translating user descriptions, identify the process type and use the corresponding template.

####Responder Process Template
**Purpose**: Stateful process handling call-type requests
**OTP Behavior**: gen_server
**Core Erlang Pattern**:
```erlang
module responder_module
  export [start_link/1, init/1, handle_call/3, handle_cast/2, handle_info/2, terminate/2, code_change/3]
  
  function start_link/1 = fun (Args) ->
    gen_server:start_link({local, responder_name}, responder_module, Args, [])
  end
  
  function init/1 = fun (Args) ->
    InitialState = {state_data},
    {ok, InitialState}
  end
  
  function handle_call/3 = fun (Request, From, State) ->
    case Request of
      {message_tag, Data} ->
        % Process request using behavior templates
        Result = process_request(Data, State),
        {reply, Result, NewState};
      _ ->
        {reply, {error, unknown_request}, State}
    end
  end
  
  function handle_cast/2 = fun (Msg, State) ->
    {noreply, State}
  end
  
  function handle_info/2 = fun (Info, State) ->
    {noreply, State}
  end
  
  function terminate/2 = fun (Reason, State) ->
    ok
  end
  
  function code_change/3 = fun (OldVsn, State, Extra) ->
    {ok, State}
  end
end
```

####Doer Process Template
**Purpose**: Stateful process handling cast-type requests
**OTP Behavior**: gen_server
**Core Erlang Pattern**:
```erlang
module doer_module
  export [start_link/1, init/1, handle_call/3, handle_cast/2, handle_info/2, terminate/2, code_change/3]
  
  function start_link/1 = fun (Args) ->
    gen_server:start_link({local, doer_name}, doer_module, Args, [])
  end
  
  function init/1 = fun (Args) ->
    InitialState = {state_data},
    {ok, InitialState}
  end
  
  function handle_call/3 = fun (Request, From, State) ->
    {reply, {error, not_supported}, State}
  end
  
  function handle_cast/2 = fun (Request, State) ->
    case Request of
      {message_tag, Data} ->
        % Process request using behavior templates
        NewState = process_request(Data, State),
        {noreply, NewState};
      _ ->
        {noreply, State}
    end
  end
  
  function handle_info/2 = fun (Info, State) ->
    {noreply, State}
  end
  
  function terminate/2 = fun (Reason, State) ->
    ok
  end
  
  function code_change/3 = fun (OldVsn, State, Extra) ->
    {ok, State}
  end
end
```

####Morpher Process Template
**Purpose**: State machine process for physical or imaginary machines
**OTP Behavior**: gen_statem
**Core Erlang Pattern**:
```erlang
module morpher_module
  export [start_link/1, init/1, callback_mode/0, handle_event/4, terminate/3, code_change/4]
  
  function start_link/1 = fun (Args) ->
    gen_statem:start_link({local, morpher_name}, morpher_module, Args, [])
  end
  
  function init/1 = fun (Args) ->
    InitialState = @initial_state,
    InitialData = {state_data},
    {ok, InitialState, InitialData}
  end
  
  function callback_mode/0 = fun () ->
    state_functions
  end
  
  function handle_event/4 = fun (EventType, EventContent, CurrentState, Data) ->
    case {EventType, EventContent, CurrentState} of
      {call, From, {message_tag, Data}} ->
        % State machine logic using behavior templates
        {next_state, NewState, NewData, {reply, From, Result}};
      {cast, {message_tag, Data}} ->
        % State machine logic using behavior templates
        {next_state, NewState, NewData};
      _ ->
        {keep_state, Data}
    end
  end
  
  function terminate/3 = fun (Reason, State, Data) ->
    ok
  end
  
  function code_change/4 = fun (OldVsn, State, Data, Extra) ->
    {ok, State, Data}
  end
end
```

####Rememberer Process Template
**Purpose**: Stateless process providing physical storage IO
**OTP Behavior**: gen_server
**Core Erlang Pattern**:
```erlang
module rememberer_module
  export [start_link/1, init/1, handle_call/3, handle_cast/2, handle_info/2, terminate/2, code_change/3]
  
  function start_link/1 = fun (Args) ->
    gen_server:start_link({local, rememberer_name}, rememberer_module, Args, [])
  end
  
  function init/1 = fun (Args) ->
    {ok, {}}
  end
  
  function handle_call/3 = fun (Request, From, State) ->
    case Request of
      {store, Key, Value} ->
        % File I/O operations
        Result = file:write_file(Key, Value),
        {reply, Result, State};
      {retrieve, Key} ->
        % File I/O operations
        Result = file:read_file(Key),
        {reply, Result, State};
      _ ->
        {reply, {error, unknown_request}, State}
    end
  end
  
  function handle_cast/2 = fun (Msg, State) ->
    {noreply, State}
  end
  
  function handle_info/2 = fun (Info, State) ->
    {noreply, State}
  end
  
  function terminate/2 = fun (Reason, State) ->
    ok
  end
  
  function code_change/3 = fun (OldVsn, State, Extra) ->
    {ok, State}
  end
end
```

####Watcher Process Template
**Purpose**: Process that watches other processes and restarts them on failure
**OTP Behavior**: supervisor
**Core Erlang Pattern**:
```erlang
module watcher_module
  export [start_link/1, init/1]
  
  function start_link/1 = fun (Args) ->
    supervisor:start_link({local, watcher_name}, watcher_module, Args)
  end
  
  function init/1 = fun (Args) ->
    ChildSpecs = [
      {child_id, {child_module, start_link, [Args]}, permanent, 5000, worker, [child_module]}
    ],
    {ok, {{one_for_one, 5, 10}, ChildSpecs}}
  end
end
```

####Bridger Process Template
**Purpose**: Process that sends messages to external hardware
**OTP Behavior**: gen_server
**Core Erlang Pattern**:
```erlang
module bridger_module
  export [start_link/1, init/1, handle_call/3, handle_cast/2, handle_info/2, terminate/2, code_change/3]
  
  function start_link/1 = fun (Args) ->
    gen_server:start_link({local, bridger_name}, bridger_module, Args, [])
  end
  
  function init/1 = fun (Args) ->
    % Initialize hardware connection
    {ok, HardwareState}
  end
  
  function handle_call/3 = fun (Request, From, State) ->
    case Request of
      {send_to_hardware, Message} ->
        % Hardware communication logic
        Result = send_hardware_message(Message, State),
        {reply, Result, State};
      _ ->
        {reply, {error, unknown_request}, State}
    end
  end
  
  function handle_cast/2 = fun (Msg, State) ->
    {noreply, State}
  end
  
  function handle_info/2 = fun (Info, State) ->
    {noreply, State}
  end
  
  function terminate/2 = fun (Reason, State) ->
    % Cleanup hardware connection
    ok
  end
  
  function code_change/3 = fun (OldVsn, State, Extra) ->
    {ok, State}
  end
end
```

####Trigger Process Template
**Purpose**: Proactive process responding to external events
**OTP Behavior**: gen_server
**Core Erlang Pattern**:
```erlang
module trigger_module
  export [start_link/1, init/1, handle_call/3, handle_cast/2, handle_info/2, terminate/2, code_change/3]
  
  function start_link/1 = fun (Args) ->
    gen_server:start_link({local, trigger_name}, trigger_module, Args, [])
  end
  
  function init/1 = fun (Args) ->
    % Set up event listeners
    {ok, EventState}
  end
  
  function handle_call/3 = fun (Request, From, State) ->
    {reply, ok, State}
  end
  
  function handle_cast/2 = fun (Msg, State) ->
    {noreply, State}
  end
  
  function handle_info/2 = fun (Event, State) ->
    case Event of
      {external_event, EventData} ->
        % Process external event and initiate workflows
        NewState = process_external_event(EventData, State),
        {noreply, NewState};
      _ ->
        {noreply, State}
    end
  end
  
  function terminate/2 = fun (Reason, State) ->
    % Cleanup event listeners
    ok
  end
  
  function code_change/3 = fun (OldVsn, State, Extra) ->
    {ok, State}
  end
end
```

###Message Protocol Translation Rules

Protocol declarations map to gen_server callback signatures and message handling patterns.

**Protocol Declaration**:
```actly
protocol ActorName:
    messages:
        messageName: {InputTypes} -> {OutputTypes}
```

**Core Erlang Translation**:
```erlang
% In handle_call/3 or handle_cast/2:
case Request of
  {messageName, InputData} ->
    % Extract input types using pattern matching
    {InputType1, InputType2} = InputData,
    % Process using behavior templates
    Result = process_message(InputData, State),
    % Return output types
    {reply, {OutputType1, OutputType2}, NewState};
  _ ->
    {reply, {error, unknown_message}, State}
end
```

###Behavior Template → Core Erlang Mappings

####Map Template
**Actly Form**: `Change All <data structure> by <calculation>`
**Core Erlang Translation**:
```erlang
let
  ProcessedData = lists:map(fun (Item) -> <calculation> end, <data structure>)
in
  ProcessedData
```

####Filter Template
**Actly Form**: `Pick From <data structure> with <calculation>`
**Core Erlang Translation**:
```erlang
let
  FilteredData = [Item || Item <- <data structure>, <calculation>]
in
  FilteredData
```

####Reduce Template
**Actly Form**: `Combine <data structure> into <result> using <calculation>`
**Core Erlang Translation**:
```erlang
let
  Result = lists:foldl(fun (Item, Acc) -> <calculation> end, InitialValue, <data structure>)
in
  Result
```

####For Each Template
**Actly Form**: `With Each of the <data structure> <calculation>`
**Core Erlang Translation**:
```erlang
let
  _ = lists:foreach(fun (Item) -> <calculation> end, <data structure>)
in
  ok
```

####Conditional Match Branching
**Actly Form**: `match-pattern <some value> <other value>`
**Core Erlang Translation**:
```erlang
case <some value> of
  <other value> ->
    % Handle match
    Result;
  _ ->
    % Handle no match
    DefaultResult
end
```

####Conditional Comparison Branching
**Actly Form**: `match-condition <some value> <comparison>`
**Core Erlang Translation**:
```erlang
if
  <comparison> ->
    % Handle condition true
    Result;
  true ->
    % Handle condition false
    DefaultResult
end
```

####Conditional Default Branching
**Actly Form**: `match-default <some value>`
**Core Erlang Translation**:
```erlang
% Used as final clause in case/if expressions
_ ->
  % Handle default case
  DefaultResult
```

####Variable Declaration
**Actly Form**: `Tag <some_value> with <some_label>`
**Core Erlang Translation**:
```erlang
let
  <some_label> = <some_value>
in
  % Use <some_label> in subsequent expressions
```

####Peek Template
**Actly Form**: `Peek into <data structure> and tag <some tag or tags>`
**Core Erlang Translation**:
```erlang
let
  <some_tag> = <data structure>
in
  % Use <some_tag> without modifying original structure
```

####Message Template
**Actly Form**: `Send <message> to <name>`
**Core Erlang Translation**:
```erlang
let
  _ = <name> ! <message>
in
  ok
```

####State Update Template
**Actly Form**: `Keep <value>`
**Core Erlang Translation**:
```erlang
% In gen_server callbacks, return new state:
{reply, Result, <value>}  % for handle_call
{noreply, <value>}         % for handle_cast
```

####Write to Physical Storage Template
**Actly Form**: `store <value> in <name>`
**Core Erlang Translation**:
```erlang
let
  Result = file:write_file(<name>, <value>)
in
  Result
```

####FindIn Template
**Actly Form**: `FindIn <data structure> using <operation>`
**Core Erlang Translation**:
```erlang
let
  Result = lists:search(fun (Item) -> <operation> end, <data structure>)
in
  Result
```

####Zip Template
**Actly Form**: `Zip <data structure1> with <data structure2> using <operation>`
**Core Erlang Translation**:
```erlang
let
  Zipped = lists:zipwith(fun (Item1, Item2) -> <operation> end, <data structure1>, <data structure2>)
in
  Zipped
```

####Unzip Template
**Actly Form**: `Unzip <data structure> into <structure1> and <structure2>`
**Core Erlang Translation**:
```erlang
let
  {<structure1>, <structure2>} = lists:unzip(<data structure>)
in
  {<structure1>, <structure2>}
```

####Tuple Decomposition Template
**Actly Form**: `match <data> and tag <tuple pattern>`
**Core Erlang Translation**:
```erlang
let
  <tuple_pattern> = <data>
in
  % Use decomposed tuple elements
```

###Complete Translation Examples

####Example 1: Responder with Conditional Logic
**User Description**: "Create a vote counter that receives votes, counts them, and responds with consensus when threshold is reached"

**Identified Elements**:
- Process Type: Responder (handles call requests)
- Behavior Templates: Variable Declaration, Conditional Comparison, State Update, Message

**Generated Core Erlang**:
```erlang
module vote_counter
  export [start_link/1, init/1, handle_call/3, handle_cast/2, handle_info/2, terminate/2, code_change/3]
  
  function start_link/1 = fun (Args) ->
    gen_server:start_link({local, vote_counter}, vote_counter, Args, [])
  end
  
  function init/1 = fun (Args) ->
    InitialState = {0, 0}, % {vote_count, threshold}
    {ok, InitialState}
  end
  
  function handle_call/3 = fun (Request, From, State) ->
    case Request of
      {vote, VoteData} ->
        let
          {CurrentCount, Threshold} = State,
          NewCount = CurrentCount + 1
        in
          if
            NewCount >= Threshold ->
              let
                Result = {consensus, NewCount}
              in
                {reply, Result, {NewCount, Threshold}};
            true ->
              let
                Result = {pending, NewCount, Threshold}
              in
                {reply, Result, {NewCount, Threshold}}
          end;
      _ ->
        {reply, {error, unknown_request}, State}
    end
  end
  
  function handle_cast/2 = fun (Msg, State) ->
    {noreply, State}
  end
  
  function handle_info/2 = fun (Info, State) ->
    {noreply, State}
  end
  
  function terminate/2 = fun (Reason, State) ->
    ok
  end
  
  function code_change/3 = fun (OldVsn, State, Extra) ->
    {ok, State}
  end
end
```

####Example 2: Morpher State Machine
**User Description**: "Create a door lock that starts locked, unlocks when key is provided, and locks when timeout occurs"

**Identified Elements**:
- Process Type: Morpher (state machine)
- Behavior Templates: State Update, Conditional Match, Message

**Generated Core Erlang**:
```erlang
module door_lock
  export [start_link/1, init/1, callback_mode/0, handle_event/4, terminate/3, code_change/4]
  
  function start_link/1 = fun (Args) ->
    gen_statem:start_link({local, door_lock}, door_lock, Args, [])
  end
  
  function init/1 = fun (Args) ->
    InitialState = @locked,
    InitialData = {},
    {ok, InitialState, InitialData}
  end
  
  function callback_mode/0 = fun () ->
    state_functions
  end
  
  function handle_event/4 = fun (EventType, EventContent, CurrentState, Data) ->
    case {EventType, EventContent, CurrentState} of
      {call, From, {key, KeyData}} when CurrentState =:= @locked ->
        let
          Result = {unlocked, KeyData}
        in
          {next_state, @unlocked, Data, {reply, From, Result}};
      {call, From, {timeout}} when CurrentState =:= @unlocked ->
        let
          Result = {locked}
        in
          {next_state, @locked, Data, {reply, From, Result}};
      _ ->
        {keep_state, Data}
    end
  end
  
  function terminate/3 = fun (Reason, State, Data) ->
    ok
  end
  
  function code_change/4 = fun (OldVsn, State, Data, Extra) ->
    {ok, State, Data}
  end
end
```

###LLM Translation Process

When translating user descriptions to Core Erlang, follow this step-by-step process:

####1. Process Analysis Phase
- **Identify process type** from user description keywords
- **Extract message protocols** from communication patterns
- **Identify behavior templates** used in the description
- **If ambiguous**: Ask clarifying questions about process type, state requirements, or message flows

####2. Scaffolding Phase
- **Generate module structure** based on identified process type
- **Set up OTP behavior callbacks** using appropriate template
- **Initialize state structure** based on requirements
- **If uncertain about state structure**: Provide options for different state organizations

####3. Behavior Translation Phase
- **Replace behavior templates** with Core Erlang equivalents
- **Handle variable scoping** with let expressions
- **Ensure proper pattern matching** in case expressions
- **If multiple valid approaches exist**: Present options with tradeoffs

####4. Integration Phase
- **Connect message handling** to behaviors
- **Ensure state flow correctness** in callbacks
- **Add error handling patterns** for robustness
- **If error handling strategy unclear**: Ask about desired error behavior

####5. Clarification Protocol
- **When uncertain about user intent**: Pause code generation
- **Ask direct, specific questions** (max 2-3 at a time)
- **Provide concrete options** when multiple valid interpretations exist
- **Explain implications** of different choices when relevant

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
