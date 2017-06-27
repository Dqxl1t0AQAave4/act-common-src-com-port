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

    }
}