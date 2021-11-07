#ifndef PACKETLISTENER_HPP
#define PACKETLISTENER_HPP

#include "utils/buffer.hpp"

namespace ccsds
{

/**
 * Listener of new spacepackets received 
 */
class SpListener
{
public:
    virtual void newSpacepacket(const IBuffer& bytes) = 0;
};

} //namespace

#endif //PACKETLISTENER_HPP
