#ifndef CCSDS_SPACEPACKET_HPP
#define CCSDS_SPACEPACKET_HPP

#include "utils/serializable.hpp"
#include "utils/buffer.hpp"
#include "spacepacket/primaryhdr.hpp"
#include "spacepacket/secondaryhdr.hpp"
#include <tuple>
#include <cstdint>
#include <climits>
#include <type_traits>

namespace ccsds
{

/**
 * Spacepacket builder template
 */
template<typename SecHdrType>
class SpBuilder : public Serializable
{
    static_assert((std::is_base_of<ISpSecondaryHeader, SecHdrType>::value), 
                    "Secondary header type must be of type ISpSecondaryHeader");
    //static_assert(BufferType::getSize() > 0 || SecHdrType::getSize() > 0, 
    //                "There shall be a User Data Field, or a Packet Secondary Header, or both (pink book, 4.1.3.2.1.2 and 4.1.3.3.2)");
public:
    SpBuilder(IBuffer& user_data_buffer)
    : user_data(user_data_buffer) {

    }

    void serialize(OBitStream& o) const override {
        o << primary_hdr << secondary_hdr << user_data;
    }
    
    std::size_t size() {
        return SpPrimaryHeader::getSize() + SecHdrType::getSize() + user_data.getSize();
    }

    OBitStream& data() {
        return user_data;
    }

    SpPrimaryHeader primary_hdr;
    SecHdrType      secondary_hdr;

private:
    OBitStream      user_data;
};

} //namespace

#endif //CCSDS_SPACEPACKET_HPP