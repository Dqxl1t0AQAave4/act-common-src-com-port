#include "stdafx.h"
#include "CppUnitTest.h"
#include "util.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

#include <com-port-api/include/byte_buffer.h>
#include <com-port-api/include/channel.h>
#include <iostream>

using namespace com_port_api;
using namespace com_port_api::channel;

namespace com_port_api_test
{

	TEST_CLASS(basic_state_diagram_test)
	{

	public:


        enum : constant_t
        {
            unknown_bit = (1 << 8)
        };


        /**
         *	<0 initial state, 1 operation, 2 flags,
         *	 3 expected result, 4 [expected state = initial_state]>
         */
        template <class ... T>
        void lock(std::wstring message,
                  std::tuple<T ...> _test_data)
        {
            basic_state_diagram diagram;
            basic_state_diagram::result_t result;
            auto test_data = std::tuple_cat(_test_data, std::make_tuple(0));

            message += L"\n";
            Logger::WriteMessage(message.c_str());

            // test that unknown bits are preserved
            state_t initial = std::get<0>(test_data);
            initial += unknown_bit;

            result = diagram.lock_op(std::get<1>(test_data),
                                     initial,
                                     std::get<2>(test_data));

            basic_state_diagram::result_t expected1;
            std::get<0>(expected1) = std::get<3>(test_data);

            if (!std::get<3>(test_data))
            {
                std::get<1>(expected1) = initial;
                std::get<2>(expected1) = std::get<2>(result);
            }
            else
            {
                std::get<1>(expected1) = std::get<4>(test_data) | unknown_bit;
                std::get<2>(expected1) = guarantee_t::acquire;
            }

            Assert::AreEqual(expected1, result, L"", LINE_INFO());
        }

        /**
         *	<0 initial state, 1 operation, 2 flags, 3 op result
         *	 4 expected result, 5 [expected state = initial_state]>
         */
        template <class ... T>
        void unlock(std::wstring message,
                    std::tuple<T ...> _test_data)
        {
            basic_state_diagram diagram;
            basic_state_diagram::result_t result;
            auto test_data = std::tuple_cat(_test_data, std::make_tuple(0));

            message += L"\n";
            Logger::WriteMessage(message.c_str());

            // test that unknown bits are preserved
            state_t initial = std::get<0>(test_data);
            initial += unknown_bit;

            result = diagram.unlock_op(std::get<1>(test_data),
                                       initial,
                                       states::none, // locked_with is unused
                                       std::get<2>(test_data),
                                       std::get<3>(test_data));

            basic_state_diagram::result_t expected1;
            std::get<0>(expected1) = std::get<4>(test_data);

            if (!std::get<4>(test_data))
            {
                std::get<1>(expected1) = initial;
                std::get<2>(expected1) = std::get<2>(result);
            }
            else
            {
                std::get<1>(expected1) = std::get<5>(test_data) | unknown_bit;
                std::get<2>(expected1) = guarantee_t::release;
            }

            Assert::AreEqual(expected1, result, L"", LINE_INFO());
        }

        /*
         
                   | main state | modifiers | operation | flags | conditions      | result         |
                   |:----------:|:---------:|:---------:|:-----:|:---------------:|:--------------:|
                   |none        |X          |open       |X      |X                |opening         |
                   |none        |X          |X          |X      |X                |[not permitted] |
                   |opening     |X          |X          |X      |X                |[not permitted] |
                   |open        |r [w]      |read       |X      |X                |open [w]        |
                   |open        |[r] w      |write      |X      |X                |open [r]        |
                   |open        |[r] [w]    |close      |[R] [W]|(r|!R)&(w|!W)    |closing [r] [w] |
                   |open        |X          |X          |X      |X                |[not permitted] |
                   |closing     |X          |X          |X      |X                |[not permitted] |
                   |closed      |X          |X          |X      |X                |[not permitted] |
                   |X           |X          |X          |X      |X                |[not permitted] |

         */

		TEST_METHOD(locking_from_none_state)
        {
            /*     | main state | modifiers | operation | flags | conditions      | result         |*/
            lock(L"|none        |X          |open       |X=no   |X                |opening         |", std::make_tuple(
                states::none, ops::open, 0, true, states::opening
            ));
            lock(L"|none        |X          |open       |X=rw   |X                |opening         |", std::make_tuple(
                states::none, ops::open, flags::readable | flags::writable, true, states::opening
            ));

            lock(L"|none        |X          |X=read     |X      |X                |[not permitted] |", std::make_tuple(
                states::none, ops::read, 0, false
            ));
            lock(L"|none        |X          |X=write    |X      |X                |[not permitted] |", std::make_tuple(
                states::none, ops::write, 0, false
            ));
            lock(L"|none        |X          |X=close    |X      |X                |[not permitted] |", std::make_tuple(
                states::none, ops::write, 0, false
            ));
		}

		TEST_METHOD(locking_from_opening_state)
        {
            /*     | main state | modifiers | operation | flags | conditions      | result         |*/
            lock(L"|opening     |X          |X=open     |X      |X                |[not permitted] |", std::make_tuple(
                states::opening, ops::open, 0, false
            ));
            lock(L"|opening     |X          |X=read     |X      |X                |[not permitted] |", std::make_tuple(
                states::opening, ops::read, 0, false
            ));
            lock(L"|opening     |X          |X=write    |X      |X                |[not permitted] |", std::make_tuple(
                states::opening, ops::write, 0, false
            ));
            lock(L"|opening     |X          |X=close    |X      |X                |[not permitted] |", std::make_tuple(
                states::opening, ops::close, 0, false
            ));
		}

		TEST_METHOD(locking_from_open_state)
        {
            /*     | main state | modifiers | operation | flags | conditions      | result         |*/
            lock(L"|open        |r [w]=0    |read       |X      |X                |open [w]        |", std::make_tuple(
                states::open|states::readable, ops::read, 0, true, states::open
            ));
            lock(L"|open        |r [w]=1    |read       |X      |X                |open [w]        |", std::make_tuple(
                states::open|states::readable|states::writable, ops::read, 0, true, states::open|states::writable
            ));
            lock(L"|open        |[r]=0 w    |write      |X      |X                |open [r]        |", std::make_tuple(
                states::open|states::writable, ops::write, 0, true, states::open
            ));
            lock(L"|open        |[r]=1 w    |write      |X      |X                |open [r]        |", std::make_tuple(
                states::open|states::readable|states::writable, ops::write, 0, true, states::open|states::readable
            ));
            lock(L"|open        |[r]=1 [w]=1|close      |[R]=1 [W]=1|(r|!R)&(w|!W)|closing [r] [w] |", std::make_tuple(
                states::open|states::readable|states::writable, ops::close, flags::readable | flags::writable, true, states::closing|states::readable|states::writable
            ));
            lock(L"|open        |[r]=0 [w]=1|close      |[R]=0 [W]=1|(r|!R)&(w|!W)|closing [r] [w] |", std::make_tuple(
                states::open|states::writable, ops::close, flags::writable, true, states::closing|states::writable
            ));
            lock(L"|open        |[r]=1 [w]=0|close      |[R]=1 [W]=0|(r|!R)&(w|!W)|closing [r] [w] |", std::make_tuple(
                states::open|states::readable, ops::close, flags::readable, true, states::closing|states::readable
            ));
            lock(L"|open        |[r]=0 [w]=0|close      |[R]=0 [W]=0|(r|!R)&(w|!W)|closing [r] [w] |", std::make_tuple(
                states::open, ops::close, 0, true, states::closing
            ));
            lock(L"|open        |[r]=0 [w]=1|X=close    |[R]=1 [W]=1|<broken>     |[not permitted] |", std::make_tuple(
                states::open|states::writable, ops::close, flags::readable | flags::writable, false
            ));
            lock(L"|open        |[r]=1 [w]=0|X=close    |[R]=1 [W]=1|<broken>     |[not permitted] |", std::make_tuple(
                states::open|states::readable, ops::close, flags::readable | flags::writable, false
            ));
            lock(L"|open        |[r]=0 [w]=0|X=close    |[R]=1 [W]=1|<broken>     |[not permitted] |", std::make_tuple(
                states::open, ops::close, flags::readable | flags::writable, false
            ));
            lock(L"|open        |[r]=0 [w]=0|X=close    |[R]=0 [W]=1|<broken>     |[not permitted] |", std::make_tuple(
                states::open, ops::close, flags::writable, false
            ));
            lock(L"|open        |[r]=0 [w]=0|X=close      |[R]=1 [W]=0|<broken>     |[not permitted] |", std::make_tuple(
                states::open, ops::close, flags::readable, false
            ));
            lock(L"|open        |X          |X=open     |X      |X                |[not permitted] |", std::make_tuple(
                states::open, ops::read, 0, false
            ));
		}

		TEST_METHOD(locking_from_closing_state)
        {
            /*     | main state | modifiers | operation | flags | conditions      | result         |*/
            lock(L"|closing     |X          |X=open     |X      |X                |[not permitted] |", std::make_tuple(
                states::closing, ops::open, 0, false
            ));
            lock(L"|closing     |X          |X=read     |X      |X                |[not permitted] |", std::make_tuple(
                states::closing, ops::read, 0, false
            ));
            lock(L"|closing     |X          |X=write    |X      |X                |[not permitted] |", std::make_tuple(
                states::closing, ops::write, 0, false
            ));
            lock(L"|closing     |X          |X=close    |X      |X                |[not permitted] |", std::make_tuple(
                states::closing, ops::close, 0, false
            ));
		}

		TEST_METHOD(locking_from_closed_state)
        {
            /*     | main state | modifiers | operation | flags | conditions      | result         |*/
            lock(L"|closed      |X          |X=open     |X      |X                |[not permitted] |", std::make_tuple(
                states::closed , ops::open, 0, false
            ));
            lock(L"|closed      |X          |X=read     |X      |X                |[not permitted] |", std::make_tuple(
                states::closed , ops::read, 0, false
            ));
            lock(L"|closed      |X          |X=write    |X      |X                |[not permitted] |", std::make_tuple(
                states::closed , ops::write, 0, false
            ));
            lock(L"|closed      |X          |X=close    |X      |X                |[not permitted] |", std::make_tuple(
                states::closed , ops::close, 0, false
            ));
		}

        /*
         
                     | main state | modifiers | operation | flags | conditions      | result         |
                     |:----------:|:---------:|:---------:|:-----:|:---------------:|:--------------:|
                     |none        |X          |X          |X      |X                |[not permitted] |
                     |opening     |X          |open       |[R] [W]|success          |open r=R w=W    |
                     |opening     |X          |open       |X      |failure          |none            |
                     |opening     |X          |X          |X      |X                |[not permitted] |
                     |open        |[w]        |read       |X      |X                |open r [w]      |
                     |open        |[r]        |write      |X      |X                |open [r] w      |
                     |open        |X          |X          |X      |X                |[not permitted] |
                     |closing     |[r] [w]    |close      |X      |X                |closed [r] [w]  |
                     |closing     |X          |X          |X      |X                |[not permitted] |
                     |closed      |X          |X          |X      |X                |[not permitted] |
                     |X           |X          |X          |X      |X                |[not permitted] |

         */

		TEST_METHOD(unlocking_from_none_state)
        {
            /*       | main state | modifiers | operation | flags | conditions      | result         |*/
            unlock(L"|none        |X          |X          |X      |X                |[not permitted] |", std::make_tuple(
                states::none, ops::open, 0, ops::result_type::result_success, false
            ));
            unlock(L"|none        |X          |X          |X      |X                |[not permitted] |", std::make_tuple(
                states::none, ops::read, 0, ops::result_type::result_success, false
            ));
            unlock(L"|none        |X          |X          |X      |X                |[not permitted] |", std::make_tuple(
                states::none, ops::write, 0, ops::result_type::result_success, false
            ));
            unlock(L"|none        |X          |X          |X      |X                |[not permitted] |", std::make_tuple(
                states::none, ops::close, 0, ops::result_type::result_success, false
            ));
		}

		TEST_METHOD(unlocking_from_opening_state)
        {
            /*       | main state | modifiers | operation | flags | conditions      | result         |*/
            unlock(L"|opening     |X          |open       |[R]=1 [W]=1|success          |open r=R w=W    |", std::make_tuple(
                states::opening, ops::open, flags::readable|flags::writable, ops::result_type::result_success, true, states::open|states::readable|states::writable
            ));
            unlock(L"|opening     |X          |open       |[R]=1 [W]=0|success          |open r=R w=W    |", std::make_tuple(
                states::opening, ops::open, flags::readable, ops::result_type::result_success, true, states::open|states::readable
            ));
            unlock(L"|opening     |X          |open       |[R]=0 [W]=1|success          |open r=R w=W    |", std::make_tuple(
                states::opening, ops::open, flags::writable, ops::result_type::result_success, true, states::open|states::writable
            ));
            unlock(L"|opening     |X          |open       |[R]=0 [W]=0|success          |open r=R w=W    |", std::make_tuple(
                states::opening, ops::open, 0, ops::result_type::result_success, true, states::open
            ));
            unlock(L"|opening     |X          |open       |X      |failure          |none            |", std::make_tuple(
                states::opening, ops::open, 0, ops::result_type::result_failure, true, states::none
            ));
            unlock(L"|opening     |X          |open       |R=1 W=1|failure          |none            |", std::make_tuple(
                states::opening, ops::open, flags::readable|flags::writable, ops::result_type::result_failure, true, states::none
            ));
            unlock(L"|opening     |X          |X=read     |X      |X                |[not permitted] |", std::make_tuple(
                states::opening, ops::read, 0, ops::result_type::result_success, false
            ));
            unlock(L"|opening     |X          |X=write    |X      |X                |[not permitted] |", std::make_tuple(
                states::opening, ops::write, 0, ops::result_type::result_success, false
            ));
            unlock(L"|opening     |X          |X=close    |X      |X                |[not permitted] |", std::make_tuple(
                states::opening, ops::close, 0, ops::result_type::result_success, false
            ));
		}

		TEST_METHOD(unlocking_from_open_state)
        {
            /*       | main state | modifiers | operation | flags | conditions      | result         |*/
            unlock(L"|open        |[w]=1      |read       |X      |X=success        |open r [w]      |", std::make_tuple(
                states::open|states::writable, ops::read, 0, ops::result_type::result_success, true, states::open|states::readable|states::writable
            ));
            unlock(L"|open        |[w]=1      |read       |X      |X=failure        |open r [w]      |", std::make_tuple(
                states::open|states::writable, ops::read, 0, ops::result_type::result_failure, true, states::open|states::readable|states::writable
            ));
            unlock(L"|open        |[w]=0      |read       |X      |X=success        |open r [w]      |", std::make_tuple(
                states::open, ops::read, 0, ops::result_type::result_success, true, states::open|states::readable
            ));
            unlock(L"|open        |[w]=0      |read       |X      |X=failure        |open r [w]      |", std::make_tuple(
                states::open, ops::read, 0, ops::result_type::result_failure, true, states::open|states::readable
            ));
            unlock(L"|open        |[r]=1      |write      |X      |X=success        |open [r] w      |", std::make_tuple(
                states::open|states::readable, ops::write, 0, ops::result_type::result_success, true, states::open|states::readable|states::writable
            ));
            unlock(L"|open        |[r]=1      |write      |X      |X=failure        |open [r] w      |", std::make_tuple(
                states::open|states::readable, ops::write, 0, ops::result_type::result_failure, true, states::open|states::readable|states::writable
            ));
            unlock(L"|open        |[r]=0      |write      |X      |X=success        |open [r] w      |", std::make_tuple(
                states::open, ops::write, 0, ops::result_type::result_success, true, states::open|states::writable
            ));
            unlock(L"|open        |[r]=0      |write      |X      |X=failure        |open [r] w      |", std::make_tuple(
                states::open, ops::write, 0, ops::result_type::result_failure, true, states::open|states::writable
            ));
            unlock(L"|open        |X:r=1      |read       |X      |X=success        |[not permitted] |", std::make_tuple(
                states::open|states::readable, ops::read, 0, ops::result_type::result_success, false
            ));
            unlock(L"|open        |X:r=1      |read       |X      |X=success        |[not permitted] |", std::make_tuple(
                states::open|states::readable, ops::read, 0, ops::result_type::result_failure, false
            ));
            unlock(L"|open        |X:w=1      |write      |X      |X=success        |[not permitted] |", std::make_tuple(
                states::open|states::writable, ops::write, 0, ops::result_type::result_success, false
            ));
            unlock(L"|open        |X:w=1      |write      |X      |X=success        |[not permitted] |", std::make_tuple(
                states::open|states::writable, ops::write, 0, ops::result_type::result_failure, false
            ));
            unlock(L"|open        |X          |open       |X      |X                |[not permitted] |", std::make_tuple(
                states::open, ops::open, 0, ops::result_type::result_success, false
            ));
            unlock(L"|open        |X          |close      |X      |X                |[not permitted] |", std::make_tuple(
                states::open, ops::close, 0, ops::result_type::result_success, false
            ));
		}

		TEST_METHOD(unlocking_from_closing_state)
        {
            /*       | main state | modifiers | operation | flags | conditions      | result         |*/
            unlock(L"|closing     |[r]=1 [w]=1|close      |X      |X=success        |closed [r] [w]  |", std::make_tuple(
                states::closing|states::readable|states::writable, ops::close, 0, ops::result_type::result_success, true, states::closed|states::readable|states::writable
            ));
            unlock(L"|closing     |[r]=1 [w]=1|close      |X      |X=failure        |closed [r] [w]  |", std::make_tuple(
                states::closing|states::readable|states::writable, ops::close, 0, ops::result_type::result_failure, true, states::closed|states::readable|states::writable
            ));
            unlock(L"|closing     |[r]=0 [w]=1|close      |X      |X=success        |closed [r] [w]  |", std::make_tuple(
                states::closing|states::writable, ops::close, 0, ops::result_type::result_success, true, states::closed|states::writable
            ));
            unlock(L"|closing     |[r]=0 [w]=1|close      |X      |X=failure        |closed [r] [w]  |", std::make_tuple(
                states::closing|states::writable, ops::close, 0, ops::result_type::result_failure, true, states::closed|states::writable
            ));
            unlock(L"|closing     |[r]=1 [w]=0|close      |X      |X=success        |closed [r] [w]  |", std::make_tuple(
                states::closing|states::readable, ops::close, 0, ops::result_type::result_success, true, states::closed|states::readable
            ));
            unlock(L"|closing     |[r]=1 [w]=0|close      |X      |X=failure        |closed [r] [w]  |", std::make_tuple(
                states::closing|states::readable, ops::close, 0, ops::result_type::result_failure, true, states::closed|states::readable
            ));
            unlock(L"|closing     |[r]=0 [w]=0|close      |X      |X=success        |closed [r] [w]  |", std::make_tuple(
                states::closing, ops::close, 0, ops::result_type::result_success, true, states::closed
            ));
            unlock(L"|closing     |[r]=0 [w]=0|close      |X      |X=failure        |closed [r] [w]  |", std::make_tuple(
                states::closing, ops::close, 0, ops::result_type::result_failure, true, states::closed
            ));
            unlock(L"|closing     |X          |X=open     |X      |X                |[not permitted] |", std::make_tuple(
                states::closing, ops::open, 0, ops::result_type::result_success, false
            ));
            unlock(L"|closing     |X          |X=read     |X      |X                |[not permitted] |", std::make_tuple(
                states::closing, ops::read, 0, ops::result_type::result_success, false
            ));
            unlock(L"|closing     |X          |X=write    |X      |X                |[not permitted] |", std::make_tuple(
                states::closing, ops::write, 0, ops::result_type::result_success, false
            ));
		}

		TEST_METHOD(unlocking_from_closed_state)
        {
            /*       | main state | modifiers | operation | flags | conditions      | result         |*/
            unlock(L"|closed      |X          |X          |X      |X                |[not permitted] |", std::make_tuple(
                states::closed, ops::open, 0, ops::result_type::result_success, false
            ));
            unlock(L"|closed      |X          |X          |X      |X                |[not permitted] |", std::make_tuple(
                states::closed, ops::read, 0, ops::result_type::result_success, false
            ));
            unlock(L"|closed      |X          |X          |X      |X                |[not permitted] |", std::make_tuple(
                states::closed, ops::write, 0, ops::result_type::result_success, false
            ));
            unlock(L"|closed      |X          |X          |X      |X                |[not permitted] |", std::make_tuple(
                states::closed, ops::close, 0, ops::result_type::result_success, false
            ));
		}

	};

}