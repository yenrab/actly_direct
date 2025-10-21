# Actly Professional Actor Examples

This document contains professional-level examples demonstrating the capabilities of each actor type in the Actly language. Each example shows real-world use cases with distributed computing across computers and boards.

## Responder Examples

### 1. Distributed Consensus/Voting System
**Purpose**: Implements a distributed voting system for achieving consensus across multiple computer nodes. This actor handles vote collection, validation, and result computation in a fault-tolerant manner.

**How it works**: The actor maintains a state of votes and participants, validates incoming votes, and computes consensus when a threshold is reached. It uses pattern matching to handle different vote types and implements timeout mechanisms for incomplete votes.

```
Responder: ConsensusVoter
    State: {votes: Dictionary, participants: Set, threshold: Number, timeout: Number}
    waitFor Message
        match Message and tag {register, ParticipantId, NodeId}
            Tag ParticipantId with NodeId
            Keep {participants: participants + ParticipantId, votes: votes}
            Send {registered, ParticipantId} to NodeId
        
        match Message and tag {vote, ParticipantId, VoteValue, Timestamp}
            match VoteValue and tag {valid, true}
                Keep {votes: votes + {ParticipantId: VoteValue}}
                Tag VoteValue with Timestamp
                changeAll votes by
                    if voteCount >= threshold
                        combine votes in Result using
                            majorityVote
                        Send {consensus, Result} to all participants
                    else
                        Send {pending, voteCount, threshold} to ParticipantId
        
        match Message and tag {timeout, VoteId}
            pickFrom votes with
                VoteId == voteId and timestamp < (currentTime - timeout)
            combine votes in Result using
                timeoutHandling
            Send {timeout, VoteId} to all participants
```

### 2. Real-time Data Validation Pipeline
**Purpose**: Validates incoming data streams in real-time, performing schema validation, anomaly detection, and data quality checks. Used in data processing pipelines between boards and computers.

**How it works**: The actor receives data streams, applies validation rules using Map and Filter templates, detects anomalies through statistical analysis, and routes validated data to downstream processors.

```
Responder: DataValidator
    State: {schema: Dictionary, rules: List, anomalies: AVL Tree}
    waitFor Message
        match Message and tag {validate, DataStream, Schema}
            changeAll DataStream by
            schemaValidation
            pickFrom DataStream with
                isValid == true
            changeAll DataStream by
                anomalyDetection
            pickFrom DataStream with
                anomalyScore < threshold
            combine DataStream in ValidData using
                dataAggregation
            Send {validated, ValidData} to DataProcessor
        
        match Message and tag {updateSchema, NewSchema}
            Keep {schema: NewSchema}
            changeAll rules by
                schemaUpdate
            Send {schemaUpdated, true} to self
        
        match Message and tag {anomalyReport, AnomalyData}
            Keep {anomalies: anomalies + AnomalyData}
            findIn anomalies using
                patternMatching
            Send {anomalyDetected, AnomalyData} to AlertManager
```

### 3. Cryptographic Hash Computation Service
**Purpose**: Provides secure cryptographic hash computation services for data integrity verification. Handles multiple hash algorithms and provides collision-resistant hashing for sensitive data.

**How it works**: The actor maintains a registry of hash algorithms, computes hashes using different cryptographic functions, and provides hash verification services. It uses state to cache frequently used hashes and implements secure random number generation.

```
Responder: CryptoHasher
    State: {algorithms: Dictionary, cache: B+ Tree, salt: String}
    waitFor Message
        match Message and tag {hash, Data, Algorithm}
            Tag Data with Algorithm
            changeAll Data by
                cryptographicHash
            Keep {cache: cache + {hash: result}}
            Send {hashResult, result, Algorithm} to caller
        
        match Message and tag {verify, Data, Hash, Algorithm}
            changeAll Data by
                cryptographicHash
            match result and tag {matches, Hash}
                Send {verified, true} to caller
            match result and tag {mismatch, Hash}
                Send {verified, false, reason} to caller
        
        match Message and tag {salt, NewSalt}
            Keep {salt: NewSalt}
            changeAll cache by
                rehashWithNewSalt
            Send {saltUpdated, true} to self
```

