#ifndef PACKETTRANSFERSERVICE_HPP
#define PACKETTRANSFERSERVICE_HPP

#include "spacepacket/primaryhdr.hpp"
#include "spacepacket/spacepacket.hpp"
#include "spacepacket/listener.hpp"

namespace ccsds
{

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
        std::size_t tx_rx_count = 0;
        std::size_t rx_count = 0;
        std::size_t tx_count = 0;
        std::size_t tx_rx_error_count = 0;
        std::size_t rx_error_count = 0;
        std::size_t tx_error_count = 0;
    };

public:

    static const std::size_t LISTENERS_MAX_SIZE = 1000;

    static SpTransferService& getInstance() {
        static SpTransferService instance;
        return instance;
    }

    template<typename SecHdr>
    void transmit(SpBuilder<SecHdr>& sp) {
        //set the sequence count depending on the context of the sender's APID
        uint16_t apid_value = sp.primary_hdr.apid.getValue();
        sp.primary_hdr.sequence_count = this->contexes[apid_value].next_count;
        sp.finalize();

        // only send valid packets
        if(sp.isValid()) {
            this->transmitBuffer(apid_value, sp.getBuffer());
            this->telemetry.tx_count++;
            this->telemetry.tx_rx_count++;
        } else {
            this->telemetry.tx_error_count++;
            this->telemetry.tx_rx_error_count++;
        }
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

    void transmitBuffer(uint16_t apid_value, IBuffer& buffer) {
        //listeners have to be notified of this new spacepacket
        this->notifyListeners(SpPrimaryHeader::PacketApid(apid_value), buffer);

        //update current context of the APID
        ++contexes[apid_value].tx_count;
        ++contexes[apid_value].next_count;
    }

    void notifyListeners(SpPrimaryHeader::PacketApid apid, IBuffer& buffer) {
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
