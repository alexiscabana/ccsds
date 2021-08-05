#ifndef CCSDS_SECONDARY_HEADER_HPP
#define CCSDS_SECONDARY_HEADER_HPP

#include "utils/serializable.hpp"
#include "utils/datafield.hpp"
#include <tuple>
#include <cstdint>
#include <climits>
#include <type_traits>

namespace ccsds
{

class ISpSecondaryHeader : public Serializable, public Deserializable
{

};

/**
 * Secondary header template
 */
template<typename TC, typename Ancilliary>
class SpSecondaryHeader :  public ISpSecondaryHeader
{
    static_assert((std::is_base_of<IField, TC>::value && std::is_base_of<IField, Ancilliary>::value), 
                    "Secondary header fields must all be data fields");
    static_assert(TC::getWidth() % CHAR_BIT == 0, 
                    "Time Code Field must consist of an integral number of octets (pink book, section 4.1.3.2.2.1)");
    static_assert(Ancilliary::getWidth() % CHAR_BIT == 0, 
                    "Ancilliary Data Field must consist of an integral number of octets (pink book, section 4.1.3.2.3)");
public:

    SpSecondaryHeader() = default;

    SpSecondaryHeader(const TC& tc, const Ancilliary& ancillary)
    : time_code(tc), ancilliary_data(ancillary) {

    }

    void serialize(OBitStream& o) const override {
        o << time_code << ancilliary_data;
    }
    
    void deserialize(IBitStream& i) override {
        i >> time_code >> ancilliary_data;
    }

    static constexpr std::size_t getSize() {
        return (TC::getWidth() + Ancilliary::getWidth()) / CHAR_BIT;
    }

    //members
    TC time_code;
    Ancilliary ancilliary_data;
};

/**
 * Empty secondary header helper class (TC and ancillary fields are 0-sized)
 */
class SpEmptySecondaryHeader : public SpSecondaryHeader<FieldCollection<>,
                                                        FieldCollection<>>
{
    void serialize(OBitStream& o) const override {
        // nothing to serialize
        (void)o;
    }
    
    void deserialize(IBitStream& i) override {
        // nothing to deserialize
        (void)i;
    }
};

} //namespace

#endif //CCSDS_SECONDARY_HEADER_HPP