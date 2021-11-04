/**************************************************************************//**
 * @file allocator.hpp
 * @author Alexis Cabana-Loriaux
 * 
 * @brief Contains classes for dynamic memory allocation
 * 
 ******************************************************************************/
#ifndef ALLOCATOR_HPP
#define ALLOCATOR_HPP

#include "utils/buffer.hpp"
#include <cstdint>
#include <cstdlib>

class IAllocator
{
public:
    typedef uint8_t         value_type;
    typedef uint8_t*        pointer;
    typedef const uint8_t*  const_pointer;
    typedef std::size_t     size_type;
    
    virtual pointer allocate(size_type nb_bytes) const = 0;
    virtual void    deallocate(pointer bytes, size_type nb_bytes) const noexcept = 0;
    
    inline UserBuffer allocateBuffer(size_type nb_bytes) const {
        return UserBuffer(this->allocate(nb_bytes), nb_bytes);
    }

    inline void deallocateBuffer(UserBuffer& buffer) const {
        this->deallocate(buffer.getStart(), buffer.getSize());
    }
};

class DefaultAllocator : public IAllocator
{
    pointer allocate(size_type nb_bytes) const override {
        pointer ret = static_cast<pointer>(std::malloc(nb_bytes));
        return ret;
    }

    void deallocate(pointer bytes, size_type nb_bytes) const noexcept override {
        (void)nb_bytes;
        std::free(bytes);
    }
};

#endif // ALLOCATOR_HPP