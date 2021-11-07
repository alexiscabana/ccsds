#ifndef OBITSTREAM_HPP
#define OBITSTREAM_HPP

#include "utils/buffer.hpp"
#include "utils/bitmask.hpp"
#include <cstdint>
#include <climits>
#include <iostream>
#include <bitset>
#include <utility>

#define MIN(a,b) ((a) < (b) ? (a) : (b))

class OBitStream
{
public:
    OBitStream()
    : cur_buffer(nullptr), cur_bit_offset(0), bad_bit(true) {

    }

    OBitStream(IBuffer& buf)
    : cur_buffer(&buf), cur_bit_offset(0), bad_bit(false) {

    }

    void attach(IBuffer& buf) {
        cur_buffer = &buf;
        cur_bit_offset = 0;
        bad_bit = false;
    }

    template<typename T>
    void put(T t, std::size_t width, bool isLittleEndian = false) {

        (void)isLittleEndian;
        if(bad_bit) {
            //invalid operation, can't use a bad stream
            return;
        }

        if(width == 0) {
            return;
        }

        if(cur_buffer == nullptr) {
            bad_bit = true;
            return;
        }

        IBuffer& buffer = *cur_buffer;
        std::size_t current_byte_i = this->cur_bit_offset / CHAR_BIT;
        uint8_t* current_byte = buffer.getStart() + current_byte_i;

        if(width > sizeof(T)*CHAR_BIT || 
           width > buffer.getSize()*CHAR_BIT - cur_bit_offset) {
            //invalid operation, can't put any more bits
            bad_bit = true;
            return;
        }

        while(width > 0) {
            // index in the current byte we should be writing from
            uint8_t bit_index = CHAR_BIT - (cur_bit_offset % CHAR_BIT);

            //clear current byte before write bits to it
            if(cur_bit_offset % CHAR_BIT == 0) {
                *current_byte = 0x00;
            }
            
            //mask relevant bits from value to put
            uint8_t nbBitsToAdd = MIN(bit_index, width);
            uint8_t value = (t >> (width - nbBitsToAdd)) & bitmask<uint8_t>(nbBitsToAdd);

            //append relevant bits to current byte, at the right position
            uint8_t shift = bit_index - nbBitsToAdd;
            *current_byte |= (value << shift);

            current_byte++;
            width          -= nbBitsToAdd;
            cur_bit_offset += nbBitsToAdd;
        }
    }

    std::size_t getSize() const {
        return cur_bit_offset / CHAR_BIT + (cur_bit_offset % CHAR_BIT > 0 ? 1 : 0);
    }

    std::size_t getWidth() const {
        return cur_bit_offset;
    }

    std::size_t getMaxSize() const {
        if(cur_buffer == nullptr) {
            return 0;
        } else {
            return cur_buffer->getSize();
        }
    }

    bool badBit() {
        return bad_bit;
    }

    friend OBitStream& operator<<(OBitStream& out, const OBitStream& other) {

        // can't transfer an out stream into itself
        if(&out == &other || 
            out.cur_buffer == nullptr ||
            other.cur_buffer == nullptr) {
            out.bad_bit = true;
        } else {
            //append to this stream
            std::size_t nb_full_bytes = other.cur_bit_offset / CHAR_BIT;
            std::size_t nb_bits       = other.cur_bit_offset % CHAR_BIT;
            
            for(std::size_t i = 0; i < nb_full_bytes; i++) {
                out.put(*(other.cur_buffer->getStart() + i), CHAR_BIT);
            }

            //might have remainder bits
            if(nb_bits > 0) {
                out.put(*(other.cur_buffer->getStart() + nb_full_bytes), nb_bits);
            }
        }
        return out;
    }

    template<typename T,
             std::enable_if_t<std::is_arithmetic<T>::value, bool> = true>
    friend OBitStream& operator<<(OBitStream& out, T value) {
        out.put(value, sizeof(T)*CHAR_BIT);
        return out;
    }

private:

    //members
    IBuffer* cur_buffer;
    std::size_t cur_bit_offset;
    bool bad_bit;
};

#endif //OBITSTREAM_HPP