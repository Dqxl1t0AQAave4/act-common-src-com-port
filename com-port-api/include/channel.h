#pragma once

#include <afxwin.h>

#include <string>
#include <exception>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <type_traits>

#include <com-port-api/include/byte_buffer.h>
#include <com-port-api/include/bit_field.h>

namespace com_port_api
{

    namespace error
    {

        /**
         *	Is thrown every time the underlying channel
         *	is unable to serve the request in correct way.
         */
        class channel_error
            : public std::runtime_error
        {
        public:
            channel_error(const std::string & msg)
                : std::runtime_error(msg)
            {
            }
        };

        class channel_low_level_error
            : public channel_error
        {
        private:
            DWORD error_code;
        public:
            channel_low_level_error(const std::string & msg,
                          DWORD error_code = ERROR_SUCCESS)
                : channel_error(msg)
                , error_code(error_code)
            {
            }
            channel_low_level_error(DWORD error_code)
                : channel_low_level_error("", error_code)
            {
            }
        };
    }


    

    namespace channel
    {

        using constant_t = unsigned int;

        using state_t = bit_field < constant_t > ;
        using flags_t = bit_field < constant_t > ;
        

        struct flags
        {
            static enum : constant_t
            {
                readable = (1 << 0),
                writable = (1 << 1)
            };
        };


        struct states
        {

            static enum : constant_t
            {
                none     = 0       ,
                opening  = (1 << 0),
                open     = (1 << 1),
                readable = (1 << 2),
                writable = (1 << 3),
                closing  = (1 << 4),
                closed   = (1 << 5)
            };

            static inline const constant_t operable_state(const flags_t &f)
            {
                state_t op;
                if (f | flags::readable)
                {
                    op += states::readable;
                }
                if (f | flags::writable)
                {
                    op += states::writable;
                }
                return op.value;
            }

            using un_op_t     = std::function < state_t (const state_t &) > ;
            using bi_op_t     = std::function < state_t (const state_t &, const state_t &) > ;
            using predicate_t = std::function < bool (const state_t &) > ;

        };
        

        struct ops
        {

            static enum : constant_t
            {
                open  = (1 << 0),
                read  = (1 << 1),
                write = (1 << 2),
                close = (1 << 3)
            };

            static enum class result_type
            {
                result_success, result_failure, guarantee
            };
        };
        

        enum class guarantee_t
        {
            release = std::memory_order_release,
            acquire = std::memory_order_acquire,
            acq_rel = std::memory_order_acq_rel
        };


        class state_diagram
        {


        public:


            /**
             *	std::get<0> -> success/failure
             *	std::get<1> -> calculated state
             *	std::get<2> -> calculated guarantee
             */
            using result_t = std::tuple < bool, state_t, guarantee_t > ;


        public:


            virtual ~state_diagram()
            {
            }


            virtual result_t lock_op  (constant_t op,
                                       state_t    started_with,
                                       flags_t    flags) const = 0;

            virtual result_t unlock_op(constant_t       op,
                                       state_t          started_with,
                                       state_t          locked_with,
                                       flags_t          flags,
                                       ops::result_type op_result) const = 0;

        };


        class state_machine
        {


        public:

            
            /**
             *	std::get<0> -> success/failure
             *	std::get<1> -> state on a moment of invocation
             *	std::get<2> -> new state set
             */
            using result_t = std::tuple < bool, state_t, state_t > ;


        public:

            virtual ~state_machine()
            {
            }
            

            virtual state_t state() const = 0;
            
            virtual result_t set_state(state_t     expected,
                                       state_t     desired,
                                       guarantee_t guarantee = guarantee_t::acq_rel) = 0;


            virtual flags_t flags() const = 0;
            
            virtual void set_flags(flags_t desired) = 0;


            virtual result_t lock_op  (const state_diagram &d,
                                       constant_t          op) = 0;

            virtual result_t unlock_op(const state_diagram &d,
                                       constant_t          op,
                                       state_t             locked_with,
                                       ops::result_type    op_result) = 0;


            virtual void provide_guarantee(guarantee_t guarantee = guarantee_t::acq_rel)
            {
                result_t result { false, state(), states::none };
                do 
                {
                    result = set_state(std::get<1>(result), std::get<1>(result), guarantee);
                } while (!std::get<0>(result));
            }

