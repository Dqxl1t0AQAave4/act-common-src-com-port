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
