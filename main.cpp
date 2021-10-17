/******************************************************************************

                              Online C++ Compiler.
               Code, Compile, Run and Debug C++ program online.
Write your code in this editor and press "Run" button to compile and execute it.

*******************************************************************************/
#include "utils/ibitstream.hpp"
#include "utils/obitstream.hpp"
#include "utils/datafield.hpp"
#include "spacepacket/primaryhdr.hpp"
#include "spacepacket/secondaryhdr.hpp"
#include "spacepacket/spacepacket.hpp"
#include "spacepacket/transfer.hpp"
#include <iostream>
#include <cstdint>

class OnReceivePrinter : public ccsds::SpListener {
    void newSpacepacket(IBuffer& bytes) override {
        IBitStream in(bytes);
        ccsds::SpPrimaryHeader pri_hdr;

        in >> pri_hdr;

        std::cout << std::string(80,'-') << std::endl;
        pri_hdr.print();
    }
};

using MySpacepacketContent = FieldCollection<Field<uint32_t>,Field<uint32_t>>;
using MySecondaryHeader = ccsds::SpSecondaryHeader<FieldCollection<>,
                                                    Field<uint32_t>>;
int main()
{
    OnReceivePrinter printer;
    Buffer<32> buf2;
    Buffer<256> bufferIdle;
    ccsds::SpBuilder<MySecondaryHeader> packet(buf2);
    ccsds::SpIdleBuilder<uint8_t, 0xFFU> idlePacket(bufferIdle);
    
    MySpacepacketContent collection;
    collection.getField<0>().setValue(0xBDDDDDDBU);
    collection.getField<1>().setValue(0xFAAAAAAFU);
    packet.secondary_hdr.ancilliary_data.setValue(0x19999991U);
    packet.data() << collection;

    idlePacket.fillIdleData(250);

    // test the transfer service
    ccsds::SpTransferService::getInstance().registerListener(&printer); // will print all spacepackets
    ccsds::SpTransferService::getInstance().transmit(packet);
    ccsds::SpTransferService::getInstance().transmit(idlePacket);
    ccsds::SpTransferService::getInstance().transmit(idlePacket);
    ccsds::SpTransferService::getInstance().transmit(idlePacket);
        
    return 0;
}