## Doer Examples

### 1. Async Log Aggregator
**Purpose**: Collects and aggregates log data from multiple board sources, performing real-time log analysis and forwarding processed logs to computer systems. Handles high-volume log streams with minimal latency.

**How it works**: The actor continuously processes log streams using functional transformations, applies filtering and aggregation rules, and batches results for efficient transmission to computer systems.

```
Doer: LogAggregator
    State: {logBuffer: Deque, patterns: List, metrics: Dictionary}
    waitFor Message
        match Message and tag {logStream, LogData, SourceId}
            changeAll LogData by
                logParsing
            pickFrom LogData with
                severity >= warning
            changeAll LogData by
                timestampNormalization
            Keep {logBuffer: logBuffer + LogData}
            changeAll logBuffer by
                batchProcessing
            Send {aggregatedLogs, batch} to ComputerSystem
        
        match Message and tag {pattern, LogPattern}
            Keep {patterns: patterns + LogPattern}
            changeAll logBuffer by
                patternMatching
            pickFrom logBuffer with
                matchesPattern == true
            Send {patternMatch, matchedLogs} to AlertSystem
        
        match Message and tag {metrics, MetricData}
            Keep {metrics: metrics + MetricData}
            combine metrics in Summary using
                metricAggregation
            Send {metrics, Summary} to MonitoringSystem
```

### 2. Data Stream Processor
**Purpose**: Processes continuous data streams from IoT devices and sensors, applying real-time transformations, filtering, and analytics. Used in industrial IoT scenarios where boards collect sensor data and computers perform analysis.

**How it works**: The actor receives data streams, applies windowing functions, performs statistical analysis, and outputs processed data to downstream systems. It maintains sliding windows of data for temporal analysis.

```
Doer: StreamProcessor
    State: {windows: Dictionary, transformations: List, output: Deque}
    waitFor Message
        match Message and tag {stream, DataPoint, Timestamp}
            changeAll DataPoint by
                dataNormalization
            Keep {windows: updateWindow(DataPoint, Timestamp)}
            changeAll windows by
                windowProcessing
            pickFrom windows with
                windowComplete == true
            changeAll windows by
                statisticalAnalysis
            combine windows in ProcessedData using
                dataAggregation
            Send {processed, ProcessedData} to AnalyticsEngine
        
        match Message and tag {transform, TransformFunction}
            Keep {transformations: transformations + TransformFunction}
            changeAll windows by
                applyTransform
            Send {transformApplied, true} to self
        
        match Message and tag {flush, WindowId}
            pickFrom windows with
                WindowId == windowId
            changeAll windows by
                forceProcessing
            Send {flushed, ProcessedData} to OutputSystem
```

### 3. Distributed Cache Invalidation System
**Purpose**: Manages cache invalidation across distributed systems, ensuring data consistency when updates occur. Handles cache dependencies and implements invalidation strategies for different data types.

**How it works**: The actor maintains a dependency graph of cached data, tracks data modifications, and propagates invalidation messages to affected cache nodes. It uses graph algorithms to determine invalidation scope.

```
Doer: CacheInvalidator
    State: {dependencies: Dictionary, invalidationQueue: Deque, strategies: List}
    waitFor Message
        match Message and tag {invalidate, DataKey, Reason}
            findIn dependencies using
                keyLookup
            changeAll dependencies by
                dependencyTraversal
            Keep {invalidationQueue: invalidationQueue + DataKey}
            changeAll invalidationQueue by
                invalidationStrategy
            pickFrom invalidationQueue with
                priority == high
            Send {invalidate, DataKey} to CacheNode
        
        match Message and tag {dependency, ParentKey, ChildKey}
            Keep {dependencies: dependencies + {ParentKey: ChildKey}}
            changeAll dependencies by
                graphUpdate
            Send {dependencyAdded, true} to self
        
        match Message and tag {strategy, InvalidationStrategy}
            Keep {strategies: strategies + InvalidationStrategy}
            changeAll invalidationQueue by
                applyStrategy
            Send {strategyApplied, true} to self
```

