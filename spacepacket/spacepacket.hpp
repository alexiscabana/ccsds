/**************************************************************************//**
 * @file spacepacket.hpp
 * @author Alexis Cabana-Loriaux
 * 
 * @brief Contains class for listeners of new packets in the Spacepacket layer
 * 
 ******************************************************************************/
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
 * @brief Base interface for spacepackets.
 * 
 * @tparam SecHdrType The secondary header type. Must be a type derived from ISpSecondaryHeader
 */
template<typename SecHdrType>
class ISpacepacket
{
    static_assert((std::is_base_of<ISpSecondaryHeader, SecHdrType>::value), 
                "Secondary header type must be of type ISpSecondaryHeader");
public:

    typedef SecHdrType SecondaryHdrType;

    /**
     * @brief Get the User Data Width object
     * 
     * @return the amount of bits currently occupied by the user data field
     */
    virtual std::size_t getUserDataWidth() const = 0;

    /**
     * @return true if the spacepacket has a defined, non-empty secondary header
     */
    bool hasSecondaryHdr() const {
        return SecHdrType::getSize() > 0;
    }

    /**
     * @brief Checks if the spacepacket, in its current form, is valid and can be transmitted on the network.
     * 
     * @details Many seperate things are required for a spacepacket to be valid. From the pink book :
     *          1. There shall be no secondary header for idle packets
     *          2. There shall be a User Data Field, or a Packet Secondary Header, or both (section 4.1.3.2.1.2 and 4.1.3.3.2)
     *          3. If present, the User Data Field shall consist of an integral number of octets (section 4.1.3.3.3)
     *          4. A Space Packet shall consist of at least 7 and at most 65542 octets (section 4.1.1.2)
     *          5. [...] ‘1’ if a Packet Secondary Header is present; it shall be ‘0’ if a Packet Secondary Header is not present (section 4.1.2.3.3.2)
     *          6. Secondary header is not permitted for Idle packets (section 4.1.3.2.1.4)
     *          7. Bits 32–47 of the Packet Primary Header shall contain the Packet Data Length. (section 4.1.2.5.1.1)
     * 
     * @return true if the packet is valid, false otherwise.
     */
    bool isValid() const {

        // check first of all if the primary header is valid
        if(!this->primary_hdr.isValid()) {
            return false;
        }
        
        //There shall be a User Data Field, or a Packet Secondary Header, or both 
        if(SecHdrType::getSize() == 0 && this->getUserDataWidth() == 0) {
            return false;
        }

        //If present, the User Data Field shall consist of an integral number of octets
        if(this->getUserDataWidth() % CHAR_BIT != 0) {
            return false;
        }

        //A Space Packet shall consist of at least 7 and at most 65542 octets
        if(this->getSize() < SPACEPACKET_MIN_SIZE || this->getSize() > SPACEPACKET_MAX_SIZE) {
            return false;
        }

        //[...] ‘1’ if a Packet Secondary Header is present; it shall be ‘0’ if a Packet Secondary Header is not present
        if((!hasSecondaryHdr() &&  primary_hdr.sec_hdr_flag.isSet()) || 
            (hasSecondaryHdr() && !primary_hdr.sec_hdr_flag.isSet())) {
            return false;
        }

        //Secondary header is not permitted for Idle packets
        if(primary_hdr.apid.isIdle() && SecHdrType::getSize() > 0) {
            return false;
        }

        // Bits 32–47 of the Packet Primary Header shall contain the Packet Data Length
        if(primary_hdr.length.getLength() != 
            (this->getUserDataWidth() / CHAR_BIT + SecHdrType::getSize())) {
            return false;
        }

        return true;
    }

    /**
     * @brief Get the total size of the spacepacket
     *        Primary Header + Secondary Header + User Data Field
     * 
     * @return the size in bytes occupied by the spacepacket 
     */
    std::size_t getSize() const {
        return SpPrimaryHeader::getSize() + SecHdrType::getSize() + (this->getUserDataWidth() / CHAR_BIT) + 
            (this->getUserDataWidth() % CHAR_BIT > 0 ? 1 : 0);
    }

    /** The primary header object */
    SpPrimaryHeader primary_hdr;
    /** The secondary header object */
    SecHdrType      secondary_hdr;
};

/**
 * @brief Spacepacket production and building class. This helper is used to create custom spacepackets
 *        dynamically. It covers the Packet Assembly Function (pink book, 4.2.2).
 * 
 * @details The utility of the class comes from the possibility to dynamically serialize data into the spacepacket
 *          user data field. @see{SpBuilder::data()}
 * 
 * @tparam SecHdrType The secondary header type. Must be a type derived from ISpSecondaryHeader
 * @tparam Allocator The allocator used by the object. Must be a type derived from IAllocator
 */
