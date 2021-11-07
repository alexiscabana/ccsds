/**************************************************************************//**
 * @file obitstream.hpp
 * @author Alexis Cabana-Loriaux
 * 
 * @brief Contains a class template for a stream encoder object
 * 
 ******************************************************************************/
#ifndef OBITSTREAM_HPP
#define OBITSTREAM_HPP

#include "utils/buffer.hpp"
#include "utils/bitmask.hpp"
#include <cstdint>
#include <climits>
#include <iostream>
#include <bitset>
#include <utility>

/**
 * @brief Class that provides an absrtaction layer over memory in order to facilitate 
 *        write operations that are not byte-aligned.
 * 
 * @details OBitStreams use an underlying buffer to encode bits. They hold an internal bit offset
 *          that keeps track of the state of the memory writing. Illegal writing (too large, buffer ended)
 *          can cause the stream's badbit to be raised, in which case the stream is not usable anymore and 
 *          cannot be recovered, unless the user attaches a new buffer. Illegal operations do no throw
 *          exceptions, it is the responsibility of the user to make sure that the stream functions
 *          correctly.
 */
class OBitStream
{
public:
    /**
     * @brief Construct an OutputBitStream without an underlying buffer. The stream
     *        is not usable by default (bad bit is set).
     */
    OBitStream()
    : cur_buffer(nullptr), cur_bit_offset(0), bad_bit(true) {

    }

    /**
     * @brief Construct an OutputBitStream with an underlying buffer.
     * 
     * @param buf The buffer to which this stream should encode information
     */
    OBitStream(IBuffer& buf)
    : cur_buffer(&buf), cur_bit_offset(0), bad_bit(false) {

    }

    /**
     * @brief Change the underlying buffer of this OutputBitStream. 
     * @note: This operation resets the stream to bit offset 0
     * 
     * @param buf The new buffer to which this stream should encode information
     */
    void attach(IBuffer& buf) {
        cur_buffer = &buf;
        cur_bit_offset = 0;
        bad_bit = false;
    }

    /**
     * @brief Put an amount of bits in the underlying buffer.
     * @note The value put in the buffer are the least significant bits of @p t
     * 
     * @param t The value that should be encoded.
     * @param width The amount of bits to put in the buffer.
     * @param isLittleEndian If the value should be stored in little-endian (least significant byte first).
     */
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
            uint8_t nbBitsToAdd = ((bit_index) < (width) ? (bit_index) : (width));
            uint8_t value = (t >> (width - nbBitsToAdd)) & bitmask<uint8_t>(nbBitsToAdd);

            //append relevant bits to current byte, at the right position
            uint8_t shift = bit_index - nbBitsToAdd;
            *current_byte |= (value << shift);

            current_byte++;
            width          -= nbBitsToAdd;
            cur_bit_offset += nbBitsToAdd;
        }
    }

    /**
     * @return Get the amount of "dirty" bytes that were written to the underlying buffer
     * @note The size is always rounded up if the current bit offset is not byte-aligned
     */ 
    std::size_t getSize() const {
        return cur_bit_offset / CHAR_BIT + (cur_bit_offset % CHAR_BIT > 0 ? 1 : 0);
    }

    /**
     * @return The amount of bits that were written to the underlying buffer
     */ 
    std::size_t getWidth() const {
        return cur_bit_offset;
    }

    /**
     * @return The maximum amount of bytes that can be written to the underlying buffer
     */ 
    std::size_t getMaxSize() const {
        if(cur_buffer == nullptr) {
            return 0;
        } else {
            return cur_buffer->getSize();
        }
    }

    /**
     * @return true of this stream is currently invalidated, false if the stream functions correctly
     */ 
    bool badBit() {
        return bad_bit;
    }

    /**
     * @brief Transfer all the bits (in the underlying buffer) encoded by this
     *        stream to another stream encoder.    
     * 
     * @param out   The stream to which data should be transfered
     * @param other The stream to transfer
     * 
     * @return The input parameter @p out (for sequential encoding)
     */ 
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

    /**
     * @brief Encode an arithemtic value in the buffer.
     * @note This encodes the full value
     * 
     * @param out The related OBitStream object
     * @param value The integral type to encode
     * 
     * @return The input parameter @p out (for sequential encoding)
     */ 
    template<typename T,
             std::enable_if_t<std::is_arithmetic<T>::value, bool> = true>
    friend OBitStream& operator<<(OBitStream& out, T value) {
        out.put(value, sizeof(T)*CHAR_BIT);
        return out;
    }

private:

    /** The underlying buffer this stream is currently attached to */
    IBuffer* cur_buffer;
    /** The current bit offset (where we are in the buffer) */
    std::size_t cur_bit_offset;
    /** The state bit of the stream */
    bool bad_bit;
};

#endif //OBITSTREAM_HPP