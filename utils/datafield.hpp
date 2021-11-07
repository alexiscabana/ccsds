/**************************************************************************//**
 * @file datafield.hpp
 * @author Alexis Cabana-Loriaux
 * 
 * @brief Contains utilities for manipulating values and group of values
 *        that are not necessarily byte-aligned.
 * 
 ******************************************************************************/
#ifndef DATAFIELD_HPP
#define DATAFIELD_HPP

#include "utils/serializable.hpp"
#include "utils/endianness.hpp"
#include "utils/bitmask.hpp"
#include <cstdint>
#include <climits>
#include <cstring>
#include <type_traits>
#include <tuple>

/**
 * @brief Base case of Field
 */
class IField : public Serializable, public Deserializable
{

};

/**
 * @brief   A field (or value) of a given bit width, that is represented on a given type. The
 *          types of field allowed are only integral types.
 *  
 * @details Fields are used to handle values that might not be byte-aligned, in a standard way.
 *          At any given moment, only the LSB bits [0..WidthBits] contain the field's value. The
 *          other bits are considered as "don't care". Fields are powerful when used with 
 *          bitstreams, because they only serialize/deserialize the amount of bits necessary to
 *          hold their value.
 * 
 * @code    
 *          Field<uint8_t, 6> field;    // 6-bit field represented on a uint8_t
 *                                      // Field can thus only take values [0..63]
 *          field.setValue(64);         // 64 = 0b01000000 
 *          field.getValue() == 0;      //true, because the set() above clears the 6 LSBs
 * @endcode 
 * 
 * @note    For obvious reasons, the bit width must always be lower than or equal to the bit width  
 *          of the original type.
 */
template<typename T, 
         std::size_t WidthBits = sizeof(T) * CHAR_BIT, 
         bool IsLittleEndian = false>
class Field : public IField
{
    // template arguments checks
    static_assert(std::is_integral<T>::value, "Field type must be of integral type");
    static_assert(WidthBits <= (sizeof(T) * CHAR_BIT), "Field width is wider than the field type");
    static_assert(WidthBits > 0, "Field width can't be of width 0");
public:
    typedef T value_type;

    Field()
    : value(0) {
        this->setValue(0);
    }
    Field(T t): value(t) {
        this->setValue(t);
    }

    void serialize(OBitStream& out) const override {
        out.put(value, WidthBits, isLittleEndian());
    }
    
    void deserialize(IBitStream& in) override {
        in.get(value, WidthBits, isLittleEndian());
    }
    
    /**
     * @returns the value contained within the field's bit width
     */
    T getValue() const {
        return value & bitmask<uint64_t>(WidthBits);
    }
    
    /**
     * @brief Sets the value of the field, within the field's bit width
     */
    void setValue(const T t) {
        value = t & bitmask<uint64_t>(WidthBits);
    }

    /**
     * @returns the amount of bits that represents the value of the field
     */
    static constexpr std::size_t getWidth() {
        return WidthBits;
    }

    /**
     * @returns if the field is represented in little endian
     */
    static constexpr bool isLittleEndian() {
        return IsLittleEndian;
    }
    
    /**
     * @returns the boolean state of the bit #n of the field
     */
    template<std::size_t n>
    inline bool getBit() const {
        return static_cast<bool>((value >> n) & 0x1);
    }
    
    /**
     * @returns the boolean state of the bit #n of the field
     */
    inline bool getBit(std::size_t n) const {
        if(n < WidthBits) {
            return static_cast<bool>((value >> n) & 0x1);
        } else {
            return false;
        }
    }
    
    /**
     * @brief Sets the boolean state of the bit #n of the field
     * 
     * @param bit The value to set in the bit
     */
    template<std::size_t n>
    inline void setBit(bool bit) {
        static_assert(n <  WidthBits, "Bit is out of range");
        value = bit ? (value  | (0x1 << n)) : (value  & ~(0x1 << n));
    }
    
    /**
     * @brief Sets the boolean state of the bit #n of the field
     * 
     * @param n Which bit to set
     * @param bit The value to set in the bit
     */
    inline void setBit(std::size_t n, bool bit) {
        if(n < WidthBits) {
            value = bit ? (value  | (0x1 << n)) : (value  & ~(0x1 << n));
        }
    }

