#pragma once

#include <afxwin.h>

#include <memory>
#include <mutex>
#include <map>
#include <condition_variable>

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


        class basic_channel_pool : public channel_pool
        {


        protected:

            /**
             *	std::get<0> -> channel pointer
             *	std::get<1> -> latest state
             */
            using entry_t = std::tuple < std::shared_ptr<channel_base>, state_t > ;


        protected:


            std::map<channel_key_t, entry_t> _pool;
            channel_key_t _next_key;
            bool _closed;

            mutable std::mutex _mx;
            mutable std::condition_variable _cv;
            mutable unsigned int _force_signals;


        public:


            basic_channel_pool()
                : _next_key(0)
                , _closed(false)
                , _force_signals(0)
            {
            }

            virtual ~basic_channel_pool() = default;


        public:


            virtual result_t try_get(predicate_t predicate) const override
            {
                std::lock_guard<std::mutex> lock(_mx);
                if (_closed) { throw error::channel_pool_closed_error("closed"); }
                return query(predicate);
            }

            virtual result_t try_get(states::predicate_t predicate) const override
            {
                std::lock_guard<std::mutex> lock(_mx);
                if (_closed) { throw error::channel_pool_closed_error("closed"); }
                return query(predicate_t([predicate] (const channel_key_t &, const channel_base &, const state_t & s) { return predicate(s); }));
            }

            virtual result_t try_get(state_t expected, bool expect_all_set) const override
            {
                std::lock_guard<std::mutex> lock(_mx);
                if (_closed) { throw error::channel_pool_closed_error("closed"); }
                return query(predicate_t([expected, expect_all_set] (const channel_key_t &, const channel_base &, const state_t & s) { return (expect_all_set ? (s & expected) : (s | expected)); }));
            }
                          

            virtual result_t wait(std::chrono::nanoseconds t, predicate_t predicate) const override
            {
                std::unique_lock<std::mutex> lock(_mx);

                if (_closed) { throw error::channel_pool_closed_error("closed"); }

                result_t query_result = query(predicate);
                if (std::get<0>(query_result))
                {
                    return query_result;
                }

                unsigned int _current_signals = _force_signals;

                if (std::chrono::nanoseconds::zero() == t)
                {
                    _cv.wait(lock, [&] () {
                        if (_closed || (_current_signals != _force_signals)) { return true; }
                        query_result = query(predicate);
                        return std::get<0>(query_result);
                    });
                }
                else
                {
                    _cv.wait_for(lock, t, [&] () {
                        if (_closed || (_current_signals != _force_signals)) { return true; }
                        query_result = query(predicate);
                        return std::get<0>(query_result);
                    });
                }

                if (_closed) { throw error::channel_pool_closed_error("closed"); }

                return query_result;
            }
                                 
            virtual result_t wait(std::chrono::nanoseconds t, states::predicate_t predicate) const override
            {
                return wait(t, predicate_t([predicate] (const channel_key_t &, const channel_base &, const state_t & s) { return predicate(s); }));
            }
                                 
            virtual result_t wait(std::chrono::nanoseconds t, state_t expected, bool expect_all_set) const override
            {
                return wait(t, predicate_t([expected, expect_all_set] (const channel_key_t &, const channel_base &, const state_t & s) { return (expect_all_set ? (s & expected) : (s | expected)); }));
            }


            virtual void signal() const override
            {
                {
                    std::lock_guard<std::mutex> lock(_mx);
                    ++_force_signals;
                }
                _cv.notify_all();
            }


            virtual result_t put(std::shared_ptr<channel_base> c) override
            {
                std::lock_guard<std::mutex> lock(_mx);
                if (_closed) { throw error::channel_pool_closed_error("closed"); }
                _pool[++_next_key] = entry_t{ c, c->get_state_machine().state() };
                _cv.notify_all();
                return result_t{ true, _next_key, std::move(c) };
            }

            virtual result_t report(channel_key_t key) override
            {
                std::lock_guard<std::mutex> lock(_mx);
                if (_closed) { throw error::channel_pool_closed_error("closed"); }
                auto iter = _pool.find(key);
                if (iter != _pool.end())
                {
                    state_t new_state = std::get<0>(iter->second)->get_state_machine().state();
                    if (new_state != std::get<1>(iter->second))
                    {
                        std::get<1>(iter->second) = new_state;
                        _cv.notify_all();
                    }
                    return result_t{ true, key, std::get<0>(iter->second) };
                }
                return result_t{ false, channel_pool::channel_none, {} };
            }

            virtual result_t get(channel_key_t key) const override
            {
                std::lock_guard<std::mutex> lock(_mx);
                if (_closed) { throw error::channel_pool_closed_error("closed"); }
                auto iter = _pool.find(key);
                if (iter != _pool.end())
                {
                    return result_t{ true, key, std::get<0>(iter->second) };
                }
                return result_t{ false, channel_pool::channel_none, {} };
            }

            virtual result_t remove(channel_key_t key) override
            {
                std::lock_guard<std::mutex> lock(_mx);
                if (_closed) { throw error::channel_pool_closed_error("closed"); }
                auto iter = _pool.find(key);
                if (iter != _pool.end())
                {
                    result_t r{ true, key, std::move(std::get<0>(iter->second)) };
                    _pool.erase(iter);
                    return r;
                }
                return result_t{ false, channel_pool::channel_none, {} };
            }


            virtual bool closed() const override
            {
                std::lock_guard<std::mutex> lock(_mx);
                return _closed;
            }

            virtual void close() override
            {
                {
                    std::lock_guard<std::mutex> lock(_mx);
                    _closed = true;
                }
                _cv.notify_all();
            }


        private:

            result_t query(const predicate_t & predicate) const
            {
                for each (auto & var in _pool)
                {
                    if (predicate(var.first,
                                  *(std::get<0>(var.second)),
                                  std::get<1>(var.second)))
                    {
                        return result_t{ true, var.first, std::get<0>(var.second) };
                    }
                }
                return result_t{ false, channel_pool::channel_none, {} };
            }

        };

    }
}