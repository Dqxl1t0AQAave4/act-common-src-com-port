#include "stdafx.h"
#include "CppUnitTest.h"
#include "util.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

#include <com-port-api/include/blocking_list_buffer.h>
#include <iostream>
#include <future>
#include <thread>

using namespace com_port_api;

namespace com_port_api_test
{

	TEST_CLASS(blocking_list_buffer_test)
	{

	public:

        TEST_METHOD(try_push_and_try_pop_return_correct_result)
        {
            blocking_list_buffer<int> b(10);
            bool r;

            std::list<int> l1 = { 1, 2, 3, 4 };
            std::list<int> l2 = { 6, 7, 8, 9 };
            
            Logger::WriteMessage(L"Try push l1\r\n");

            r = b.try_push(l1);

            Assert::IsTrue(r, L"", LINE_INFO());
            Assert::AreEqual(0u, l1.size(), L"", LINE_INFO());
            
            Logger::WriteMessage(L"Try push l2\r\n");

            r = b.try_push(l2);

            Assert::IsTrue(r, L"", LINE_INFO());
            Assert::AreEqual(0u, l2.size(), L"", LINE_INFO());
            
            Logger::WriteMessage(L"try pop\r\n");

            r = b.try_pop(l1);

            Assert::IsTrue(r, L"", LINE_INFO());
            Assert::AreEqual(8u, l1.size(), L"", LINE_INFO());
            Assert::AreEqual(std::list<int>({ 1, 2, 3, 4, 6, 7, 8, 9 }), l1, L"", LINE_INFO());
		}

        TEST_METHOD(push_and_pop_return_correct_result)
        {
            blocking_list_buffer<int> b(10);
            bool r;

            std::list<int> l1 = { 1, 2, 3, 4 };
            std::list<int> l2 = { 6, 7, 8, 9 };
            
            Logger::WriteMessage(L"Push l1\r\n");

            r = b.push(l1);

            Assert::IsTrue(r, L"", LINE_INFO());
            Assert::AreEqual(0u, l1.size(), L"", LINE_INFO());
            
            Logger::WriteMessage(L"Push l2\r\n");

            r = b.push(l2);

            Assert::IsTrue(r, L"", LINE_INFO());
            Assert::AreEqual(0u, l2.size(), L"", LINE_INFO());
            
            Logger::WriteMessage(L"pop\r\n");

            r = b.pop(l1);

            Assert::IsTrue(r, L"", LINE_INFO());
            Assert::AreEqual(8u, l1.size(), L"", LINE_INFO());
            Assert::AreEqual(std::list<int>({ 1, 2, 3, 4, 6, 7, 8, 9 }), l1, L"", LINE_INFO());
		}

        TEST_METHOD(buffer_is_weak_sized)
        {
            blocking_list_buffer<int> b(1);
            bool r;

            std::list<int> l1 = { 1, 2, 3, 4 };
            
            Logger::WriteMessage(L"Push l1\r\n");

            r = b.try_push(l1);

            Assert::IsTrue(r, L"", LINE_INFO());
            Assert::AreEqual(0u, l1.size(), L"", LINE_INFO());
            
            Logger::WriteMessage(L"pop\r\n");

            r = b.try_pop(l1);

            Assert::IsTrue(r, L"", LINE_INFO());
            Assert::AreEqual(4u, l1.size(), L"", LINE_INFO());
            Assert::AreEqual(std::list<int>({ 1, 2, 3, 4 }), l1, L"", LINE_INFO());
		}

        TEST_METHOD(try_push_does_nothing_if_buffer_closed)
        {
            blocking_list_buffer<int> b(1);
            bool r;

            std::list<int> l1 = { 1, 2, 3, 4 };

            b.close();
            
            Logger::WriteMessage(L"Try push l1\r\n");

            r = b.try_push(l1);

            Assert::IsFalse(r, L"", LINE_INFO());
            Assert::AreEqual(std::list<int>({ 1, 2, 3, 4 }), l1, L"", LINE_INFO());
		}

        TEST_METHOD(try_pop_does_nothing_if_buffer_closed)
        {
            blocking_list_buffer<int> b(1);
            bool r;

            std::list<int> l1 = { 1, 2, 3, 4 };
            
            Logger::WriteMessage(L"Try push l1\r\n");

            r = b.try_push(l1);

            Assert::IsTrue(r, L"", LINE_INFO());
            Assert::AreEqual(0u, l1.size(), L"", LINE_INFO());

            b.close();
            
            Logger::WriteMessage(L"try pop\r\n");

            r = b.try_pop(l1);

            Assert::IsFalse(r, L"", LINE_INFO());
            Assert::AreEqual(0u, l1.size(), L"", LINE_INFO());
		}

        TEST_METHOD(push_does_nothing_if_buffer_closed)
        {
            blocking_list_buffer<int> b(1);
            bool r;

            std::list<int> l1 = { 1, 2, 3, 4 };

            b.close();
            
            Logger::WriteMessage(L"Push l1\r\n");

            r = b.push(l1);

            Assert::IsFalse(r, L"", LINE_INFO());
            Assert::AreEqual(std::list<int>({ 1, 2, 3, 4 }), l1, L"", LINE_INFO());
		}

        TEST_METHOD(pop_does_nothing_if_buffer_closed)
        {
            blocking_list_buffer<int> b(1);
            bool r;

            std::list<int> l1 = { 1, 2, 3, 4 };
            
            Logger::WriteMessage(L"Push l1\r\n");

            r = b.push(l1);

            Assert::IsTrue(r, L"", LINE_INFO());
            Assert::AreEqual(0u, l1.size(), L"", LINE_INFO());

            b.close();
            
            Logger::WriteMessage(L"pop\r\n");

            r = b.pop(l1);

            Assert::IsFalse(r, L"", LINE_INFO());
            Assert::AreEqual(0u, l1.size(), L"", LINE_INFO());
		}

        TEST_METHOD(push_returns_immediately_if_closed)
        {
            blocking_list_buffer<int> b(1);
            bool r;

            std::list<int> l1 = { 1, 2, 3, 4 };
            std::list<int> l2 = { 6, 7, 8, 9 };
            
            Logger::WriteMessage(L"Push l1\r\n");

            r = b.push(l1);

            Assert::IsTrue(r, L"", LINE_INFO());
            Assert::AreEqual(0u, l1.size(), L"", LINE_INFO());

            std::thread notifier([&] () {
                std::this_thread::sleep_for(std::chrono::seconds(1));
            
                Logger::WriteMessage(L"Close\r\n");

                b.close();
            });
            
            Logger::WriteMessage(L"Push l2\r\n");

            r = b.push(l2);

            Assert::IsFalse(r, L"", LINE_INFO());
            Assert::AreEqual(std::list<int>({ 6, 7, 8, 9 }), l2, L"", LINE_INFO());

            notifier.join();
		}

        TEST_METHOD(pop_returns_immediately_if_closed)
        {
            blocking_list_buffer<int> b(1);
            bool r;

            std::list<int> l1 = { 1, 2, 3, 4 };
            std::list<int> l2 = { };

            std::thread notifier([&] () {
                std::this_thread::sleep_for(std::chrono::seconds(1));
            
                Logger::WriteMessage(L"Close\r\n");

                b.close();
            });
            
            Logger::WriteMessage(L"pop\r\n");

            r = b.pop(l2);

            Assert::IsFalse(r, L"", LINE_INFO());
            Assert::AreEqual(0u, l2.size(), L"", LINE_INFO());

            notifier.join();
		}

        TEST_METHOD(push_actually_waits)
        {
            blocking_list_buffer<int> b(1);
            bool r;

            std::list<int> l1 = { 1, 2, 3, 4 };
            std::list<int> l2 = { 6, 7, 8, 9 };
            
            Logger::WriteMessage(L"Push l1\r\n");

            r = b.push(l1);

            Assert::IsTrue(r, L"", LINE_INFO());
            Assert::AreEqual(0u, l1.size(), L"", LINE_INFO());

            std::thread notifier([&] () {
                std::this_thread::sleep_for(std::chrono::seconds(1));
            
                Logger::WriteMessage(L"Close\r\n");

                b.pop(l1);
            });
            
            Logger::WriteMessage(L"Push l2\r\n");

            r = b.push(l2);

            Assert::AreEqual(std::list<int>({ 1, 2, 3, 4 }), l1, L"", LINE_INFO());

            Assert::IsTrue(r, L"", LINE_INFO());
            Assert::AreEqual(0u, l2.size(), L"", LINE_INFO());

            notifier.join();
		}

        TEST_METHOD(pop_actually_waits)
        {
            blocking_list_buffer<int> b(1);
            bool r;

            std::list<int> l1 = { 1, 2, 3, 4 };
            std::list<int> l2 = { };

            std::thread notifier([&] () {
                std::this_thread::sleep_for(std::chrono::seconds(1));
            
                Logger::WriteMessage(L"Close\r\n");

                b.push(l1);
            });
            
            Logger::WriteMessage(L"pop\r\n");

            r = b.pop(l2);

            Assert::AreEqual(0u, l1.size(), L"", LINE_INFO());

            Assert::IsTrue(r, L"", LINE_INFO());
            Assert::AreEqual(std::list<int>({ 1, 2, 3, 4 }), l2, L"", LINE_INFO());

            notifier.join();
		}

        TEST_METHOD(push_waits_no_more_than_timeout)
        {
            blocking_list_buffer<int> b(1);
            bool r;

            std::list<int> l1 = { 1, 2, 3, 4 };
            std::list<int> l2 = { 6, 7, 8, 9 };
            
            Logger::WriteMessage(L"Push l1\r\n");

            r = b.push(l1);

            Assert::IsTrue(r, L"", LINE_INFO());
            Assert::AreEqual(0u, l1.size(), L"", LINE_INFO());

            std::promise<void> p;

            std::thread failer([&] () {
                if (std::future_status::ready !=
                    p.get_future().wait_for(std::chrono::seconds(2)))
                {
                    Logger::WriteMessage(L"waiting too long\r\n");
                    b.close();
                }
            });
            
            Logger::WriteMessage(L"Push l2\r\n");

            r = b.push(std::chrono::seconds(1), l2);

            p.set_value();

            Assert::AreEqual(0u, l1.size(), L"", LINE_INFO());
            
            Assert::IsTrue(r, L"", LINE_INFO());
            Assert::AreEqual(std::list<int>({ 6, 7, 8, 9 }), l2, L"", LINE_INFO());

            failer.join();
		}

        TEST_METHOD(pop_waits_no_more_than_timeout)
        {
            blocking_list_buffer<int> b(1);
            bool r;

            std::list<int> l1 = { };

            std::promise<void> p;

            std::thread failer([&] () {
                if (std::future_status::ready !=
                    p.get_future().wait_for(std::chrono::seconds(2)))
                {
                    Logger::WriteMessage(L"waiting too long\r\n");
                    b.close();
                }
            });
            
            Logger::WriteMessage(L"pop\r\n");

            r = b.pop(std::chrono::seconds(1), l1);

            p.set_value();

            Assert::IsTrue(r, L"", LINE_INFO());
            Assert::AreEqual(0u, l1.size(), L"", LINE_INFO());

            failer.join();
		}
	};
}