    template<std::enable_if_t<!std::is_floating_point<T>::value, bool> = true>
    auto& operator++() {
        setValue(getValue() + 1);
        return *this;
    }
    template<std::enable_if_t<!std::is_floating_point<T>::value, bool> = true>
    auto operator++(int) {
        Field<T, WidthBits, IsLittleEndian> ret(getValue());
        setValue(getValue() + 1);
        return ret;
    }

    template<std::enable_if_t<!std::is_floating_point<T>::value, bool> = true>
    auto& operator--() {
        setValue(getValue() - 1);
        return *this;
    }
    template<std::enable_if_t<!std::is_floating_point<T>::value, bool> = true>
    auto operator--(int) {
        Field<T, WidthBits, IsLittleEndian> ret(getValue());
        setValue(getValue() - 1);
        return ret;
    }
    
private:
    /** Variable of type T to hold the value on a byte-aligned type */
    T value;
};


/**
 * @brief   An abstract array of fields that are of the same type and bit width. @see{Field}. 
 *  
 * @code    
 *          Field<5, uint8_t, 6> field; // Five (5) 6-bit fields represented on a uint8_t
 * @endcode 
 */
template<std::size_t Size, 
         typename T, 
         std::size_t WidthBits = sizeof(T) * CHAR_BIT, 
         bool IsLittleEndian = false>
class FieldArray : public IField
{
    // template arguments checks
    static_assert(std::is_integral<T>::value, "Field type must be of integral type");
    static_assert(WidthBits <= (sizeof(T) * CHAR_BIT), "Field width is wider than the field type");
    static_assert(WidthBits > 0, "Field width can't be of width 0");
    static_assert(Size > 0, "Array field must contain at least 1 element");
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
 * @brief   A collection of single Fields, FieldArrays or even other FieldCollections.
 *          Can contain 0 fields (empty collection), and can have a total bit width that is not divisible by 8.
 *          FieldCollections are a simple way to hierarchically organize Fields,
 *          as well as group them together intuitively in a certain order.
 *          @see{Field}, @see{FieldArray}. 
 * 
 * @details Every field is stored in a multi-type tuple for easy access. De/serializing FieldCollections will
 *          simply de/serialize each field on by one, in the order specified by the user as template parameter.
 * 
 * @code    
 *          FieldCollection<> empty;                    // A collection of zero fields 
 * 
 *          FieldCollection<Field<uint8_t, 6>,          // A collection containing multiple fields of different types
 *                          Field<uint8_t, 4>,          // Note that the total width of my_collection would then be 16 bits
 *                          FieldArray<3, uint8_t, 2>,
 *                          FieldCollection<>> my_collection;
 * @endcode 
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

    /**
     * @returns A reference to the field at position @p index within the collection
     */
    template<std::size_t index>
    auto& getField() {
        static_assert(index < (sizeof...(Rest) + 1), "Field index out of range");
        return std::get<index>(field_tuple);
    }

    /**
     * @returns The amount of fields in the collection
     */
    static constexpr std::size_t getNbFields() {
        return sizeof...(Rest) + 1;
    }

    /**
     * @returns The combined width of the fields
     */
    static constexpr std::size_t getWidth() {
        return (T::getWidth() + ... + Rest::getWidth());
    }

private:
    /** Fields present in this collection */
    std::tuple<T, Rest...> field_tuple;
};

/**
 * @brief   The special case of a 1-bit Field.
 *          @see{Field}. 
 */
class Flag : public Field<uint8_t, 1> {
public:

    /**
     * @returns true if the flag is set
     */
    inline bool isSet() const {
        return this->getBit<0>();
    }

    /**
     * @brief Sets the flag to 1 (true)
     */
    inline void set() {
        this->setBit<0>(true);
    }

    /**
     * @brief Sets the flag to 0 (false)
     */
    inline void reset() {
        this->setBit<0>(false);
    }
};

/**
 * @brief   The special case of an empty Field (0-bit Field).
 *          @see{Field} and @see{FieldCollection}. 
 */
using FieldEmpty = FieldCollection<>;

#endif //DATAFIELD_HPP
