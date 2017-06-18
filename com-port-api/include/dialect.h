#pragma once

#include <exception>
#include <string>

#include <byte_buffer.h>

namespace com_port_api
{

class dialect_exception
    : public std::runtime_error
{

public:

    dialect_exception(std::string message)
        : std::runtime_error(message)
    {
    }
};


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


    const size_t max_allowed_ipacket_size;
    const size_t max_allowed_opacket_size;


    dialect(size_t max_allowed_ipacket_size, size_t max_allowed_opacket_size)
        : max_allowed_ipacket_size(max_allowed_ipacket_size)
        , max_allowed_opacket_size(max_allowed_opacket_size)
    {
    }

    virtual ~dialect()
    {
    }


    /**
     *	Reads one packet from `src` to the specified `dst`.
     *	
     *	May skip necessary amount of leading bytes.
     *	
     *	Returns `true` on success, `false` otherwise.
     */
    virtual bool read(ipacket_t &dst, byte_buffer &src) = 0;


    /**
     *	Writes one packet specified by `src` to `dst` buffer.
     *	
     *	Returns `true` on success, `false` otherwise.
     *	
     *	Throws `dialect_exception` in case if `dst` capacity
     *	is lower than amount of bytes need to represent `src`.
     */
    virtual bool write(byte_buffer &dst, const opacket_t &src) = 0;
};

}