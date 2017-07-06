#include "stdafx.h"
#include "CppUnitTest.h"
#include "util.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

#include <com-port-api/include/channel.h>
#include <com-port-api/include/channel_pool.h>
#include <iostream>
#include <future>
#include <thread>

using namespace com_port_api;
using namespace com_port_api::channel;

namespace com_port_api_test
{

	TEST_CLASS(basic_channel_pool_test)
	{

	public:

        TEST_METHOD(put_returns_correct_result)
        {
            auto ch1 = std::make_shared<channel_base>(
                std::make_unique<basic_state_diagram>(),
                std::make_unique<atomic_state_machine>());
            auto ch2 = std::make_shared<channel_base>(
                std::make_unique<basic_state_diagram>(),
                std::make_unique<atomic_state_machine>());

            basic_channel_pool pool;
            channel_pool::result_t r;
            
            Logger::WriteMessage(L"Put ch1\r\n");

            r = pool.put(ch1);

            Assert::IsTrue(std::get<0>(r), L"", LINE_INFO());
            Assert::AreEqual(channel_pool::channel_none + 1, std::get<1>(r), L"", LINE_INFO());
            Assert::IsTrue(ch1 == std::get<2>(r), L"", LINE_INFO());
            
            Logger::WriteMessage(L"Put ch2\r\n");

            r = pool.put(ch2);

            Assert::IsTrue(std::get<0>(r), L"", LINE_INFO());
            Assert::AreEqual(channel_pool::channel_none + 2, std::get<1>(r), L"", LINE_INFO());
            Assert::IsTrue(ch2 == std::get<2>(r), L"", LINE_INFO());
            
            Logger::WriteMessage(L"Put ch1 again\r\n");

            r = pool.put(ch1);

            Assert::IsTrue(std::get<0>(r), L"", LINE_INFO());
            Assert::AreEqual(channel_pool::channel_none + 3, std::get<1>(r), L"", LINE_INFO());
            Assert::IsTrue(ch1 == std::get<2>(r), L"", LINE_INFO());
		}

        TEST_METHOD(put_fails_on_closed_pool)
        {
            auto ch1 = std::make_shared<channel_base>(
                std::make_unique<basic_state_diagram>(),
                std::make_unique<atomic_state_machine>());

            basic_channel_pool pool;
            channel_pool::result_t r;

            pool.close();

            try
            {
                r = pool.put(ch1);
                Assert::Fail(L"should not occur", LINE_INFO());
            }
            catch (const error::channel_pool_closed_error &)
            {
            }
		}

		TEST_METHOD(get_returns_correct_results)
        {
            auto ch1 = std::make_shared<channel_base>(
                std::make_unique<basic_state_diagram>(),
                std::make_unique<atomic_state_machine>());
            auto ch2 = std::make_shared<channel_base>(
                std::make_unique<basic_state_diagram>(),
                std::make_unique<atomic_state_machine>());

            basic_channel_pool pool;
            channel_pool::result_t r;
            
            Logger::WriteMessage(L"Put ch1\r\n");
            
            r = pool.put(ch1);

            Assert::IsTrue(std::get<0>(r), L"", LINE_INFO());
            Assert::AreEqual(channel_pool::channel_none + 1, std::get<1>(r), L"", LINE_INFO());
            Assert::IsTrue(ch1 == std::get<2>(r), L"", LINE_INFO());
            
            Logger::WriteMessage(L"Get [1]\r\n");

            r = pool.get(channel_pool::channel_none + 1);

            Assert::IsTrue(std::get<0>(r), L"", LINE_INFO());
            Assert::AreEqual(channel_pool::channel_none + 1, std::get<1>(r), L"", LINE_INFO());
            Assert::IsTrue(ch1 == std::get<2>(r), L"", LINE_INFO());
            
            Logger::WriteMessage(L"Put ch2\r\n");

            r = pool.put(ch2);

            Assert::IsTrue(std::get<0>(r), L"", LINE_INFO());
            Assert::AreEqual(channel_pool::channel_none + 2, std::get<1>(r), L"", LINE_INFO());
            Assert::IsTrue(ch2 == std::get<2>(r), L"", LINE_INFO());
            
            Logger::WriteMessage(L"Get [1]\r\n");

            r = pool.get(channel_pool::channel_none + 1);

            Assert::IsTrue(std::get<0>(r), L"", LINE_INFO());
            Assert::AreEqual(channel_pool::channel_none + 1, std::get<1>(r), L"", LINE_INFO());
            Assert::IsTrue(ch1 == std::get<2>(r), L"", LINE_INFO());
            
            Logger::WriteMessage(L"Get [2]\r\n");

            r = pool.get(channel_pool::channel_none + 2);

            Assert::IsTrue(std::get<0>(r), L"", LINE_INFO());
            Assert::AreEqual(channel_pool::channel_none + 2, std::get<1>(r), L"", LINE_INFO());
            Assert::IsTrue(ch2 == std::get<2>(r), L"", LINE_INFO());
            
            Logger::WriteMessage(L"Get [3]\r\n");

            r = pool.get(channel_pool::channel_none + 3);

            Assert::IsFalse(std::get<0>(r), L"", LINE_INFO());
            Assert::AreEqual(channel_pool::channel_none + 0, std::get<1>(r), L"", LINE_INFO());
            Assert::IsFalse(static_cast<bool>(std::get<2>(r)), L"", LINE_INFO());
		}

        TEST_METHOD(get_fails_on_closed_pool)
        {
            auto ch1 = std::make_shared<channel_base>(
                std::make_unique<basic_state_diagram>(),
                std::make_unique<atomic_state_machine>());

            basic_channel_pool pool;
            channel_pool::result_t r;
            
            Logger::WriteMessage(L"Put ch1\r\n");
            
            r = pool.put(ch1);

            Assert::IsTrue(std::get<0>(r), L"", LINE_INFO());
            Assert::AreEqual(channel_pool::channel_none + 1, std::get<1>(r), L"", LINE_INFO());
            Assert::IsTrue(ch1 == std::get<2>(r), L"", LINE_INFO());

            pool.close();
            
            Logger::WriteMessage(L"Get [1]\r\n");

            try
            {
                r = pool.get(channel_pool::channel_none + 1);
                Assert::Fail(L"should not occur", LINE_INFO());
            }
            catch (const error::channel_pool_closed_error &)
            {
            }
		}

		TEST_METHOD(try_get_returns_correct_results)
        {
            auto ch1 = std::make_shared<channel_base>(
                std::make_unique<basic_state_diagram>(),
                std::make_unique<atomic_state_machine>());
            auto ch2 = std::make_shared<channel_base>(
                std::make_unique<basic_state_diagram>(),
                std::make_unique<atomic_state_machine>());

            basic_channel_pool pool;
            channel_pool::result_t r;
            
            Logger::WriteMessage(L"Put ch1\r\n");
            
            r = pool.put(ch1);

            Assert::IsTrue(std::get<0>(r), L"", LINE_INFO());
            Assert::AreEqual(channel_pool::channel_none + 1, std::get<1>(r), L"", LINE_INFO());
            Assert::IsTrue(ch1 == std::get<2>(r), L"", LINE_INFO());
            
            Logger::WriteMessage(L"Put ch2\r\n");

            r = pool.put(ch2);

            Assert::IsTrue(std::get<0>(r), L"", LINE_INFO());
            Assert::AreEqual(channel_pool::channel_none + 2, std::get<1>(r), L"", LINE_INFO());
            Assert::IsTrue(ch2 == std::get<2>(r), L"", LINE_INFO());
            
            Logger::WriteMessage(L"Try get [0]\r\n");

            r = pool.try_get(channel_pool::predicate_t([] (const channel_pool::channel_key_t & key, const channel_base &, const state_t &)
                { return (key == channel_pool::channel_none); }
            ));

            Assert::IsFalse(std::get<0>(r), L"", LINE_INFO());
            Assert::AreEqual(channel_pool::channel_none + 0, std::get<1>(r), L"", LINE_INFO());
            Assert::IsFalse(static_cast<bool>(std::get<2>(r)), L"", LINE_INFO());
            
            Logger::WriteMessage(L"Try get [1]\r\n");

            r = pool.try_get(channel_pool::predicate_t([] (const channel_pool::channel_key_t & key, const channel_base &, const state_t &)
                { return (key == channel_pool::channel_none + 1); }
            ));

            Assert::IsTrue(std::get<0>(r), L"", LINE_INFO());
            Assert::AreEqual(channel_pool::channel_none + 1, std::get<1>(r), L"", LINE_INFO());
            Assert::IsTrue(ch1 == std::get<2>(r), L"", LINE_INFO());
            
            Logger::WriteMessage(L"Try get [2]\r\n");

            r = pool.try_get(channel_pool::predicate_t([] (const channel_pool::channel_key_t & key, const channel_base &, const state_t &)
                { return (key == channel_pool::channel_none + 2); }
            ));

            Assert::IsTrue(std::get<0>(r), L"", LINE_INFO());
            Assert::AreEqual(channel_pool::channel_none + 2, std::get<1>(r), L"", LINE_INFO());
            Assert::IsTrue(ch2 == std::get<2>(r), L"", LINE_INFO());
		}

        TEST_METHOD(try_get_fails_on_closed_pool)
        {
            auto ch1 = std::make_shared<channel_base>(
                std::make_unique<basic_state_diagram>(),
                std::make_unique<atomic_state_machine>());

            basic_channel_pool pool;
            channel_pool::result_t r;
            
            Logger::WriteMessage(L"Put ch1\r\n");
            
            r = pool.put(ch1);

            Assert::IsTrue(std::get<0>(r), L"", LINE_INFO());
            Assert::AreEqual(channel_pool::channel_none + 1, std::get<1>(r), L"", LINE_INFO());
            Assert::IsTrue(ch1 == std::get<2>(r), L"", LINE_INFO());

            pool.close();
            
            Logger::WriteMessage(L"Try get [1]\r\n");

            try
            {
                r = pool.try_get(channel_pool::predicate_t([] (const channel_pool::channel_key_t & key, const channel_base &, const state_t &)
                    { return (key == channel_pool::channel_none + 1); }
                ));
                Assert::Fail(L"should not occur", LINE_INFO());
            }
            catch (const error::channel_pool_closed_error &)
            {
            }
		}

		TEST_METHOD(report_updates_cached_channel_states)
        {
            auto sm_p = std::make_unique<atomic_state_machine>();

            auto &sm = *sm_p;

            auto ch1 = std::make_shared<channel_base>(
                std::make_unique<basic_state_diagram>(),
                std::move(sm_p));

            basic_channel_pool pool;
            channel_pool::result_t r;
            
            Logger::WriteMessage(L"Put ch1\r\n");
            
            r = pool.put(ch1);

            Assert::IsTrue(std::get<0>(r), L"", LINE_INFO());
            Assert::AreEqual(channel_pool::channel_none + 1, std::get<1>(r), L"", LINE_INFO());
            Assert::IsTrue(ch1 == std::get<2>(r), L"", LINE_INFO());
            
            Assert::AreEqual(state_t(states::none), sm.state(), L"", LINE_INFO());

            Logger::WriteMessage(L"Try get [open]\r\n");

            r = pool.try_get(channel_pool::predicate_t([] (const channel_pool::channel_key_t &, const channel_base &, const state_t & s)
                { return (s & states::open); }
            ));

            Assert::IsFalse(std::get<0>(r), L"", LINE_INFO());
            Assert::AreEqual(channel_pool::channel_none + 0, std::get<1>(r), L"", LINE_INFO());
            Assert::IsFalse(static_cast<bool>(std::get<2>(r)), L"", LINE_INFO());

            Logger::WriteMessage(L"Modify state to [open]\r\n");

            sm.set_state(states::none, states::open);
            
            Assert::AreEqual(state_t(states::open), sm.state(), L"", LINE_INFO());

            Logger::WriteMessage(L"Try get [open]\r\n");

            r = pool.try_get(channel_pool::predicate_t([] (const channel_pool::channel_key_t &, const channel_base &, const state_t & s)
                { return (s & states::open); }
            ));

            Assert::IsFalse(std::get<0>(r), L"", LINE_INFO());
            Assert::AreEqual(channel_pool::channel_none + 0, std::get<1>(r), L"", LINE_INFO());
            Assert::IsFalse(static_cast<bool>(std::get<2>(r)), L"", LINE_INFO());

            Logger::WriteMessage(L"Report [1]\r\n");

            r = pool.report(channel_pool::channel_none + 1);

            Assert::IsTrue(std::get<0>(r), L"", LINE_INFO());
            Assert::AreEqual(channel_pool::channel_none + 1, std::get<1>(r), L"", LINE_INFO());
            Assert::IsTrue(ch1 == std::get<2>(r), L"", LINE_INFO());

            Logger::WriteMessage(L"Try get [open]\r\n");

            r = pool.try_get(channel_pool::predicate_t([] (const channel_pool::channel_key_t &, const channel_base &, const state_t & s)
                { return (s & states::open); }
            ));

            Assert::IsTrue(std::get<0>(r), L"", LINE_INFO());
            Assert::AreEqual(channel_pool::channel_none + 1, std::get<1>(r), L"", LINE_INFO());
            Assert::IsTrue(ch1 == std::get<2>(r), L"", LINE_INFO());
		}

		TEST_METHOD(wait_returns_correct_results_immediately)
        {
            auto ch1 = std::make_shared<channel_base>(
                std::make_unique<basic_state_diagram>(),
                std::make_unique<atomic_state_machine>());
            auto ch2 = std::make_shared<channel_base>(
                std::make_unique<basic_state_diagram>(),
                std::make_unique<atomic_state_machine>());

            basic_channel_pool pool;
            channel_pool::result_t r;
            
            Logger::WriteMessage(L"Put ch1\r\n");
            
            r = pool.put(ch1);

            Assert::IsTrue(std::get<0>(r), L"", LINE_INFO());
            Assert::AreEqual(channel_pool::channel_none + 1, std::get<1>(r), L"", LINE_INFO());
            Assert::IsTrue(ch1 == std::get<2>(r), L"", LINE_INFO());
            
            Logger::WriteMessage(L"Put ch2\r\n");

            r = pool.put(ch2);

            Assert::IsTrue(std::get<0>(r), L"", LINE_INFO());
            Assert::AreEqual(channel_pool::channel_none + 2, std::get<1>(r), L"", LINE_INFO());
            Assert::IsTrue(ch2 == std::get<2>(r), L"", LINE_INFO());
            
            Logger::WriteMessage(L"Wait [1]\r\n");

            r = pool.wait(std::chrono::nanoseconds::zero(), channel_pool::predicate_t([] (const channel_pool::channel_key_t & key, const channel_base &, const state_t &)
                { return (key == channel_pool::channel_none + 1); }
            ));

            Assert::IsTrue(std::get<0>(r), L"", LINE_INFO());
            Assert::AreEqual(channel_pool::channel_none + 1, std::get<1>(r), L"", LINE_INFO());
            Assert::IsTrue(ch1 == std::get<2>(r), L"", LINE_INFO());
            
            Logger::WriteMessage(L"Wait [2]\r\n");

            r = pool.wait(std::chrono::nanoseconds::zero(), channel_pool::predicate_t([] (const channel_pool::channel_key_t & key, const channel_base &, const state_t &)
                { return (key == channel_pool::channel_none + 2); }
            ));

            Assert::IsTrue(std::get<0>(r), L"", LINE_INFO());
            Assert::AreEqual(channel_pool::channel_none + 2, std::get<1>(r), L"", LINE_INFO());
            Assert::IsTrue(ch2 == std::get<2>(r), L"", LINE_INFO());
		}

        TEST_METHOD(wait_fails_on_closed_pool_immediately)
        {
            auto ch1 = std::make_shared<channel_base>(
                std::make_unique<basic_state_diagram>(),
                std::make_unique<atomic_state_machine>());

            basic_channel_pool pool;
            channel_pool::result_t r;
            
            Logger::WriteMessage(L"Put ch1\r\n");
            
            r = pool.put(ch1);

            Assert::IsTrue(std::get<0>(r), L"", LINE_INFO());
            Assert::AreEqual(channel_pool::channel_none + 1, std::get<1>(r), L"", LINE_INFO());
            Assert::IsTrue(ch1 == std::get<2>(r), L"", LINE_INFO());

            pool.close();
            
            Logger::WriteMessage(L"Wait [1]\r\n");

            try
            {
                r = pool.wait(std::chrono::nanoseconds::zero(), channel_pool::predicate_t([] (const channel_pool::channel_key_t & key, const channel_base &, const state_t &)
                    { return (key == channel_pool::channel_none + 1); }
                ));
                Assert::Fail(L"should not occur", LINE_INFO());
            }
            catch (const error::channel_pool_closed_error &)
            {
            }
		}

        TEST_METHOD(wait_actually_waits)
        {
            auto ch1 = std::make_shared<channel_base>(
                std::make_unique<basic_state_diagram>(),
                std::make_unique<atomic_state_machine>());

            basic_channel_pool pool;
            channel_pool::result_t r;

            std::thread notifier([&] () {
                std::this_thread::sleep_for(std::chrono::seconds(1));
            
                Logger::WriteMessage(L"Put ch1\r\n");
            
                r = pool.put(ch1);
                
                Assert::IsTrue(std::get<0>(r), L"", LINE_INFO());
                Assert::AreEqual(channel_pool::channel_none + 1, std::get<1>(r), L"", LINE_INFO());
                Assert::IsTrue(ch1 == std::get<2>(r), L"", LINE_INFO());
            });
            
            Logger::WriteMessage(L"Wait [1]\r\n");

            r = pool.wait(std::chrono::nanoseconds::zero(), channel_pool::predicate_t([] (const channel_pool::channel_key_t & key, const channel_base &, const state_t &)
                { return (key == channel_pool::channel_none + 1); }
            ));

            Assert::IsTrue(std::get<0>(r), L"", LINE_INFO());
            Assert::AreEqual(channel_pool::channel_none + 1, std::get<1>(r), L"", LINE_INFO());
            Assert::IsTrue(ch1 == std::get<2>(r), L"", LINE_INFO());

            notifier.join();
		}

        TEST_METHOD(wait_returns_if_force_signalled)
        {
            auto ch1 = std::make_shared<channel_base>(
                std::make_unique<basic_state_diagram>(),
                std::make_unique<atomic_state_machine>());

            basic_channel_pool pool;
            channel_pool::result_t r;

            std::thread notifier([&] () {
                std::this_thread::sleep_for(std::chrono::seconds(1));
            
                Logger::WriteMessage(L"Signal\r\n");

                pool.signal();
            });
            
            Logger::WriteMessage(L"Wait [1]\r\n");

            r = pool.wait(std::chrono::nanoseconds::zero(), channel_pool::predicate_t([] (const channel_pool::channel_key_t & key, const channel_base &, const state_t &)
                { return (key == channel_pool::channel_none + 1); }
            ));

            Assert::IsFalse(std::get<0>(r), L"", LINE_INFO());
            Assert::AreEqual(channel_pool::channel_none + 0, std::get<1>(r), L"", LINE_INFO());
            Assert::IsFalse(static_cast<bool>(std::get<2>(r)), L"", LINE_INFO());

            notifier.join();
		}

        TEST_METHOD(wait_fails_on_closed_pool_with_wait)
        {
            auto ch1 = std::make_shared<channel_base>(
                std::make_unique<basic_state_diagram>(),
                std::make_unique<atomic_state_machine>());

            basic_channel_pool pool;
            channel_pool::result_t r;

            std::thread notifier([&] () {
                std::this_thread::sleep_for(std::chrono::seconds(1));
            
                Logger::WriteMessage(L"Signal\r\n");

                pool.close();
            });
            
            Logger::WriteMessage(L"Wait [1]\r\n");

            try
            {
                r = pool.wait(std::chrono::nanoseconds::zero(), channel_pool::predicate_t([] (const channel_pool::channel_key_t & key, const channel_base &, const state_t &)
                    { return (key == channel_pool::channel_none + 1); }
                ));
                Assert::Fail(L"should not occur", LINE_INFO());
            }
            catch (const error::channel_pool_closed_error &)
            {
            }

            notifier.join();
		}

        TEST_METHOD(wait_returns_if_timeout_exceeded)
        {
            auto ch1 = std::make_shared<channel_base>(
                std::make_unique<basic_state_diagram>(),
                std::make_unique<atomic_state_machine>());

            basic_channel_pool pool;
            channel_pool::result_t r;

            std::promise<void> p;

            std::thread failer([&] () {
                if (std::future_status::ready !=
                    p.get_future().wait_for(std::chrono::seconds(1)))
                {
                    Logger::WriteMessage(L"waiting too long\r\n");
                    pool.signal();
                }
            });

            r = pool.wait(std::chrono::nanoseconds::zero(), channel_pool::predicate_t([] (const channel_pool::channel_key_t & key, const channel_base &, const state_t &)
                { return (key == channel_pool::channel_none + 1); }
            ));

            p.set_value();

            Assert::IsFalse(std::get<0>(r), L"", LINE_INFO());
            Assert::AreEqual(channel_pool::channel_none + 0, std::get<1>(r), L"", LINE_INFO());
            Assert::IsFalse(static_cast<bool>(std::get<2>(r)), L"", LINE_INFO());

            failer.join();
		}
	};
}