/**************************************************************************//**
 * @file endianness.hpp
 * @author Alexis Cabana-Loriaux
 * 
 * @brief Contains utilities for determining and changing endianness
 * 
 ******************************************************************************/
#ifndef ENDIANNESS_HPP
#define ENDIANNESS_HPP

#include <type_traits>
#include <utility>
#include <cstdint>
#include <climits>

// endianness check can't be done legally in c++17 (i.e not using undefined behavior like type punning) without using macros
#if defined(__BYTE_ORDER) && __BYTE_ORDER == __BIG_ENDIAN || \
    defined(__BIG_ENDIAN__) || \
    defined(__ARMEB__) || \
    defined(__THUMBEB__) || \
    defined(__AARCH64EB__) || \
    defined(_MIBSEB) || defined(__MIBSEB) || defined(__MIBSEB__) || \
    defined(_M_PPC)
    // It's a big-endian target architecture
    #define ENDIANNESS_BIG

#elif defined(__BYTE_ORDER) && __BYTE_ORDER == __LITTLE_ENDIAN || \
    defined(__LITTLE_ENDIAN__) || \
    defined(__ARMEL__) || \
    defined(__THUMBEL__) || \
    defined(__AARCH64EL__) || \
    defined(_MIPSEL) || defined(__MIPSEL) || defined(__MIPSEL__) || \
    defined(_M_IX86) || defined(_M_X64) || defined(_M_IA64) || defined(_M_ARM) 
    // It's a little-endian target architecture
    #define ENDIANNESS_LITTLE

#else
#error "I don't know what architecture this is!"
#endif

/**
 * @returns true if the system for which this program is compiled, is in little-endian
 */
constexpr inline bool isSystemLE() {
#if defined(ENDIANNESS_BIG)
    return false;
#elif defined(ENDIANNESS_LITTLE)
    return true;
#endif
}

namespace {
    template<typename T,
            std::size_t... N>
    constexpr inline T swapEndian_impl(T t, std::index_sequence<N...>) {
        return (((t >> N*CHAR_BIT & std::uint8_t(-1)) << (sizeof(T)-1-N)*CHAR_BIT) | ...);
    }
}

/**
 * @brief A compile-time endianness swap utility.
 * 
 * @returns The value when its endianness is swapped
 * @note Taken from https://stackoverflow.com/questions/36936584/how-to-write-constexpr-swap-function-to-change-endianess-of-an-integer
 */
template<typename T,
        typename U = std::make_unsigned_t<T>,
        std::enable_if_t<std::is_integral<T>::value, bool> = true>
constexpr inline T swapEndian(T t) {
    return swapEndian_impl<U>(t, std::make_index_sequence<sizeof(T)>{});
}


#endif // ENDIANNESS_HPP