## Morpher Examples

### 1. TCP Connection State Machine
**Purpose**: Manages TCP connection states for network communication between computers. Implements the complete TCP state machine with proper state transitions, error handling, and connection lifecycle management.

**How it works**: The actor transitions between TCP states (CLOSED, LISTEN, SYN_SENT, etc.) based on incoming messages, maintains connection state, and handles timeout scenarios. It uses state machine patterns for reliable network communication.

```
Morpher: TCPConnection
    State: {currentState: closed, connectionId: String, timeout: Number}
    waitFor Message
        match Message and tag {connect, RemoteAddress, Port}
            match currentState and tag {closed}
                Keep {currentState: syn_sent}
                Send {syn, connectionId} to RemoteAddress
                Send {timeout, connectionId} to self
        
        match Message and tag {syn_ack, ConnectionId}
            match currentState and tag {syn_sent}
                Keep {currentState: established}
                Send {ack, connectionId} to RemoteAddress
                Send {connected, connectionId} to Application
        
        match Message and tag {data, ConnectionId, Payload}
            match currentState and tag {established}
                Keep {currentState: established}
                Send {dataReceived, Payload} to Application
        
        match Message and tag {close, ConnectionId}
            match currentState and tag {established}
                Keep {currentState: fin_wait_1}
                Send {fin, connectionId} to RemoteAddress
        
        match Message and tag {timeout, ConnectionId}
            match currentState and tag {syn_sent}
                Keep {currentState: closed}
                Send {connectionFailed, connectionId} to Application
```

### 2. IoT Device State Machine
**Purpose**: Manages the operational state of IoT devices on boards, handling sensor data collection, power management, and communication states. Implements device lifecycle management for embedded systems.

**How it works**: The actor transitions between device states (sleep, active, collecting, transmitting) based on sensor readings, power levels, and communication requirements. It optimizes power consumption while maintaining data collection schedules.

```
Morpher: IoTDevice
    State: {deviceState: sleep, batteryLevel: Number, sensorData: List}
    waitFor Message
        match Message and tag {wake, Trigger}
            match deviceState and tag {sleep}
                Keep {deviceState: active}
                Send {sensorRead, all} to SensorArray
                Send {powerCheck, batteryLevel} to self
        
        match Message and tag {sensorData, Data, Timestamp}
            match deviceState and tag {active}
                Keep {sensorData: sensorData + Data}
                changeAll sensorData by
                    dataValidation
                pickFrom sensorData with
                    isValid == true
                Keep {deviceState: collecting}
                Send {dataCollected, sensorData} to self
        
        match Message and tag {transmit, Data}
            match deviceState and tag {collecting}
                Keep {deviceState: transmitting}
                Send {data, Data} to ComputerSystem
                match batteryLevel and tag {low}
                    Keep {deviceState: sleep}
                match batteryLevel and tag {good}
                    Keep {deviceState: active}
        
        match Message and tag {powerLow, BatteryLevel}
            Keep {deviceState: sleep}
            Send {powerSave, true} to PowerManager
```

### 3. Distributed Lock Manager
**Purpose**: Implements distributed locking mechanism for coordinating access to shared resources across multiple computer nodes. Handles lock acquisition, release, and timeout scenarios in distributed systems.

**How it works**: The actor manages lock states, implements lock hierarchies, and handles deadlock detection. It uses consensus algorithms to ensure lock consistency across distributed nodes.

