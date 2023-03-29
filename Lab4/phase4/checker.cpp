/*
 * File:	checker.cpp
 *
 * Description:	This file contains the public and private function and
 *		variable definitions for the semantic checker for Simple C.
 *
 *		If a symbol is redeclared, the redeclaration is discarded
 *		and the original declaration is retained.
 *
 *		Extra functionality:
 *		- inserting an undeclared symbol with the error type
 */

# include <iostream>
# include "lexer.h"
# include "checker.h"
# include "tokens.h"
# include "Symbol.h"
# include "Scope.h"
# include "Type.h"
# include <cassert>

using namespace std;

static Scope *outermost, *toplevel;
static const Type error;
static Type integer(INT);
static Type lone(LONG);
static string redefined = "redefinition of '%s'";
static string redeclared = "redeclaration of '%s'";
static string conflicting = "conflicting types for '%s'";
static string undeclared = "'%s' undeclared";
static string void_object = "'%s' has type void";
/*
E1. invalid return type
E2. invalid type for test expression
E3. lvalue required in expression
E4. invalid operands to binary operator
E5. invalid operand to unary operator
E6. called object is not a function
E7. invalid arguments to called function
*/
static string E1 = "invalid return type";
static string E2 = "invalid type for test expression";
static string E3 = "lvalue required in expression";
static string E4 = "invalid operands to binary %s";
static string E5 = "invalid operand to unary %s";
static string E6 = "called object is not a function";
static string E7 = "invalid arguments to called function";


/*
 * Function:	openScope
 *
 * Description:	Create a scope and make it the new top-level scope.
 */

Scope *openScope()
{
    toplevel = new Scope(toplevel);

    if (outermost == nullptr)
	outermost = toplevel;

    return toplevel;
}


/*
 * Function:	closeScope
 *
 * Description:	Remove the top-level scope, and make its enclosing scope
 *		the new top-level scope.
 */

Scope *closeScope()
{
    Scope *old = toplevel;
    toplevel = toplevel->enclosing();
    return old;
}


/*
 * Function:	defineFunction
 *
 * Description:	Define a function with the specified NAME and TYPE.  A
 *		function is always defined in the outermost scope.  This
 *		definition always replaces any previous definition or
 *		declaration.
 */

Symbol *defineFunction(const string &name, const Type &type)
{
    cout << name << ": " << type << endl;
    Symbol *symbol = outermost->find(name);

    if (symbol != nullptr) {
	if (symbol->type().isFunction() && symbol->type().parameters()) {
	    report(redefined, name);
	    delete symbol->type().parameters();

	} else if (type != symbol->type())
	    report(conflicting, name);

	outermost->remove(name);
	delete symbol;
    }

    symbol = new Symbol(name, type);
    outermost->insert(symbol);
    return symbol;
}


/*
 * Function:	declareFunction
 *
 * Description:	Declare a function with the specified NAME and TYPE.  A
 *		function is always declared in the outermost scope.  Any
 *		redeclaration is discarded.
 */

Symbol *declareFunction(const string &name, const Type &type)
{
    cout << name << ": " << type << endl;
    Symbol *symbol = outermost->find(name);

    if (symbol == nullptr) {
	symbol = new Symbol(name, type);
	outermost->insert(symbol);

    } else if (type != symbol->type()) {
	report(conflicting, name);
	delete type.parameters();

    } else
	delete type.parameters();

    return symbol;
}


/*
 * Function:	declareVariable
 *
 * Description:	Declare a variable with the specified NAME and TYPE.  Any
 *		redeclaration is discarded.
 */

Symbol *declareVariable(const string &name, const Type &type)
{
    cout << name << ": " << type << endl;
    Symbol *symbol = toplevel->find(name);

    if (symbol == nullptr) {
	if (type.specifier() == VOID && type.indirection() == 0)
	    report(void_object, name);

	symbol = new Symbol(name, type);
	toplevel->insert(symbol);

    } else if (outermost != toplevel)
	report(redeclared, name);

    else if (type != symbol->type())
	report(conflicting, name);

    return symbol;
}


/*
 * Function:	checkIdentifier
 *
 * Description:	Check if NAME is declared.  If it is undeclared, then
 *		declare it as having the error type in order to eliminate
 *		future error messages.
 */

Symbol *checkIdentifier(const string &name)
{
    Symbol *symbol = toplevel->lookup(name);

    if (symbol == nullptr) {
        report(undeclared, name);
        symbol = new Symbol(name, error);
        toplevel->insert(symbol);
    }

    return symbol;
}




Type checkLogical(const Type &left, const Type &right, const string &op) { // and, or
    if (error == left || error == right) return error;
    if (left.isPredicate() && right.isPredicate()) return integer;
    report(E4, op);
    return error;
}

Type checkEquality(const Type &left, const Type &right, const string &op) {
    if (error == left || error == right) return error;
    if (left.isCompatibleWith(right)) {
        return integer;
    }
    else {
        report(E4, op);
        return error;
    }
}

