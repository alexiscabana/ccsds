#ifndef BITMASK_HPP
#define BITMASK_HPP

#include <climits>

template <typename R>
static constexpr R bitmask(unsigned int const onecount)
{
    return static_cast<R>(-(onecount != 0))
        & (static_cast<R>(-1) >> ((sizeof(R) * CHAR_BIT) - onecount));
}

#endif // BITMASK_HPP