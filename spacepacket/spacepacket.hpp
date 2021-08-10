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

enum {
    SPACEPACKET_MIN_SIZE = 7,
    SPACEPACKET_MAX_SIZE = 65542
};

/**
 * Spacepacket builder template
 */
template<typename SecHdrType,
        uint8_t IdleDataPattern = 0xFFU> // Pattern of Idle Data is set by the mission
class SpBuilder : public Serializable
{
    static_assert((std::is_base_of<ISpSecondaryHeader, SecHdrType>::value), 
                    "Secondary header type must be of type ISpSecondaryHeader");
public:
    SpBuilder(IBuffer& user_data_buffer)
    : user_data(user_data_buffer) {

    }

    void serialize(OBitStream& o) const override {
        // If present, the Packet Secondary Header shall follow, without gap, the Packet Primary Header (pink book,4.1.3.2.1.1)
        // The user data field must also follow the secondary header (or primary header if no secondary header) (pink book, 4.1.3.3.1)
        o << primary_hdr << secondary_hdr << user_data;
    }
    
    std::size_t getSize() {
        return SpPrimaryHeader::getSize() + SecHdrType::getSize() + user_data.getSize();
    }

    OBitStream& data() {
        return user_data;
    }

    void fillIdleData(std::size_t nb_bytes) {
        for(std::size_t i = 0; i < nb_bytes; i++) {
            user_data.put(IdleDataPattern, CHAR_BIT);
        }
    }

    bool isValid() {
        //There shall be a User Data Field, or a Packet Secondary Header, or both (pink book, 4.1.3.2.1.2 and 4.1.3.3.2)
        if(SecHdrType::getSize() == 0 && user_data.getSize() == 0) {
            return false;
        }

        //If present, the User Data Field shall consist of an integral number of octets (4.1.3.3.3)
        if(user_data.getWidth() % CHAR_BIT != 0) {
            return false;
        }

        //A Space Packet shall consist of at least 7 and at most 65542 octets (pink book, 4.1.1.2)
        if(this->getSize() < SPACEPACKET_MIN_SIZE || this->getSize() > SPACEPACKET_MAX_SIZE) {
            return false;
        }

        //[...] ‘1’ if a Packet Secondary Header is present; it shall be ‘0’ if a Packet Secondary Header is not present (pink book, 4.1.2.3.3.2)
        if((SecHdrType::getSize() == 0 && primary_hdr.sec_hdr_flag.isSet()) || 
            (SecHdrType::getSize() > 0 && !primary_hdr.sec_hdr_flag.isSet())) {
            return false;
        }

        // The Secondary Header Flag shall be set to ‘0’ for Idle Packets. (pink book, 4.1.2.3.3.4)
        if(primary_hdr.apid.isIdle() && primary_hdr.sec_hdr_flag.isSet()) {
            return false;
        }

        //Secondary header is not permitted for Idle packets (pink book, 4.1.3.2.1.4)
        if(primary_hdr.apid.isIdle() && SecHdrType::getSize() > 0) {
            return false;
        }

        // Bits 32–47 of the Packet Primary Header shall contain the Packet Data Length. (pink book, 4.1.2.5.1.1)
        if(primary_hdr.length.getLength() != this->getSize()) {
            return false;
        }

        return true;
    }

    SpPrimaryHeader primary_hdr;
    SecHdrType      secondary_hdr;

private:
    OBitStream      user_data;
};

/**
 * Spacepacket Dissector template
 */
template<typename SecHdrType, 
        typename ...Fields>
class SpDissector : public Deserializable
{
public:
    SpDissector() 
    : is_pri_hdr_provided(false), is_sec_hdr_provided(false) {

    }

    SpDissector(SpPrimaryHeader& pri_hdr)
    : primary_hdr(pri_hdr), is_pri_hdr_provided(true), is_sec_hdr_provided(false) {

    }

    SpDissector(SpPrimaryHeader& pri_hdr, SecHdrType& sec_hdr)
    : primary_hdr(pri_hdr), secondary_hdr(sec_hdr), is_pri_hdr_provided(true), is_sec_hdr_provided(true) {

    }

    void deserialize(IBitStream& i) override {

        // Only deserialize primary and secondary headers if they were not already provided at construction
        if(!is_pri_hdr_provided) {
            i >> primary_hdr;
            is_pri_hdr_provided = false;
        }

        if(!is_sec_hdr_provided) {
            i >> secondary_hdr;
            is_sec_hdr_provided = false;
        }

        std::apply([&](auto&&... args){ (void(i >> args), ...); }, field_tuple);
    }

    template<std::size_t index>
    auto& getField() {
        static_assert(index < (sizeof...(Fields) + 1), "Field index out of range");
        return std::get<index>(field_tuple);
    }

    SpPrimaryHeader         primary_hdr;
    SecHdrType              secondary_hdr;

private:
    std::tuple<Fields...>   field_tuple;
    bool                    is_pri_hdr_provided;
    bool                    is_sec_hdr_provided;
};

} //namespace

#endif //CCSDS_SPACEPACKET_HPP