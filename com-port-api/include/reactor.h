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
#include <memory>

#include <byte_buffer.h>
#include <com-port.h>
#include <dialect.h>
#include <logger.h>

namespace com_port_api
{
    
class reactor_stopped
    : public std::runtime_error
{

public:

    reactor_stopped()
        : std::runtime_error("")
    {
    }
};


/**
 *	The class provides a basic functionality
 *	for all the reactor objects, i.e. objects
 *	which provide a buffered non-blocking IO
 *	operations over com_port.
 *	
 *	This base class is a virtual class with
 *	a single abstract function `loop` to override.
 */
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

    /**
     *	Background worker thread
     */
    std::thread            reactor_thread;

    /**
     *	Mutex and condition variable for internal
     *	usage only
     */
    mutex_t                mutex;
    condition_t            cv;


    // guarded by `mutex`


    /**
     *	Indicates if this reactor is working
     */
    bool                   working;

    /**
     *	The port obtained from external code (thread)
     *	to be fetched and moved to `current_port`
     */
    com_port               port;
    bool                   port_changed;

    /**
     *	Buffer sizes, packet queues, flags obtained
     *	from external code (thread) to be moved to
     *	worker thread local objects
     */
    std::size_t            ibuffer_size;
    std::size_t            obuffer_size;
    std::list<opacket_t>   oqueue;
    bool                   use_iqueue;


    // thread-local


    /**
     *	The current port used as the data source and target
     */
    com_port               current_port;

    /**
     *	The current buffers
     */
    byte_buffer            ibuffer;
    byte_buffer            obuffer;


public:


    mutex_t                iqueue_mutex;


    // guarded by `iqueue_mutex`


    /**
     *	The public packet queue
     *	
     *	The queue may be weak-sized, i.e. its size
     *	will not grow if the current size is greater
     *	or equals `iqueue_length`
     *	
     *	Turning off `use_iqueue` variable
     *	does not affect `iqueue_length` value
     *	and `iqueue` capacity.
     *	
     *	The `iqueue` can be accessed by the user
     *	ONLY IF `iqueue_mutex` IS ACQUIRED BY THE USER
     *	
     *	After the user interaction with `iqueue` finished,
     *	IT MUST ME CLEARED BY THE USER MANUALLY
     *	
     *	See usage example
     */
    std::size_t            iqueue_length;
    std::list<ipacket_t>   iqueue;


public:


    /**
     *	Creates the new reactor and immediately starts
     *	its working thread.
     *	
     *	Providing properly open port actually starts
     *	the reactor.
     */
    reactor_base(std::size_t ibuffer_size  = 5000,
                 std::size_t obuffer_size  = 5000,
                 std::size_t iqueue_length = 1000,
                 bool        use_iqueue    = true)
                 : ibuffer(ibuffer_size)
                 , obuffer(obuffer_size)
                 , ibuffer_size(ibuffer_size)
                 , obuffer_size(obuffer_size)
                 , iqueue_length(iqueue_length)
                 , use_iqueue(use_iqueue)
    {
    }


