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
#include <dialect.h>
#include <logger.h>


class reactor_stopped
    : public std::runtime_error
{

public:

    reactor_stopped()
        : std::runtime_error("")
    {
    }
};


template<class I, class O> class reactor_base
{

public:

    using ipacket_t = I;
    using opacket_t = O;

    using mutex_t = std::mutex;
    using guard_t = std::lock_guard < mutex_t > ;

protected:

    using ulock_t     = std::unique_lock < mutex_t > ;
    using condition_t = std::condition_variable;

    std::thread            reactor_thread;
                           
    mutex_t                mutex;
    condition_t            cv;

    // guarded by `mutex`
    bool                   working;
    com_port               port;
    bool                   port_changed;
    std::size_t            ibuffer_size;
    std::size_t            obuffer_size;
    std::vector<opacket_t> oqueue;
    bool                   use_iqueue;

    // thread-local
    com_port               current_port;
    byte_buffer            ibuffer;
    byte_buffer            obuffer;

public:

    mutex_t                iqueue_mutex;

    // guarded by `iqueue_mutex`
    std::size_t            iqueue_length;
    std::list<ipacket_t>   iqueue;

public:

    reactor_base(std::size_t ibuffer_size  = 5000,
                 std::size_t obuffer_size  = 5000,
                 std::size_t iqueue_length = 1000,
                 bool        use_iqueue    = true)
                 : ibuffer(buffer_size)
                 : obuffer(buffer_size)
                 , ibuffer_size(ibuffer_size)
                 , obuffer_size(obuffer_size)
                 , iqueue_length(iqueue_length)
                 , use_iqueue(use_iqueue)
    {
        reactor_thread = std::thread(&reactor::start, this);
    }

    virtual ~reactor_base()
    {
        stop();
    }

    virtual void supply_port(com_port port)
    {
        {
            guard_t guard(mutex);
            this->port = std::move(port);
            this->port_changed = true;
        }
        cv.notify_one();
    }

    virtual void supply_opacket(opacket_t packet)
    {
        {
            guard_t guard(mutex);
            oqueue.push_back(packet);
        }
    }

    virtual void supply_ibuffer_size(std::size_t buffer_size)
    {
        {
            guard_t guard(mutex);
            this->ibuffer_size = buffer_size;
        }
    }

    virtual void supply_obuffer_size(std::size_t buffer_size)
    {
        {
            guard_t guard(mutex);
            this->obuffer_size = buffer_size;
        }
    }

    virtual void supply_use_iqueue(bool use)
    {
        {
            guard_t guard(mutex);
            this->use_iqueue = use;
        }
    }

    virtual void supply_iqueue_length(std::size_t queue_length)
    {
        {
            guard_t guard(iqueue_mutex);
            this->iqueue_length = queue_length;
        }
    }

    virtual void stop()
    {
        {
            guard_t guard(mutex);
            working = false;
        }
        cv.notify_one();
    }

    virtual void join()
    {
        reactor_thread.join();
    }

protected:

    virtual void start()
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
        catch (const reactor_stopped &)
        {
            current_port.close();
            {
                guard_t guard(mutex);
                port.close();
            }
            return;
        }
    }

    virtual com_port & fetch_port()
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
                throw reactor_stopped();
            }
            current_port = std::move(port);
            port_changed = false;
        }
        if (!working)
        {
            throw reactor_stopped();
        }
        return current_port;
    }
    
    virtual void loop() = 0;
};


template<class I, class O> class reactor : public reactor_base<I, O>
{

public:

    using dialect_t = dialect < I, O > ;

protected:

    dialect_t processor;

public:

    reactor(dialect_t processor,
            std::size_t ibuffer_size  = 5000,
            std::size_t obuffer_size  = 5000,
            std::size_t iqueue_length = 1000)
            : reactor_base(ibuffer_size, obuffer_size, iqueue_length)
            , processor(processor)
    {
    }

protected:

    virtual void loop()
    {
        std::vector<ipacket_t> ipacket_buffer;
        std::list<opacket_t>   opacket_buffer;

        bool use_iqueue;

        for(;;)
        {
            {
                guard_t guard(mutex);
                ibuffer.capacity(ibuffer_size);
                obuffer.capacity(obuffer_size);
                use_iqueue = this->use_iqueue;
            }
            if (!fetch_port().read(buffer))
            {
                continue;
            }

            buffer.flip();

            for(;;)
            {
                ipacket_t packet;
                if (!processor.read(packet, ibuffer))
                {
                    break;
                }
                if (use_iqueue)
                {
                    ipacket_buffer.push_back(packet);
                }
            }

            ibuffer.compact();

            {
                guard_t guard(iqueue_mutex);
            
                if (use_iqueue)
                {
                    std::size_t can_put = min(iqueue_length - iqueue.size(), packet_buffer.size());
                    iqueue.insert(iqueue.end(),
                                  packet_buffer.begin(), packet_buffer.begin() + can_put);
                    packet_buffer.clear();
                }
            
                command_buffer.insert(command_buffer.end(), oqueue.begin(), oqueue.end());
                oqueue.clear();
            }

            while (!opacket_buffer.empty())
            {
                opacket_t packet = opacket_buffer.front();
                bool written = processor.write(obuffer, packet);
                
                obuffer.flip();
            
                while (fetch_port().write(obuffer) && obuffer.remaining())
                    ;
            
                buffer.compact();
            
                if (written)
                {
                    opacket_buffer.pop_front();
                }
            }
        }
    }
};