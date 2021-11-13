/**************************************************************************//**
 * @file secondaryhdr.hpp
 * @author Alexis Cabana-Loriaux
 * 
 * @brief Contains class for handling of space packet secondary headers and 
 *        their fields.
 * 
 ******************************************************************************/
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
/**
 * @brief Base class for spacepackets secondary headers
 * 
 */
class ISpSecondaryHeader : public Serializable, public Deserializable
{

};

/**
 * @brief Represents a spacepacket primary header
 * 
 * @tparam TC The timecode field. Must be a type derived from IField.
 * @tparam Ancillary The ancillary data field. Must be a type derived from IField.
 */
template<typename TC, typename Ancillary>
class SpSecondaryHeader :  public ISpSecondaryHeader
{
    static_assert((std::is_base_of<IField, TC>::value && std::is_base_of<IField, Ancillary>::value), 
                    "Secondary header fields must all be data fields");
    static_assert(TC::getWidth() % CHAR_BIT == 0, 
                    "Time Code Field must consist of an integral number of octets (pink book, section 4.1.3.2.2.1)");
    static_assert(Ancillary::getWidth() % CHAR_BIT == 0, 
                    "Ancillary Data Field must consist of an integral number of octets (pink book, section 4.1.3.2.3)");
public:

    SpSecondaryHeader() = default;

    SpSecondaryHeader(const TC& tc, const Ancillary& ancillary)
    : time_code(tc), ancillary_data(ancillary) {

    }

    void serialize(OBitStream& o) const override {
        o << time_code << ancillary_data;
    }
    
    void deserialize(IBitStream& i) override {
        i >> time_code >> ancillary_data;
    }

    static constexpr std::size_t getSize() {
        return (TC::getWidth() + Ancillary::getWidth()) / CHAR_BIT;
    }

    /** The timecode field */
    TC time_code;
    /** The ancillary data field */
    Ancillary ancillary_data;
};

/**
 * Empty secondary header helper class (TC and ancillary fields are 0-sized)
 */
using SpEmptySecondaryHeader = SpSecondaryHeader<FieldCollection<>,
                                                 FieldCollection<>>;
} //namespace

#endif //CCSDS_SECONDARY_HEADER_HPP