    /**
     *	Stops the reactor.
     *	
     *	Calls to `stop` and sequentially to `join`.
     *  Call to `join` is required since worker thread
     *  can still rely on class fields which otherwise
     *  be destroyed.
     *  
     *  Note that subclass must override the destructor
     *  (in the same way) in order to "delay" destruction
     *  of its field until the worker thread stop.
     */
    virtual ~reactor_base()
    {
        stop();
        join();
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


    /**
     *	Asynchronously stops the reactor by setting
     *	`working` flag to `false`.
     *	
     *	Use `join` to await reactor termination.
     */
    virtual void stop()
    {
        {
            guard_t guard(mutex);
            working = false;
        }
        cv.notify_one();
    }


    /**
     *	Synchronously creates and starts a worker thread.
     *	
     *	Implementations may override this function
     *	in order to provide another method of thread creation.
     */
    virtual void start()
    {
        reactor_thread = std::thread(&reactor_base::run, this);
    }


    /**
     *	Waits for this reactor termination.
     */
    virtual void join()
    {
        if (reactor_thread.joinable())
        {
            reactor_thread.join();
        }
    }


protected:


    /**
     *	The method is invoked by the worker thread constructor.
     *	
     *	Invokes `loop` method.
     *	
     *	Handles `reactor_stopped` exception thrown from
     *	`loop` method, closing this reactor and port.
     */
    virtual void run()
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


    /**
     *	Performs `current_port` substitution.
     *	
     *	Fetches externally supplied port (waits for it) and
     *	replaces `current_port` with it, closing previous
     *	`current_port` if necessary.
     *	
     *	Throws `reactor_stopped` exception if `working` variable
     *	changed to `false`.
     *	
     *  Returns `current_port` reference.
     */
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
    

    /**
     *	The main working method to be overridden.
     *	
     *	The implementation must follow these rules:
     *  
     *      - be an (infinite or finite) loop
     *      - use `fetch_port()` to obtain a current port
     *      - manually fetch current buffer and queue sizes, etc.
     *  
     *  The implementation is recommended to:
     *  
     *      - make use of all the provided variables in
     *        expected way
     */
    virtual void loop() = 0;
};


/**
 *	The default implementation of `reactor_base` class.
 *	
 *	The implementation relies on `dialect` class in
 *	packet encoding/decoding.
 *	
 *	The `dialect` must satisfy the following conditions:
 *	
 *	    - be default constructible
 *	    - have definitions for `ipacket_t` and `opacket_t` types
 *	    - implement `read` operation in the following way:
 *	          - signature: bool read(ipacket_t &dst, byte_buffer &src)
 *	          - return   : `true` on success / `false` otherwise
 *	          - throw    : nothing
 *	    - implement `write` operation in the following way:
 *	          - signature: bool write(byte_buffer &dst, const opacket_t &src)
 *	          - return   : `true` on success / `false` otherwise
 *	          - throw    : nothing
 *	
 *	See `dialect.h`.
 */
template<class D>
class reactor
    : public reactor_base < typename D::ipacket_t,
                            typename D::opacket_t >
{

public:

    using dialect_t = D;

protected:

    dialect_t processor;

public:

    reactor(std::size_t ibuffer_size  = 5000,
            std::size_t obuffer_size  = 5000,
            std::size_t iqueue_length = 1000,
            bool        use_iqueue    = true)
            : reactor_base(ibuffer_size, obuffer_size, iqueue_length, use_iqueue)
            , processor(std::move(processor))
    {
    }


    /**
     *	Stops the reactor.
     *	
     *	See the base class note on this method.
     */
    virtual ~reactor() override
    {
        stop();
        join();
    }


    dialect_t & dialect()
    {
        return processor;
    }


protected:

    virtual void loop() override
    {
        std::list<ipacket_t>   ipacket_buffer;
        std::list<opacket_t>   opacket_buffer;

        bool use_iqueue; // local, overlaps

        for(;;)
        {
            // fetch buffer-related and queue-related variables
            {
                guard_t guard(mutex);
                ibuffer.capacity(ibuffer_size);
                obuffer.capacity(obuffer_size);
                use_iqueue = this->use_iqueue;
            }

            // read from the port; try again on failure
            if (!fetch_port().read(ibuffer))
            {
                continue;
            }

            // prepare buffer for reading
            ibuffer.flip();

            // read all the packets available in the buffer
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

            // prepare buffer for further writing
            ibuffer.compact();

            // move read packets to iqueue
            // move oqueue entries to local buffer
            {
                guard_t guard(iqueue_mutex);
            
                if (use_iqueue)
                {
                    // use weak-sized `iqueue`
                    if (iqueue.size() < iqueue_length)
                    {
                        iqueue.splice(iqueue.end(), ipacket_buffer);
                    }
                }
            
                opacket_buffer.splice(opacket_buffer.end(), oqueue);
            }

            // send local buffer to the port
            while (!opacket_buffer.empty())
            {
                opacket_t packet = opacket_buffer.front();
                bool written = processor.write(obuffer, packet);
                
                // prepare buffer for reading
                obuffer.flip();
            
                // write to the port
                while (fetch_port().write(obuffer) && obuffer.remaining())
                    ;
            
                // prepare buffer for further writing
                obuffer.compact();
            
                if (written)
                {
                    opacket_buffer.pop_front();
                }
            }
        }
    }
};

}