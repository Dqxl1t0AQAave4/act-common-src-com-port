#include "stdafx.h"
#include "CppUnitTest.h"
#include "util.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

#include <com-port-api/include/byte_buffer.h>
#include <com-port-api/include/channel.h>
#include <iostream>
#include <future>
#include <thread>
#include <atomic>

using namespace com_port_api;
using namespace com_port_api::channel;

namespace com_port_api_test
{

    template < class state_machine_t >
    class test_channel_base : public channel_base
    {
    public:

        test_channel_base()
            : channel_base(std::make_unique<basic_state_diagram>(),
                           std::make_unique<state_machine_t>())
        {
        }

        virtual result_t do_as(constant_t op,
                               std::function < void () > fn) override
        {
            return channel_base::do_as(op, fn);
        }
        virtual result_t do_as(constant_t op,
                               std::function < void (success_handler_t,
                                                     error_handler_t) > fn,
                               success_handler_t success_cb,
                               error_handler_t   failure_cb) override
        {
            return channel_base::do_as(op, fn, success_cb, failure_cb);
        }

        state_machine & machine()
        {
            return *_machine;
        }
    };

	TEST_CLASS(memory_consistency_test)
	{

	public:

		TEST_METHOD(open_to_read_consistency_with_atomic_state_machine)
        {
            for (std::size_t i = 0; i < 1000; i++)
            {
                test_channel_base<atomic_state_machine> cb;
                test_channel_base<atomic_state_machine>::result_t r;

                cb.machine().set_flags(flags::readable);
                
                std::atomic<bool> c(false);
                int k = 0;

                std::thread t([&] () {
                    cb.do_as(ops::open, [&] () {
                        while (k < 1000) k++;
                    });
                });
                do
                {
                    r = cb.do_as(ops::read, [&] () {
                        int kk = k;
                        if ((kk != 0) && (kk != 1000))
                        {
                            c.store(true, std::memory_order_relaxed);
                        }
                    });
                }
                while (!std::get<0>(r));

                t.join();

                if (c)
                {
                    Assert::Fail(L"inconsistency detected", LINE_INFO());
                }
            }
		}

		TEST_METHOD(open_to_read_consistency_with_blocking_state_machine)
        {
            for (std::size_t i = 0; i < 1000; i++)
            {
                test_channel_base<blocking_state_machine> cb;
                test_channel_base<blocking_state_machine>::result_t r;

                cb.machine().set_flags(flags::readable);
                
                std::atomic<bool> c(false);
                int k = 0;

                std::thread t([&] () {
                    cb.do_as(ops::open, [&] () {
                        while (k < 1000) k++;
                    });
                });
                do
                {
                    r = cb.do_as(ops::read, [&] () {
                        int kk = k;
                        if ((kk != 0) && (kk != 1000))
                        {
                            c.store(true, std::memory_order_relaxed);
                        }
                    });
                }
                while (!std::get<0>(r));

                t.join();

                if (c)
                {
                    Assert::Fail(L"inconsistency detected", LINE_INFO());
                }
            }
		}

		TEST_METHOD(open_to_write_consistency_with_atomic_state_machine)
        {
            for (std::size_t i = 0; i < 1000; i++)
            {
                test_channel_base<atomic_state_machine> cb;
                test_channel_base<atomic_state_machine>::result_t r;

                cb.machine().set_flags(flags::writable);
                
                std::atomic<bool> c(false);
                int k = 0;

                std::thread t([&] () {
                    cb.do_as(ops::open, [&] () {
                        while (k < 1000) k++;
                    });
                });
                do
                {
                    r = cb.do_as(ops::write, [&] () {
                        int kk = k;
                        if ((kk != 0) && (kk != 1000))
                        {
                            c.store(true, std::memory_order_relaxed);
                        }
                    });
                }
                while (!std::get<0>(r));

                t.join();

                if (c)
                {
                    Assert::Fail(L"inconsistency detected", LINE_INFO());
                }
            }
		}

		TEST_METHOD(open_to_write_consistency_with_blocking_state_machine)
        {
            for (std::size_t i = 0; i < 1000; i++)
            {
                test_channel_base<blocking_state_machine> cb;
                test_channel_base<blocking_state_machine>::result_t r;

                cb.machine().set_flags(flags::writable);
                
                std::atomic<bool> c(false);
                int k = 0;

                std::thread t([&] () {
                    cb.do_as(ops::open, [&] () {
                        while (k < 1000) k++;
                    });
                });
                do
                {
                    r = cb.do_as(ops::write, [&] () {
                        int kk = k;
                        if ((kk != 0) && (kk != 1000))
                        {
                            c.store(true, std::memory_order_relaxed);
                        }
                    });
                }
                while (!std::get<0>(r));

                t.join();

                if (c)
                {
                    Assert::Fail(L"inconsistency detected", LINE_INFO());
                }
            }
		}

		TEST_METHOD(open_to_close_consistency_with_atomic_state_machine)
        {
            for (std::size_t i = 0; i < 1000; i++)
            {
                test_channel_base<atomic_state_machine> cb;
                test_channel_base<atomic_state_machine>::result_t r;
                
                std::atomic<bool> c(false);
                int k = 0;

                std::thread t([&] () {
                    cb.do_as(ops::open, [&] () {
                        while (k < 1000) k++;
                    });
                });
                do
                {
                    r = cb.do_as(ops::close, [&] () {
                        int kk = k;
                        if ((kk != 0) && (kk != 1000))
                        {
                            c.store(true, std::memory_order_relaxed);
                        }
                    });
                }
                while (!std::get<0>(r));

                t.join();

                if (c)
                {
                    Assert::Fail(L"inconsistency detected", LINE_INFO());
                }
            }
		}

		TEST_METHOD(open_to_close_consistency_with_blocking_state_machine)
        {
            for (std::size_t i = 0; i < 1000; i++)
            {
                test_channel_base<blocking_state_machine> cb;
                test_channel_base<blocking_state_machine>::result_t r;
                
                std::atomic<bool> c(false);
                int k = 0;

                std::thread t([&] () {
                    cb.do_as(ops::open, [&] () {
                        while (k < 1000) k++;
                    });
                });
                do
                {
                    r = cb.do_as(ops::close, [&] () {
                        int kk = k;
                        if ((kk != 0) && (kk != 1000))
                        {
                            c.store(true, std::memory_order_relaxed);
                        }
                    });
                }
                while (!std::get<0>(r));

                t.join();

                if (c)
                {
                    Assert::Fail(L"inconsistency detected", LINE_INFO());
                }
            }
		}

		TEST_METHOD(read_to_close_consistency_with_atomic_state_machine)
        {
            for (std::size_t i = 0; i < 1000; i++)
            {
                test_channel_base<atomic_state_machine> cb;
                test_channel_base<atomic_state_machine>::result_t r;

                cb.machine().set_flags(flags::readable);
                cb.machine().set_state(states::none, states::open | states::readable);
                
                std::atomic<bool> c(false);
                int k = 0;

                std::thread t([&] () {
                    cb.do_as(ops::read, [&] () {
                        while (k < 1000) k++;
                    });
                });
                do
                {
                    r = cb.do_as(ops::close, [&] () {
                        int kk = k;
                        if ((kk != 0) && (kk != 1000))
                        {
                            c.store(true, std::memory_order_relaxed);
                        }
                    });
                }
                while (!std::get<0>(r));

                t.join();

                if (c)
                {
                    Assert::Fail(L"inconsistency detected", LINE_INFO());
                }
            }
		}

		TEST_METHOD(read_to_close_consistency_with_blocking_state_machine)
        {
            for (std::size_t i = 0; i < 1000; i++)
            {
                test_channel_base<blocking_state_machine> cb;
                test_channel_base<blocking_state_machine>::result_t r;

                cb.machine().set_flags(flags::readable);
                cb.machine().set_state(states::none, states::open | states::readable);
                
                std::atomic<bool> c(false);
                int k = 0;

                std::thread t([&] () {
                    cb.do_as(ops::read, [&] () {
                        while (k < 1000) k++;
                    });
                });
                do
                {
                    r = cb.do_as(ops::close, [&] () {
                        int kk = k;
                        if ((kk != 0) && (kk != 1000))
                        {
                            c.store(true, std::memory_order_relaxed);
                        }
                    });
                }
                while (!std::get<0>(r));

                t.join();

                if (c)
                {
                    Assert::Fail(L"inconsistency detected", LINE_INFO());
                }
            }
		}

		TEST_METHOD(write_to_close_consistency_with_atomic_state_machine)
        {
            for (std::size_t i = 0; i < 1000; i++)
            {
                test_channel_base<atomic_state_machine> cb;
                test_channel_base<atomic_state_machine>::result_t r;

                cb.machine().set_flags(flags::writable);
                cb.machine().set_state(states::none, states::open | states::writable);
                
                std::atomic<bool> c(false);
                int k = 0;

                std::thread t([&] () {
                    cb.do_as(ops::write, [&] () {
                        while (k < 1000) k++;
                    });
                });
                do
                {
                    r = cb.do_as(ops::close, [&] () {
                        int kk = k;
                        if ((kk != 0) && (kk != 1000))
                        {
                            c.store(true, std::memory_order_relaxed);
                        }
                    });
                }
                while (!std::get<0>(r));

                t.join();

                if (c)
                {
                    Assert::Fail(L"inconsistency detected", LINE_INFO());
                }
            }
		}
        
		TEST_METHOD(write_to_close_consistency_with_blocking_state_machine)
        {
            for (std::size_t i = 0; i < 1000; i++)
            {
                test_channel_base<blocking_state_machine> cb;
                test_channel_base<blocking_state_machine>::result_t r;

                cb.machine().set_flags(flags::writable);
                cb.machine().set_state(states::none, states::open | states::writable);
                
                std::atomic<bool> c(false);
                int k = 0;

                std::thread t([&] () {
                    cb.do_as(ops::write, [&] () {
                        while (k < 1000) k++;
                    });
                });
                do
                {
                    r = cb.do_as(ops::close, [&] () {
                        int kk = k;
                        if ((kk != 0) && (kk != 1000))
                        {
                            c.store(true, std::memory_order_relaxed);
                        }
                    });
                }
                while (!std::get<0>(r));

                t.join();

                if (c)
                {
                    Assert::Fail(L"inconsistency detected", LINE_INFO());
                }
            }
		}
        
        // inconsistency proved; enable the test to make sure
        BEGIN_TEST_METHOD_ATTRIBUTE(read_to_write_inconsistency_with_atomic_state_machine)
            TEST_IGNORE()
        END_TEST_METHOD_ATTRIBUTE()
		TEST_METHOD(read_to_write_inconsistency_with_atomic_state_machine)
        {
            for (std::size_t i = 0; i < 1000; i++)
            {
                test_channel_base<atomic_state_machine> cb;
                test_channel_base<atomic_state_machine>::result_t r;

                cb.machine().set_flags(flags::writable | flags::readable);
                cb.machine().set_state(states::none, states::open | states::writable | states::readable);
                
                std::atomic<bool> c(false);
                int k = 0;

                std::thread t([&] () {
                    cb.do_as(ops::read, [&] () {
                        while (k < 1000) k++;
                    });
                });
                do
                {
                    r = cb.do_as(ops::write, [&] () {
                        int kk = k;
                        if ((kk != 0) && (kk != 1000))
                        {
                            c.store(true, std::memory_order_relaxed);
                        }
                    });
                }
                while (!std::get<0>(r));

                t.join();

                if (c)
                {
                    Assert::Fail(L"inconsistency detected", LINE_INFO());
                }
            }
		}

        // inconsistency proved; enable the test to make sure
        BEGIN_TEST_METHOD_ATTRIBUTE(read_to_write_inconsistency_with_blocking_state_machine)
            TEST_IGNORE()
        END_TEST_METHOD_ATTRIBUTE()
		TEST_METHOD(read_to_write_inconsistency_with_blocking_state_machine)
        {
            for (std::size_t i = 0; i < 1000; i++)
            {
                test_channel_base<blocking_state_machine> cb;
                test_channel_base<blocking_state_machine>::result_t r;

                cb.machine().set_flags(flags::writable | flags::readable);
                cb.machine().set_state(states::none, states::open | states::writable | states::readable);
                
                std::atomic<bool> c(false);
                int k = 0;

                std::thread t([&] () {
                    cb.do_as(ops::read, [&] () {
                        while (k < 1000) k++;
                    });
                });
                do
                {
                    r = cb.do_as(ops::write, [&] () {
                        int kk = k;
                        if ((kk != 0) && (kk != 1000))
                        {
                            c.store(true, std::memory_order_relaxed);
                        }
                    });
                }
                while (!std::get<0>(r));

                t.join();

                if (c)
                {
                    Assert::Fail(L"inconsistency detected", LINE_INFO());
                }
            }
		}
	};
}