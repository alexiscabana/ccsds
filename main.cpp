/******************************************************************************

                              Online C++ Compiler.
               Code, Compile, Run and Debug C++ program online.
Write your code in this editor and press "Run" button to compile and execute it.

*******************************************************************************/
#include "utils/obitstream.hpp"
#include "utils/ibitstream.hpp"
#include "utils/datafield.hpp"
#include "spacepacket/primaryhdr.hpp"
#include "spacepacket/secondaryhdr.hpp"
#include "spacepacket/spacepacket.hpp"
#include "spacepacket/transfer.hpp"
#include <iostream>
#include <cstdint>

int main()
{
    Buffer<32> buf1;
    Buffer<32> buf2;
    OBitStream os(buf1);
    //IBitStream is;
    //ccsds::sp_primaryhdr h;
    FieldCollection<FieldArray<3,uint8_t,4>,Field<uint32_t>> collection;
    ccsds::SpEmptySecondaryHeader empty;


    ccsds::SpBuilder<ccsds::SpEmptySecondaryHeader> builder(buf2);
    using MySpacepacketContent = FieldCollection<FieldArray<3,uint8_t,4>,Field<uint8_t,4>,Field<uint32_t>>;
    
    
    ccsds::SpDissector<ccsds::SpEmptySecondaryHeader, MySpacepacketContent> other;
    
    //h.sechdrflag.reset();

    collection.getField<1>().setValue(0xFAAAAAAFU);
    builder.primary_hdr.apid.setIdle();
    builder.primary_hdr.sequence_flags.setValue(ccsds::SpPrimaryHeader::SequenceFlags::LAST_SEGMENT_VALUE);
    //os << sec1;
    //std::cout << "building sp" << std::endl;
    builder.data() << collection;
    //std::cout << "outputting" << std::endl;
    //os << sp;
    buf2.print();
    builder.primary_hdr.print();
    
    //os << empty;
    //os << sp;
    //is >> sec1;

    ccsds::SpTransferService::getInstance();
    
    /*
    std::cout << sec1.getNbFields() << std::endl;
    std::cout << sec2.getNbFields() << std::endl;
    */
    //std::cout << builder.getUserDataWidth() << std::endl;
    //std::cout << builder.isValid() << std::endl;

    //std::cout << static_cast<unsigned int>(sec1.getField<1>().getValue()) << std::endl;
    
    return 0;
}
