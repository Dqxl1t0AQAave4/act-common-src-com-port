#pragma once

#include <afxwin.h>

#include <string>
#include <exception>
#include <functional>

#include <byte_buffer.h>
#include <bit_field.h>

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


            virtual result_t lock_op  (state_diagram &d,
                                       constant_t    op) = 0;

            virtual result_t unlock_op(state_diagram    &d,
                                       constant_t       op,
                                       state_t          locked_with,
                                       ops::result_type op_result) = 0;


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

        };

    }
}