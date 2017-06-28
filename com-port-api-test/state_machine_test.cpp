#include "stdafx.h"
#include "CppUnitTest.h"
#include "util.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

#include <com-port-api/include/byte_buffer.h>
#include <com-port-api/include/channel.h>
#include <iostream>
#include <future>
#include <thread>

using namespace com_port_api;
using namespace com_port_api::channel;

namespace com_port_api_test
{

	TEST_CLASS(atomic_state_machine_test)
	{

	public:

		TEST_METHOD(set_state)
        {
            atomic_state_machine sm;
            atomic_state_machine::result_t r;
            
            Assert::AreEqual(state_t(states::none), sm.state(), L"", LINE_INFO());
            
            r = sm.set_state(states::none, 123);
            
            Assert::IsTrue(std::get<0>(r), L"", LINE_INFO());
            Assert::AreEqual(state_t(states::none), std::get<1>(r), L"", LINE_INFO());
            Assert::AreEqual(state_t(123), std::get<2>(r), L"", LINE_INFO());
            Assert::AreEqual(state_t(123), sm.state(), L"", LINE_INFO());
            
            r = sm.set_state(125, 126);
            
            Assert::IsFalse(std::get<0>(r), L"", LINE_INFO());
            Assert::AreEqual(state_t(123), std::get<1>(r), L"", LINE_INFO());
            Assert::AreEqual(state_t(123), std::get<2>(r), L"", LINE_INFO());
            Assert::AreEqual(state_t(123), sm.state(), L"", LINE_INFO());
		}

		TEST_METHOD(set_flags)
        {
            atomic_state_machine sm;
            
            Assert::AreEqual(flags_t(0), sm.flags(), L"", LINE_INFO());
            
            sm.set_flags(123);

            Assert::AreEqual(flags_t(123), sm.flags(), L"", LINE_INFO());
		}

		TEST_METHOD(provide_guarantee)
        {
            atomic_state_machine sm;
            
            Assert::AreEqual(state_t(0), sm.state(), L"", LINE_INFO());
            
            sm.set_state(0, 123);

            Assert::AreEqual(state_t(123), sm.state(), L"", LINE_INFO());

            // test that no changes performed
            sm.provide_guarantee();

            Assert::AreEqual(state_t(123), sm.state(), L"", LINE_INFO());
		}

		TEST_METHOD(lock_op)
        {
            class s_d : public state_diagram
            {
                
                virtual result_t lock_op  (constant_t op,
                                           state_t    started_with,
                                           flags_t    flags) const override
                {
                    if (op == 426)
                    {
                        Assert::AreEqual(state_t(123), started_with, L"", LINE_INFO());
                        Assert::AreEqual(flags_t(234), flags,        L"", LINE_INFO());
                        return result_t{ true, 124, guarantee_t::acquire };
                    }
                    else if (op == 427)
                    {
                        Assert::AreEqual(state_t(125), started_with, L"", LINE_INFO());
                        Assert::AreEqual(flags_t(235), flags,        L"", LINE_INFO());
                        return result_t{ false, 125, guarantee_t::acquire };
                    }
                    Assert::Fail(L"unexpected", LINE_INFO());
                    throw std::runtime_error("unexpected");
                }

                virtual result_t unlock_op(constant_t       op,
                                           state_t          started_with,
                                           state_t          locked_with,
                                           flags_t          flags,
                                           ops::result_type op_result) const override
                {
                    Assert::Fail(L"unexpected call", LINE_INFO());
                    throw std::runtime_error("unexpected call");
                }
            };

            atomic_state_machine sm;
            atomic_state_machine::result_t r;
            s_d sd;
            
            Assert::AreEqual(state_t(0), sm.state(), L"", LINE_INFO());
            Assert::AreEqual(flags_t(0), sm.flags(), L"", LINE_INFO());
            
            sm.set_state(0, 123);
            sm.set_flags(234);

            Assert::AreEqual(state_t(123), sm.state(), L"", LINE_INFO());
            Assert::AreEqual(flags_t(234), sm.flags(), L"", LINE_INFO());

            r = sm.lock_op(sd, 426);

            Assert::IsTrue(std::get<0>(r), L"", LINE_INFO());
            Assert::AreEqual(state_t(123), std::get<1>(r), L"", LINE_INFO());
            Assert::AreEqual(state_t(124), std::get<2>(r), L"", LINE_INFO());
            Assert::AreEqual(state_t(124), sm.state(), L"", LINE_INFO());
            
            sm.set_state(124, 125);
            sm.set_flags(235);

            Assert::AreEqual(state_t(125), sm.state(), L"", LINE_INFO());
            Assert::AreEqual(flags_t(235), sm.flags(), L"", LINE_INFO());

            r = sm.lock_op(sd, 427);

            Assert::IsFalse(std::get<0>(r), L"", LINE_INFO());
            Assert::AreEqual(state_t(125), std::get<1>(r), L"", LINE_INFO());
            Assert::AreEqual(state_t(125), std::get<2>(r), L"", LINE_INFO());
            Assert::AreEqual(state_t(125), sm.state(), L"", LINE_INFO());
		}

		TEST_METHOD(unlock_op)
        {
            class s_d : public state_diagram
            {
                
                virtual result_t lock_op  (constant_t op,
                                           state_t    started_with,
                                           flags_t    flags) const override
                {
                    Assert::Fail(L"unexpected call", LINE_INFO());
                    throw std::runtime_error("unexpected call");
                }

                virtual result_t unlock_op(constant_t       op,
                                           state_t          started_with,
                                           state_t          locked_with,
                                           flags_t          flags,
                                           ops::result_type op_result) const override
                {
                    if (op == 426)
                    {
                        Assert::AreEqual(state_t(123), started_with, L"", LINE_INFO());
                        Assert::AreEqual(state_t(321), locked_with,  L"", LINE_INFO());
                        Assert::AreEqual(flags_t(234), flags,        L"", LINE_INFO());
                        Assert::AreEqual(ops::result_type::result_success, op_result, L"", LINE_INFO());
                        return result_t{ true, 124, guarantee_t::acquire };
                    }
                    else if (op == 427)
                    {
                        Assert::AreEqual(state_t(125), started_with, L"", LINE_INFO());
                        Assert::AreEqual(state_t(521), locked_with,  L"", LINE_INFO());
                        Assert::AreEqual(flags_t(235), flags,        L"", LINE_INFO());
                        Assert::AreEqual(ops::result_type::result_failure, op_result, L"", LINE_INFO());
                        return result_t{ false, 125, guarantee_t::acquire };
                    }
                    Assert::Fail(L"unexpected", LINE_INFO());
                    throw std::runtime_error("unexpected");
                }
            };

            atomic_state_machine sm;
            atomic_state_machine::result_t r;
            s_d sd;
            
            Assert::AreEqual(state_t(0), sm.state(), L"", LINE_INFO());
            Assert::AreEqual(flags_t(0), sm.flags(), L"", LINE_INFO());
            
            sm.set_state(0, 123);
            sm.set_flags(234);

            Assert::AreEqual(state_t(123), sm.state(), L"", LINE_INFO());
            Assert::AreEqual(flags_t(234), sm.flags(), L"", LINE_INFO());

            r = sm.unlock_op(sd, 426, 321, ops::result_type::result_success);

            Assert::IsTrue(std::get<0>(r), L"", LINE_INFO());
            Assert::AreEqual(state_t(123), std::get<1>(r), L"", LINE_INFO());
            Assert::AreEqual(state_t(124), std::get<2>(r), L"", LINE_INFO());
            Assert::AreEqual(state_t(124), sm.state(), L"", LINE_INFO());
            
            sm.set_state(124, 125);
            sm.set_flags(235);

            Assert::AreEqual(state_t(125), sm.state(), L"", LINE_INFO());
            Assert::AreEqual(flags_t(235), sm.flags(), L"", LINE_INFO());

            r = sm.unlock_op(sd, 427, 521, ops::result_type::result_failure);

            Assert::IsFalse(std::get<0>(r), L"", LINE_INFO());
            Assert::AreEqual(state_t(125), std::get<1>(r), L"", LINE_INFO());
            Assert::AreEqual(state_t(125), std::get<2>(r), L"", LINE_INFO());
            Assert::AreEqual(state_t(125), sm.state(), L"", LINE_INFO());
		}

	};

    