            virtual bool wait_unconditionally(states::predicate_t predicate) const = 0;

        };


        using error_handler_t   = std::function < void (const error::channel_error &) > ;
        using success_handler_t = std::function < void () > ;
        using result_handler_t  = std::function < void (byte_buffer &) > ;


        template <class SD, class SM,
                  typename = std::enable_if_t<std::is_base_of<typename state_diagram, SD>::value
                                           && std::is_base_of<typename state_machine, SM>::value>>
        class channel_base
        {


        public:


            using state_diagram_t = SD;
            using state_machine_t = SM;
            
            /**
             *	std::get<0> -> success/failure
             *	std::get<1> -> state on a moment of invocation
             *	std::get<2> -> new state set
             */
            using result_t = std::tuple <bool, state_t, state_t> ;


        protected:


            state_diagram_t _diagram;
            state_machine_t _machine;


        public:


            channel_base(const state_diagram_t &diagram,
                         const state_machine_t &machine)
                         : _diagram(diagram), _machine(machine)
            {
            }

            channel_base()
            {
            }

            virtual ~channel_base()
            {
            }


        public:

            const state_diagram_t & get_state_diagram()
            {
                return _diagram;
            }

            const state_machine_t & get_state_machine()
            {
                return _machine;
            }

        public:


            virtual result_t read (byte_buffer &dst) { throw error::channel_error("unsupported"); };
            virtual result_t write(byte_buffer &src) { throw error::channel_error("unsupported"); };
            virtual result_t open () { throw error::channel_error("unsupported"); };
            virtual result_t close() { throw error::channel_error("unsupported"); };

            
            virtual result_t read (byte_buffer       &dst,
                                   result_handler_t  success_cb,
                                   error_handler_t   failure_cb) { throw error::channel_error("unsupported"); };
                                                 
            virtual result_t write(byte_buffer       &src,
                                   result_handler_t  success_cb,
                                   error_handler_t   failure_cb) { throw error::channel_error("unsupported"); };

            virtual result_t open (success_handler_t success_cb,
                                   error_handler_t   failure_cb) { throw error::channel_error("unsupported"); };

            virtual result_t close(success_handler_t success_cb,
                                   error_handler_t   failure_cb) { throw error::channel_error("unsupported"); };


        protected:


            virtual result_t do_as(constant_t op,
                                   std::function < void () > fn)
            {
                state_machine_t::result_t state_transition_result =
                    _machine.lock_op(_diagram, op);

                state_t locked_with = std::get<1>(state_transition_result);

                if (!std::get<0>(state_transition_result))
                {
                    return result_t{ false, locked_with, locked_with };
                }

                try
                {
                    fn();

                    state_transition_result =
                        _machine.unlock_op(_diagram,
                                           op,
                                           locked_with,
                                           ops::result_type::result_success);

                    assert(std::get<0>(state_transition_result));

                    return result_t{ true, locked_with, std::get<2>(state_transition_result) };
                }
                catch (...)
                {
                    state_transition_result =
                        _machine.unlock_op(_diagram,
                                           op,
                                           locked_with,
                                           ops::result_type::result_failure);

                    assert(std::get<0>(state_transition_result));

                    throw;
                }
            }

