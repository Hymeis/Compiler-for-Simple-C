#ifndef SYMBOL_H
#define SYMBOL_H

#include <string>
#include "type.h"
typedef std::string string;

class Symbol {
    string _name;
    Type _type;
public:
    Symbol();
    Symbol(const string& name, const Type& type);
    const string &getName() const {return _name;}
    const Type &getType() const {return _type;}
    // ...
};




#endif /* SYMBOL_H */