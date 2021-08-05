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
    : buffer(buf), cur_bit_offset(0) {

    }

    template<typename T>
    void put(T t, std::size_t width, bool isLittleEndian = false) {

        if(width > sizeof(T)*CHAR_BIT) {
            return; // TODO: maybe set badbit?
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

    std::size_t getSize() {
        return cur_bit_offset / CHAR_BIT;
    }

    std::size_t getWidth() {
        return cur_bit_offset;
    }

    std::size_t getMaxSize() {
        return buffer.getSize();
    }

    friend OBitStream& operator<<(OBitStream& out, const OBitStream& other) {
        // can't transfer an out stream into itself
        if(&out == &other) {
            //TODO: set badbit?
        } else {
            //TODO: append the other stream
        }
        return out;
    }
private:
    void putBits(uint8_t byte, std::size_t width) {
        if(width > 0) {
            std::bitset<sizeof(uint8_t) * CHAR_BIT> bits(byte);
            std::cout << "putting " << width << " bits: " << bits << std::endl;

        }
    }

    //members
    IBuffer& buffer;
    std::size_t cur_bit_offset;
};

#endif //OBITSTREAM_HPP