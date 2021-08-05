#ifndef SERIALIZABLE_HPP
#define SERIALIZABLE_HPP

#include "utils/ibitstream.hpp"
#include "utils/obitstream.hpp"

class Serializable
{
public:
    Serializable() = default;
    virtual void serialize(OBitStream& o) const = 0;
    friend OBitStream& operator<<(OBitStream& o, const Serializable& s) {
        s.serialize(o);
        return o;
    }
};

class Deserializable
{
public:
    Deserializable() = default;
    virtual void deserialize(IBitStream& i) = 0;
    friend IBitStream& operator>>(IBitStream& i, Deserializable& s) {
        s.deserialize(i);
        return i;
    }
};

#endif //SERIALIZABLE_HPP