            virtual result_t do_as(constant_t op,
                                   std::function < void (success_handler_t,
                                                         error_handler_t) > fn,
                                   success_handler_t success_cb,
                                   error_handler_t   failure_cb)
            {
                state_machine_t::result_t state_transition_result =
                    _machine.lock_op(_diagram, op);

                state_t locked_with = std::get<1>(state_transition_result);
                state_t transition_result = std::get<2>(state_transition_result);

                if (!std::get<0>(state_transition_result))
                {
                    return result_t{ false, locked_with, locked_with };
                }

                success_handler_t _success_cb = [=, this] ()
                {
                    state_machine_t::result_t _state_transition_result =
                    _machine.unlock_op(_diagram,
                                       op,
                                       locked_with,
                                       ops::result_type::result_success);

                    assert(std::get<0>(_state_transition_result));

                    success_cb();
                };

                error_handler_t _failure_cb = [=, this] (const error::channel_error &err)
                {
                    state_machine_t::result_t _state_transition_result =
                    _machine.unlock_op(_diagram,
                                       op,
                                       locked_with,
                                       ops::result_type::result_failure);

                    assert(std::get<0>(_state_transition_result));

                    failure_cb(err);
                };

                try
                {
                    fn(_success_cb, _failure_cb);
                }
                catch (const error::channel_error &err)
                {
                    state_transition_result =
                        _machine.unlock_op(_diagram,
                                           op,
                                           locked_with,
                                           ops::result_type::result_failure);

                    assert(std::get<0>(state_transition_result));

                    failure_cb(err);

                    return result_t{ true, locked_with, transition_result };
                }
                catch (...)
                {
                    state_transition_result =
                        _machine.unlock_op(_diagram,
                                           op,
                                           locked_with,
                                           ops::result_type::result_failure);

                    assert(std::get<0>(state_transition_result));

                    throw;
                }
                
                state_transition_result =
                    _machine.unlock_op(_diagram,
                                       op,
                                       locked_with,
                                       ops::result_type::guarantee);

                assert(std::get<0>(state_transition_result));

                return result_t{ true, locked_with, transition_result };
            }
        };

        
        class atomic_state_machine : public state_machine
        {

        protected:

            std::atomic<state_t> _state;
            std::atomic<state_t> _flags;

        public:

            atomic_state_machine()
            {
            }

            atomic_state_machine(const atomic_state_machine &other)
            {
                _state = other.state();
                _flags = other.flags();
            }

            virtual state_t state() const override
            {
                return _state.load(std::memory_order_relaxed);
            }
            
            virtual result_t set_state(state_t expected,
                                       state_t desired,
                                       guarantee_t guarantee = guarantee_t::acq_rel) override
            {
                bool success = _state.compare_exchange_strong(
                    expected, desired, static_cast<std::memory_order>(guarantee),
                                       std::memory_order_relaxed);

                return result_t{ success, expected, (success ? desired : expected) };
            }


            virtual flags_t flags() const override
            {
                return _flags.load(std::memory_order_relaxed);
            }
            
            virtual void set_flags(flags_t desired) override
            {
                return _flags.store(desired, std::memory_order_relaxed);
            }


            virtual result_t lock_op  (const state_diagram &d,
                                       constant_t    op) override
            {
                result_t r;

                do
                {
                    flags_t current_flags = flags();
                    state_t current_state = state();
                    state_diagram::result_t state_transition_result
                        = d.lock_op(op, current_state, current_flags);

                    if (!std::get<0>(state_transition_result))
                    {
                        return result_t{ false, current_state, current_state };
                    }

                    r = set_state(current_state,
                                  std::get<1>(state_transition_result),
                                  std::get<2>(state_transition_result));
                }
                while (!std::get<0>(r));

                return r;
            }

            virtual result_t unlock_op(const state_diagram &d,
                                       constant_t          op,
                                       state_t             locked_with,
                                       ops::result_type    op_result) override
            {
                result_t r;

                do
                {
                    flags_t current_flags = flags();
                    state_t current_state = state();
                    state_diagram::result_t state_transition_result
                        = d.unlock_op(op, current_state, locked_with, current_flags, op_result);

                    if (!std::get<0>(state_transition_result))
                    {
                        return result_t{ false, current_state, current_state };
                    }

                    if (op_result == ops::result_type::guarantee)
                    {
                        provide_guarantee(std::get<2>(state_transition_result));
                        return result_t{ true, current_state, current_state };
                    }

                    r = set_state(current_state,
                                  std::get<1>(state_transition_result),
                                  std::get<2>(state_transition_result));
                }
                while (!std::get<0>(r));

                return r;
            }


            virtual bool wait_unconditionally(states::predicate_t predicate) const override
            {
                return false;
            }

        };

        
        class blocking_state_machine : public state_machine
        {

        protected:

            mutable std::mutex _mx;
            mutable std::condition_variable _cv;
            mutable unsigned int _force_signals;

            state_t _state;
            std::atomic<state_t> _flags;

        public:

            blocking_state_machine()
                : _force_signals(0)
            {
            }

