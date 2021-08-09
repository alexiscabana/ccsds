#ifndef DATAFIELD_HPP
#define DATAFIELD_HPP

#include "utils/serializable.hpp"
#include "utils/endianness.hpp"
#include <cstdint>
#include <climits>
#include <cstring>
#include <type_traits>
#include <tuple>

class IField : public Serializable, public Deserializable
{

};

template<typename T, 
         std::size_t WidthBits = sizeof(T) * CHAR_BIT, 
         bool IsLittleEndian = false>
class Field : public IField
{
    // template arguments checks
    static_assert(std::is_arithmetic<T>::value, "Field type must be of arthmetic");
    static_assert(WidthBits <= (sizeof(T) * CHAR_BIT), "Field width is wider than the field type");
    static_assert(WidthBits > 0, "Field width can't be of width 0");
    static_assert(!std::is_floating_point<T>::value || (sizeof(T) * CHAR_BIT) == WidthBits, "Floating point fields type must be represented in their integral size "
                                                                                            "(IEEE standard)");
public:
    Field()
    : value(0) {

    }
    Field(T t): value(t) {}
    Field(T& t): value(t) {}

    void serialize(OBitStream& out) const override {
        out.put(value, WidthBits, isLittleEndian());
    }
    
    void deserialize(IBitStream& in) override {
        in.get(value, WidthBits, isLittleEndian());
    }
    
    T getValue() const {
        return value;
    }
    
    void setValue(const T t) {
        value = t & ONES(WidthBits);
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
        return static_cast<bool>((value >> n) & 0x1);
    }
    
    inline bool getBit(std::size_t n) const {
        if(n < WidthBits) {
            return static_cast<bool>((value >> n) & 0x1);
        } else {
            return false;
        }
    }
    
    template<std::size_t n,
            std::enable_if_t<!std::is_floating_point<T>::value, bool> = true>
    inline void setBit(bool bit) {
        static_assert(n <  WidthBits, "Bit is out of range");
        value = bit ? (value  | (0x1 << n)) : (value  & ~(0x1 << n));
    }
    
    inline void setBit(std::size_t n, bool bit) {
        if(n < WidthBits) {
            value = bit ? (value  | (0x1 << n)) : (value  & ~(0x1 << n));
        }
    }
    
private:
    T value;
};


template<std::size_t Size, 
         typename T, std::size_t WidthBits = sizeof(T) * CHAR_BIT, 
         bool IsLittleEndian = false>
class FieldArray : public IField
{
    // template arguments checks
    static_assert(std::is_arithmetic<T>::value, "Field type specified in template must be of arthmetic type");
    static_assert(WidthBits <= (sizeof(T) * CHAR_BIT), "Field width is wider than the field type");
    static_assert(WidthBits > 0, "Field width can't be of width 0");
    static_assert(Size > 0, "Array field must contain at least 1 element");
    static_assert(!std::is_floating_point<T>::value || (sizeof(T) * CHAR_BIT) == WidthBits, 
                    "Floating point fields type must be represented in their integral size "
                    "(IEEE standard)");
public:
    FieldArray() = default;
    FieldArray(T* t, std::size_t s) {
        for(std::size_t i=0; i < std::min<std::size_t>(Size,s); i++) {
            new (&values[i]) decltype(values[0])(t[i]); 
        }
    }

    void serialize(OBitStream& out) const override {
        for(std::size_t i=0; i < Size; i++) {
            out << values[i];
        }
    }
    
    void deserialize(IBitStream& in) override {
        for(std::size_t i=0; i < Size; i++) {
            in >> values[i];
        }
    }
    
    T getValue(const std::size_t index) const {
        return values[index].getValue();
    }
    
    void setValue(const std::size_t index, const T t) {
        values[index].setValue(t);
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
        return static_cast<bool>((values[index].getValue() >> n) & 0x1);
    }
    
    template<std::size_t n,
            std::enable_if_t<!std::is_floating_point<T>::value, bool> = true>
    inline void setBit(const std::size_t index, bool bit) {
        static_assert(n <  WidthBits, "Bit is out of range");
        values[index].setValue(bit ? (values[index].getValue()  | (0x1 << n)) : (values[index].getValue()  & ~(0x1 << n)));
    }
    
private:

    Field<T,WidthBits,IsLittleEndian> values[Size];
};

/**
 * Base case of collection template
 */
template <typename... T>
class FieldCollection : public IField
{
public:
    FieldCollection() = default;

    void serialize(OBitStream& o) const override {
        //nothing to serialize, empty header
        (void)o;
    } 

    void deserialize(IBitStream& i) override {
        //nothing to deserialize, empty header
        (void)i;
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
class FieldCollection<T, Rest...> : public IField
{
    static_assert((std::is_base_of<IField, T>::value && ... && std::is_base_of<IField, Rest>::value), 
                    "Collection content must all be data fields");
public:
    FieldCollection() = default;
    FieldCollection(const T& first, const Rest&... rest)
    : field_tuple(first, rest...) {

    }

    void serialize(OBitStream& o) const override {
        std::apply([&](auto&&... args){ (void(o << args), ...); }, field_tuple);
    }
    
    void deserialize(IBitStream& i) override {
        std::apply([&](auto&&... args){ (void(i >> args), ...); }, field_tuple);
    }

    template<std::size_t index>
    auto& getField() {
        static_assert(index < (sizeof...(Rest) + 1), "Field index out of range");
        return std::get<index>(field_tuple);
    }

    static constexpr std::size_t getNbFields() {
        return sizeof...(Rest) + 1;
    }

    static constexpr std::size_t getWidth() {
        return (T::getWidth() + ... + Rest::getWidth());
    }

private:
    //members
    std::tuple<T, Rest...> field_tuple;
};

class Flag : public Field<uint8_t, 1> {
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