```
Morpher: LockManager
    State: {locks: Dictionary, waitQueue: Deque, timeout: Number}
    waitFor Message
        match Message and tag {acquire, LockId, RequesterId, Priority}
            match locks and tag {available, LockId}
                Keep {locks: locks + {LockId: RequesterId}}
                Send {lockAcquired, LockId} to RequesterId
            match locks and tag {locked, LockId}
                Keep {waitQueue: waitQueue + {LockId: RequesterId}}
                Send {lockQueued, LockId, Position} to RequesterId
        
        match Message and tag {release, LockId, RequesterId}
            match locks and tag {locked, LockId}
                Keep {locks: locks - {LockId: RequesterId}}
                pickFrom waitQueue with
                    LockId == lockId
                changeAll waitQueue by
                    prioritySort
                Send {lockAvailable, LockId} to nextRequester
        
        match Message and tag {timeout, LockId}
            pickFrom locks with
                LockId == lockId and timestamp < (currentTime - timeout)
            Keep {locks: locks - {LockId: requesterId}}
            Send {lockTimeout, LockId} to requesterId
```

## Rememberer Examples

### 1. Time-series Database for Sensor Data
**Purpose**: Stores and retrieves time-series data from IoT sensors, providing efficient storage and querying capabilities for sensor data streams. Handles high-frequency data insertion and time-range queries.

**How it works**: The actor uses specialized data structures for time-series data, implements data compression, and provides efficient querying by time ranges. It handles data retention policies and automatic data aging.

```
Rememberer: TimeSeriesDB
    Type: B+ Tree
    State: {dataPoints: B+ Tree, retention: Number, compression: Boolean}
    waitFor Message
        match Message and tag {store, DataPoint, Timestamp, SensorId}
            Tag DataPoint with Timestamp
            Tag DataPoint with SensorId
            store DataPoint in dataPoints
            changeAll dataPoints by
                compressionCheck
            pickFrom dataPoints with
                timestamp < (currentTime - retention)
            changeAll dataPoints by
                dataAging
            Send {stored, DataPoint, Timestamp} to caller
        
        match Message and tag {query, StartTime, EndTime, SensorId}
            findIn dataPoints using
                timeRangeQuery
            pickFrom dataPoints with
                timestamp >= StartTime and timestamp <= EndTime
            changeAll dataPoints by
                dataAggregation
            combine dataPoints in Result using
                timeSeriesAggregation
            Send {queryResult, Result} to caller
        
        match Message and tag {aggregate, TimeWindow, Function}
            changeAll dataPoints by
                windowAggregation
            combine dataPoints in Result using
                Function
            Send {aggregateResult, Result} to caller
```

### 2. Distributed Key-Value Store
**Purpose**: Implements a distributed key-value store with replication and consistency guarantees. Handles data replication across multiple nodes and provides eventual consistency.

**How it works**: The actor maintains multiple replicas of data, implements replication protocols, and handles consistency checks. It uses vector clocks for conflict resolution and implements quorum-based operations.

```
Rememberer: DistributedKVStore
    Type: Dictionary
    State: {data: Dictionary, replicas: List, version: Number}
    waitFor Message
        match Message and tag {put, Key, Value, Version}
            Tag Value with Version
            store Value in data
            Keep {version: version + 1}
            changeAll replicas by
                replication
            Send {put, Key, Value, Version} to replicas
            Send {putComplete, Key, Version} to caller
        
        match Message and tag {get, Key}
            findIn data using
                keyLookup
            pickFrom data with
                Key == key
            Send {getResult, Value, Version} to caller
        
        match Message and tag {replicate, Key, Value, Version}
            match data and tag {conflict, Key}
                changeAll data by
                    conflictResolution
                combine data in Result using
                    mergeStrategy
                Keep {data: Result}
            match data and tag {consistent, Key}
                Keep {data: data + {Key: Value}}
            Send {replicationComplete, Key, Version} to self
        
        match Message and tag {consistency, Key}
            changeAll replicas by
                consistencyCheck
            combine replicas in Result using
                quorumCheck
            Send {consistencyResult, Result} to caller
```

### 3. Event Sourcing Ledger
**Purpose**: Implements event sourcing for maintaining an immutable audit trail of all system events. Provides event replay capabilities and temporal querying for system state reconstruction.