            blocking_state_machine(const blocking_state_machine &other)
                : _force_signals(0)
            {
                _state = other.state();
                _flags = other.flags();
            }

            virtual state_t state() const override
            {
                std::lock_guard<std::mutex> guard(_mx);
                return _state;
            }
            
            virtual result_t set_state(state_t expected,
                                       state_t desired,
                                       guarantee_t guarantee = guarantee_t::acq_rel) override
            {
                state_t old;
                bool success, signal;
                {
                    std::lock_guard<std::mutex> guard(_mx);

                    old     = _state;
                    success = (old == expected);
                    signal  = (old != desired);

                    if (success && signal)
                    {
                        _state = desired;
                    }
                }

                if (success && signal)
                {
                    _cv.notify_all();
                }

                return result_t{ success, old, (success ? desired : old) };
            }


            virtual flags_t flags() const override
            {
                return _flags.load(std::memory_order_relaxed);
            }
            
            virtual void set_flags(flags_t desired) override
            {
                return _flags.store(desired, std::memory_order_relaxed);
            }


            virtual result_t lock_op  (const state_diagram &d,
                                       constant_t          op) override
            {
                flags_t current_flags = flags();
                state_t current_state;

                std::lock_guard<std::mutex> guard(_mx);
                
                current_state = _state;

                state_diagram::result_t state_transition_result
                    = d.lock_op(op, current_state, current_flags);

                if (!std::get<0>(state_transition_result))
                {
                    return result_t{ false, current_state, current_state };
                }

                set_state_unprotected(std::get<1>(state_transition_result));

                return result_t{ true, current_state, std::get<1>(state_transition_result) };
            }

            virtual result_t unlock_op(const state_diagram &d,
                                       constant_t          op,
                                       state_t             locked_with,
                                       ops::result_type    op_result) override
            {
                flags_t current_flags = flags();
                state_t current_state;

                std::lock_guard<std::mutex> guard(_mx);
                
                current_state = _state;

                state_diagram::result_t state_transition_result
                    = d.unlock_op(op, current_state, locked_with, current_flags, op_result);

                if (!std::get<0>(state_transition_result))
                {
                    return result_t{ false, current_state, current_state };
                }

                if (op_result == ops::result_type::guarantee)
                {
                    /* providing a guarantee is unnecessary since mutex locking
                       and unlocking already provides acq_rel guarantee */
                    /* provide_guarantee(std::get<2>(state_transition_result)); */
                    return result_t{ true, current_state, current_state };
                }

                set_state_unprotected(std::get<1>(state_transition_result));

                return result_t{ true, current_state, std::get<1>(state_transition_result) };
            }


            virtual bool wait_unconditionally(states::predicate_t predicate) const override
            {
                result_t result = _wait
                (
                    [&] (std::unique_lock<std::mutex> &lock, std::function<bool()> pred)
                    {
                        _cv.wait(lock, pred);
                    },
                    predicate,
                    true
                );
                return std::get<0>(result);
            }

            void notify() const
            {
                {
                    std::unique_lock<std::mutex> lock(_mx);
                    ++_force_signals;
                }
                _cv.notify_all();
            }

            template<class _Rep, class _Period>
            result_t wait(const std::chrono::duration<_Rep, _Period>& _Rel_time,
                          std::function<bool (const state_t &)> predicate) const
            {
                return _wait
                (
                    [&] (std::unique_lock<std::mutex> &lock, std::function<bool()> pred)
                    {
                        _cv.wait_for(lock, _Rel_time, pred);
                    },
                    predicate
                );
            }


            template<class _Rep, class _Period>
            result_t wait(const std::chrono::duration<_Rep, _Period>& _Rel_time,
                          state_t expected,
                          bool expect_all_set) const
            {
                return _wait
                (
                    [&] (std::unique_lock<std::mutex> &lock, std::function<bool()> pred)
                    {
                        _cv.wait_for(lock, _Rel_time, pred);
                    },
                    [expected, expect_all_set](const state_t &s)
                    { return (expect_all_set ? (s & expected) : (s | expected)); }
                );
            }


