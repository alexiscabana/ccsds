#ifndef CCSDS_PRIMARY_HEADER_HPP
#define CCSDS_PRIMARY_HEADER_HPP

#include "utils/datafield.hpp"
#include "utils/serializable.hpp"

namespace ccsds
{

class sp_primaryhdr : public serializable
{
public:
    // pink book, section 4.1.2 specifies the bit width of each primary header field
    enum field_width_bit {
        PACKET_VERSION_WIDTH = 3,
        PACKET_TYPE_WIDTH = 1,
        SECONDARY_HEADER_TYPE_WIDTH = 1,
        APID_WIDTH = 11,
        SEQUENCE_FLAGS_WIDTH = 2,
        SEQUENCE_COUNT_WIDTH = 14,
        PACKET_LENGTH_WIDTH = 16,
    };

    struct pktversion        : public field<uint8_t, PACKET_VERSION_WIDTH> {};
    struct pkt_type          : public flag {
        bool isTelemetry() const {
            // if bit is 0, packet type is telemetry (pink book, section 4.1.2.3.2.3)
            return !this->isSet();
        }

        bool isTelecommand() const {
            // if bit is 1, packet type is telecommand (pink book, section 4.1.2.3.2.3)
            return this->isSet();
        }
    };
    struct secondaryhdrflag  : public flag {
        bool isPresent() const {
            // if bit is 1, there is a secondary header present (pink book, section 4.1.2.3.3.2)
            return this->isSet();
        }
    };
    struct pktapid           : public field<uint16_t, APID_WIDTH> {
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
    struct seqflags          : public field<uint8_t, SEQUENCE_FLAGS_WIDTH> {
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
    struct seqcount          : public field<uint16_t, SEQUENCE_COUNT_WIDTH> {};
    struct pktlength         : public field<uint16_t> {
        uint16_t getLength() const {
            // the field contains a length count that equals one fewer than the length (in octets) (pink book, section 4.1.2.5.1.2)
            return this->getValue() + 1;
        }

        void setLength(uint16_t length) {
            // the field contains a length count that equals one fewer than the length (in octets) (pink book, section 4.1.2.5.1.2)
            this->setValue(length - 1);
        }
    };
    
    sp_primaryhdr() = default;
    void serialize(obitstream& o) const override {
        o << version_field << type_field << sechdrflag_field << apid_field 
            << sequenceflags_field << sequencecount_field << length_field;
    }
    
    void deserialize(ibitstream& i) override {
        i >> version_field >> type_field >> sechdrflag_field >> apid_field 
            >> sequenceflags_field >> sequencecount_field >> length_field;
    }

    //members
    pktversion          version_field;
    pkt_type            type_field;
    secondaryhdrflag    sechdrflag_field;
    pktapid             apid_field;
    seqflags            sequenceflags_field;
    seqcount            sequencecount_field;
    pktlength           length_field;
};

} //namespace

#endif //CCSDS_PRIMARY_HEADER_HPP