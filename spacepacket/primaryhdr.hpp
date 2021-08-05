#ifndef CCSDS_PRIMARY_HEADER_HPP
#define CCSDS_PRIMARY_HEADER_HPP

#include "utils/datafield.hpp"
#include "utils/serializable.hpp"

namespace ccsds
{

class SpPrimaryHeader : public Serializable, public Deserializable
{
public:
    // pink book, section 4.1.2 specifies the bit width of each primary header field
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
        SIZE = 6
    };
    
    struct PacketVersion        : public Field<uint8_t, PACKET_VERSION_WIDTH> {};
    struct PacketType           : public Flag {
        bool isTelemetry() const {
            // if bit is 0, packet type is telemetry (pink book, section 4.1.2.3.2.3)
            return !this->isSet();
        }

        bool isTelecommand() const {
            // if bit is 1, packet type is telecommand (pink book, section 4.1.2.3.2.3)
            return this->isSet();
        }
    };
    struct SecondaryHdrFlag     : public Flag {
        bool isPresent() const {
            // if bit is 1, there is a secondary header present (pink book, section 4.1.2.3.3.2)
            return this->isSet();
        }
    };
    struct PacketApid           : public Field<uint16_t, APID_WIDTH> {
        enum {
            IDLE_VALUE = 0b11111111111,
        };

        bool isIdle() const {
            // for idle packet, the APID shall be all ones (pink book, section 4.1.2.3.4.4)
            return this->getValue() == IDLE_VALUE;
        }

        void setIdle() {
            // for idle packet, the APID shall be all ones (pink book, section 4.1.2.3.4.4)
            this->setValue(IDLE_VALUE);
        }
    };
    struct SequenceFlags        : public Field<uint8_t, SEQUENCE_FLAGS_WIDTH> {
        enum {
            CONTINUATION_VALUE = 0b00,
            FIRST_SEGMENT_VALUE = 0b01,
            LAST_SEGMENT_VALUE = 0b10,
            UNSEGMENTED_VALUE = 0b11,
        }; 

        bool isContinuationSegment() const {
            // sequence flag is '00' if the Space Packet contains a continuation segment of User Data; (pink book, section 4.1.2.4.2.2a)
            return this->getValue() == CONTINUATION_VALUE;
        }

        bool isFirstSegment() const {
            // sequence flag is '01' if the Space Packet contains the first segment of User Data; (pink book, section 4.1.2.4.2.2b)
            return this->getValue() == FIRST_SEGMENT_VALUE;
        }

        bool isLastSegment() const {
            // sequence flag is '10' if the Space Packet contains the last segment of User Data; (pink book, section 4.1.2.4.2.2c)
            return this->getValue() == LAST_SEGMENT_VALUE;
        }

        bool isUnsegmented() const {
            // sequence flag is '11' if the Space Packet contains unsegmented User Data; (pink book, section 4.1.2.4.2.2d)
            return this->getValue() == UNSEGMENTED_VALUE;
        }
    };
    
    struct SequenceCount        : public Field<uint16_t, SEQUENCE_COUNT_WIDTH> {};
    struct PacketLength         : public Field<uint16_t> {
        uint16_t getLength() const {
            // the field contains a length count that equals one fewer than the length (in octets) (pink book, section 4.1.2.5.1.2)
            return this->getValue() + 1;
        }

        void setLength(uint16_t length) {
            // the field contains a length count that equals one fewer than the length (in octets) (pink book, section 4.1.2.5.1.2)
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

    static constexpr std::size_t getSize() {
        return SIZE;
    }

    //members
    PacketVersion       version;
    PacketType          type;
    SecondaryHdrFlag    sec_hdr_flag;
    PacketApid          apid;
    SequenceFlags       sequence_flags;
    SequenceCount       sequence_count;
    PacketLength        length;
};

} //namespace

#endif //CCSDS_PRIMARY_HEADER_HPP