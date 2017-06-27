# ********* BLOCKING / NON-BLOCKING IO API **********



# Operation overview

There are four types of operations that potentially interact with low-level API and a number of addition operations to support channel state machine.

Channel IO operations are:

1. `open`  -- opens the channel to allow reading and writing
2. `read`  -- reads data from the channel
3. `write` -- writes data to the channel
4. `close` -- closes the channel and releases all resources it holds

State machine operations are:

1. `state`  -- returns the current state of the channel
2. `wait`   -- blocks the current thread until an appropriate state is set
3. `signal` -- forces all pending `wait` operations return immediately

Parallel execution of `open`, `close` and `read`/`write` operations is not permitted. However, `read` and `write` operations may be executed in-parallel. No one IO-related operation (including `read` and `write`) may be executed in-parallel with itself.

Implementations are free to define their own operations.



# Channel flags

Channel features, such as readability, etc. may be controlled via flags on channel creation or right after it to improve channel performance or change some default behavior.

There are two default flags that must be supported by all implementations:

1. `readable` - set if the channel implements `read` operation
2. `writable` - set if the channel implements `write` operation

Implementations are free to define additional flags.



# Channel state machine overview

There are four main state bits and two supplementary state bits:

1. `open`     - set if `open` operation succeeds
2. `readable` - set if channel is ready to perform `read` operation
3. `writable` - set if channel is ready to perform `write` operation
4. `closed`   - set if `close` operation succeeded
5. `opening`  - set at `open` operation start
6. `closing`  - set at `close` operation start

Supplementary state bits are used internally to forbid parallel execution of `open`, `read`/`write` and `close` operations. They may not be used safely as targets in `wait` operation since no signaling is required to be performed on change of that bits.

The channel may be read-only, write-only or both readable and writable, depending on channel implementation and how it was created. Let `operable` be `readable` and `writable` bit combination which takes channel defaults into account. `operable` state will be used next to generalize description of some operations.

Subclasses are free to define their own state bits.



# Blocking operations


## `open` operation

Opens the channel.

Returns `false` immediately if the operation is not permitted. The operation is permitted if the channel is currently in clear state (e.g. just created or implementation-defined reset operation performed).

