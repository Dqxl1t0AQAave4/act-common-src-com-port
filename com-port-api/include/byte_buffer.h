#pragma once

#include <vector>

/*
 * Inspired by the `java.nio.ByteBuffer`.
 * 
 * See it and usage examples for more details.
 */

class byte_buffer
{

private:

    std::vector<char> _data0; // to simplify implementation... ugly, but...
    std::vector<char> _data;

    std::size_t _position;
    std::size_t _limit;

    char * tmp()
    {
        return this->_data0.data();
    }

public:

    /**
     *	Creates new byte buffer with the following parameters:
     *	
     *	```
     *	position = 0
     *	limit    = initial
     *	capacity = initial
     *	```
     */
    byte_buffer(std::size_t initial = 0)
        : _position(0)
        , _limit(initial)
        , _data(initial)
        , _data0(initial)
    {
    }

    /**
     *	Returns a pointer to the backing byte array
     *	with `capacity` elements.
     */
    char * buffer()
    {
        return this->_data.data();
    }

    /**
     *	Returns a pointer to the `position`-th element
     *	of the backing byte array. The resulting array
     *	will then have `capacity - position` size.
     *	
     *	Equivalent of `buffer() + position()`.
     */
    char * data()
    {
        return buffer() + position();
    }

    /**
     *	Performs the following:
     *	
     *	```
     *	limit    = position
     *	position = 0
     *	```
     */
    byte_buffer & flip()
    {
        limit(position()).position(0);
        return *this;
    }
    
    /**
     *	Returns the current position.
     */
    std::size_t position() const
    {
        return this->_position;
    }

    /**
     *	Sets the current position.
     *	
     *	The function does not perform any assertions.
     */
    byte_buffer & position(std::size_t new_position)
    {
        this->_position = new_position;
        return *this;
    }

    /**
     *	Increments the current position.
     *	
     *	The function does not perform any assertions.
     */
    byte_buffer & increase_position(std::size_t increment)
    {
        position(position() + increment);
        return *this;
    }

    /**
     *	Returns the current capacity of the buffer.
     */
    std::size_t capacity() const
    {
        return this->_data.size();
    }

    /**
     *	Sets the new capacity of the buffer.
     *	
     *	The function does not perform any assertions.
     */
    byte_buffer & capacity(std::size_t new_capacity)
    {
        this->_data.resize(new_capacity);
        this->_data0.resize(new_capacity);
        return *this;
    }
    
    /**
     *	Returns the amount of free space in the buffer.
     *	
     *	Equivalent of `limit() - position()`.
     */
    std::size_t remaining() const
    {
        return limit() - position();
    }
        
    /**
     *	Returns the current limit position.
     */
    std::size_t limit() const
    {
        return this->_limit;
    }
        
    /**
     *	Sets the current limit position.
     *
     *	The function does not perform any assertions.
     */
    byte_buffer & limit(std::size_t new_limit)
    {
        this->_limit = new_limit;
        return *this;
    }

    /**
     *	Sets the buffer position to 0.
     */
    byte_buffer & clear()
    {
        position(0);
        return *this;
    }

    /**
     *	Sets the buffer position to 0
     *	and limit co capacity.
     *	
     *	```
     *	position = 0
     *	limit    = capacity
     *	```
     */
    byte_buffer & reset()
    {
        position(0).limit(capacity());
        return *this;
    }

    /**
     *	Compacts this buffer.
     *	
     *	Moves all the unprocessed data (between `position`
     *	(inclusive) and `limit` (exclusive)) to the begin
     *	of the buffer.
     *	
     *	Sets the current `position` to `remains` and `limit`
     *	to `capacity`:
     *	
     *	```
     *	data[position, limit] -> data[0, position]
     *	
     *	position = remains
     *	limit = capacity
     *	```
     */
    byte_buffer & compact()
    {
        std::size_t remains = remaining();
        if (remains == 0)
        {
            position(0).limit(capacity());
            return *this;
        }
        memcpy_s(tmp(), capacity(), data(), remains);
        memcpy_s(buffer(), capacity(), tmp(), remains);
        position(remains).limit(capacity());
        return *this;
    }

    /**
     *  Inserts up to `size` bytes to this
     *  buffer and increases its `position` to the
     *  number of bytes actually inserted.
     *  
     *	Returns the number of unprocessed bytes
     *	remaining in the `in` array.
     *	
     *	```
     *	n = min(remaining, size);
     *	
     *	data[0, n] <- in[0, n]
     *	position += n
     *	
     *	return (size - n)
     *	```
     */
    std::size_t put(const char *in, std::size_t size)
    {
        std::size_t new_position = position() + size;
        std::size_t remains = remaining();
        if (new_position > limit())
        {
            if (remains != 0)
            {
                memcpy_s(data(), remains, in, remains);
                position(limit());
                return size - remains;
            }
            else
            {
                return size;
            }
        }
        else
        {
            memcpy_s(data(), remains, in, size);
            increase_position(size);
            return 0;
        }
    }
    
    /**
     *  Takes up to `size` bytes from this
     *  buffer and increases its `position` to the
     *  number of bytes actually taken.
     *  
     *	Returns the number of unprocessed bytes
     *	remaining in the `out` array.
     *	
     *	```
     *	n = min(remaining, size);
     *	
     *	data[0, n] -> out[0, n]
     *	position += n
     *	
     *	return (size - n)
     *	```
     */
    std::size_t get(char *out, std::size_t size)
    {
        std::size_t new_position = this->_position + size;
        std::size_t remains = remaining();
        if (new_position > limit())
        {
            if (remains != 0)
            {
                memcpy_s(out, size, data(), remains);
                position(limit());
                return size - remains;
            }
            else
            {
                return size;
            }
        }
        else
        {
            memcpy_s(out, size, data(), size);
            increase_position(size);
            return 0;
        }
    }
        
    /**
     *  Takes up to one byte from this buffer
     *  and increases its `position` to the
     *  number of bytes actually taken.
     *  
     *	Returns `true` if byte actually taken,
     *	`false` otherwise.
     *	
     *	Equivalent of `get(&byte, 1) == 0`.
     */
    bool get(char & byte)
    {
        return get(&byte, 1) == 0;
    }
};