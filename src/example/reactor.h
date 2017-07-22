#pragma once

#include <act-common/reactor.h>
#include <thread>

namespace {
namespace example
{

    using namespace com_port_api;

    class custom_dialect : public dialect < char, char >
    {

    public:

        bool read(char &dst, byte_buffer &src)
        {
            char c;
            if (src.get(c))
            {
                dst = c;
                return true;
            }
            return false;
        }

        bool write(byte_buffer &dst, const char &src)
        {
            char c = src;
            if (dst.put(&c, 1) == 0)
            {
                return true;
            }
            return false;
        }
    };

    using reactor_t = reactor < custom_dialect > ;

    void reactor_example_setup()
    {
        // creating and starting reactor with default parameters
        reactor_t r;
        r.start();

        // opening port is required since reactor
        // does not know anything of the port to use
        com_port port;
        port.open(com_port_options("COM3", CBR_4800, 8, true, ODDPARITY, ONESTOPBIT));

        // move port to reactor
        r.supply_port(std::move(port));

        // send some packets
        r.supply_opacket(reactor_t::opacket_t(56));
        r.supply_opacket(reactor_t::opacket_t(57));
        r.supply_opacket(reactor_t::opacket_t(58));

        // wait for new packets
        std::this_thread::sleep_for(std::chrono::seconds(1));

        std::list<reactor_t::ipacket_t> packets;
        {
            // lock on the iqueue_mutex to be sure
            // that no concurrent updates will be performed
            // note, that it freezes the reactor!
            reactor_t::guard_t guard(r.iqueue_mutex);

            // read all the pending packets
            // clear the iqueue!!!
            packets.splice(packets.end(), r.iqueue);
        }

        // stop the reactor and wait for its actual termination
        // note that this process may still lasts for a long time
        // (configurable), since WinAPI file operations are blocking
        r.stop();
        r.join();
    }

}
}
