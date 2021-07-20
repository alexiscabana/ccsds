#ifndef DATAFIELD_HPP
#define DATAFIELD_HPP

#include "utils/serializable.hpp"
#include "utils/endianness.hpp"
#include <cstdint>
#include <climits>
#include <cstring>
#include <type_traits>
#include <tuple>

class ifield
{

};

template<typename T, std::size_t WidthBits = sizeof(T)*CHAR_BIT, bool IsLittleEndian = false>
class field : public serializable, public ifield
{
    // template arguments checks
    static_assert(std::is_arithmetic<T>::value, "Field type must be of arthmetic");
    static_assert(WidthBits <= (sizeof(T) * CHAR_BIT), "Field width is wider than the field type");
    static_assert(WidthBits > 0, "Field width can't be of width 0");
    static_assert(!std::is_floating_point<T>::value || (sizeof(T) * CHAR_BIT) == WidthBits, "Floating point fields type must be represented in their integral size "
                                                                                            "(IEEE standard)");
public:
    field() = default;
    field(T t): field(t) {}
    field(T& t): field(t) {}

    void serialize(obitstream& out) const override {
        out.put(m_field, WidthBits, isLittleEndian());
    }
    
    void deserialize(ibitstream& in) override {
        in.get(m_field, WidthBits, isLittleEndian());
    }
    
    T getValue() const {
        return m_field;
    }
    
    void setValue(const T t) {
        m_field = t;
    }

    static constexpr std::size_t getWidth() {
        return WidthBits;
    }

    static constexpr bool isLittleEndian() {
        return IsLittleEndian;
    }
    
    template<std::size_t n,
            std::enable_if_t<!std::is_floating_point<T>::value, bool> = true>
    inline bool getBit() const {
        static_assert(n <  WidthBits, "Bit is out of range");
        return static_cast<bool>((m_field >> n) & 0x1);
    }
    
    template<std::size_t n,
            std::enable_if_t<!std::is_floating_point<T>::value, bool> = true>
    inline void setBit(bool bit) {
        static_assert(n <  WidthBits, "Bit is out of range");
        m_field = bit ? (m_field  | (0x1 << n)) : (m_field  & ~(0x1 << n));
    }
    
private:
    T m_field;
};


template<std::size_t Size, typename T, std::size_t WidthBits = sizeof(T)*CHAR_BIT, bool IsLittleEndian = false>
class fieldarray : public serializable, public ifield
{
    // template arguments checks
    static_assert(std::is_arithmetic<T>::value, "Field type specified in template must be of arthmetic type");
    static_assert(WidthBits <= (sizeof(T) * CHAR_BIT), "Field width is wider than the field type");
    static_assert(WidthBits > 0, "Field width can't be of width 0");
    static_assert(Size > 0, "Array field must contain at least 1 element");
    static_assert(!std::is_floating_point<T>::value || (sizeof(T) * CHAR_BIT) == WidthBits, "Floating point fields type must be represented in their integral size "
                                                                                            "(IEEE standard)");
public:
    fieldarray() = default;
    fieldarray(T* t, std::size_t s) {
        for(std::size_t i=0; i < std::min<std::size_t>(Size,s); i++) {
            new (&m_fields[i]) decltype(m_fields[0])(t[i]); 
        }
    }

    void serialize(obitstream& out) const override {
        for(std::size_t i=0; i < Size; i++) {
            out << m_fields[i];
        }
    }
    
    void deserialize(ibitstream& in) override {
        for(std::size_t i=0; i < Size; i++) {
            in >> m_fields[i];
        }
    }
    
    T getValue(const std::size_t index) const {
        return m_fields[index].getValue();
    }
    
    void setValue(const std::size_t index, const T t) {
        m_fields[index].setValue(t);
    }

    static constexpr std::size_t getWidth() {
        return WidthBits * Size;
    }

    static constexpr bool isLittleEndian() {
        return IsLittleEndian;
    }
    
    template<std::size_t n,
            std::enable_if_t<!std::is_floating_point<T>::value, bool> = true>
    inline bool getBit(const std::size_t index) const {
        static_assert(n <  WidthBits, "Bit is out of range");
        return static_cast<bool>((m_fields[index].getValue() >> n) & 0x1);
    }
    
    template<std::size_t n,
            std::enable_if_t<!std::is_floating_point<T>::value, bool> = true>
    inline void setBit(const std::size_t index, bool bit) {
        static_assert(n <  WidthBits, "Bit is out of range");
        m_fields[index].setValue(bit ? (m_fields[index].getValue()  | (0x1 << n)) : (m_fields[index].getValue()  & ~(0x1 << n)));
    }
    
private:

    field<T,WidthBits,IsLittleEndian> m_fields[Size];
};

/**
 * Base case of collection template
 */
template <typename... T>
class fieldcollection : public serializable, public ifield
{
public:
    fieldcollection() = default;

    void serialize(obitstream& o) const override {
        //nothing to serialize, empty header
    } 

    void deserialize(ibitstream& i) override {
        //nothing to deserialize, empty header
    }

    static constexpr std::size_t getNbFields() {
        return 0;
    }

    static constexpr std::size_t getWidth() {
        return 0;
    }
};

/**
 * Partial specialization of collection template with >1 data field
 */
template<typename T, typename... Rest>
class fieldcollection<T, Rest...> : public serializable, public ifield
{
    static_assert((std::is_base_of<ifield, T>::value && ... && std::is_base_of<ifield, Rest>::value), 
                    "Collection content must all be data fields");
public:
    fieldcollection() = default;
    fieldcollection(const T& first, const Rest&... rest)
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

    static constexpr std::size_t getWidth() {
        return (T::getWidth() + ... + Rest::getWidth());
    }

private:
    //members
    std::tuple<T, Rest...> fields;
};

class flag : public field<uint8_t, 1> {
public:
    inline bool isSet() const {
        return this->getBit<0>();
    }

    inline void set() {
        this->setBit<0>(true);
    }

    inline void reset() {
        this->setBit<0>(false);
    }
};

#endif //DATAFIELD_HPP
