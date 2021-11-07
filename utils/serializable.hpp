/**************************************************************************//**
 * @file serializable.hpp
 * @author Alexis Cabana-Loriaux
 * 
 * @brief Contains an interfaces for encoding and decoding objects
 * 
 ******************************************************************************/
#ifndef SERIALIZABLE_HPP
#define SERIALIZABLE_HPP

#include "utils/ibitstream.hpp"
#include "utils/obitstream.hpp"

/**
 * @brief Interface for objects that can be serialized to a bitstream
 * 
 */
class Serializable
{
public:
    Serializable() = default;

    /**
     * @brief Serialize this object in an output bitstream
     * 
     * @param o The output bitstream
     */
    virtual void serialize(OBitStream& o) const = 0;

    /**
     * @brief Serialize an object in an output bitstream
     * 
     * @param o The output bitstream
     * @param s The serializable object
     * 
     * @return The input parameter @p o
     */
    friend OBitStream& operator<<(OBitStream& o, const Serializable& s) {
        s.serialize(o);
        return o;
    }
};

/**
 * @brief Interface for objects that can be deserialized from a bitstream
 * 
 */
class Deserializable
{
public:
    Deserializable() = default;

    /**
     * @brief Deserialize this object from an input bitstream
     * 
     * @param i The input bitstream
     */
    virtual void deserialize(IBitStream& i) = 0;

    /**
     * @brief Deserialize an object from an input bitstream
     * 
     * @param i The input bitstream
     * @param d The deserializable object
     * 
     * @return The input parameter @p i
     */
    friend IBitStream& operator>>(IBitStream& i, Deserializable& d) {
        d.deserialize(i);
        return i;
    }
};

#endif //SERIALIZABLE_HPP