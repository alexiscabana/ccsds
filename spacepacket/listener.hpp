/**************************************************************************//**
 * @file listener.hpp
 * @author Alexis Cabana-Loriaux
 * 
 * @brief Contains class for listeners of new packets in the Spacepacket layer
 * 
 ******************************************************************************/
#ifndef PACKETLISTENER_HPP
#define PACKETLISTENER_HPP

#include "utils/buffer.hpp"

namespace ccsds
{

/**
 * @brief Listener of new spacepackets received.
 * @note This listener can only be notified of new spacepackets if it was 
 *       previously registered as a listener to the spacepacket transfer
 *       service. @see{SpTransferService}.
 */
class SpListener
{
public:
    /**
     * @brief Callback of new spacepackets in the layer. This can be called by the spacepacket transfer service 
     *        two situations:
     *          - When a spacepacket is sent by another producer on the same system (1)
     *          - When a spacepacket is sent by a communication sub-layer (2)
     * 
     * @verbatim
     *      --------------                 --------------
     *      | Producer 1 |      .....      | Listener N |
     *      --------------                 --------------
     *              |                            ^
     *              |                            | (1)(2)
     *              | (1)                        |
     *          --------------------------------------
     *          |          Spacepacket Layer         |
     *          --------------------------------------
     *                             ^
     *                             | (2)
     *          --------------------------------------
     *          |              Sub-Layer             |
     *          --------------------------------------
     * @endverbatim
     * 
     * @param bytes The buffer representing the spacepacket broadcasted in the layer
     * 
     * @note It is up to the user to interpret the spacepacket bytes. @see{SpExtractor} and @see{SpDissector}.
     */
    virtual void newSpacepacket(const IBuffer& bytes) = 0;
};

} //namespace

#endif //PACKETLISTENER_HPP
