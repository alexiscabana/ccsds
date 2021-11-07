/**************************************************************************//**
 * @file printable.hpp
 * @author Alexis Cabana-Loriaux
 * 
 * @brief Contains an interface for printing a particular object
 * 
 ******************************************************************************/
#ifndef PRINTABLE_HPP
#define PRINTABLE_HPP

#include <cstdio>

class Printable
{
public:
    /**
     * @brief Print a representation of this object.
     */ 
    virtual void print() const = 0;
};

#endif //PRINTABLE_HPP
