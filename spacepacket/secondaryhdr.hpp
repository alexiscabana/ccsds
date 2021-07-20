#ifndef CCSDS_SECONDARY_HEADER_HPP
#define CCSDS_SECONDARY_HEADER_HPP

#include "utils/serializable.hpp"
#include <tuple>
#include <cstdint>
#include <climits>
#include <type_traits>

namespace ccsds
{

/**
 * Base case of secondary header template
 */
template <typename... T>
class sp_secondaryhdr : public serializable
{
public:
    sp_secondaryhdr() = default;

    void serialize(obitstream& o) const override {
        //nothing to serialize, empty header
    } 

    void deserialize(ibitstream& i) override {
        //nothing to deserialize, empty header
    }

    static constexpr std::size_t getNbFields() {
        return 0;
    }

    static constexpr std::size_t getSizeBits() {
        return 0;
    }
};

/**
 * Partial specialization of secondary header template with >1 data field
 */
template<typename T, typename... Rest>
class sp_secondaryhdr<T, Rest...> : public serializable
{
    static_assert((std::is_base_of<ifield, T>::value && ... && std::is_base_of<ifield, Rest>::value), 
                    "Secondary header fields must all be data fields");
    static_assert((T::getWidth() + ... + Rest::getWidth()) % CHAR_BIT == 0, 
                    "Spacepacket secondary headers must consist of an integral number of octets (pink book, section 4.1.3.2.1.3)");

public:
    sp_secondaryhdr() = default;
    sp_secondaryhdr(const T& first, const Rest&... rest)
    : fields(first, rest...) {

    }

    void serialize(obitstream& o) const override {
        std::apply([&](auto&&... args){ (void(o << args), ...); }, fields);
    }
    
    void deserialize(ibitstream& i) override {
        std::apply([&](auto&&... args){ (void(i >> args), ...); }, fields);
    }

    template<std::size_t index>
    auto& getField() {
        static_assert(index < (sizeof...(Rest) + 1), "Field index out of range");
        return std::get<index>(fields);
    }

    static constexpr std::size_t getNbFields() {
        return sizeof...(Rest) + 1;
    }

    static constexpr std::size_t getSizeBits() {
        return (T::getWidth() + ... + Rest::getWidth());
    }

private:
    //members
    std::tuple<T, Rest...> fields;
};

} //namespace

#endif //CCSDS_SECONDARY_HEADER_HPP