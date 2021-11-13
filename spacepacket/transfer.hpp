/**************************************************************************//**
 * @file transfer.hpp
 * @author Alexis Cabana-Loriaux
 * 
 * @brief Contains a definition of a Spacepacket communication layer
 * 
 ******************************************************************************/
#ifndef PACKETTRANSFERSERVICE_HPP
#define PACKETTRANSFERSERVICE_HPP

#include "utils/allocator.hpp"
#include "utils/commlayer.hpp"
#include "spacepacket/primaryhdr.hpp"
#include "spacepacket/spacepacket.hpp"
#include "spacepacket/listener.hpp"

namespace ccsds
{

/**
 * Service of spacepacket transfer
 */
template<typename Allocator = DefaultAllocator>
class SpTransferService : public ICommunicationLayer
{
    static_assert(std::is_base_of<IAllocator, Allocator>::value, "The chosen allocator is not valid");

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

        bool matchesAll() {
            return matchAll;
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

    struct Telemetry {
        std::size_t rx_count = 0;
        std::size_t tx_count = 0;
        std::size_t rx_error_count = 0;
        std::size_t tx_error_count = 0;
    };

public:
    SpTransferService(std::size_t nb_listeners_max = 1000, const Allocator& alloc = Allocator())
    : nb_listeners_max(nb_listeners_max), nb_listeners(0), allocator(alloc) {

        listener_buffer = this->allocator.allocateBuffer(nb_listeners_max * sizeof(ListenerEntry));
        listener_entries = reinterpret_cast<ListenerEntry*>(listener_buffer.getStart());
    }

    ~SpTransferService() {
        this->allocator.deallocateBuffer(listener_buffer);
    }

    template<typename SecHdr, typename A>
    void transmit(SpBuilder<SecHdr, A>& sp) {
        //set the sequence count depending on the context of the sender's APID
        uint16_t apid_value = sp.primary_hdr.apid.getValue();
        sp.primary_hdr.sequence_count = this->contexes[apid_value].next_count;
        sp.finalize();

        // only send valid packets
        if(sp.isValid()) {
            this->transmitValidBuffer(apid_value, sp.getBuffer(), false);
            this->telemetry.tx_count++;
        } else {
            this->telemetry.tx_error_count++;
        }
    }

    template<typename ...T>
    void transmit(SpDissector<T...>& sp) {

        //set the sequence count depending on the context of the sender's APID
        uint16_t apid_value = sp.primary_hdr.apid.getValue();
        sp.primary_hdr.sequence_count = this->contexes[apid_value].next_count;
        sp.finalize();

        // only send valid packets
        if(sp.isValid()) {
            //serialize to buffer and transmit
            UserBuffer buffer = this->allocator.allocateBuffer(sp.getSize());
            sp.toBuffer(buffer);
            this->transmitValidBuffer(apid_value, buffer, false);

            //cleanup
            this->allocator.deallocateBuffer(buffer);
            this->telemetry.tx_count++;
        } else {
            this->telemetry.tx_error_count++;
        }
    }

    void registerListener(SpListener* listener) {
        if(listener == nullptr || nb_listeners >= nb_listeners_max) {
            return;
        }

        SpPrimaryHeader::PacketApid any;

        // add in watchers
        listener_entries[nb_listeners].listener = listener;
        new (&listener_entries[nb_listeners].matcher) ListenerPredicate(any, true);
        nb_listeners++;
    }

    void registerListener(SpListener* listener, uint16_t apid_value) {
        if(listener == nullptr || nb_listeners >= nb_listeners_max) {
            return;
        }

        SpPrimaryHeader::PacketApid match(apid_value);

        // add in watchers
        listener_entries[nb_listeners].listener = listener;
        new (&listener_entries[nb_listeners].matcher) ListenerPredicate(match);
        nb_listeners++;
    }

    void unregisterListener(SpListener* listener) {
        for(uint32_t i = 0; i < nb_listeners; i++) {
            if(listener_entries[i].listener == listener) {
                //switch to the listener at the end
                listener_entries[i] = listener_entries[nb_listeners - 1];
                nb_listeners--;
                break;
            }
        }
    }
    
    void connectUpperLayer(ICommunicationLayer& upper_layer) override {
        (void)upper_layer;
        //do nothing, the spacepacket layer cannot have an upper layer
    }

private:
    void receiveFromSubLayer(const IBuffer& buffer) override {
        // TODO: validate RX spacepacket
        // for now just assume SP is valid
        IBitStream in(buffer);
        SpPrimaryHeader pri_hdr;
        in >> pri_hdr;

        uint16_t apid_value = pri_hdr.apid.getValue();

        if(!pri_hdr.apid.isIdle()) {
            //validate that the count is sequential
            auto next_count = this->contexes[apid_value].next_count;

            if(next_count.getValue() == pri_hdr.sequence_count.getValue()) {
                this->transmitValidBuffer(apid_value, buffer, true);
                this->telemetry.rx_count++;
            } else {
                this->telemetry.rx_error_count++;
            }
        }
        else
        {
            this->transmitValidBuffer(apid_value, buffer, true);
            this->telemetry.rx_count++;
        }
    }

    void receiveFromUpperLayer(const IBuffer& bytes) override {
        (void)bytes;
        //unused, Spacepacket layer is an application layer
    }

    void transmitValidBuffer(uint16_t apid_value, const IBuffer& buffer, bool isSubLayerBuffer) {
        //listeners have to be notified of this new spacepacket
        this->notifyListeners(SpPrimaryHeader::PacketApid(apid_value), buffer);

        // only transmit to sub-layer if the buffer doesn't already come from that layer
        if(!isSubLayerBuffer) {
            this->pushToSubLayer(buffer);
        }

        //update current context of the APID
        isSubLayerBuffer ? ++contexes[apid_value].rx_count : 
                           ++contexes[apid_value].tx_count;
        ++contexes[apid_value].next_count;
    }

    void notifyListeners(SpPrimaryHeader::PacketApid apid, const IBuffer& buffer) {
        for(uint32_t i = 0; i < nb_listeners; i++) {
            if(listener_entries[i].matcher(apid)) {
                listener_entries[i].listener->newSpacepacket(buffer);
            }
        }
    }

    const Allocator& allocator;
    std::size_t nb_listeners;
    const std::size_t nb_listeners_max;
    ListenerEntry* listener_entries;
    UserBuffer listener_buffer;

    ApidContext contexes[SpPrimaryHeader::PacketApid::IDLE_VALUE + 1];
    Telemetry telemetry;
};

} //namespace

#endif //PACKETTRANSFERSERVICE_HPP