**How it works**: The actor stores events in chronological order, implements event versioning, and provides replay functionality. It maintains event metadata and supports event filtering and aggregation.

```
Rememberer: EventLedger
    Type: Finger Tree
    State: {events: Finger Tree, version: Number, metadata: Dictionary}
    waitFor Message
        match Message and tag {append, Event, EventType, Timestamp}
            Tag Event with Timestamp
            Tag Event with EventType
            store Event in events
            Keep {version: version + 1}
            changeAll events by
                eventIndexing
            Send {eventStored, Event, Version} to caller
        
        match Message and tag {replay, StartVersion, EndVersion}
            findIn events using
                versionRangeQuery
            pickFrom events with
                version >= StartVersion and version <= EndVersion
            changeAll events by
                eventReplay
            combine events in Result using
                stateReconstruction
            Send {replayResult, Result} to caller
        
        match Message and tag {query, EventType, StartTime, EndTime}
            findIn events using
                eventTypeQuery
            pickFrom events with
                EventType == eventType and timestamp >= StartTime and timestamp <= EndTime
            changeAll events by
                eventFiltering
            combine events in Result using
                eventAggregation
            Send {queryResult, Result} to caller
        
        match Message and tag {snapshot, Version}
            findIn events using
                snapshotQuery
            pickFrom events with
                version <= Version
            combine events in Result using
                snapshotCreation
            Send {snapshot, Result, Version} to caller
```

## Watcher Examples

### 1. Supervisor Tree for Fault-Tolerant Service Mesh
**Purpose**: Implements a supervisor tree pattern for managing fault-tolerant services across distributed systems. Monitors child processes, handles failures, and implements restart strategies.

**How it works**: The actor maintains a tree of supervised processes, monitors their health, and implements restart policies. It uses exponential backoff for restart delays and implements circuit breaker patterns.

```
Watcher: ServiceSupervisor
    State: {children: Dictionary, restartCount: Dictionary, policies: List}
    waitFor Message
        match Message and tag {supervise, ProcessId, RestartPolicy}
            Keep {children: children + {ProcessId: RestartPolicy}}
            Send {monitor, ProcessId} to self
            Send {supervised, ProcessId} to ProcessId
        
        match Message and tag {processFailed, ProcessId, Reason}
            findIn children using
                processLookup
            pickFrom children with
                ProcessId == processId
            changeAll children by
                restartPolicy
            match restartCount and tag {exceeded, ProcessId}
                Send {permanentFailure, ProcessId, Reason} to AlertSystem
            match restartCount and tag {within, ProcessId}
                Keep {restartCount: restartCount + {ProcessId: count + 1}}
                Send {restart, ProcessId} to ProcessManager
                Send {restartScheduled, ProcessId, Delay} to self
        
        match Message and tag {restartScheduled, ProcessId, Delay}
            changeAll children by
                exponentialBackoff
            Send {restart, ProcessId} to ProcessManager
        
        match Message and tag {healthCheck, ProcessId}
            Send {ping, ProcessId} to ProcessId
            Send {healthTimeout, ProcessId} to self
```

### 2. Board Health Monitor
**Purpose**: Monitors the health and performance of board-based systems, detecting hardware failures, resource exhaustion, and communication issues. Implements proactive monitoring and alerting.

**How it works**: The actor continuously monitors board metrics, detects anomalies, and implements restart policies. It handles communication failures between computers and boards and implements graceful degradation.

