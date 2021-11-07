/**************************************************************************//**
 * @file ibitstream.hpp
 * @author Alexis Cabana-Loriaux
 * 
 * @brief Contains a class template for a stream decoder object
 * 
 ******************************************************************************/
#ifndef IBITSTREAM_HPP
#define IBITSTREAM_HPP

#include "utils/bitmask.hpp"
#include "utils/buffer.hpp"
#include <cstdint>
#include <climits>
#include <iostream>

/**
 * @brief Class that provides an absrtaction layer over memory in order to facilitate 
 *        read operations that are not byte-aligned.
 * 
 * @details IBitStreams use an underlying buffer to interpret bits. They hold an internal bit offset
 *          that keeps track of the state of the memory reading. Illegal readings (too large, buffer ended)
 *          can cause the stream's badbit to be raised, in which case the stream is not usable anymore and 
 *          cannot be recovered, unless the user attaches a new buffer. Illegal operations do no throw
 *          exceptions, it is the responsibility of the user to make sure that the stream functions
 *          correctly.
 */
class IBitStream
{
public:
    /**
     * @brief Construct an InputBitStream without an underlying buffer. The stream
     *        is not usable by default (bad bit is set).
     */
    IBitStream()
    : cur_buffer(nullptr), cur_bit_offset(0), bad_bit(true) {

    }

    /**
     * @brief Construct an InputBitStream with an underlying buffer.
     * 
     * @param buf The buffer from which this stream should decode information
     */
    IBitStream(const IBuffer& buf)
    : cur_buffer(&buf), cur_bit_offset(0), bad_bit(false) {

    }

    /**
     * @brief Change the underlying buffer of this InputBitStream. 
     * @note: This operation resets the stream to bit offset 0
     * 
     * @param buf The new buffer from which this stream should decode information
     */
    void attach(const IBuffer& buf) {
        cur_buffer = &buf; 
        cur_bit_offset = 0;
        bad_bit = false;
    }

    /**
     * @brief Get an amount of bits from the underlying buffer.
     * @note The value requested is stored in the least significant bits of @p t
     * 
     * @param t The integral type in which the value should be stored.
     * @param width The amount of bits to fetch from the buffer.
     * @param isLittleEndian If the value was stored in little-endian (if the least significant byte was put first).
     */
    template<typename T>
    void get(T& t, std::size_t width, bool isLittleEndian = false) {

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
        
        const IBuffer& buffer = *cur_buffer;
        std::size_t current_byte_i = this->cur_bit_offset / CHAR_BIT;
        uint8_t* current_byte = buffer.getStart() + current_byte_i;

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
            uint8_t nbBitsToGet = ((width) < (bit_index) ? (width) : (bit_index));  // take max 8 bits
            uint8_t value = (*current_byte >> (bit_index - nbBitsToGet)) & bitmask<uint8_t>(nbBitsToGet);

            // append bits
            t <<= nbBitsToGet; 
            t |= value;

            current_byte++;
            width          -= nbBitsToGet;
            cur_bit_offset += nbBitsToGet;
        }
    }

    /**
     * @return Get the amount of "dirty" bytes that were read from the underlying buffer
     * @note The size is always rounded up if the current bit offset is not byte-aligned
     */ 
    std::size_t getSize() const {
        return cur_bit_offset / CHAR_BIT + (cur_bit_offset % CHAR_BIT > 0 ? 1 : 0);
    }

    /**
     * @return The amount of bits that were read from the underlying buffer
     */ 
    std::size_t getWidth() const {
        return cur_bit_offset;
    }

    /**
     * @return The maximum amount of bytes that can be read from the underlying buffer
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
     * @brief Decode an arithemtic value from the buffer.
     * @note This decodes the full value 
     * 
     * @param in The related IBitStream object
     * @param value The integral type in which the value should be stored
     * 
     * @return The input parameter @p in (for sequential decoding)
     */ 
    template<typename T,
             std::enable_if_t<std::is_arithmetic<T>::value, bool> = true>
    friend IBitStream& operator>>(IBitStream& in, T& value) {
        in.get(value, sizeof(T)*CHAR_BIT);
        return in;
    }

private:

    /** The underlying buffer this stream is currently attached to */
    const IBuffer* cur_buffer;
    /** The current bit offset (where we are in the buffer) */
    std::size_t cur_bit_offset;
    /** The state bit of the stream */
    bool bad_bit;
};

#endif //IBITSTREAM_HPP