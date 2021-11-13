/**************************************************************************//**
 * @file primaryhdr.hpp
 * @author Alexis Cabana-Loriaux
 * 
 * @brief Contains class for handling of space packet primary headers and their
 *        fields.
 * 
 ******************************************************************************/
#ifndef CCSDS_PRIMARY_HEADER_HPP
#define CCSDS_PRIMARY_HEADER_HPP

#include "utils/datafield.hpp"
#include "utils/serializable.hpp"
#include "utils/printable.hpp"

namespace ccsds
{

/**
 * @brief Represents a spacepacket primary header.
 */
class SpPrimaryHeader : public Serializable, public Deserializable, public Printable
{
public:
    /**
     * @brief Width of each field in the primary header (in bits).
     * @note see pink book, section 4.1.2
     */
    enum BitWidthField {
        PACKET_VERSION_WIDTH = 3,
        PACKET_TYPE_WIDTH = 1,
        SECONDARY_HEADER_TYPE_WIDTH = 1,
        APID_WIDTH = 11,
        SEQUENCE_FLAGS_WIDTH = 2,
        SEQUENCE_COUNT_WIDTH = 14,
        PACKET_LENGTH_WIDTH = 16,
    };

    enum {
        /** Size (in bytes) occupied by an encoded primary header in a buffer */
        SIZE = 6
    };
    
    /**
     * @brief Represents the Packet version field.
     */
    struct PacketVersion        : public Field<uint8_t, PACKET_VERSION_WIDTH> {};
    
    /**
     * @brief Represents the Packet type field.
     */
    struct PacketType           : public Flag {
        /**
         * @return true if the field is 0 (telemetry), false otherwise
         * @note see pink book, section 4.1.2.3.2.3
         */
        bool isTelemetry() const {
            // if bit is 0, packet type is telemetry
            return !this->isSet();
        }

        /**
         * @return true if the field is 1 (telecommand), false otherwise
         * @note see pink book, section 4.1.2.3.2.3
         */
        bool isTelecommand() const {
            // if bit is 1, packet type is telecommand
            return this->isSet();
        }

        /**
         * @brief Set the packet type to 0 (telemetry)
         * @note see pink book, section 4.1.2.3.2.3
         */
        void setTelemetry() {
            // if bit is 0, packet type is telemetry
            this->reset();
        }

        /**
         * @brief Set the packet type to 1 (telecommand)
         * @note see pink book, section 4.1.2.3.2.3
         */
        void setTelecommand() {
            // if bit is 1, packet type is telecommand
            this->set();
        }
    };
    /**
     * @brief Represents the packet secondary header flag
     */
    struct SecondaryHdrFlag     : public Flag {
        /**
         * @return true if the flag is set, false otherwise
         * @note see pink book, section 4.1.2.3.3.2
         */
        bool isPresent() const {
            // if bit is 1, there is a secondary header present
            return this->isSet();
        }
    };
    /**
     * @brief Represents the packet APID
     */
    struct PacketApid           : public Field<uint16_t, APID_WIDTH> {
        enum {
            IDLE_VALUE = 0b11111111111,
        };

        /**
         * 
         */
        PacketApid() = default;
        PacketApid(uint16_t apid) : Field(apid) {}

        bool isIdle() const {
            // for idle packet, the APID shall be all ones (pink book, section 4.1.2.3.4.4)
            return this->getValue() == IDLE_VALUE;
        }

        void setIdle() {
            // for idle packet, the APID shall be all ones (pink book, section 4.1.2.3.4.4)
            this->setValue(IDLE_VALUE);
        }
    };
    /**
     * @brief Represents the Packet sequence flags field.
     */
    struct SequenceFlags        : public Field<uint8_t, SEQUENCE_FLAGS_WIDTH> {
        /**
         * @brief Possible values of the primary header sequence flags
         */
        enum {
            CONTINUATION_VALUE = 0b00,
            FIRST_SEGMENT_VALUE = 0b01,
            LAST_SEGMENT_VALUE = 0b10,
            UNSEGMENTED_VALUE = 0b11,
        }; 

        /**
         * @return true if this packet is tagged as a continuation segment, false otherwise
         * @note see pink book, section 4.1.2.4.2.2a
         */
        bool isContinuationSegment() const {
            // sequence flag is '00' if the Space Packet contains a continuation segment of User Data
            return this->getValue() == CONTINUATION_VALUE;
        }