            result_t wait(states::predicate_t predicate) const
            {
                return _wait
                (
                    [&] (std::unique_lock<std::mutex> &lock, std::function<bool()> pred)
                    {
                        _cv.wait(lock, pred);
                    },
                    predicate
                );
            }


            result_t wait(state_t expected, bool expect_all_set) const
            {
                return _wait
                (
                    [&] (std::unique_lock<std::mutex> &lock, std::function<bool()> pred)
                    {
                        _cv.wait(lock, pred);
                    },
                    [expected, expect_all_set] (const state_t &s)
                    { return (expect_all_set ? (s & expected) : (s | expected)); }
                );
            }


        protected:

            
            virtual void set_state_unprotected(state_t desired)
            {
                if (_state == desired)
                {
                    return;
                }
                _state = desired;
                _cv.notify_all();
            }


        private:

            
            result_t _wait
            (
                std::function < void (std::unique_lock<std::mutex> &, std::function<bool()>) > wait_operation,
                states::predicate_t expected,
                bool unconditionally = false
            ) const
            {
                std::unique_lock<std::mutex> lock(_mx);

                state_t started_with = _state;

                unsigned int signals = _force_signals;

                if (expected(_state))
                {
                    return result_t{ true, started_with, _state };
                }

                if (_state & states::closed)
                {
                    return result_t{ false, started_with, _state };
                }

                wait_operation(lock, [=] ()
                {
                    return
                    (
                        /* must fail the operation in case if
                           `signal()` method called */
                        ((!unconditionally) && (signals != _force_signals)) ||
                        /* handles normal case */
                        expected(_state)                                    ||
                        (_state & states::closed)
                    );
                });

                return result_t{ expected(_state), started_with, _state };
            }

        };
        

        class basic_state_diagram : public state_diagram
        {
            
        public:

            virtual result_t lock_op  (constant_t op,
                                       state_t    started_with,
                                       flags_t    flags) const override
            {
                guarantee_t guarantee = guarantee_t::acquire;

                switch (op)
                {
                case ops::open:
                    if ((started_with | (states::opening | states::open |
                                         states::closing | states::closed)))
                    {
                        break;
                    }
                    return result_t{ true, started_with + states::opening, guarantee };
                case ops::close:
                    if ((started_with | (states::opening | states::closing | states::closed))
                        || !(started_with & (states::operable_state(flags) | states::open)))
                    {
                        break;
                    }
                    return result_t{ true, started_with - states::open + states::closing, guarantee };
                case ops::read:
                    if (!(started_with & (states::open | states::readable)))
                    {
                        break;
                    }
                    return result_t{ true, started_with - states::readable, guarantee };
                case ops::write:
                    if (!(started_with & (states::open | states::writable)))
                    {
                        break;
                    }
                    return result_t{ true, started_with - states::writable, guarantee };
                default:
                    break;
                }
                return result_t{ false, started_with, guarantee };
            }

            virtual result_t unlock_op(constant_t       op,
                                       state_t          started_with,
                                       state_t          locked_with,
                                       flags_t          flags,
                                       ops::result_type op_result) const override
            {
                guarantee_t guarantee = guarantee_t::release;
                if (op_result == ops::result_type::guarantee)
                {
                    return result_t{ true, started_with, guarantee };
                }
                
                switch (op)
                {
                case ops::open:
                    if (!(started_with & states::opening))
                    {
                        break;
                    }
                    if (op_result == ops::result_type::result_success)
                    {
                        return result_t{ true, ((started_with - states::opening)
                                                 + (states::open | states::operable_state(flags))), guarantee };
                    }
                    return result_t{ true, started_with - states::opening, guarantee };
                case ops::close:
                    if (!(started_with & states::closing))
                    {
                        break;
                    }
                    return result_t{ true, (started_with - states::closing) + states::closed, guarantee };
                case ops::read:
                    if (!(started_with & states::open) || (started_with & states::readable))
                    {
                        break;
                    }
                    return result_t{ true, started_with + states::readable, guarantee };
                case ops::write:
                    if (!(started_with & states::open) || (started_with & states::writable))
                    {
                        break;
                    }
                    return result_t{ true, started_with + states::writable, guarantee };
                default:
                    break;
                }
                return result_t{ false, started_with, guarantee };
            }
        };


    }
}