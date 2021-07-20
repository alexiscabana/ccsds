#ifndef IBITSTREAM_HPP
#define IBITSTREAM_HPP

#include <cstdint>
#include <climits>
#include <iostream>

#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define ONES(n)  ((0x1 << (n)) - 1)

class ibitstream
{
public:
    template<typename T>
    void get(T& t, std::size_t width, bool isLittleEndian = false) {

        if(width > sizeof(T)*CHAR_BIT) {
            return; // TODO: maybe set badbit?
        } else if(width == 0) {
            return;
        }

        t = 0; // init value

        if(isLittleEndian) {
            // least significant bits come first in the stream (in byte form)
            uint8_t currentByte, byteNo = 0 ;
            do {
                uint8_t nbBitsToGet = MIN(width, CHAR_BIT);  // take max 8 bits 
                this->getBits(currentByte, nbBitsToGet);
                t |= (static_cast<uint64_t>(currentByte) << (byteNo * CHAR_BIT));
                width -= nbBitsToGet;
                byteNo++;
            } while(width > 0);
        } else {
            // most significant bits come first in the stream
            uint8_t currentByte;
            do {
                t <<= CHAR_BIT;
                uint8_t nbBitsToGet = ((width - 1) % CHAR_BIT) + 1; // take minimum 1 bit 
                this->getBits(currentByte, nbBitsToGet);
                t |= currentByte;
                width -= nbBitsToGet;
            } while(width > 0);
        }
    }
private:
    void getBits(uint8_t& byte, std::size_t width) {
        if(width > 0) {
            std::cout << "getting " << width << " bits" << std::endl;
        }
    }
};

#endif //IBITSTREAM_HPP