```
Watcher: BoardHealthMonitor
    State: {boards: Dictionary, metrics: Dictionary, thresholds: List}
    waitFor Message
        match Message and tag {monitor, BoardId, Metrics}
            Keep {boards: boards + {BoardId: Metrics}}
            changeAll Metrics by
                healthAnalysis
            pickFrom Metrics with
                healthScore < threshold
            Send {boardUnhealthy, BoardId, Metrics} to AlertSystem
            Send {restart, BoardId} to BoardManager
        
        match Message and tag {metrics, BoardId, MetricData}
            Keep {metrics: metrics + {BoardId: MetricData}}
            changeAll metrics by
                trendAnalysis
            pickFrom metrics with
                trend == declining
            Send {trendAlert, BoardId, Trend} to AlertSystem
        
        match Message and tag {communication, BoardId, Status}
            match Status and tag {failed, BoardId}
                Send {boardOffline, BoardId} to AlertSystem
                Send {retry, BoardId} to self
            match Status and tag {recovered, BoardId}
                Send {boardOnline, BoardId} to AlertSystem
        
        match Message and tag {retry, BoardId}
            changeAll boards by
                retryLogic
            Send {ping, BoardId} to BoardId
            Send {retryTimeout, BoardId} to self
```

### 3. Cascading Failure Prevention System
**Purpose**: Prevents cascading failures in distributed systems by monitoring system load, implementing circuit breakers, and coordinating graceful degradation. Handles overload scenarios and resource exhaustion.

**How it works**: The actor monitors system metrics, detects overload conditions, and implements circuit breaker patterns. It coordinates with other system components to prevent resource exhaustion and maintain system stability.

```
Watcher: FailurePrevention
    State: {circuits: Dictionary, load: Dictionary, thresholds: List}
    waitFor Message
        match Message and tag {monitor, SystemId, LoadMetrics}
            Keep {load: load + {SystemId: LoadMetrics}}
            changeAll LoadMetrics by
                loadAnalysis
            pickFrom LoadMetrics with
                load > threshold
            Send {overload, SystemId, LoadMetrics} to CircuitBreaker
            Send {throttle, SystemId} to SystemId
        
        match Message and tag {circuitOpen, SystemId, Reason}
            Keep {circuits: circuits + {SystemId: open}}
            Send {circuitBreaker, SystemId, open} to LoadBalancer
            Send {circuitTimeout, SystemId} to self
        
        match Message and tag {circuitTimeout, SystemId}
            changeAll circuits by
                circuitReset
            Send {circuitTest, SystemId} to SystemId
            Send {circuitTimeout, SystemId} to self
        
        match Message and tag {circuitTest, SystemId, Result}
            match Result and tag {success, SystemId}
                Keep {circuits: circuits - {SystemId: open}}
                Send {circuitClosed, SystemId} to LoadBalancer
            match Result and tag {failure, SystemId}
                Send {circuitOpen, SystemId, testFailure} to self
```

## Bridger Examples

### 1. MQTT Bridge for IoT Devices
**Purpose**: Bridges MQTT communication between IoT devices on boards and computer systems. Handles protocol translation, message routing, and connection management for IoT ecosystems.

**How it works**: The actor translates between MQTT protocol and internal message formats, manages device connections, and implements quality of service guarantees. It handles device authentication and message filtering.

```
Bridger: MQTTBridge
    State: {connections: Dictionary, topics: Set, qos: Dictionary}
    waitFor Message
        match Message and tag {connect, DeviceId, Credentials}
            changeAll Credentials by
                authentication
            match authentication and tag {valid, DeviceId}
                Keep {connections: connections + {DeviceId: connected}}
                Send {connected, DeviceId} to DeviceId
                Send {deviceOnline, DeviceId} to ComputerSystem
            match authentication and tag {invalid, DeviceId}
                Send {connectionFailed, DeviceId} to DeviceId
        
        match Message and tag {publish, Topic, Payload, QoS}
            Tag Payload with Topic
            Tag Payload with QoS
            changeAll Payload by
                messageTranslation
            Send {message, Payload, Topic} to ComputerSystem
            match QoS and tag {guaranteed, Topic}
                Send {ack, Topic} to DeviceId
        
        match Message and tag {subscribe, Topic, DeviceId}
            Keep {topics: topics + {Topic: DeviceId}}
            Send {subscribed, Topic} to DeviceId
            Send {subscription, Topic, DeviceId} to ComputerSystem
        
        match Message and tag {message, Payload, Topic}
            findIn topics using
                topicLookup
            pickFrom topics with
                Topic == topic
            Send {message, Payload} to DeviceId
```

