#include "type.h"
#include "tokens.h"
using namespace std;
typedef std::string string;

Type::Type(int specifier, unsigned indirection):_declarator(SCALAR),_specifier(specifier),_indirection(indirection)
{

}

Type::Type(int specifier, unsigned indirection, unsigned long length):_declarator(ARRAY),_specifier(specifier),_indirection(indirection),_length(length)
{

}

Type::Type(int specifier, unsigned indirection, Parameters *parameters):_declarator(FUNCTION),_specifier(specifier),_indirection(indirection),_parameters(parameters)
{

}

string printIndirection(int indirection) {
    string str = "";
    if (indirection > 0) str += " ";
    while (indirection-- > 0) {
        str += "*";
    }
    return str;
}

string printSpecifier(int specifier) {
    if (specifier == VOID) {
        return "void";
    }
    else if (specifier == INT) {
        return "int";
    }
    else if (specifier == CHAR) {
        return "char";
    }
    else if (specifier == LONG) {
        return "long";
    }
    else {
        return "Wrong typespec";
    }
}

std::ostream& operator << (std::ostream &ostr, const Type &type){
    if (type.isArray()) {
        ostr << printSpecifier(type.getSpecifier()) << printIndirection(type.getIndirection()) << "[" << type.getLength() << "]";
    }
    else if (type.isFunction()) {
        ostr << printSpecifier(type.getSpecifier()) << printIndirection(type.getIndirection()) << "(" << ")";
    }
    else if (type.isScalar()) {
        ostr << printSpecifier(type.getSpecifier()) << printIndirection(type.getIndirection());
    }
    else if (type.isError()){
        // TBD
    }
    return ostr;
}