#ifndef IBITSTREAM_HPP
#define IBITSTREAM_HPP

#include "utils/bitmask.hpp"
#include "utils/buffer.hpp"
#include <cstdint>
#include <climits>
#include <iostream>

#define MIN(a,b) ((a) < (b) ? (a) : (b))

class IBitStream
{
public:
    IBitStream(const IBuffer& buf)
    : buffer(buf), cur_bit_offset(0), bad_bit(false) {

    }

    template<typename T>
    void get(T& t, std::size_t width, bool isLittleEndian = false) {

        (void)isLittleEndian;
        std::size_t current_byte_i = this->cur_bit_offset / CHAR_BIT;
        uint8_t* current_byte = buffer.getStart() + current_byte_i;
        if(bad_bit) {
            //invalid operation, can't use a bad stream
            return;
        }

        if(width == 0) {
            return;
        }

        if(width > sizeof(T)*CHAR_BIT ||
           width > buffer.getSize()*CHAR_BIT - cur_bit_offset) {
            //invalid operation, can't get more bits than the amount given
            bad_bit = true;
            return;
        }

        //init value
        t = 0;
        
        while(width > 0) {
            // index in the current byte we should be reading from
            uint8_t bit_index = CHAR_BIT - (cur_bit_offset % CHAR_BIT);

            // get relevant bits of current byte
            uint8_t nbBitsToGet = MIN(width, bit_index);  // take max 8 bits
            uint8_t value = (*current_byte >> (bit_index - nbBitsToGet)) & bitmask<uint8_t>(nbBitsToGet);

            // append bits
            t <<= nbBitsToGet; 
            t |= value;

            current_byte++;
            width          -= nbBitsToGet;
            cur_bit_offset += nbBitsToGet;
        }
    }

    std::size_t getSize() const {
        return cur_bit_offset / CHAR_BIT + (cur_bit_offset % CHAR_BIT > 0 ? 1 : 0);
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


    template<typename T,
             std::enable_if_t<std::is_arithmetic<T>::value, bool> = true>
    friend IBitStream& operator>>(IBitStream& in, T& value) {
        in.get(value, sizeof(T)*CHAR_BIT);
        return in;
    }

private:

    //members
    const IBuffer& buffer;
    std::size_t cur_bit_offset;
    bool bad_bit;
};

#endif //IBITSTREAM_HPP