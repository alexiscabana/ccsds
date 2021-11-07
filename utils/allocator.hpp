/**************************************************************************//**
 * @file allocator.hpp
 * @author Alexis Cabana-Loriaux
 * 
 * @brief Contains classes for dynamic memory allocation of memory sections
 * 
 ******************************************************************************/
#ifndef ALLOCATOR_HPP
#define ALLOCATOR_HPP

#include "utils/buffer.hpp"
#include <cstdint>
#include <cstdlib>

/**
 * @brief Base interface for allocator objects
 * 
 * @details Similar to the STL, allocators are used to give the user more granular control
 *          over memory used by an instance of a given class. For example, one can specify an
 *          allocator type to std::vector to retain control of dynamic memory de/allocation.
 * WARNING: To avoid memory leaks, every allocate() function should be paired with an
 *          equivalent deallocate() function.
 */
class IAllocator
{
public:
    typedef uint8_t         value_type;
    typedef uint8_t*        pointer;
    typedef const uint8_t*  const_pointer;
    typedef std::size_t     size_type;
    
    /**
     * @brief Request allocation of a contiguous amount of bytes
     * 
     * @param nb_bytes The amount of bytes to allocate
     * @return A pointer to the first byte of the memory that was allocated
     */
    virtual pointer allocate(size_type nb_bytes) const = 0;

    /**
     * @brief Request deallocation of previously allocated memory
     * 
     * @param bytes The pointer to the first byte of the previously allocated memory
     * @param nb_bytes The amount of bytes that were allocated
     */
    virtual void    deallocate(pointer bytes, size_type nb_bytes) const noexcept = 0;
    
    /**
     * @brief Utility to request allocation of a buffer (memory abstraction)
     * 
     * @param nb_bytes The amount of bytes that were allocated
     * @return The buffer object created
     */
    inline UserBuffer allocateBuffer(size_type nb_bytes) const {
        return UserBuffer(this->allocate(nb_bytes), nb_bytes);
    }

    /**
     * @brief Utility to request deallocation of a buffer
     * 
     * @param buffer The buffer object previously allocated
     */
    inline void deallocateBuffer(UserBuffer& buffer) const {
        this->deallocate(buffer.getStart(), buffer.getSize());
    }
};

/**
 * @brief Default allocator that uses malloc() and free() functions.
 */
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