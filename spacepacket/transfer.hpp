#ifndef PACKETTRANSFERSERVICE_HPP
#define PACKETTRANSFERSERVICE_HPP

#include "spacepacket/primaryhdr.hpp"
#include "spacepacket/spacepacket.hpp"

namespace ccsds
{

/**
 * Listener of new spacepackets received 
 */
class SpListener
{
public:
    virtual void newSpacepacket(IBuffer& bytes) = 0;
};

/**
 * Service of spacepacket transfer
 */
class SpTransferService
{
    /**
     * Predicate for matching spacepackets
     */
    class ListenerPredicate {
    public:
        ListenerPredicate() = default;
        ListenerPredicate(SpPrimaryHeader::PacketApid apid, bool matchAll = false)
        : apid(apid), matchAll(matchAll) {

        }

        bool operator()(SpPrimaryHeader::PacketApid other_apid) {
            return matchAll || apid.getValue() == other_apid.getValue();
        }

    private:
        SpPrimaryHeader::PacketApid apid;
        bool matchAll;
    };

    struct ListenerEntry {
        SpListener* listener;
        ListenerPredicate matcher;
    };

    struct ApidContext {
        std::size_t rx_count = 0;
        std::size_t tx_count = 0;
        SpPrimaryHeader::SequenceCount next_count; //count is 0 by default
    };

    struct Telmetry {
        ApidContext contexes[SpPrimaryHeader::PacketApid::IDLE_VALUE];
    };

public:

    static const std::size_t LISTENERS_MAX_SIZE = 1000;

    static SpTransferService& getInstance() {
        static SpTransferService instance;
        return instance;
    }

    template<typename SecHdr>
    void transmit(SpBuilder<SecHdr>& sp) {
        sp.finalize();
        this->transmitBuffer(sp.getBuffer());
    }

    void registerListener(SpListener* listener) {
        if(listener == nullptr || nb_listeners >= LISTENERS_MAX_SIZE) {
            return;
        }

        SpPrimaryHeader::PacketApid any;

        // add in watchers
        listenerEntries[nb_listeners].listener = listener;
        new (&listenerEntries[nb_listeners].matcher) ListenerPredicate(any, true);
        nb_listeners++;
    }

    void registerListener(SpListener* listener, uint16_t apid_value) {
        if(listener == nullptr || nb_listeners >= LISTENERS_MAX_SIZE) {
            return;
        }

        SpPrimaryHeader::PacketApid match(apid_value);

        // add in watchers
        listenerEntries[nb_listeners].listener = listener;
        new (&listenerEntries[nb_listeners].matcher) ListenerPredicate(match);
        nb_listeners++;
    }

    void unregisterListener(SpListener* listener) {
        for(uint32_t i = 0; i < nb_listeners; i++) {
            if(listenerEntries[i].listener == listener) {
                //switch to the listener at the end
                listenerEntries[i] = listenerEntries[nb_listeners - 1];
                nb_listeners--;
                break;
            }
        }
    }

private:
    SpTransferService()
    : nb_listeners(0) {
        
    }

    void transmitBuffer(IBuffer& buffer) {
        IBitStream istream(buffer);
        SpPrimaryHeader pri_header;

        istream >> pri_header;

        //update current context
        auto apid_value = pri_header.apid.getValue();
        ++contexes[apid_value].tx_count;
        ++contexes[apid_value].next_count;

        //listeners have to be notified of this new spacepacket
        this->notifyListeners(pri_header.apid, buffer);
    }

    void notifyListeners(SpPrimaryHeader::PacketApid& apid, IBuffer& buffer) {
        for(uint32_t i = 0; i < nb_listeners; i++) {
            if(listenerEntries[i].matcher(apid)) {
                listenerEntries[i].listener->newSpacepacket(buffer);
            }
        }
    }

    std::size_t nb_listeners;
    ListenerEntry listenerEntries[LISTENERS_MAX_SIZE];
    ApidContext contexes[SpPrimaryHeader::PacketApid::IDLE_VALUE];
    Telmetry telemetry;
};

} //namespace

#endif //PACKETTRANSFERSERVICE_HPP
