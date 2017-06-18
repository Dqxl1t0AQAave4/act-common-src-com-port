#pragma once


#include <afxwin.h>

#include <vector>
#include <list>
#include <functional>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <exception>
#include <cassert>

#include <byte_buffer.h>
#include <com-port.h>
#include <logger.h>
#include "act-photo.h"


// ======================= worker ========================================


class worker_stopped
    : public std::runtime_error
{

public:

    worker_stopped()
        : std::runtime_error("")
    {
    }
};


class worker
{

public:

    using mutex_t = std::mutex;
    using guard_t = std::lock_guard < mutex_t > ;

private:

    using ulock_t     = std::unique_lock < mutex_t > ;
    using condition_t = std::condition_variable;

    std::thread                       worker_thread;
                                      
    mutex_t                           mutex;
    condition_t                       cv;

    // guarded by `mutex`
    bool                              working;
    com_port                          port;
    bool                              port_changed;
    std::size_t                       buffer_size;
    std::vector<act_photo::command_t> commands;

    // thread-local
    com_port                          current_port;
    byte_buffer                       buffer;

public:

    mutex_t                           queue_mutex;

    // guarded by `queue_mutex`
    std::size_t                       queue_length;
    std::list<act_photo::packet_t>    packet_queue;

public:

    worker(std::size_t buffer_size = 5000,
           std::size_t queue_length = 1000)
           : buffer(buffer_size)
           , buffer_size(buffer_size)
           , queue_length(queue_length)
    {
        worker_thread = std::thread(&worker::start, this);
    }

    ~worker()
    {
        stop();
    }

    void supply_port(com_port port)
    {
        {
            guard_t guard(mutex);
            this->port = std::move(port);
            this->port_changed = true;
        }
        cv.notify_one();
    }

    void supply_command(act_photo::command_t command)
    {
        {
            guard_t guard(mutex);
            commands.push_back(command);
        }
    }

    void supply_buffer_size(std::size_t buffer_size)
    {
        {
            guard_t guard(mutex);
            this->buffer_size = buffer_size;
        }
    }

    void supply_queue_length(std::size_t queue_length)
    {
        {
            guard_t guard(queue_mutex);
            this->queue_length = queue_length;
        }
    }

    void stop()
    {
        {
            guard_t guard(mutex);
            working = false;
        }
        cv.notify_one();
    }

    void join()
    {
        worker_thread.join();
    }

private:

    void start()
    {
        try
        {
            {
                guard_t guard(mutex);
                this->working = true;
                this->port_changed = false;
            }

            loop();
        }
        catch (const worker_stopped &)
        {
            current_port.close();
            {
                guard_t guard(mutex);
                port.close();
            }
            return;
        }
    }

    com_port & fetch_port()
    {
        ulock_t guard(mutex);
        if (port_changed)
        {
            current_port = std::move(port);
            port_changed = false;
        }
        while (!current_port.open())
        {
            cv.wait(guard, [&] { return port_changed || !working; });
            if (!working)
            {
                throw worker_stopped();
            }
            current_port = std::move(port);
            port_changed = false;
        }
        if (!working)
        {
            throw worker_stopped();
        }
        return current_port;
    }
    
    void loop()
    {
        char packet_bytes[act_photo::packet_body_size];

        std::vector<act_photo::packet_t> packet_buffer;
        packet_buffer.reserve(buffer.capacity() / act_photo::packet_body_size);

        std::list<act_photo::command_t> command_buffer;

        for (;;)
        {
            {
                guard_t guard(mutex);
                buffer.capacity(buffer_size);
            }
            if (!fetch_port().read(buffer))
            {
                continue;
            }

            buffer.flip();

            do
            {
                if (!detect_packet_start()) break;
                buffer.get(packet_bytes, act_photo::packet_body_size);
                packet_buffer.push_back(act_photo::read_packet(packet_bytes));
            }
            while (buffer.remaining() >= act_photo::packet_size);

            buffer.reset();

            {
                guard_t guard(queue_mutex);

                std::size_t can_put = min(queue_length - packet_queue.size(), packet_buffer.size());
                packet_queue.insert(packet_queue.end(),
                                    packet_buffer.begin(), packet_buffer.begin() + can_put);
                packet_buffer.clear();

                command_buffer.insert(command_buffer.end(), commands.begin(), commands.end());
                commands.clear();
            }

            while (!command_buffer.empty())
            {
                act_photo::command_t command = command_buffer.front();
                // for simplicity as the buffer size must commonly be much greater
                assert(command.bytes.size() + 2 <= buffer.remaining());
                act_photo::serialize(command, buffer.data());
                buffer.increase_position(command.bytes.size() + 2);
                
                buffer.flip();

                while (fetch_port().write(buffer) && buffer.remaining())
                    ;

                buffer.reset();

                command_buffer.pop_front();
            }
        }
    }

    bool detect_packet_start()
    {
        char c;
        bool detected = false;
        while (!detected)
        {
            if (!buffer.get(c))
            {
                break;
            }
            if (c == act_photo::packet_delimiter)
            {
                if ((buffer.remaining() >= act_photo::packet_size - 1))
                {
                    char checksum = buffer.data()[act_photo::packet_size - 2];
                    char s = act_photo::read_checksum(buffer.data() - 1); // the buffer position is at least 1
                    if (checksum == s)
                    {
                        detected = true;
                        break;
                    }
                }
            }
        }
        if (buffer.remaining() < act_photo::packet_size - 1)
        {
            return false;
        }
        return detected;
    }
};