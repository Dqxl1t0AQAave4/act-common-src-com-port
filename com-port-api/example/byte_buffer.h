#pragma once

#include <com-port-api/include/byte_buffer.h>
#include <iostream>
#include <thread>

#ifndef min
#define min(a,b) ((a)<(b))?(a):(b)
#endif

namespace {
namespace example
{

    using namespace com_port_api;

    class data_source
    {
        std::iostream *stream;

    public:

        data_source(/* ... */)
        {
            /* ... */
        }

        void open()
        {
            /* ... */
        }

        void close()
        {
            /* ... */
        }

        bool read(byte_buffer &b)
        {
            // read available amount of data from stream
            // up to `remaining` number of bytes in the buffer
            std::streamsize read_bytes = stream->readsome(b.data(), b.remaining());

            // if (!operation succeeded) return false;

            // move buffer position
            b.increase_position(static_cast<size_t>(read_bytes));
            return (read_bytes != 0);
        }

        bool write(byte_buffer &b)
        {
            // write all the data from this buffer
            stream->write(b.data(), b.remaining());

            // if (!operation succeeded) return false;

            b.increase_position(b.remaining());

            return true;
        }

    };

    void byte_buffer_reading_example()
    {
        data_source some_data_source /* = ... */;

        byte_buffer b(100);

        // read data into the buffer
        some_data_source.read(b);
        // the buffer position now indicates
        // a number of bytes actually read

        // prepare buffer for reading
        b.flip();

        // limit now is at the position
        size_t top = min(5, b.remaining());

        // read up to 5 elements
        for (size_t i = 0; i < top; i++)
        {
            std::cout << b.data()[i] << " ";
        }
        // move position
        b.increase_position(top);

        if (top == 10)
        {
            std::cout << std::endl << std::endl;

            // read remaining
            for (size_t i = 0; i < b.remaining(); i++)
            {
                std::cout << b.data()[i] << " ";
            }
            // move position
            b.increase_position(b.remaining());
        }

        // example output:
        // 
        // ```
        // 10 22 0 123 1 
        // 
        // 4 1 5 6 7 1 99 72 1 73 66 4 
        // ```
    }

    void byte_buffer_big_data_reading_example()
    {
        data_source some_data_source /* = ... */;
        data_source some_other_data_source /* = ... */;

        byte_buffer b(100);

        // read a piece of data into the buffer
        while (!b.remaining() || some_data_source.read(b))
        {
            // if we are not able to read from buffer,
            // wait until we can
            if (!b.remaining())
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                continue;
            }

            // flip the buffer to allow reading from its begin
            b.flip();

            // try to write all the data read from `some_data_source`
            some_other_data_source.write(b);

            // if any, move unprocessed data to the begin of this buffer;
            // move position to the end of unprocessed block
            b.compact();
            
            // try again
        }
    }
}
}
