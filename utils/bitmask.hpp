/**************************************************************************//**
 * @file bitmask.hpp
 * @author Alexis Cabana-Loriaux
 * 
 * @brief Contains utilities for creating and manipulating bitmasks
 * 
 ******************************************************************************/
#ifndef BITMASK_HPP
#define BITMASK_HPP

#include <climits>

/**
 * @brief Create a bitmask with a given amount of ones, starting from bit 0,
 *        increasing in position
 * 
 * @param onecount The amount of ones in the bitmask
 * 
 * @return The bitmask
 */
template <typename R>
static constexpr R bitmask(const unsigned int onecount)
{
    return static_cast<R>(-(onecount != 0))
        & (static_cast<R>(-1) >> ((sizeof(R) * CHAR_BIT) - onecount));
}

#endif // BITMASK_HPP