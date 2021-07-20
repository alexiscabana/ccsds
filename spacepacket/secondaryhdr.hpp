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
 * Partial specialization of secondary header template with >1 data field
 */
template<typename TC, typename Ancilliary>
class sp_secondaryhdr : public serializable
{
    static_assert((std::is_base_of<ifield, TC>::value && std::is_base_of<ifield, Ancilliary>::value), 
                    "Secondary header fields must all be data fields");
    static_assert(TC::getWidth() % CHAR_BIT == 0, 
                    "Time Code Field must consist of an integral number of octets (pink book, section 4.1.3.2.2.1)");
    static_assert(Ancilliary::getWidth() % CHAR_BIT == 0, 
                    "Ancilliary Data Field must consist of an integral number of octets (pink book, section 4.1.3.2.3)");
public:
    sp_secondaryhdr() = default;
    sp_secondaryhdr(const TC& tc, const Ancilliary& ancillary)
    : m_timecode(tc), m_ancilliary(ancillary) {

    }

    void serialize(obitstream& o) const override {
        o << m_timecode << m_ancilliary;
    }
    
    void deserialize(ibitstream& i) override {
        i >> m_timecode >> m_ancilliary;
    }

    auto& getTCField() {
        return m_timecode;
    }

    auto& getAncilliaryField() {
        return m_ancilliary;
    }

    static constexpr std::size_t getSizeBits() {
        return (TC::getWidth() + Ancilliary::getWidth());
    }

private:
    //members
    TC m_timecode;
    Ancilliary m_ancilliary;
};

} //namespace

#endif //CCSDS_SECONDARY_HEADER_HPP