### 2. SPI/I2C Hardware Interface
**Purpose**: Provides low-level hardware communication for board-based systems using SPI and I2C protocols. Handles device enumeration, data transfer, and error recovery for embedded systems.

**How it works**: The actor manages hardware communication protocols, handles device discovery, and implements error recovery mechanisms. It provides abstraction for hardware-specific operations and manages device state.

```
Bridger: HardwareInterface
    State: {devices: Dictionary, protocols: List, errors: Deque}
    waitFor Message
        match Message and tag {discover, Protocol}
            changeAll Protocol by
                deviceDiscovery
            pickFrom Protocol with
                deviceFound == true
            Keep {devices: devices + {DeviceId: Protocol}}
            Send {deviceFound, DeviceId, Protocol} to DeviceManager
        
        match Message and tag {read, DeviceId, Register, Length}
            changeAll DeviceId by
                hardwareRead
            pickFrom DeviceId with
                readSuccess == true
            Send {data, DeviceId, Data} to caller
            match readSuccess and tag {false, DeviceId}
                Send {error, DeviceId, readError} to ErrorHandler
        
        match Message and tag {write, DeviceId, Register, Data}
            changeAll DeviceId by
                hardwareWrite
            pickFrom DeviceId with
                writeSuccess == true
            Send {writeComplete, DeviceId} to caller
            match writeSuccess and tag {false, DeviceId}
                Send {error, DeviceId, writeError} to ErrorHandler
        
        match Message and tag {error, DeviceId, ErrorType}
            Keep {errors: errors + {DeviceId: ErrorType}}
            changeAll errors by
                errorRecovery
            pickFrom errors with
                recoverable == true
            Send {retry, DeviceId} to self
```

### 3. Multi-Protocol Gateway
**Purpose**: Implements a gateway that translates between different communication protocols used by boards and computers. Handles protocol conversion, message routing, and connection management for heterogeneous systems.

**How it works**: The actor maintains protocol mappings, translates messages between different formats, and manages connections across protocol boundaries. It implements message queuing and retry mechanisms for reliable communication.

```
Bridger: ProtocolGateway
    State: {protocols: Dictionary, mappings: Dictionary, queues: Dictionary}
    waitFor Message
        match Message and tag {translate, Message, FromProtocol, ToProtocol}
            findIn mappings using
                protocolLookup
            pickFrom mappings with
                FromProtocol == fromProtocol and ToProtocol == toProtocol
            changeAll Message by
                protocolTranslation
            Send {translated, Message, ToProtocol} to TargetSystem
        
        match Message and tag {route, Message, SourceProtocol, TargetProtocol}
            changeAll Message by
                messageRouting
            pickFrom Message with
                routable == true
            Keep {queues: queues + {Message: TargetProtocol}}
            Send {queued, Message, Position} to caller
        
        match Message and tag {deliver, Message, TargetProtocol}
            changeAll Message by
                protocolDelivery
            pickFrom Message with
                deliverySuccess == true
            Send {delivered, Message} to TargetSystem
            match deliverySuccess and tag {false, Message}
                Send {retry, Message, TargetProtocol} to self
        
        match Message and tag {retry, Message, TargetProtocol}
            changeAll Message by
                retryLogic
            pickFrom Message with
                retryCount < maxRetries
            Send {deliver, Message, TargetProtocol} to self
            match retryCount and tag {exceeded, Message}
                Send {deliveryFailed, Message} to ErrorHandler
```

## Summary

These examples demonstrate the full capabilities of the Actly language across all actor types, showing:

- **Real-world applications**: Distributed systems, IoT, networking, data processing
- **Algorithmic scenarios**: State machines, consensus algorithms, data structures
- **Distributed computing**: Computer-to-computer, board-to-board, computer-to-board, board-to-computer communication
- **Professional patterns**: Fault tolerance, monitoring, protocol translation, data management

Each example includes purpose descriptions, implementation explanations, and comprehensive code demonstrating the language's capabilities for professional software development.
