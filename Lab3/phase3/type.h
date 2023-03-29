#ifndef TYPY_H
#define TYPE_H

#include <vector>
#include <cstdlib>
#include <ostream>
#include <cassert>
typedef std::vector<class Type> Parameters;
// TBD

class Type {
    enum{ARRAY, ERROR, FUNCTION, SCALAR} _declarator;
    int _specifier;
    unsigned _indirection;
    unsigned long _length;
    Parameters *_parameters; // exists: (void), DNE: ()

    public:
        Type(int specifier, unsigned indirection = 0);
        Type(int specifier, unsigned indirection, unsigned long length); // ARRAY
        Type(int specifier, unsigned indirection, Parameters *parameters); // FUNCTION
        Type();
        bool isArray() const { return _declarator == ARRAY;}
        bool isError() const { return _declarator == ERROR;}
        bool isFunction() const { return _declarator == FUNCTION;}
        bool isScalar() const { return _declarator == SCALAR;}
        int getDeclarator() const {return _declarator;}
        int getSpecifier() const {return _specifier;}
        unsigned getIndirection() const {return _indirection;}
        unsigned long getLength() const {return _length;}
        Parameters *getParameters() const {return _parameters;} 

        bool operator == (const Type &rhs) const {
            if (_declarator != rhs.getDeclarator()) return false;
            if (_declarator == ERROR) return true;
            if (_specifier != rhs.getSpecifier()) return false;
            if (_indirection != rhs.getIndirection()) return false;
            if (_declarator == SCALAR) return true;
            if (_declarator == ARRAY) {
                return _length == rhs.getLength();
            }
            assert(_declarator == FUNCTION);
            if (!_parameters || !rhs._parameters) return true;
            return *_parameters == *rhs._parameters;
        }
        
        bool operator != (const Type &rhs) const {
            return !operator==(rhs);
        }
        
};

std::ostream &operator<<(std::ostream &ostr, const Type &type);

#endif /* TYPE_H */ 