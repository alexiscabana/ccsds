#ifndef PACKETTRANSFERSERVICE_HPP
#define PACKETTRANSFERSERVICE_HPP

#include "spacepacket/spacepacket.hpp"
#include "spacepacket/primaryhdr.hpp"

namespace ccsds
{

/**
 * Listener of new spacepackets received 
 */
class SpListener
{
public:
    virtual void newSpacepacket() = 0;
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

public:

    static const std::size_t LISTENERS_MAX_SIZE = 1000;

    static SpTransferService& getInstance() {
        static SpTransferService instance;
        return instance;
    }

    void push(IBuffer& sp) {
        (void)sp;
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

    std::size_t nb_listeners;
    ListenerEntry listenerEntries[LISTENERS_MAX_SIZE];
};

} //namespace

#endif //PACKETTRANSFERSERVICE_HPP
