#ifndef OBITSTREAM_HPP
#define OBITSTREAM_HPP

#include "utils/endianness.hpp"
#include "utils/buffer.hpp"
#include <cstdint>
#include <climits>
#include <iostream>
#include <bitset>
#include <utility>

#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define ONES(n)  ((0x1 << (n)) - 1)

class OBitStream
{
public:
    OBitStream(IBuffer& buf)
    : buffer(buf), cur_bit_offset(0), bad_bit(false) {

    }

    template<typename T>
    void put(T t, std::size_t width, bool isLittleEndian = false) {

        if(bad_bit) {
            //invalid operation, can't use a bad stream
            return;
        }

        if(width > sizeof(T)*CHAR_BIT) {
            //invalid operation, can't put more bits than the amount given
            bad_bit = true;
            return;
        }
        
        if(isLittleEndian) {
            // least significant bits go first in the stream, per byte
            while(width > 0) {
                uint8_t nbBitsToAdd = MIN(width, CHAR_BIT); // take maximum 8 bits 
                uint8_t mask = ONES(nbBitsToAdd);
                this->putBits(t & mask, nbBitsToAdd);
                t >>= nbBitsToAdd;
                width -= nbBitsToAdd;
            }
        } else {
            // most significant bits go first in the stream
            while(width > 0) {
                uint8_t nbBitsToAdd = ((width - 1) % CHAR_BIT) + 1; // take minimum 1 bit 
                uint8_t mask = ONES(nbBitsToAdd);
                uint8_t byteNo = (width - 1) / CHAR_BIT;
                this->putBits((t >> (byteNo * CHAR_BIT)) & mask, nbBitsToAdd);
                width -= nbBitsToAdd;
            }
        }
    }

    std::size_t getSize() const {
        return cur_bit_offset / CHAR_BIT;
    }

    std::size_t getWidth() const {
        return cur_bit_offset;
    }

    std::size_t getMaxSize() const {
        return buffer.getSize();
    }

    bool badBit() {
        return bad_bit;
    }

    friend OBitStream& operator<<(OBitStream& out, const OBitStream& other) {

        // can't transfer an out stream into itself
        if(&out == &other) {
            out.bad_bit = true;
        } else {
            //append to this stream
            std::size_t nb_full_bytes = other.cur_bit_offset / CHAR_BIT;
            std::size_t nb_bits       = other.cur_bit_offset % CHAR_BIT;
            
            for(std::size_t i = 0; i < nb_full_bytes; i++) {
                out.putBits(*(other.buffer.getStart() + i), CHAR_BIT);
            }

            //might have remainder bits
            if(nb_bits > 0) {
                out.putBits(*(other.buffer.getStart() + nb_full_bytes), nb_bits);
            }
        }
        return out;
    }
private:
    void putBits(uint8_t byte, std::size_t width) {
        
        std::size_t byte_no = this->cur_bit_offset / CHAR_BIT;
        std::size_t bit_no  = this->cur_bit_offset % CHAR_BIT;
        uint8_t* cur_byte_ptr = this->buffer.getStart() + byte_no;
        uint8_t value = byte & ONES(width);

        // clear to all zeros (only bit positions we're about to write)
        *cur_byte_ptr &= ONES(bit_no);
        // append bits
        *cur_byte_ptr |= value << bit_no;

        // check if we need to do a second write because not all bits could be written
        if((bit_no + width) > CHAR_BIT) {
            //go to next byte
            cur_byte_ptr++;

            *cur_byte_ptr = 0;
            *cur_byte_ptr |= value >> (CHAR_BIT - bit_no);
        }

        this->cur_bit_offset += width;
    }

    //members
    IBuffer& buffer;
    std::size_t cur_bit_offset;
    bool bad_bit;
};

#endif //OBITSTREAM_HPP