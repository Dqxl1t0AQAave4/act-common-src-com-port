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

    using testing_channel_base = channel_base < basic_state_diagram, atomic_state_machine > ;
    

    enum : constant_t
    {
        initial = (1 << 8)
    };

    class test_channel_base : public testing_channel_base
    {
    public:

        test_channel_base()
        {
            _machine.set_flags(flags::readable | flags::writable);
            _machine.set_state(states::none, initial);
        }

        virtual result_t do_as(constant_t op,
                               std::function < void () > fn) override
        {
            return testing_channel_base::do_as(op, fn);
        }
        virtual result_t do_as(constant_t op,
                               std::function < void (success_handler_t,
                                                     error_handler_t) > fn,
                               success_handler_t success_cb,
                               error_handler_t   failure_cb) override
        {
            return testing_channel_base::do_as(op, fn, success_cb, failure_cb);
        }

        state_machine_t & machine()
        {
            return _machine;
        }
    };

	TEST_CLASS(channel_base_test)
	{

	public:

		TEST_METHOD(blocking_do_as_performs_valid_state_transitions_on_success)
        {
            test_channel_base cb;
            state_machine &sm = cb.machine();
            test_channel_base::result_t r;

            Assert::AreEqual(state_t(initial), sm.state(), L"", LINE_INFO());

            r = cb.do_as(ops::open, [&] () {
                Assert::AreEqual(state_t(states::opening | initial), sm.state(), L"", LINE_INFO());
            });

            Assert::AreEqual(state_t(states::open | initial | states::readable | states::writable), sm.state(), L"", LINE_INFO());
            Assert::IsTrue(std::get<0>(r), L"", LINE_INFO());
            Assert::AreEqual(state_t(initial), std::get<1>(r), L"", LINE_INFO());
            Assert::AreEqual(sm.state(), std::get<2>(r), L"", LINE_INFO());
		}

		TEST_METHOD(blocking_do_as_performs_valid_state_transitions_on_success_with_state_change_while_processing)
        {
            test_channel_base cb;
            state_machine &sm = cb.machine();
            test_channel_base::result_t r;

            Assert::AreEqual(state_t(initial), sm.state(), L"", LINE_INFO());

            r = cb.do_as(ops::open, [&] () {
                Assert::AreEqual(state_t(states::opening | initial), sm.state(), L"", LINE_INFO());
                sm.set_state(sm.state(), sm.state() - initial);
                Assert::AreEqual(state_t(states::opening), sm.state(), L"", LINE_INFO());
            });

            Assert::AreEqual(state_t(states::open | states::readable | states::writable), sm.state(), L"", LINE_INFO());
            Assert::IsTrue(std::get<0>(r), L"", LINE_INFO());
            Assert::AreEqual(state_t(initial), std::get<1>(r), L"", LINE_INFO());
            Assert::AreEqual(sm.state(), std::get<2>(r), L"", LINE_INFO());
		}

		TEST_METHOD(blocking_do_as_performs_valid_state_transitions_on_failure_and_rethrows_original_exception)
        {
            test_channel_base cb;
            state_machine &sm = cb.machine();
            test_channel_base::result_t r;

            Assert::AreEqual(state_t(initial), sm.state(), L"", LINE_INFO());

            try
            {
                cb.do_as(ops::open, [&] () {
                    Assert::AreEqual(state_t(states::opening | initial), sm.state(), L"", LINE_INFO());
                    throw error::channel_error("unsupported");
                });
                Assert::Fail(L"exception expected", LINE_INFO());
            }
            catch (const error::channel_error &er)
            {
                Assert::AreEqual("unsupported", er.what(), L"", LINE_INFO());
            }

            Assert::AreEqual(state_t(initial), sm.state(), L"", LINE_INFO());

            try
            {
                cb.do_as(ops::open, [&] () {
                    Assert::AreEqual(state_t(states::opening | initial), sm.state(), L"", LINE_INFO());
                    throw 123;
                });
                Assert::Fail(L"exception expected", LINE_INFO());
            }
            catch (const int &er)
            {
                Assert::AreEqual(123, er, L"", LINE_INFO());
            }

            Assert::AreEqual(state_t(initial), sm.state(), L"", LINE_INFO());
		}

		TEST_METHOD(blocking_do_as_fails_if_channel_is_in_wrong_state)
        {
            test_channel_base cb;
            state_machine &sm = cb.machine();
            test_channel_base::result_t r;

            sm.set_state(initial, initial | states::opening);

            Assert::AreEqual(state_t(initial | states::opening), sm.state(), L"", LINE_INFO());

            r = cb.do_as(ops::open, [&] () {
                Assert::Fail(L"should not be called", LINE_INFO());
            });

            Assert::AreEqual(state_t(states::opening | initial), sm.state(), L"", LINE_INFO());
            Assert::IsFalse(std::get<0>(r), L"", LINE_INFO());
            Assert::AreEqual(sm.state(), std::get<1>(r), L"", LINE_INFO());
            Assert::AreEqual(sm.state(), std::get<2>(r), L"", LINE_INFO());
		}

		TEST_METHOD(nonblocking_do_as_do_not_calls_handlers_by_itself)
        {
            test_channel_base cb;
            state_machine &sm = cb.machine();
            test_channel_base::result_t r;

            Assert::AreEqual(state_t(initial), sm.state(), L"", LINE_INFO());

            r = cb.do_as(ops::open,
            [&] (success_handler_t s, error_handler_t e) {
                Assert::AreEqual(state_t(states::opening | initial), sm.state(), L"", LINE_INFO());
            },
            [&] () {
                Assert::Fail(L"success handler never called", LINE_INFO());
            },
            [&] (const error::channel_error &err) {
                Assert::Fail(L"error handler never called", LINE_INFO());
            });

            Assert::AreEqual(state_t(states::opening | initial), sm.state(), L"", LINE_INFO());
            Assert::IsTrue(std::get<0>(r), L"", LINE_INFO());
            Assert::AreEqual(state_t(initial), std::get<1>(r), L"", LINE_INFO());
            Assert::AreEqual(sm.state(), std::get<2>(r), L"", LINE_INFO());
		}

		TEST_METHOD(nonblocking_do_as_performs_valid_state_transitions_on_success)
        {
            test_channel_base cb;
            state_machine &sm = cb.machine();
            test_channel_base::result_t r;

            Assert::AreEqual(state_t(initial), sm.state(), L"", LINE_INFO());

            bool called = false;

            r = cb.do_as(ops::open,
            [&] (success_handler_t s, error_handler_t e) {
                Assert::AreEqual(state_t(states::opening | initial), sm.state(), L"", LINE_INFO());
                s();
                Assert::AreEqual(state_t(states::open | initial | states::readable | states::writable), sm.state(), L"", LINE_INFO());
            },
            [&] () {
                called = true;
            },
            [&] (const error::channel_error &err) {
                Assert::Fail(L"error handler never called", LINE_INFO());
            });
            
            Assert::IsTrue(called, L"", LINE_INFO());
            Assert::AreEqual(state_t(states::open | initial | states::readable | states::writable), sm.state(), L"", LINE_INFO());
            
            Assert::IsTrue(std::get<0>(r), L"", LINE_INFO());
            Assert::AreEqual(state_t(initial), std::get<1>(r), L"", LINE_INFO());
            // differs from final state
            Assert::AreEqual(state_t(states::opening | initial), std::get<2>(r), L"", LINE_INFO());
		}

		TEST_METHOD(nonblocking_do_as_performs_valid_state_transitions_on_failure)
        {
            test_channel_base cb;
            state_machine &sm = cb.machine();
            test_channel_base::result_t r;

            Assert::AreEqual(state_t(initial), sm.state(), L"", LINE_INFO());

            bool called = false;

            r = cb.do_as(ops::open,
            [&] (success_handler_t s, error_handler_t e) {
                Assert::AreEqual(state_t(states::opening | initial), sm.state(), L"", LINE_INFO());
                e(error::channel_error("unexpected"));
                Assert::AreEqual(state_t(initial), sm.state(), L"", LINE_INFO());
            },
            [&] () {
                Assert::Fail(L"error handler never called", LINE_INFO());
            },
            [&] (const error::channel_error &err) {
                Assert::AreEqual("unexpected", err.what(), L"", LINE_INFO());
                called = true;
            });
            
            Assert::IsTrue(called, L"", LINE_INFO());
            Assert::AreEqual(state_t(initial), sm.state(), L"", LINE_INFO());
            
            Assert::IsTrue(std::get<0>(r), L"", LINE_INFO());
            Assert::AreEqual(state_t(initial), std::get<1>(r), L"", LINE_INFO());
            Assert::AreEqual(state_t(initial | states::opening), std::get<2>(r), L"", LINE_INFO());
		}

		TEST_METHOD(nonblocking_do_as_performs_valid_state_transitions_on_failure_with_channel_error)
        {
            test_channel_base cb;
            state_machine &sm = cb.machine();
            test_channel_base::result_t r;

            Assert::AreEqual(state_t(initial), sm.state(), L"", LINE_INFO());

            bool called = false;

            r = cb.do_as(ops::open,
            [&] (success_handler_t s, error_handler_t e) {
                Assert::AreEqual(state_t(states::opening | initial), sm.state(), L"", LINE_INFO());
                throw error::channel_error("unexpected");
            },
            [&] () {
                Assert::Fail(L"error handler never called", LINE_INFO());
            },
            [&] (const error::channel_error &err) {
                Assert::AreEqual("unexpected", err.what(), L"", LINE_INFO());
                called = true;
            });
            
            Assert::IsTrue(called, L"", LINE_INFO());
            Assert::AreEqual(state_t(initial), sm.state(), L"", LINE_INFO());
            
            Assert::IsTrue(std::get<0>(r), L"", LINE_INFO());
            Assert::AreEqual(state_t(initial), std::get<1>(r), L"", LINE_INFO());
            Assert::AreEqual(state_t(initial | states::opening), std::get<2>(r), L"", LINE_INFO());
		}

		TEST_METHOD(nonblocking_do_as_performs_valid_state_transitions_on_failure_with_exception)
        {
            test_channel_base cb;
            state_machine &sm = cb.machine();
            test_channel_base::result_t r;

            Assert::AreEqual(state_t(initial), sm.state(), L"", LINE_INFO());

            try {
                r = cb.do_as(ops::open,
                [&] (success_handler_t s, error_handler_t e) {
                    Assert::AreEqual(state_t(states::opening | initial), sm.state(), L"", LINE_INFO());
                    throw 123;
                },
                [&] () {
                    Assert::Fail(L"error handler never called", LINE_INFO());
                },
                [&] (const error::channel_error &err) {
                    Assert::Fail(L"error handler never called", LINE_INFO());
                });
                Assert::Fail(L"should not be executed", LINE_INFO());
            }
            catch (const int &err)
            {
                Assert::AreEqual(123, err, L"", LINE_INFO());
            }

            Assert::AreEqual(state_t(initial), sm.state(), L"", LINE_INFO());
		}

		TEST_METHOD(nonblocking_do_as_returns_if_started_with_invalid_state)
        {
            test_channel_base cb;
            state_machine &sm = cb.machine();
            test_channel_base::result_t r;

            sm.set_state(initial, states::closing | initial);

            Assert::AreEqual(state_t(initial | states::closing), sm.state(), L"", LINE_INFO());

            r = cb.do_as(ops::open,
            [&] (success_handler_t s, error_handler_t e) {
                Assert::Fail(L"never called", LINE_INFO());
            },
            [&] () {
                Assert::Fail(L"never called", LINE_INFO());
            },
            [&] (const error::channel_error &err) {
                Assert::Fail(L"error handler never called", LINE_INFO());
            });
            
            Assert::AreEqual(state_t(initial | states::closing), sm.state(), L"", LINE_INFO());
            
            Assert::IsFalse(std::get<0>(r), L"", LINE_INFO());
            Assert::AreEqual(state_t(initial | states::closing), std::get<1>(r), L"", LINE_INFO());
            Assert::AreEqual(state_t(initial | states::closing), std::get<2>(r), L"", LINE_INFO());
		}
	};
}