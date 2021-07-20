#ifndef SERIALIZABLE_HPP
#define SERIALIZABLE_HPP

#include "utils/ibitstream.hpp"
#include "utils/obitstream.hpp"

class serializable
{
public:
    serializable() = default;
    virtual void serialize(obitstream& o) const = 0;
    virtual void deserialize(ibitstream& i) = 0;
    friend obitstream& operator<<(obitstream& o, const serializable& s) {
        s.serialize(o);
        return o;
    }
    friend ibitstream& operator>>(ibitstream& i, serializable& s) {
        s.deserialize(i);
        return i;
    }
};

#endif //SERIALIZABLE_HPP