Otherwise sets `opening` state bit and calls to underlying API to actually open the channel. If operation completes with success, sets `open` state bit, `operable` state bits, clears `opening` state bit and returns `true. Otherwise clears `opening` state bit and throws an instance of `channel_error`.

### Effects

* Sets `opening` state bit at the start of operation if operation is permitted, clears `opening` state bit on exit (either with success or failure)
* Sets `open` state on success
* Sets `operable` state bits on success
    
### Returns

* `false` immediately if the operation is not permitted
* `true` on success
    
### Throws

* `channel_error` on failure


## `close` operation

Closes the channel.

Returns `false` immediately if the operation is not permitted. The operation is permitted if the channel is currently in `open` state and `operable` (i.e. there is no pending reading or writing).

Otherwise sets `closing` state bit, clears `open` state bit and calls to underlying API to actually close the channel. If operation completes with success, sets `closed` state bit, clears `closing` state bit and returns `true`. Otherwise clears `closing` state bit, sets `closed` state bit and throws an instance of `channel_error`.

Note, that `closed` state bit is always set after the operation completes either with success or failure. In most cases inability to close the channel indicates low-level API error, so it is much safer to just close the high-level channel and leave system (or user) to decide what to do next.

### Effects

* Sets `closing` state bit at the start of operation if operation is permitted, clears `closing` state bit on exit (either with success or failure)
* Sets `closed` state on exit (either with success or failure)
* Clears `open` state bit at the start of operation if operation is permitted
    
### Returns

* `false` immediately if the operation is not permitted
* `true` on success
    
### Throws

* `channel_error` on failure


## `read` operation

Reads available data from the channel to the given buffer.

Returns `false` immediately if the operation is not permitted. The operation is permitted if the channel is currently in `open` state and `readable`.

Otherwise clears `readable` state bit and calls to underlying API to actually read the data. If operation completes with success, restores `readable` state bit and returns `true`. Otherwise restores `readable` state bit and throws an instance of `channel_error`.

The number of bytes actually read may vary from 0 to the number of remaining bytes in passed buffer.

### Effects

* Clears `readable` state bit at the start of operation if operation is permitted, sets `readable` state bit on exit (either with success or failure)

### Returns

* `false` immediately if the operation is not permitted
* `true` on success

### Throws

* `channel_error` on failure


## `write` operation

Writes the data from the given buffer to the channel.

Returns `false` immediately if the operation is not permitted. The operation is permitted if the channel is currently in `open` state and `writable`.

Otherwise clears `writable` state bit and calls to underlying API to actually write the data. If operation completes with success, restores `writable` state bit and returns `true`. Otherwise restores `writable` state bit and throws an instance of `channel_error`.

The number of bytes actually written may vary from 0 to the number of remaining bytes in passed buffer.

### Effects

* Clears `writable` state bit at the start of operation if operation is permitted, sets `writable` state bit on exit (either with success or failure)

### Returns

* `false` immediately if the operation is not permitted
* `true` on success

### Throws

* `channel_error` on failure



# Non-Blocking operations

In case of non-blocking IO all operations return `true` on exit (either with success or failure). No exception is thrown. The passed callback functions are used to receive report of status with which non-blocking operation finishes.

If non-blocking operation finishes with success, the corresponding callback function is called to report success of operation.

If non-blocking operation finishes with failure, the corresponding callback function is called with an instance of `channel_error` to report failure of operation.

Implementations may provide additional operations and their forms to allow extended reporting. 



# State machine operations


## `state` operation

Returns the current state of the channel.


## `signal` operation

Forces all `wait` operations to return immediately.


## `wait` operation

Waits until an appropriate channel state.

The operation blocks current thread until one of the following conditions is met:

1. The channel switches its state to expected one
2. The channel switches its state to `closed`
3. The operation takes timeout argument and the specified timeout reaches
4. `signal` operation is called

In all cases except (1) the operation returns `false`.



# Implementation details


## `state_diagram` class

The class is a base class responsible for state transition calculation.

It contains two main mathods: `lock_op` and `unlock_op`. The first one calculates a state which is to be set for the channel after a certain operation starts. The last one calculates a state which is to be set for the channel after a certaint operation ends with a certain result (success or failure).

The default implmenetation, `basic_state_diagram`, for channel that implements `open-read-write-close` operations implements the following state diagrams according to above requirements:

`lock_op` diagram

| main state | modifiers | operation | flags | conditions      | result         |
|:----------:|:---------:|:---------:|:-----:|:---------------:|:--------------:|
|none        |X          |open       |X      |X                |opening         |
|none        |X          |X          |X      |X                |[not permitted] |
|opening     |X          |X          |X      |X                |[not permitted] |
|open        |r [w]      |read       |X      |X                |open [w]        |
|open        |[r] w      |write      |X      |X                |open [r]        |
|open        |[r] [w]    |close      |[R] [W]|(r\|!R)&(w\|!W)  |closing [r] [w] |
|open        |X          |X          |X      |X                |[not permitted] |
|closing     |X          |X          |X      |X                |[not permitted] |
|closed      |X          |X          |X      |X                |[not permitted] |
|X           |X          |X          |X      |X                |[not permitted] |

`unlock_op` diagram

| main state | modifiers | operation | flags | conditions      | result         |
|:----------:|:---------:|:---------:|:-----:|:---------------:|:--------------:|
|none        |X          |X          |X      |X                |[not permitted] |
|opening     |X          |open       |[R] [W]|success          |open r=R w=W    |
|opening     |X          |open       |X      |failure          |none            |
|opening     |X          |X          |X      |X                |[not permitted] |
|open        |[w]        |read       |X      |X                |open r [w]      |
|open        |[r]        |write      |X      |X                |open [r] w      |
|open        |X          |X          |X      |X                |[not permitted] |
|closing     |[r] [w]    |close      |X      |X                |closed [r] [w]  |
|closing     |X          |X          |X      |X                |[not permitted] |
|closed      |X          |X          |X      |X                |[not permitted] |
|X           |X          |X          |X      |X                |[not permitted] |

With `guarantee` result type the default implementation always returns `true` for any state transitions providing `release` guarantee type.

In default implementation all permitted state transitions require at least `acquire` (for `lock_op`) and at least `release` (for `unlock_op`) guarantee. `unlock_op` operation relies only on `started_with` state.

Listed tables above are a bit ideal. Default implementation _preserves_ all unknown state bits, so any additional bits may be used.


## `state_machine` class

The class holds `state` and actually performs state transitions calculated by `state_diagram`.

The default implementation contains two `state_machine`s:

1. `atomic_state_machine` uses `std::atomic` to interoperate with state. Does not support wait mechanism, but supports different guarantee levels by choosing an appropriate `std::memory_order` for the specific operations.
2. `blocking_state_machine` is implemented using `std::mutex` and `std::condition_variable`. Implements `acq_rel` guarantee only. Allows to wait for an appropriate state using `wait` operation semantics, described above.


## `channel_base` class

The class is a base for all channels.

Provides a generic `do_as` operation that allows to perform the given function as any operation that is known by the particular `state_diagram`, specified at channel creation time, e.g. as `open` or `close`.

Default implementation has blocking and non-blocking variants of `do_as`.

Let "`do_` operation" be any operation started with `do_as` operation, "`do_xxx` operation" -- `xxx` operation started with `do_as` operation.

`do_` operations may throw any exceptions. They are handled by `do_as` operation to ensure correct state transitions. In case of blocking `do_as` exception is just rethrown. Non-blocking variant of `do_as` passes all `channel_error` exceptions to the corresponding handler, but rethrows exceptions of other types.

Non-blocking `do_` operations shouldn't worry about state transitions at all since all necessary "external work" is passed to them in the corresponding callback. They just need to ensure the callback is executed correctly and in a proper time.

Since parallel execution of `open`, `close` and `read`/`write` operations is not permitted, and only one operation of same type may be executing in a certain moment of time, due to the use of guarantee types there is a number of guarantees and assumptions for `do_` operations in default implementation:

1. `do_open`, `do_close` and `do_read`/`do_write` cannot be executing simultaneously
2. `do_read` and `do_write` can be executing simultaneously
3. All modifications in memory performed by `do_open` are visible in all `do_` operations
4. All modifications in memory performed by `do_close` are visible in all `do_` operations
5. All modifications in memory performed by `do_read` are visible in `do_open`, `do_close` and next `do_read` operations and might not be visible in `do_write`
6. All modifications in memory performed by `do_write` are visible in `do_open`, `do_close` and next `do_write` operations and might not be visible in `do_read`
7. All modifications in memory performed before `do_open` are visible in `do_open`

Pay attention to (5) and (6).

Due to these guarantees all per-operation internal variables used by `do_` operations may be accessed without any extra synchronization. All per-implementation internal variables (used across multiple `do_` operations) may also be accessed without any extra synchronization if and only if the variables are not modified by `do_read` and read by `do_write` (and vice versa) or modified by both `do_read` and `do_write`.

Note, that to provide (7), it is required to execute `provide_guarantee` with at least `release` guarantee type on the underlying state machine.

Note, that implementations must close the channel in proper way before its destruction. It is quite recommended to call `close` in a loop in the destructor of implementing class.

