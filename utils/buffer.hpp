/**************************************************************************//**
 * @file buffer.hpp
 * @author Alexis Cabana-Loriaux
 * 
 * @brief Contains utilities for creating and manipulating buffers
 *        and memory.
 * 
 ******************************************************************************/
#ifndef BUFFER_HPP
#define BUFFER_HPP

#include "utils/printable.hpp"
#include <cstdint>

/**
 * @brief Interface that abstracts a section of memory in the CCSDS environment.
 *        IBuffers provides useful abstraction of memory so the user keeps full control
 *        of the memory management, at all times.
 */ 
class IBuffer : public Printable
{
public:
    /**
     * @brief Get the start address of the buffer section
     * 
     * @return The start address
     */
    virtual uint8_t*    getStart()       = 0;

    /**
     * @brief Get the size of the buffer section
     * 
     * @return The size
     */
    virtual std::size_t getSize()  const = 0;

    void print() override {
        for(std::size_t i = 0; i < this->getSize(); i++) {
            printf("%02X ",*(this->getStart() + i));
        }
        printf("\n");
    }
};

/**
 * @brief Template class for a buffer of a given amount of bytes. 
 *        WARNING: The memory is held inside the instance.
 */
template<std::size_t Size>
class Buffer : public IBuffer
{
    static_assert(Size > 0, "Buffer must be at least one byte");
public:
    Buffer() = default;

    uint8_t* getStart() override {
        return &bytes[0];
    }

    std::size_t getSize() const override {
        return Size;
    }

private:
    /** The section of memory wrapped by this buffer instance */
    uint8_t bytes[Size] = { 0 };
};

/**
 * @brief Template class for a buffer of a given amount of bytes, where
 *        memory is held outside of this instance. UserBuffers are used to 
 *        interact with the CCSDS environment while refering to memory already
 *        managed by the user. Destroying this object does not release the memory
 *        pointed to!
 */
class UserBuffer : public IBuffer
{
public:
    /**
     * @brief Constructor
     * 
     * @param buffer the start address of the memory section to refer to
     * @param max_size the size of the memory section to refer to
     */
    UserBuffer(void* buffer, std::size_t max_size)
    : max_size(max_size), buf_start(static_cast<uint8_t*>(buffer)) {

    }

    uint8_t* getStart() override {
        return buf_start;
    }

    std::size_t getSize() const override {
        return max_size;
    }
    
private:
    /** The size of the memory section  */
    std::size_t max_size;
    /** The start address of the memory section */
    uint8_t*    buf_start;
};



#endif //BUFFER_HPP