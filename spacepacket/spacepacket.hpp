#ifndef CCSDS_SPACEPACKET_HPP
#define CCSDS_SPACEPACKET_HPP

#include "utils/serializable.hpp"
#include "utils/buffer.hpp"
#include "utils/allocator.hpp"
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

        // check first of all if the primary header is valid
        if(!this->primary_hdr.isValid()) {
            return false;
        }
        
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
        if((!hasSecondaryHdr() &&  primary_hdr.sec_hdr_flag.isSet()) || 
            (hasSecondaryHdr() && !primary_hdr.sec_hdr_flag.isSet())) {
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
template<typename SecHdrType, typename Allocator = DefaultAllocator>
class SpBuilder : public ISpacepacket<SecHdrType>, public Serializable
{
public:
    SpBuilder(std::size_t total_size, const Allocator& alloc = Allocator())
    : allocator(alloc) {
        // we allocate only the hinted size 
        total_buffer = this->allocator.allocateBuffer(total_size);
        //buffer segment where user data will get serialized
        user_data_buffer = UserBuffer(total_buffer.getStart() + SpPrimaryHeader::getSize() + SecHdrType::getSize(),
                                      total_buffer.getSize() - SpPrimaryHeader::getSize() - SecHdrType::getSize());
        user_data.attach(user_data_buffer);
    }
    ~SpBuilder() {
        this->allocator.deallocateBuffer(total_buffer);
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

        //TODO check for user data field that is not byte-aligned
    }

    IBuffer& getBuffer() {
        return total_buffer;
    }

protected:
    const Allocator& allocator;
    UserBuffer total_buffer;
    UserBuffer user_data_buffer;
    OBitStream user_data;
};

/**
 * Idle spacepacket template
 */
template<typename PatternType = uint8_t, 
        PatternType IdleDataPattern = 0xFFU, 
        typename Allocator = DefaultAllocator>
class SpIdleBuilder : public SpBuilder<SpEmptySecondaryHeader, Allocator>
{
    static_assert(std::is_unsigned<PatternType>::value, 
                    "Only unsigned Idle packet pattern are supported.");

public:
    SpIdleBuilder(std::size_t total_size, const Allocator& alloc = Allocator())
    : SpBuilder<SpEmptySecondaryHeader, Allocator>(total_size, alloc) {
        this->primary_hdr.apid.setValue(SpPrimaryHeader::PacketApid::IDLE_VALUE);

        if(total_size > SpPrimaryHeader::getSize()) {
            std::size_t packet_data_field_size = total_size - SpPrimaryHeader::getSize();

            //Fill all the packet data field bytes to the given pattern
            std::size_t nb_full_pattern = packet_data_field_size / sizeof(PatternType);
            uint8_t nb_remainder_bytes  = packet_data_field_size % sizeof(PatternType);
            
            for(std::size_t i = 0; i < nb_full_pattern ; i++) {
                this->user_data.put(IdleDataPattern, sizeof(PatternType)*CHAR_BIT);
            }

            if(nb_remainder_bytes > 0) {
                // put the beginning of the pattern as the remainder
                this->user_data.put((IdleDataPattern >> (sizeof(PatternType) - nb_remainder_bytes)*CHAR_BIT), 
                                    nb_remainder_bytes*CHAR_BIT);
            }
        }
    }
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
 * Spacepacket Dissector template
 */
template<typename SecHdrType, 
        typename ...Fields>
class SpDissector : public ISpacepacket<SecHdrType>, public Deserializable, public Serializable
{
    static_assert((true && ... && std::is_base_of<IField, Fields>::value), 
                    "Spacepacket fields must all derive from IField");
    static_assert((0 + ... + Fields::getWidth()) % CHAR_BIT == 0, 
                    "Spacepacket user data field must fit in an integral number of octet");
    static_assert(SecHdrType::getSize() > 0 || (0 + ... + Fields::getWidth()) > 0, 
                    "There shall be a User Data Field, or a Packet Secondary Header, or both (pink book, 4.1.3.2.1.2 and 4.1.3.3.2)");
public:
    SpDissector() = default;

    void fromBuffer(IBuffer& buffer) {
        IBitStream in(buffer);
        this->deserialize(in);
    }

    void toBuffer(IBuffer& buffer) {
        OBitStream out(buffer);
        this->serialize(out);
    }

    void deserialize(IBitStream& i) override {
        i >> this->primary_hdr >> this->secondary_hdr;
        std::apply([&](auto&&... args){ (void(i >> args), ...); }, field_tuple);
    }

    void serialize(OBitStream& o) const override {
        o << this->primary_hdr << this->secondary_hdr;
        std::apply([&](auto&&... args){ (void(o << args), ...); }, field_tuple);
    }

    std::size_t getUserDataWidth() override {
        return (0 + ... + Fields::getWidth());
    }

    template<std::size_t index>
    auto& getField() {
        static_assert(index < (sizeof...(Fields) + 1), "Field index out of range");
        return std::get<index>(field_tuple);
    }

    void finalize() {
        if(this->hasSecondaryHdr()) {
            this->primary_hdr.sec_hdr_flag.set();
        }

        // [...] field shall contain a length count C that equals [...] the Packet Data Field (pink book, 4.1.2.5.1.2)
        // Packet Data Field is comprised of the secondary header and the user data
        this->primary_hdr.length.setLength(SecHdrType::getSize() + this->getUserDataWidth() / CHAR_BIT);
    }

private:
    std::tuple<Fields...>   field_tuple;
};

} //namespace

#endif //CCSDS_SPACEPACKET_HPP