#pragma once

#include <vector>

/*
 * Inspired by the `java.nio.ByteBuffer`.
 */

class byte_buffer
{

private:

    std::vector<char> _data0;
    std::vector<char> _data;

    std::size_t _position;
    std::size_t _limit;

    char * tmp()
    {
        return this->_data0.data();
    }

public:

    byte_buffer(std::size_t initial = 0)
        : _position(0)
        , _limit(initial)
        , _data(initial)
        , _data0(initial)
    {
    }

    char * buffer()
    {
        return this->_data.data();
    }

    char * data()
    {
        return buffer() + position();
    }

    byte_buffer & flip()
    {
        limit(position()).position(0);
        return *this;
    }

    std::size_t position() const
    {
        return this->_position;
    }

    byte_buffer & position(std::size_t new_position)
    {
        this->_position = new_position;
        return *this;
    }

    byte_buffer & increase_position(std::size_t increment)
    {
        position(position() + increment);
        return *this;
    }

    std::size_t capacity() const
    {
        return this->_data.size();
    }

    byte_buffer & capacity(std::size_t new_capacity)
    {
        this->_data.resize(new_capacity);
        this->_data0.resize(new_capacity);
        return *this;
    }

    std::size_t remaining() const
    {
        return limit() - position();
    }

    std::size_t limit() const
    {
        return this->_limit;
    }

    byte_buffer & limit(std::size_t new_limit)
    {
        this->_limit = new_limit;
        return *this;
    }

    byte_buffer & clear()
    {
        position(0);
        return *this;
    }

    byte_buffer & reset()
    {
        position(0).limit(capacity());
        return *this;
    }

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

    bool get(char & byte)
    {
        return get(&byte, 1) == 0;
    }
};