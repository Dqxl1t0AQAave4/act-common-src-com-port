#pragma once

#include <afxwin.h>

#include <memory>

#include <com-port-api/include/channel.h>

namespace com_port_api
{

    namespace error
    {

        class channel_pool_closed_error
            : public std::runtime_error
        {
        public:
            channel_pool_closed_error(const std::string & msg)
                : std::runtime_error(msg)
            {
            }
        };
    }

    namespace channel
    {


        class channel_pool
        {


        public:
            

            using channel_key_t = constant_t;

            /**
             *	std::get<0> -> success/failure
             *	std::get<1> -> channel key
             *	std::get<2> -> channel
             */
            using result_t = std::tuple < bool, channel_key_t, std::shared_ptr<channel_base> > ;

            using predicate_t = std::function < bool (const channel_key_t &, const channel_base &, const state_t &) > ;
            

        public:


            enum : channel_key_t
            {
                channel_none = 0
            };


        public:


            channel_pool() = default;

            virtual ~channel_pool() = default;


        public:


            virtual result_t try_get(predicate_t predicate) const = 0;

            virtual result_t try_get(states::predicate_t predicate) const = 0;

            virtual result_t try_get(state_t expected, bool expect_all_set) const = 0;

                                 
            virtual result_t wait(std::chrono::nanoseconds t, predicate_t predicate) const = 0;
                                 
            virtual result_t wait(std::chrono::nanoseconds t, states::predicate_t predicate) const = 0;
                                 
            virtual result_t wait(std::chrono::nanoseconds t, state_t expected, bool expect_all_set) const = 0;


            virtual void signal() const = 0;


            virtual result_t put(std::shared_ptr<channel_base> c) = 0;

            virtual result_t report(channel_key_t key) = 0;

            virtual result_t get(channel_key_t key) const = 0;

            virtual result_t remove(channel_key_t key) = 0;


            virtual bool closed() const = 0;

            virtual void close() = 0;
        };
    }
}