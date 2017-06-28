#include "CppUnitTest.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

#include <com-port-api/include/channel.h>
#include <iostream>

using namespace com_port_api;
using namespace com_port_api::channel;


template<> static std::wstring Microsoft::VisualStudio::CppUnitTestFramework::ToString<bit_field<constant_t>> (const bit_field<constant_t>& t) { RETURN_WIDE_STRING(t.value); }
template<> static std::wstring Microsoft::VisualStudio::CppUnitTestFramework::ToString<guarantee_t> (const guarantee_t& t) { RETURN_WIDE_STRING(static_cast<constant_t>(t)); }
template<> static std::wstring Microsoft::VisualStudio::CppUnitTestFramework::ToString<state_diagram::result_t> (const state_diagram::result_t& t) { RETURN_WIDE_STRING("tuple{" << std::get<0>(t) << ", " << std::get<1>(t).value << ", " << static_cast<constant_t>(std::get<2>(t)) << "}"); }
template<> static std::wstring Microsoft::VisualStudio::CppUnitTestFramework::ToString<ops::result_type> (const ops::result_type& t) { RETURN_WIDE_STRING(static_cast<constant_t>(t)); }