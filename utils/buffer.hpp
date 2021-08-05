#ifndef BUFFER_HPP
#define BUFFER_HPP

#include <cstdint>

class IBuffer
{
public:
    virtual const uint8_t*  getStart() const = 0;
    virtual std::size_t     getSize()  const = 0;
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

    const uint8_t* getStart() const override {
        return &bytes[0];
    }

    std::size_t getSize() const override {
        return Size;
    }

private:
    uint8_t bytes[Size];
};

/**
 * 
 */
class UserBuffer : public IBuffer
{
public:
    UserBuffer(void* buffer, std::size_t max_size)
    : max_size(max_size), buf_start(buffer) {

    }

    const uint8_t* getStart() const override {
        return static_cast<uint8_t*>(buf_start);
    }

    std::size_t getSize() const override {
        return max_size;
    }
    
private:
    std::size_t max_size;
    void*       buf_start;
};



#endif //BUFFER_HPP