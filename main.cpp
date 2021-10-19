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

using MySpacepacketContent = FieldCollection<Field<uint32_t>,Field<uint32_t>>;
using MySecondaryHeader = ccsds::SpSecondaryHeader<FieldEmpty, Field<uint32_t>>;
using MySpacepacket = ccsds::SpBuilder<MySecondaryHeader>;
using MyIdleSpacepacket = ccsds::SpIdleBuilder<uint8_t, 0xFFU>;

using AParticularPacketDefinitionClass = ccsds::SpDissector<MySecondaryHeader, 
                                        Field<uint64_t>, 
                                        Field<uint8_t,4>,
                                        Flag,Flag,Flag,Flag,
                                        Field<uint32_t,24>,
                                        Field<uint8_t>>;

class NewSpacepacketPrinter : public ccsds::SpListener {
    void newSpacepacket(IBuffer& bytes) override {
        IBitStream in(bytes);
        ccsds::SpPrimaryHeader pri_hdr;

        // constructing the packet from the buffer autmatically dissects it and all the data is available to the user
        AParticularPacketDefinitionClass packet(bytes);

        in >> pri_hdr;

        std::cout << std::string(80,'-') << std::endl;
        pri_hdr.print();

        if(!pri_hdr.apid.isIdle()) {
            bytes.print();
            std::cout<< packet.getField<2>().isSet()<<std::endl;
        }
    }
};

int main()
{
    NewSpacepacketPrinter printer;

    Buffer<22> buf2;
    Buffer<256> bufferIdle;

    MySpacepacket packet(buf2);
    MyIdleSpacepacket idlePacket(bufferIdle);
    
    /*MySpacepacketContent collection;
    collection.getField<0>().setValue(0xBDDDDDDBU);
    collection.getField<1>().setValue(0xFAAAAAAFU);*/

    packet.secondary_hdr.ancilliary_data.setValue(0x19999991U);
    packet.data() << 0xEEEECCCCB000000BU;
    packet.data() << 0xFAAAAAAFU;

    idlePacket.fillIdleData(25);

    // test the transfer service
    ccsds::SpTransferService::getInstance().registerListener(&printer); // will print all spacepackets
    ccsds::SpTransferService::getInstance().transmit(packet);

    //ccsds::SpTransferService::getInstance().transmit(idlePacket);
    //ccsds::SpTransferService::getInstance().transmit(idlePacket);

    return 0;
}