Type checkRelational(const Type &left, const Type &right, const string &op) {
    cout << "left: " << left << " right: " << right << endl;
    if (error == left || error == right) return error;
    if ((left.isNumeric() && right.isNumeric()) || (left == right && left.isPredicate())) {
        return integer;
    }
    else {
        report(E4, op);
        return error;
    }
}

Type checkAddition(const Type &left, const Type &right) {
    // ptr + int, int + ptr, (int + int) (promotions)
    if (error == left || error == right) return error;
    if (left.isNumeric() && right.isNumeric()) {
        return (left.specifier() == LONG || right.specifier() == LONG) ? lone : integer;
    }
    else {
        if (left.isPointer() && right.isPointer()) {
            report(E4, "+");
            return error;
        }
        else { // only one is a pointer
            if ((left.specifier() == VOID && left.indirection() == 1) || (right.specifier() == VOID && right.indirection() == 1)) { // return error if one is void*
                report(E4, "+");
                return error;
            }
            else {
                return left.isPointer() ? left : right;
            }
        }
    }
}

Type checkSubtraction(const Type &left, const Type &right) {
    // ptr - ptr, ptr - int, int - int (promotions)
    if (error == left || error == right) return error;
    if (left.isNumeric() && right.isNumeric()) {
        return (left.specifier() == LONG || right.specifier() == LONG) ? lone : integer;
    }
    else {
        if (left.isNumeric() || (left.specifier() == VOID && left.indirection() == 1)) { // left is not a ptr or left is void*
                report(E4, "-");
                return error;
        }
        else {
            if (left == right) { // both are pointers of the same type (cannot be void*)
                return lone;
            }
            else {
                if (right.isNumeric()) { // ptr - int
                    return left;
                }
                else { // both are pointers, but not the same type
                    report(E4, "-");
                    return error;
                }
            }
        }
    }
}

Type checkMultiplicative(const Type &left, const Type &right, const string &op) {
    if (error == left || error == right) return error;
    if (!left.isNumeric() || !right.isNumeric()) {
        report(E4, op);
        return error;
    }
    return (left.specifier() == LONG || right.specifier() == LONG) ? lone : integer;
}

Type checkNot(const Type &right) {
    if (error == right) return error;
    if (right.isPredicate()) return integer;
    report(E5);
    return error;
}

Type checkNegation(const Type &right) {
    if (error == right) return error;
    if (!right.isNumeric()) {
        report(E5, "-");
        return error;
    }
    return (right.specifier() == LONG) ? lone : integer;
}

Type checkDerefrence(const Type &right) {
    if (error == right) return error;
    if (!right.isPointer() || (right.indirection() == 1 && right.specifier() == VOID)) {
        report(E5, "*");
        return error;
    }
    return Type(right.specifier(), right.indirection() - 1);
}

Type checkAddress(const Type &right, bool &lvalue) {
    if (error == right) return error;
    if (!lvalue) {
        report(E3);
        return error;
    }
    //Type t = right;
    //t.setIndirection(right.indirection() + 1);
    return Type(right.specifier(), right.indirection() + 1);
}

Type checkSizeof(const Type &right) {
    if (error == right) return error;
    if (right.isPredicate()) return lone;
    report(E5, "sizeof");
    return error;
}


Type checkPostfix(const Type &left, const Type &right) {
    if (error == left || error == right) return error;
    if (!left.isPointer() || (left.indirection() == 1 && left.specifier() == VOID) || !right.isNumeric()) {
        report(E4, "[]");
        return error;
    }
    return Type(left.specifier(),left.indirection() - 1);
}



Type checkFunctionType(const Type &prev) {
    if (error == prev) return error;
    if (!prev.isFunction()) {
        report(E6);
        return error;
    }
    return prev;
}

void checkAssignment(const Type &left, const Type &right, bool &lvalue) {
    cout << "left: " << left << " right: " << right << endl;
    if (error == left || error == right) return;
    if (!left.isCompatibleWith(right)) {
        report(E4, "=");
        return;
    }
    if (!lvalue) {
        report(E3);
        return;
    }
}

Type checkReturn(const Type &functionType, const Type &returnType) {
    if (functionType == error || returnType == error) return error;
    if (!returnType.isCompatibleWith(functionType)) {
        report(E1);
        return error;
    }
    return returnType;
}

Type checkConditional(const Type &right, const string &op) {
    if (error == right) return error;
    if (!right.isPredicate()) {
        report(E2);
        return error;
    }
    return right;
}

Type checkParameterTypes(const Type &t) { // for both declared and defined functions
    if (error == t) return error;
    if (!t.isPredicate()) {
        report(E7);
        return error;
    }
    return t;
}

Type checkArguments(const Type &prev, const Type &curr) { // for defined functions only
    if (error == prev || error == curr) return error;
    if (!prev.isCompatibleWith(curr)) {
        report(E7);
        return error;
    }
    return curr;
}

Type reportE7(const Type &t) { // for defined functions only: called when parameter length mismatch exists
    if (error != t) report(E7);
    return error;
}