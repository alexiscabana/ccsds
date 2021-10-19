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
 * Base interface for spacepackets
 */
template<typename SecHdrType>
class ISpacepacket
{
    static_assert((std::is_base_of<ISpSecondaryHeader, SecHdrType>::value), 
                "Secondary header type must be of type ISpSecondaryHeader");
public:

    typedef SecHdrType SecondaryHdrType;

    virtual std::size_t getUserDataWidth() = 0;

    bool hasSecondaryHdr() {
        return SecHdrType::getSize() > 0;
    }

    bool isValid() {
        
        //There shall be a User Data Field, or a Packet Secondary Header, or both (pink book, 4.1.3.2.1.2 and 4.1.3.3.2)
        if(SecHdrType::getSize() == 0 && this->getUserDataWidth() == 0) {
            return false;
        }

        //If present, the User Data Field shall consist of an integral number of octets (4.1.3.3.3)
        if(this->getUserDataWidth() % CHAR_BIT != 0) {
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
        if(primary_hdr.length.getLength() != 
            (this->getUserDataWidth() / CHAR_BIT + SecHdrType::getSize())) {
            return false;
        }

        return true;
    }

     std::size_t getSize() {
        return SpPrimaryHeader::getSize() + SecHdrType::getSize() + (this->getUserDataWidth() / CHAR_BIT) + 
            (this->getUserDataWidth() % CHAR_BIT > 0 ? 1 : 0);
     }

    SecHdrType      secondary_hdr;
    SpPrimaryHeader primary_hdr;
};

/**
 * Spacepacket builder template
 */
template<typename SecHdrType>
class SpBuilder : public ISpacepacket<SecHdrType>, public Serializable
{
public:
    SpBuilder(IBuffer& buffer)
    : user_data_buffer(buffer.getStart() + SpPrimaryHeader::getSize() + SecHdrType::getSize(),
                       buffer.getSize() - SpPrimaryHeader::getSize() - SecHdrType::getSize()),
      user_data(user_data_buffer), 
      total_buffer(buffer) {

    }

    void serialize(OBitStream& o) const override {
        // If present, the Packet Secondary Header shall follow, without gap, the Packet Primary Header (pink book,4.1.3.2.1.1)
        // The user data field must also follow the secondary header (or primary header if no secondary header) (pink book, 4.1.3.3.1)
        o << this->primary_hdr << this->secondary_hdr << user_data;
    }
    
    std::size_t getUserDataWidth() override {
        return user_data.getWidth();
    }

    OBitStream& data() {
        return user_data;
    }

    void finalize() {
        // serialize the primary and secondary header using the same complete buffer, but at
        // the beginning
        OBitStream beginning(this->getBuffer());

        if(this->hasSecondaryHdr()) {
            this->primary_hdr.sec_hdr_flag.set();
        }

        // [...] field shall contain a length count C that equals [...] the Packet Data Field (pink book, 4.1.2.5.1.2)
        // Packet Data Field is comprised of the secondary header and the user data
        this->primary_hdr.length.setLength(SecHdrType::getSize() + user_data.getSize());

        //the first few bytes were skipped to keep space to write both headers
        beginning << this->primary_hdr << this->secondary_hdr;
    }

    IBuffer& getBuffer() {
        return total_buffer;
    }

protected:
    UserBuffer user_data_buffer;
    OBitStream user_data;
    IBuffer& total_buffer;
};

/**
 * Spacepacket extractor template
 */
template<typename SecHdrType>
class SpExtractor : public ISpacepacket<SecHdrType>
{
public:
    SpExtractor(IBuffer& buffer)
    : stream(buffer), buffer(buffer) {
        //start by deserializing the two headers
        stream >> this->primary_hdr >> this->secondary_hdr;
    }

    std::size_t getUserDataWidth() override {
        // spacepacket is alreadyformed, so the user data zone is simply described as the complete buffer minus both headers
        return (buffer.getSize() - SpPrimaryHeader::getSize() - SecHdrType::getSize()) * CHAR_BIT;
    }

    IBitStream& data() {
        return stream;
    }

    IBuffer& getBuffer() {
        return buffer;
    }

protected:
    IBitStream stream;
    IBuffer& buffer;
};

/**
 * Idle spacepacket template
 */
template<typename PatternType, PatternType IdleDataPattern = 0xFFU>
class SpIdleBuilder : public SpBuilder<SpEmptySecondaryHeader>
{
    static_assert(std::is_unsigned<PatternType>::value, 
                    "Only unsigned Idle packet pattern are supported.");

public:
    SpIdleBuilder(IBuffer& user_data_buffer)
    : SpBuilder(user_data_buffer) {
        this->primary_hdr.apid.setValue(SpPrimaryHeader::PacketApid::IDLE_VALUE);
    }

    void fillIdleData(std::size_t nb_pattern) {
        for(std::size_t i = 0; i < nb_pattern; i++) {
            user_data.put(IdleDataPattern, sizeof(PatternType)*CHAR_BIT);
        }
    }
};

/**
 * Spacepacket Dissector template
 */
template<typename SecHdrType, 
        typename ...Fields>
class SpDissector : public ISpacepacket<SecHdrType>, public Deserializable
{
    static_assert((true && ... && std::is_base_of<IField, Fields>::value), 
                    "RX Spacepacket fields must all derive from IField");
    static_assert((0 + ... + Fields::getWidth()) % CHAR_BIT == 0, 
                    "RX Spacepacket definition must fit in an integral number of octet");
    static_assert(SecHdrType::getSize() > 0|| (0 + ... + Fields::getWidth()) > 0, 
                    "There shall be a User Data Field, or a Packet Secondary Header, or both (pink book, 4.1.3.2.1.2 and 4.1.3.3.2)");
public:
    SpDissector() = default;

    void deserialize(IBitStream& i) override {
        std::apply([&](auto&&... args){ (void(i >> args), ...); }, field_tuple);
    }

    std::size_t getUserDataWidth() override {
        return (0 + ... + Fields::getWidth());
    }

    template<std::size_t index>
    auto& getField() {
        static_assert(index < (sizeof...(Fields) + 1), "Field index out of range");
        return std::get<index>(field_tuple);
    }

private:
    std::tuple<Fields...>   field_tuple;
};

} //namespace

#endif //CCSDS_SPACEPACKET_HPP