        /**
         * @return true if this packet is tagged as a first segment, false otherwise
         * @note see pink book, section 4.1.2.4.2.2b
         */
        bool isFirstSegment() const {
            // sequence flag is '01' if the Space Packet contains the first segment of User Data
            return this->getValue() == FIRST_SEGMENT_VALUE;
        }

        /**
         * @return true if this packet is tagged as a last segment, false otherwise
         * @note see pink book, section 4.1.2.4.2.2c
         */
        bool isLastSegment() const {
            // sequence flag is '10' if the Space Packet contains the last segment of User Data
            return this->getValue() == LAST_SEGMENT_VALUE;
        }

        /**
         * @return true if this packet is tagged as unsegmented, false otherwise
         * @note see pink book, section 4.1.2.4.2.2d
         */
        bool isUnsegmented() const {
            // sequence flag is '11' if the Space Packet contains unsegmented User Data
            return this->getValue() == UNSEGMENTED_VALUE;
        }

        const char* getName() const {
            switch(this->getValue()) {
                case CONTINUATION_VALUE:  return "Continuation Segment";
                case FIRST_SEGMENT_VALUE: return "First Segment";
                case LAST_SEGMENT_VALUE:  return "Last Segment";
                case UNSEGMENTED_VALUE:   return "Unsegmented";
                default :                 return "Unkown"; // should never happen, all possibilities are covered
            }
        }
    };
    
    /**
     * @brief Represents the sequence count of the packet. For a given APID (except idle), the sequence count must always be
     *        incremented by 1 when a new packet is produced
     */
    struct SequenceCount        : public Field<uint16_t, SEQUENCE_COUNT_WIDTH> {};
    
    /**
     * @brief Represents the length of the packet data field
     */
    struct PacketLength         : public Field<uint16_t> {
        /**
         * @brief Get the Length attribute of the primary header
         * 
         * @return the length of the packet data field (amount of bytes after the primary header)
         * @note see pink book, section 4.1.2.5.1.2
         */
        uint16_t getLength() const {
            // the field contains a length count that equals one fewer than the length (in octets)
            return this->getValue() + 1;
        }

        /**
         * @brief Set the Length attribute of the primary header
         * 
         * @param length amount of bytes after the primary header
         * @note see pink book, section 4.1.2.5.1.2
         */
        void setLength(uint16_t length) {
            // the field contains a length count that equals one fewer than the length (in octets)
            this->setValue(length - 1);
        }
    };
    
    SpPrimaryHeader() = default;
    
    void serialize(OBitStream& o) const override {
        o << version << type << sec_hdr_flag << apid 
            << sequence_flags << sequence_count << length;
    }
    
    void deserialize(IBitStream& i) override {
        i >> version >> type >> sec_hdr_flag >> apid 
            >> sequence_flags >> sequence_count >> length;
    }

    void print() const override {
        printf("Version     : %u\n", this->version.getValue());
        printf("Type        : %s\n", (this->type.isTelecommand() ? "Telecommand" : "Telemetry"));
        printf("Sec. Header : %s\n", (this->sec_hdr_flag.isSet() ? "Yes" : "No"));
        printf("APID        : ");
        if(this->apid.isIdle()) {
            printf("Idle ");
        } else {
            printf("%u ", this->apid.getValue());
        }
        printf("(hex : %02X)\n", this->apid.getValue());
        printf("Seq. Flags  : %s\n", this->sequence_flags.getName());
        printf("Seq. Count  : %u\n", this->sequence_count.getValue());
        printf("Length      : %u\n", this->length.getLength());
    }

    static constexpr std::size_t getSize() {
        return SIZE;
    }

    /**
     * @brief Check if, only from the primary header, it is possible to tag
     *        the packet as invalid 
     * 
     * @return true if the packet is valid, false otherwise
     * @note see pink book, 4.1.2.3.3.4
     */
    bool isValid() const {
        // The Secondary Header Flag shall be set to ‘0’ for Idle Packets.
        if(apid.isIdle() && sec_hdr_flag.isSet()) {
            return false;
        }

        return true;
    }

    /** The version of the spacepacket */
    PacketVersion       version;
    /** The type of the spacepacket */
    PacketType          type;
    /** The secondary header flag present bit of the spacepacket */
    SecondaryHdrFlag    sec_hdr_flag;
    /** The APID (producer) of the spacepacket */
    PacketApid          apid;
    /** The sequence flag of the spacepacket */
    SequenceFlags       sequence_flags;
    /** The sequence count of the spacepacket */
    SequenceCount       sequence_count;
    /** The length (in bytes) of the spacepacket */
    PacketLength        length;
};

} //namespace

#endif //CCSDS_PRIMARY_HEADER_HPP