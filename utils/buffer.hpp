#ifndef BUFFER_HPP
#define BUFFER_HPP

#include "utils/printable.hpp"
#include <cstdint>

class IBuffer : public Printable
{
public:
    virtual uint8_t*    getStart()       = 0;
    virtual std::size_t getSize()  const = 0;

    void print() override {
        for(std::size_t i = 0; i < this->getSize(); i++) {
            printf("%02X ",*(this->getStart() + i));
        }
        printf("\n");
    }
};

/**
 * 
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
    uint8_t bytes[Size] = { 0 };
};

/**
 * 
 */
class UserBuffer : public IBuffer
{
public:
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
    std::size_t max_size;
    uint8_t*    buf_start;
};



#endif //BUFFER_HPP