	TEST_CLASS(blocking_state_machine_test)
	{

	public:

		TEST_METHOD(set_state)
        {
            blocking_state_machine sm;
            blocking_state_machine::result_t r;
            
            Assert::AreEqual(state_t(states::none), sm.state(), L"", LINE_INFO());
            
            r = sm.set_state(states::none, 123);
            
            Assert::IsTrue(std::get<0>(r), L"", LINE_INFO());
            Assert::AreEqual(state_t(states::none), std::get<1>(r), L"", LINE_INFO());
            Assert::AreEqual(state_t(123), std::get<2>(r), L"", LINE_INFO());
            Assert::AreEqual(state_t(123), sm.state(), L"", LINE_INFO());
            
            r = sm.set_state(125, 126);
            
            Assert::IsFalse(std::get<0>(r), L"", LINE_INFO());
            Assert::AreEqual(state_t(123), std::get<1>(r), L"", LINE_INFO());
            Assert::AreEqual(state_t(123), std::get<2>(r), L"", LINE_INFO());
            Assert::AreEqual(state_t(123), sm.state(), L"", LINE_INFO());
		}

		TEST_METHOD(set_flags)
        {
            blocking_state_machine sm;
            
            Assert::AreEqual(flags_t(0), sm.flags(), L"", LINE_INFO());
            
            sm.set_flags(123);

            Assert::AreEqual(flags_t(123), sm.flags(), L"", LINE_INFO());
		}

		TEST_METHOD(provide_guarantee)
        {
            blocking_state_machine sm;
            
            Assert::AreEqual(state_t(0), sm.state(), L"", LINE_INFO());
            
            sm.set_state(0, 123);

            Assert::AreEqual(state_t(123), sm.state(), L"", LINE_INFO());

            // test that no changes performed
            sm.provide_guarantee();

            Assert::AreEqual(state_t(123), sm.state(), L"", LINE_INFO());
		}

		TEST_METHOD(lock_op)
        {
            class s_d : public state_diagram
            {
                
                virtual result_t lock_op  (constant_t op,
                                           state_t    started_with,
                                           flags_t    flags) const override
                {
                    if (op == 426)
                    {
                        Assert::AreEqual(state_t(123), started_with, L"", LINE_INFO());
                        Assert::AreEqual(flags_t(234), flags,        L"", LINE_INFO());
                        return result_t{ true, 124, guarantee_t::acquire };
                    }
                    else if (op == 427)
                    {
                        Assert::AreEqual(state_t(125), started_with, L"", LINE_INFO());
                        Assert::AreEqual(flags_t(235), flags,        L"", LINE_INFO());
                        return result_t{ false, 125, guarantee_t::acquire };
                    }
                    Assert::Fail(L"unexpected", LINE_INFO());
                    throw std::runtime_error("unexpected");
                }

                virtual result_t unlock_op(constant_t       op,
                                           state_t          started_with,
                                           state_t          locked_with,
                                           flags_t          flags,
                                           ops::result_type op_result) const override
                {
                    Assert::Fail(L"unexpected call", LINE_INFO());
                    throw std::runtime_error("unexpected call");
                }
            };

            blocking_state_machine sm;
            blocking_state_machine::result_t r;
            s_d sd;
            
            Assert::AreEqual(state_t(0), sm.state(), L"", LINE_INFO());
            Assert::AreEqual(flags_t(0), sm.flags(), L"", LINE_INFO());
            
            sm.set_state(0, 123);
            sm.set_flags(234);

            Assert::AreEqual(state_t(123), sm.state(), L"", LINE_INFO());
            Assert::AreEqual(flags_t(234), sm.flags(), L"", LINE_INFO());

            r = sm.lock_op(sd, 426);

            Assert::IsTrue(std::get<0>(r), L"", LINE_INFO());
            Assert::AreEqual(state_t(123), std::get<1>(r), L"", LINE_INFO());
            Assert::AreEqual(state_t(124), std::get<2>(r), L"", LINE_INFO());
            Assert::AreEqual(state_t(124), sm.state(), L"", LINE_INFO());
            
            sm.set_state(124, 125);
            sm.set_flags(235);

            Assert::AreEqual(state_t(125), sm.state(), L"", LINE_INFO());
            Assert::AreEqual(flags_t(235), sm.flags(), L"", LINE_INFO());

            r = sm.lock_op(sd, 427);

            Assert::IsFalse(std::get<0>(r), L"", LINE_INFO());
            Assert::AreEqual(state_t(125), std::get<1>(r), L"", LINE_INFO());
            Assert::AreEqual(state_t(125), std::get<2>(r), L"", LINE_INFO());
            Assert::AreEqual(state_t(125), sm.state(), L"", LINE_INFO());
		}

		TEST_METHOD(unlock_op)
        {
            class s_d : public state_diagram
            {
                
                virtual result_t lock_op  (constant_t op,
                                           state_t    started_with,
                                           flags_t    flags) const override
                {
                    Assert::Fail(L"unexpected call", LINE_INFO());
                    throw std::runtime_error("unexpected call");
                }

                virtual result_t unlock_op(constant_t       op,
                                           state_t          started_with,
                                           state_t          locked_with,
                                           flags_t          flags,
                                           ops::result_type op_result) const override
                {
                    if (op == 426)
                    {
                        Assert::AreEqual(state_t(123), started_with, L"", LINE_INFO());
                        Assert::AreEqual(state_t(321), locked_with,  L"", LINE_INFO());
                        Assert::AreEqual(flags_t(234), flags,        L"", LINE_INFO());
                        Assert::AreEqual(ops::result_type::result_success, op_result, L"", LINE_INFO());
                        return result_t{ true, 124, guarantee_t::acquire };
                    }
                    else if (op == 427)
                    {
                        Assert::AreEqual(state_t(125), started_with, L"", LINE_INFO());
                        Assert::AreEqual(state_t(521), locked_with,  L"", LINE_INFO());
                        Assert::AreEqual(flags_t(235), flags,        L"", LINE_INFO());
                        Assert::AreEqual(ops::result_type::result_failure, op_result, L"", LINE_INFO());
                        return result_t{ false, 125, guarantee_t::acquire };
                    }
                    Assert::Fail(L"unexpected", LINE_INFO());
                    throw std::runtime_error("unexpected");
                }
            };

            blocking_state_machine sm;
            blocking_state_machine::result_t r;
            s_d sd;
            
            Assert::AreEqual(state_t(0), sm.state(), L"", LINE_INFO());
            Assert::AreEqual(flags_t(0), sm.flags(), L"", LINE_INFO());
            
            sm.set_state(0, 123);
            sm.set_flags(234);

            Assert::AreEqual(state_t(123), sm.state(), L"", LINE_INFO());
            Assert::AreEqual(flags_t(234), sm.flags(), L"", LINE_INFO());

            r = sm.unlock_op(sd, 426, 321, ops::result_type::result_success);

            Assert::IsTrue(std::get<0>(r), L"", LINE_INFO());
            Assert::AreEqual(state_t(123), std::get<1>(r), L"", LINE_INFO());
            Assert::AreEqual(state_t(124), std::get<2>(r), L"", LINE_INFO());
            Assert::AreEqual(state_t(124), sm.state(), L"", LINE_INFO());
            
            sm.set_state(124, 125);
            sm.set_flags(235);

            Assert::AreEqual(state_t(125), sm.state(), L"", LINE_INFO());
            Assert::AreEqual(flags_t(235), sm.flags(), L"", LINE_INFO());

            r = sm.unlock_op(sd, 427, 521, ops::result_type::result_failure);

            Assert::IsFalse(std::get<0>(r), L"", LINE_INFO());
            Assert::AreEqual(state_t(125), std::get<1>(r), L"", LINE_INFO());
            Assert::AreEqual(state_t(125), std::get<2>(r), L"", LINE_INFO());
            Assert::AreEqual(state_t(125), sm.state(), L"", LINE_INFO());
		}

		TEST_METHOD(wait_actually_waits)
        {
            blocking_state_machine sm;
            blocking_state_machine::result_t r;
            
            Assert::AreEqual(state_t(0), sm.state(), L"", LINE_INFO());
            
            sm.set_state(0, states::opening);

            Assert::AreEqual(state_t(states::opening), sm.state(), L"", LINE_INFO());

            std::thread notifier([&] () {
                std::this_thread::sleep_for(std::chrono::seconds(1));

                Assert::AreEqual(state_t(states::opening), sm.state(), L"", LINE_INFO());

                sm.set_state(states::opening, states::open);

                Assert::AreEqual(state_t(states::open), sm.state(), L"", LINE_INFO());
            });
            
            Assert::AreEqual(state_t(states::opening), sm.state(), L"", LINE_INFO());

            r = sm.wait([](const state_t &s) { return (s == states::open); });

            Assert::IsTrue(std::get<0>(r), L"", LINE_INFO());
            Assert::AreEqual(state_t(states::opening), std::get<1>(r), L"", LINE_INFO());
            Assert::AreEqual(state_t(states::open), std::get<2>(r), L"", LINE_INFO());
            Assert::AreEqual(state_t(states::open), sm.state(), L"", LINE_INFO());

            notifier.join();
		}

		TEST_METHOD(wait_returns_immediately_if_machine_is_in_requested_state)
        {
            blocking_state_machine sm;
            blocking_state_machine::result_t r;
            
            Assert::AreEqual(state_t(0), sm.state(), L"", LINE_INFO());
            
            sm.set_state(0, states::open);

            Assert::AreEqual(state_t(states::open), sm.state(), L"", LINE_INFO());

            std::promise<void> p;

            std::thread failer([&] () {
                if (std::future_status::ready !=
                    p.get_future().wait_for(std::chrono::seconds(1)))
                {
                    Logger::WriteMessage(L"waiting too long\n");
                    sm.notify();
                }
            });

            // must return immediately
            r = sm.wait([](const state_t &s) { return (s == states::open); });

            Assert::IsTrue(std::get<0>(r), L"", LINE_INFO());
            Assert::AreEqual(state_t(states::open), std::get<1>(r), L"", LINE_INFO());
            Assert::AreEqual(state_t(states::open), std::get<2>(r), L"", LINE_INFO());
            Assert::AreEqual(state_t(states::open), sm.state(), L"", LINE_INFO());

            p.set_value();
            failer.join();
		}

		TEST_METHOD(wait_returns_if_closed_state_occur)
        {
            blocking_state_machine sm;
            blocking_state_machine::result_t r;
            
            Assert::AreEqual(state_t(0), sm.state(), L"", LINE_INFO());
            
            sm.set_state(0, states::opening);

            Assert::AreEqual(state_t(states::opening), sm.state(), L"", LINE_INFO());

            std::thread notifier([&] () {
                std::this_thread::sleep_for(std::chrono::seconds(1));

                Assert::AreEqual(state_t(states::opening), sm.state(), L"", LINE_INFO());

                sm.set_state(states::opening, states::closed);

                Assert::AreEqual(state_t(states::closed), sm.state(), L"", LINE_INFO());
            });
            
            Assert::AreEqual(state_t(states::opening), sm.state(), L"", LINE_INFO());

            r = sm.wait([](const state_t &s) { return (s == states::open); });

            Assert::IsFalse(std::get<0>(r), L"", LINE_INFO());
            Assert::AreEqual(state_t(states::opening), std::get<1>(r), L"", LINE_INFO());
            Assert::AreEqual(state_t(states::closed), std::get<2>(r), L"", LINE_INFO());
            Assert::AreEqual(state_t(states::closed), sm.state(), L"", LINE_INFO());

            notifier.join();
		}

		TEST_METHOD(wait_returns_if_notify_called)
        {
            blocking_state_machine sm;
            blocking_state_machine::result_t r;
            
            Assert::AreEqual(state_t(0), sm.state(), L"", LINE_INFO());
            
            sm.set_state(0, states::open);

            Assert::AreEqual(state_t(states::open), sm.state(), L"", LINE_INFO());

            std::promise<void> p;

            std::thread notifier([&] () {
                if (std::future_status::ready !=
                    p.get_future().wait_for(std::chrono::seconds(1)))
                {
                    sm.notify();
                }
            });

            r = sm.wait([](const state_t &s) { return (s == states::closed); });

            Assert::IsFalse(std::get<0>(r), L"", LINE_INFO());
            Assert::AreEqual(state_t(states::open), std::get<1>(r), L"", LINE_INFO());
            Assert::AreEqual(state_t(states::open), std::get<2>(r), L"", LINE_INFO());
            Assert::AreEqual(state_t(states::open), sm.state(), L"", LINE_INFO());

            p.set_value();
            notifier.join();
		}

		TEST_METHOD(wait_returns_if_timeout_exceeded)
        {
            blocking_state_machine sm;
            blocking_state_machine::result_t r;
            
            Assert::AreEqual(state_t(0), sm.state(), L"", LINE_INFO());
            
            sm.set_state(0, states::open);

            Assert::AreEqual(state_t(states::open), sm.state(), L"", LINE_INFO());

            std::promise<void> p;
            std::future<void> f = p.get_future();

            std::thread failer([&] () {
                if (std::future_status::ready !=
                    f.wait_for(std::chrono::seconds(2)))
                {
                    sm.notify();
                }
            });

            r = sm.wait(std::chrono::seconds(1),
                        [](const state_t &s) { return (s == states::closed); });
            
            Assert::IsFalse(std::future_status::ready == f.wait_for(std::chrono::milliseconds(1)), L"", LINE_INFO());
            Assert::IsFalse(std::get<0>(r), L"", LINE_INFO());
            Assert::AreEqual(state_t(states::open), std::get<1>(r), L"", LINE_INFO());
            Assert::AreEqual(state_t(states::open), std::get<2>(r), L"", LINE_INFO());
            Assert::AreEqual(state_t(states::open), sm.state(), L"", LINE_INFO());

            p.set_value();
            failer.join();
		}

	};

}