#pragma once

#include <exception>
#include <string>

#include <byte_buffer.h>

namespace com_port_api
{


/**
 *	Encapsulates logic of packet packing and unpacking.
 *	
 *	`ipacket_t` - input packet type
 *	`opacket_t` - output packet type
 */
template<class I, class O>
class dialect
{

public:

    using ipacket_t = I;
    using opacket_t = O;


    /**
     *	Reads one packet from `src` to the specified `dst`.
     *	
     *	May skip necessary amount of leading bytes.
     *	
     *	Returns `true` on success, `false` otherwise.
     */
    // bool read(ipacket_t &dst, byte_buffer &src);


    /**
     *	Writes one packet specified by `src` to `dst` buffer.
     *	
     *	Returns `true` on success, `false` otherwise.
     */
    // bool write(byte_buffer &dst, const opacket_t &src);
};

}