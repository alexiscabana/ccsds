#ifndef CCSDS_SPACEPACKET_HPP
#define CCSDS_SPACEPACKET_HPP

#include "utils/serializable.hpp"
#include "utils/buffer.hpp"
#include "spacepacket/primaryhdr.hpp"
#include "spacepacket/secondaryhdr.hpp"
#include <tuple>
#include <cstdint>
#include <climits>
#include <type_traits>

namespace ccsds
{

/**
 * Spacepacket builder template
 */
template<typename SecHdrType>
class SpBuilder : public Serializable
{
    static_assert((std::is_base_of<ISpSecondaryHeader, SecHdrType>::value), 
                    "Secondary header type must be of type ISpSecondaryHeader");
    //static_assert(BufferType::getSize() > 0 || SecHdrType::getSize() > 0, 
    //                "There shall be a User Data Field, or a Packet Secondary Header, or both (pink book, 4.1.3.2.1.2 and 4.1.3.3.2)");
public:
    SpBuilder(IBuffer& user_data_buffer)
    : user_data(user_data_buffer) {

    }

    void serialize(OBitStream& o) const override {
        o << primary_hdr << secondary_hdr << user_data;
    }
    
    std::size_t size() {
        return SpPrimaryHeader::getSize() + SecHdrType::getSize() + user_data.getSize();
    }

    OBitStream& data() {
        return user_data;
    }

    SpPrimaryHeader primary_hdr;
    SecHdrType      secondary_hdr;

private:
    OBitStream      user_data;
};

/**
 * Spacepacket Dissector template
 */
template<typename SecHdrType, typename ...Fields>
class SpDissector : public Deserializable
{
public:
    SpDissector() 
    : is_pri_hdr_provided(false), is_sec_hdr_provided(false) {

    }

    SpDissector(SpPrimaryHeader& pri_hdr)
    : primary_hdr(pri_hdr), is_pri_hdr_provided(true), is_sec_hdr_provided(false) {

    }

    SpDissector(SpPrimaryHeader& pri_hdr, SecHdrType& sec_hdr)
    : primary_hdr(pri_hdr), secondary_hdr(sec_hdr), is_pri_hdr_provided(true), is_sec_hdr_provided(true) {

    }

    void deserialize(IBitStream& i) override {

        // Only deserialize primary and secondary headers if they were not already provided at construction
        if(!is_pri_hdr_provided) {
            i >> primary_hdr;
        }

        if(!is_sec_hdr_provided) {
            i >> secondary_hdr;
        }

        std::apply([&](auto&&... args){ (void(i >> args), ...); }, field_tuple);
    }

    template<std::size_t index>
    auto& getField() {
        static_assert(index < (sizeof...(Fields) + 1), "Field index out of range");
        return std::get<index>(field_tuple);
    }

    SpPrimaryHeader         primary_hdr;
    SecHdrType              secondary_hdr;

private:
    std::tuple<Fields...>   field_tuple;
    bool                    is_pri_hdr_provided;
    bool                    is_sec_hdr_provided;
};

} //namespace

#endif //CCSDS_SPACEPACKET_HPP