template<typename SecHdrType, typename Allocator = DefaultAllocator>
class SpBuilder : public ISpacepacket<SecHdrType>, public Serializable
{
    static_assert(std::is_base_of<IAllocator, Allocator>::value, "The chosen allocator is not valid");

public:
    /**
     * @brief Construct a new SpBuilder object
     * 
     * @param total_size The projected, total size of the spacepacket in bytes, including primary and secondary headers
     * @param alloc The allocator to use for dynamic memory management
     * 
     * @note Once the buffer has been allocated, no other allocation occur
     */
    SpBuilder(std::size_t total_size, const Allocator& alloc = Allocator())
    : allocator(alloc) {
        // we allocate for the total size 
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
    
    std::size_t getUserDataWidth() const override {
        return user_data.getWidth();
    }

    /**
     * @brief Access the user data field. This is used to serialize data directly in the spacepacket
     *        user data field buffer. It is possible to use Fields to acheive non-byte-aligned serialization.
     *        @see{Field}.
     * @code
     *          SpBuilder<MySecHdr> packet(256);                // 256 bytes are allocated
     *          Field<uint8_t, 6> field1(0b00101010);           // 6-bit field represented on a uint8_t
     *          uint16_t field2 = 42;
     *          
     *          // serialize the data in the packet
     *          packet.data() << field1 << field2;              // there are now 6+16=22 bytes in the packet user data field
     * @endcode
     * 
     * @return A direct reference to the user data field output bitstream 
     */
    OBitStream& data() {
        return user_data;
    }

    /**
     * @brief Finalize the current spacepacket building operation by :
     *        1. Setting the secondary header flag (in the primary header) if necessary
     *        2. Setting the length (in the primary header)
     *        3. Serializing the primary and secondary header at the beginning of the buffer
     */
    void finalize() {
        // serialize the primary and secondary header using the same complete buffer, but at
        // the beginning
        OBitStream beginning(this->getBuffer());

        if(this->hasSecondaryHdr()) {
            this->primary_hdr.sec_hdr_flag.set();
        }

        // Length is comprised of the secondary header and the user data
        this->primary_hdr.length.setLength(SecHdrType::getSize() + user_data.getSize());

        //the first few bytes were skipped to keep space to write both headers
        beginning << this->primary_hdr << this->secondary_hdr;

        //TODO check for user data field that is not byte-aligned
    }

    /**
     * @brief Get the Buffer used to serialize by this builder instance
     * 
     * @return the buffer reference
     */
    IBuffer& getBuffer() {
        return total_buffer;
    }

protected:
    /** Memory allocator */
    const Allocator& allocator;
    /** Buffer of bytes allocated for the entire spacepacket */
    UserBuffer total_buffer;
    /** Section of the total buffer used for user data */
    UserBuffer user_data_buffer;
    /** Stream to serialize data in the user data portion of the spacepacket buffer */
    OBitStream user_data;
};

/**
 * @brief Idle Spacepacket production and building class. This helper is used to create custom idle spacepackets
 *        dynamically. It covers the Packet Assembly Function (pink book, 4.2.2).
 * 
 * @tparam PatternType The idle data pattern type (uint8_t, uint16_t, etc.)
 * @tparam IdleDataPattern The idle data pattern. Every mission has a different idle data pattern.
 * @tparam Allocator The allocator used by the object. Must be a type derived from IAllocator
 */
template<typename PatternType = uint8_t, 
        PatternType IdleDataPattern = 0xFFU, 
        typename Allocator = DefaultAllocator>
class SpIdleBuilder : public SpBuilder<SpEmptySecondaryHeader, Allocator>
{
    static_assert(std::is_base_of<IAllocator, Allocator>::value, "The chosen allocator is not valid");
    static_assert(std::is_unsigned<PatternType>::value, 
                    "Only unsigned Idle packet pattern are supported.");

public:

    /**
     * @brief Construct a new SpIdleBuilder object with already filled-in idle data.
     * 
     * @param total_size The projected, total size of the spacepacket in bytes, including primary header
     * @param alloc The allocator to use for dynamic memory management
     */
    SpIdleBuilder(const std::size_t total_size, const Allocator& alloc = Allocator())
    : SpBuilder<SpEmptySecondaryHeader, Allocator>(total_size, alloc) {
        this->primary_hdr.apid.setValue(SpPrimaryHeader::PacketApid::IDLE_VALUE);

        if(total_size > SpPrimaryHeader::getSize()) {
            std::size_t packet_data_field_size = total_size - SpPrimaryHeader::getSize();

            //Fill all the packet data field bytes with the given pattern
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
 * @brief Spacepacket extraction and reading class. This helper is used to read/interpret custom spacepackets
 *        dynamically. It covers the Packet Extraction Function (pink book, 4.3.2). The extractor is guaranteed
 *        to never write to the user buffer. The primary and secondary headers are deserialized at construction
 *        and are thus valid after construction.
 * 
 * @details The utility of the class comes from the possibility to dynamically deserialize data into the spacepacket
 *          user data field. @see{SpExtractor::data()}
 * 
 * @tparam SecHdrType The secondary header type. Must be a type derived from ISpSecondaryHeader
 */
template<typename SecHdrType>
class SpExtractor : public ISpacepacket<SecHdrType>
{
public:
    /**
     * @brief Construct a new SpExtractor object
     * 
     * @param buffer The buffer binded to this extractor, that will be used to deserialize the data.
     */
    SpExtractor(const IBuffer& buffer)
    : stream(buffer), buffer(buffer) {
        //start by deserializing the two headers
        stream >> this->primary_hdr >> this->secondary_hdr;
    }

    std::size_t getUserDataWidth() const override {
        // spacepacket is alreadyformed, so the user data zone is simply described as the complete buffer minus both headers
        return (buffer.getSize() - SpPrimaryHeader::getSize() - SecHdrType::getSize()) * CHAR_BIT;
    }

    /**
     * @brief Access the user data field. This is used to deserialize data directly from the spacepacket's
     *        user data field buffer. It is possible to use Fields to acheive non-byte-aligned deserialization.
     *        @see{Field}.
     * @code
     *          SpExtractor<MySecHdr> packet(somebuffer);       // 256 bytes are allocated
     *          Field<uint8_t, 6> field1;                       // 6-bit field represented on a uint8_t
     *          uint16_t field2;
     *          
     *          packet.data() >> field1 >> field2;              // deserialize the data from the packet
     * @endcode
     * 
     * @return A direct reference to the user data field input bitstream 
     */
    IBitStream& data() {
        return stream;
    }

    /**
     * @brief Get the Buffer used to deserialize by this extractor instance
     * 
     * @return the buffer reference
     */
    const IBuffer& getBuffer() {
        return buffer;
    }

protected:
    IBitStream stream;
    const IBuffer& buffer;
};

/**
 * @brief This helper is used to dissect AND create spacepacket that have a defined format at compilation.
 *        It covers the Packet Extraction Function. The fields can be accessed individually in a tuple.
 * 
 * @details The utility of the class comes from the possibility of dissecting a complete spacepacket directly,
 *          provided the format is known at compilation.
 * @code        
 *              // dissecting a serialized packet
 *              SpDissector<MySecondaryHeader,
 *                          Field<uint32_t>,                        // Field 0
 *                          Field<uint16_t>,                        // Field 1
 *                          Field<uint8_t>,                         // Field 2
 *                          FieldArray<10,uint32_t>> packet;        //definition of the spacepacket format
 * 
 *              packet.fromBuffer(buffer);                          //dissection of the packet
 *              //...   
 *              packet.toBuffer(other_buffer);                      //serialization of the packet
 * @endcode
 * 
 * @tparam SecHdrType The secondary header type. Must be a type derived from ISpSecondaryHeader
 * @tparam Fields The list of IField types that define the format of the spacepacket's user data field
 * 
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

    /**
     * @brief Deserialize this spacepacket from a buffer
     * 
     * @param buffer The buffer
     */
    void fromBuffer(const IBuffer& buffer) {
        IBitStream in(buffer);
        this->deserialize(in);
    }

    /**
     * @brief Serialize this spacepacket to a buffer
     * 
     * @param buffer The buffer
     */
    void toBuffer(IBuffer& buffer) const {
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

    std::size_t getUserDataWidth() const override {
        return (0 + ... + Fields::getWidth());
    }

    /**
     * @brief Get the Field object
     * 
     * @tparam index The index of the field to get. 
     * @return a direct reference to the field
     */
    template<std::size_t index>
    auto& getField() {
        static_assert(index < (sizeof...(Fields) + 1), "Field index out of range");
        return std::get<index>(field_tuple);
    }

    /**
     * @brief Finalize the current spacepacket building operation 
     */
    void finalize() {
        if(this->hasSecondaryHdr()) {
            this->primary_hdr.sec_hdr_flag.set();
        }

        // [...] field shall contain a length count C that equals [...] the Packet Data Field (pink book, 4.1.2.5.1.2)
        // Packet Data Field is comprised of the secondary header and the user data
        this->primary_hdr.length.setLength(SecHdrType::getSize() + this->getUserDataWidth() / CHAR_BIT);
    }

private:
    /** Container for all of the spacepacket's fields */
    std::tuple<Fields...>   field_tuple;
};

} //namespace

#endif //CCSDS_SPACEPACKET_HPP