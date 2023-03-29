/*
 * File:	checker.h
 *
 * Description:	This file contains the public function declarations for the
 *		semantic checker for Simple C.
 */

# ifndef CHECKER_H
# define CHECKER_H
# include "Scope.h"
# include <string>

typedef std::string string;

Scope *openScope();
Scope *closeScope();

Symbol *defineFunction(const std::string &name, const Type &type);
Symbol *declareFunction(const std::string &name, const Type &type);
Symbol *declareVariable(const std::string &name, const Type &type);
Symbol *checkIdentifier(const std::string &name);

Type checkLogical(const Type &left, const Type &right, const string &op);
Type checkRelational(const Type &left, const Type &right, const string &op);
Type checkEquality(const Type &left, const Type &right, const string &op);
Type checkAddition(const Type &left, const Type &right);
Type checkSubtraction(const Type &left, const Type &right);
Type checkMultiplicative(const Type &left, const Type &right, const string &op);
// prefix expressions
Type checkNot(const Type &right);
Type checkNegation(const Type &right);
Type checkDerefrence(const Type &right);
Type checkAddress(const Type &right, bool &lvalue);
Type checkSizeof(const Type &right);

Type checkPostfix(const Type &left, const Type &right);
Type checkFunctionType(const Type& prev);
// statements
Type checkReturn(const Type &functionType, const Type &returnType);
Type checkConditional(const Type &right, const string &op);
void checkAssignment(const Type &left, const Type &right, bool &lvalue);
// Functions
Type checkParameterTypes(const Type &t);
Type checkArguments(const Type &prev, const Type &curr);
Type reportE7(const Type &t);
# endif /* CHECKER_H */
