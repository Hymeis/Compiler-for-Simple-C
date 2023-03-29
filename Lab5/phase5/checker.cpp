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
 *		- scaling the operands and results of pointer arithmetic
 *		- explicit type conversions and promotions
 */

# include <iostream>
# include "lexer.h"
# include "checker.h"
# include "tokens.h"
# include "Symbol.h"
# include "Scope.h"
# include "Type.h"


using namespace std;

static Scope *outermost, *toplevel;
static const Type error, voidptr(VOID, 1);
static const Type integer(INT), character(CHAR), longint(LONG);

static string redefined = "redefinition of '%s'";
static string redeclared = "redeclaration of '%s'";
static string conflicting = "conflicting types for '%s'";
static string undeclared = "'%s' undeclared";
static string void_object = "'%s' has type void";

static string invalid_return = "invalid return type";
static string invalid_test = "invalid type for test expression";
static string invalid_lvalue = "lvalue required in expression";
static string invalid_operands = "invalid operands to binary %s";
static string invalid_operand = "invalid operand to unary %s";
static string invalid_function = "called object is not a function";
static string invalid_arguments = "invalid arguments to called function";


/*
 * Function:	debug
 */

static void debug(const string &str, const Type &t1, const Type &t2)
{
    cout << "line " << yylineno << ": " << str << " " << t1 << " to " << t2 << endl;
}


/*
 * Function:	promote
 *
 * Description:	Perform type promotion on the given expression.  An array
 *		is promoted to a pointer by explicitly inserting an address
 *		operator.  A character is promoted to an integer by
 *		explicitly inserting a type cast.
 */

static Type promote(Expression *&expr)
{
    if (expr->type().isArray()) {
	debug("promoting", expr->type(), expr->type().promote());
	expr = new Address(expr, expr->type().promote());

    } else if (expr->type() == character) {
	debug("promoting", character, integer);
	expr = new Cast(expr, integer);
    }

    return expr->type();
}


/*
 * Function:	cast
 *
 * Description:	Cast the given expression to the given type by inserting a
 *		cast operation.  As an optimization, an integer can always
 *		be converted to a long integer without an explicit cast.
 */

static Expression *cast(Expression *expr, const Type &type)
{
    unsigned long value;


    if (expr->isNumber(value))
	if (expr->type() == integer && type == longint) {
	    delete expr;
	    return new Number(value);
	}

    return new Cast(expr, type);
}


/*
 * Function:	convert
 *
 * Description:	Attempt to convert the given expression to the given type
 *		as if by assignment.  We only do promotion in the case of
 *		an array because we don't want to promote a char to an int
 *		in case we are assigning to a char.  Also, there's no point
 *		in promoting a char to an int if we're just going to coerce
 *		the int to a long in the next step.
 */

static Type convert(Expression *&expr, Type type)
{
    if (expr->type().isArray() && type.isPointer())
	promote(expr);

    if (expr->type() != type && expr->type().isNumeric() && type.isNumeric()) {
	debug("assigning", expr->type(), type);
	expr = cast(expr, type);
    }

    return expr->type();
}


/*
 * Function:	extend
 *
 * Description:	Attempt to extend the type of the given expression to the
 *		given type.  The type of the given expression is only
 *		extended, not truncated.  We ensure this by checking that
 *		either the source type is char or the target type is long.
 */

static Type extend(Expression *&expr, Type type)
{
    if (expr->type() != type && expr->type().isNumeric() && type.isNumeric())
	if (expr->type() == character || type == longint) {
	    debug("extending", expr->type(), type);
	    expr = cast(expr, type);
	}

    return promote(expr);
}


/*
 * Function:	scale
 *
 * Description:	Scale the result of pointer arithmetic.
 */

static Expression *scale(Expression *expr, unsigned size)
{
    unsigned long value;


    if (expr->isNumber(value)) {
	delete expr;
	return new Number(value * size);
    }

    extend(expr, longint);
    return new Multiply(expr, new Number(size), longint);
}


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


/*
 * Function:	checkCall
 *
 * Description:	Check a function call expression: symbol (args).  The
 *		symbol must have a function type, and the number and types
 *		of the arguments must agree.
 */

Expression *checkCall(Symbol *symbol, Expressions &args)
{
    const Type &t = symbol->type();
    Type result = error;
    Parameters *params;


    if (t != error) {
	if (t.isFunction()) {
	    params = t.parameters();
	    result = Type(t.specifier(), t.indirection());

	    if (params == nullptr) {
		for (auto &arg : args) {
		    const Type &t = promote(arg);

		    if (t != error && !t.isPredicate()) {
			report(invalid_arguments);
			result = error;
			break;
		    }
		}

	    } else if (params->size() != args.size())
		report(invalid_arguments);

	    else
		for (unsigned i = 0; i < args.size(); i ++) {
		    const Type &t = convert(args[i], (*params)[i]);

		    if (!t.isCompatibleWith((*params)[i])) {
			report(invalid_arguments);
			result = error;
			break;
		    }
		}

	} else
	    report(invalid_function);
    }

    return new Call(symbol, args, result);
}


/*
 * Function:	checkArray
 *
 * Description:	Check an array expression: left [right].  The left operand
 *		must have type "pointer to T" after promotion and the right
 *		operand must have a numeric type.  The result has type T.
 */

Expression *checkArray(Expression *left, Expression *right)
{
    const Type t1 = promote(left);
    Type t2 = right->type();
    Type result = error;


    if (t1 != error && t2 != error) {
	if (t1.isPointer() && t2.isNumeric() && t1 != voidptr) {
	    right = scale(right, t1.deref().size());
	    result = t1.deref();
	} else
	    report(invalid_operands, "[]");
    }

    return new Dereference(new Add(left, right, t1), result);
}


/*
 * Function:	checkNot
 *
 * Description:	Check a logical negation expression: ! expr.  The operand
 *		must have a predicate type, and the result has type int.
 */

Expression *checkNot(Expression *expr)
{
    const Type &t = promote(expr);
    Type result = error;


    if (t != error) {
	if (t.isPredicate())
	    result = integer;
	else
	    report(invalid_operand, "!");
    }

    return new Not(expr, result);
}


/*
 * Function:	checkNegate
 *
 * Description:	Check an arithmetic negation expression: - expr.  The
 *		operand must have a numeric type, and the result has that
 *		type.
 */

Expression *checkNegate(Expression *expr)
{
    const Type &t = promote(expr);
    Type result = error;


    if (t != error) {
	if (t.isNumeric())
	    result = t;
	else
	    report(invalid_operand, "-");
    }

    return new Negate(expr, result);
}


/*
 * Function:	checkDereference
 *
 * Description:	Check a dereference expression: * expr.  The operand must
 *		have type "pointer to T," where T is not void.
 */

Expression *checkDereference(Expression *expr)
{
    const Type &t = promote(expr);
    Type result = error;


    if (t != error) {
	if (t.isPointer() && t != voidptr)
	    result = t.deref();
	else
	    report(invalid_operand, "*");
    }

    return new Dereference(expr, result);
}


/*
 * Function:	checkAddress
 *
 * Description:	Check an address expression: & expr.  The operand must be
 *		an lvalue, and if the operand has type T, then the result
 *		type "pointer to T."
 */

Expression *checkAddress(Expression *expr)
{
    const Type &t = expr->type();
    Type result = error;


    if (t != error) {
	if (expr->lvalue())
	    result = Type(t.specifier(), t.indirection() + 1);
	else
	    report(invalid_lvalue);
    }

    return new Address(expr, result);
}


/*
 * Function:	checkSizeof
 *
 * Description:	Check a sizeof expression: sizeof expr.  The operand must
 *		be a predicate type, and the result has type long.
 */

Expression *checkSizeof(Expression *expr)
{
    const Type &t = expr->type();


    if (t != error && !t.isPredicate()) {
	report(invalid_operand, "sizeof");
	return new Number(0);
    }

    return new Number(t.size());
}


/*
 * Function:	checkMultiplicative
 *
 * Description:	Check a multiplication, division, or remainder expression:
 *		both operands must have numeric types, and the result has
 *		type long if either operand has type long, and has type int
 *		otherwise.
 */

static Type
checkMultiplicative(Expression *&left, Expression *&right, const string &op)
{
    const Type &t1 = extend(left, right->type());
    const Type &t2 = extend(right, left->type());
    Type result = error;


    if (t1 != error && t2 != error) {
	if (t1.isNumeric() && t2.isNumeric())
	    result = t1;
	else
	    report(invalid_operands, op);
    }

    return result;
}


/*
 * Function:	checkMultiply
 *
 * Description:	Check a multiplication expression: left * right.
 */

Expression *checkMultiply(Expression *left, Expression *right)
{
    Type t = checkMultiplicative(left, right, "*");
    return new Multiply(left, right, t);
}


/*
 * Function:	checkDivide
 *
 * Description:	Check a division expression: left / right.
 */

Expression *checkDivide(Expression *left, Expression *right)
{
    Type t = checkMultiplicative(left, right, "/");
    return new Divide(left, right, t);
}

/*
 * Function:	checkRemainder
 *
 * Description:	Check a remainder expression: left % right.
 */

Expression *checkRemainder(Expression *left, Expression *right)
{
    Type t = checkMultiplicative(left, right, "%");
    return new Remainder(left, right, t);
}


/*
 * Function:	checkAdd
 *
 * Description:	Check an addition expression: left + right.  If both
 *		operands have numeric types, then the result has type long
 *		if either operand has type long, and has type int
 *		otherwise; if one operand has a pointer type and the other
 *		has a numeric type, then the result has that pointer type.
 */

Expression *checkAdd(Expression *left, Expression *right)
{
    Type t1 = left->type();
    Type t2 = right->type();
    Type result = error;


    if (t1 != error && t2 != error) {
	if (t1.isNumeric() && t2.isNumeric()) {
	    t1 = extend(left, t2);
	    t2 = extend(right, t1);
	    result = t1;

	} else if (t1.isPointer() && t2.isNumeric() && t1 != voidptr) {
	    t1 = promote(left);
	    right = scale(right, t1.deref().size());
	    result = t1;

	} else if (t1.isNumeric() && t2.isPointer() && t2 != voidptr) {
	    t2 = promote(right);
	    left = scale(left, t2.deref().size());
	    result = t2;

	} else
	    report(invalid_operands, "+");
    }

    return new Add(left, right, result);
}


/*
 * Function:	checkSubtract
 *
 * Description:	Check a subtraction expression: left - right.  If both
 *		operands have a numeric type, then the result has type long
 *		if either operand has type long and has type int otherwise;
 *		if the left operand has a pointer type and the right
 *		operand has a numeric type, then the result has that
 *		pointer type; if both operands have identical pointer
 *		types, then the result has type long.
 */

Expression *checkSubtract(Expression *left, Expression *right)
{
    Type t1 = left->type();
    Type t2 = right->type();
    Type result = error;
    Expression *expr;


    if (t1 != error && t2 != error) {
	if (t1.isNumeric() && t2.isNumeric()) {
	    t1 = extend(left, t2);
	    t2 = extend(right, t1);
	    result = t1;

	} else {
	    t1 = promote(left);

	    if (t1.isPointer() && t2.isNumeric() && t1 != voidptr) {
		right = scale(right, t1.deref().size());
		result = t1;

	    } else {
		t2 = promote(right);

		if (t1.isPointer() && t1 == t2 && t1 != voidptr)
		    result = longint;
		else
		    report(invalid_operands, "-");
	    }
	}
    }

    expr = new Subtract(left, right, result);

    if (t1.isPointer() && t1 == t2)
	expr = new Divide(expr, new Number(t1.deref().size()), longint);

    return expr;
}


/*
 * Function:	checkRelational
 *
 * Description:	Check a relational expression: the types of both operands
 *		must be numeric or identical predicate types, and the
 *		result has type int.  Since we explicitly insert coercions,
 *		two numeric types will be the same size after sign
 *		extension.
 */

static Type
checkRelational(Expression *&left, Expression *&right, const string &op)
{
    const Type &t1 = extend(left, right->type());
    const Type &t2 = extend(right, left->type());
    Type result = error;


    if (t1 != error && t2 != error) {
	if (t1 == t2 && t1.isPredicate())
	    result = integer;
	else
	    report(invalid_operands, op);
    }

    return result;
}


/*
 * Function:	checkLessThan
 *
 * Description:	Check a less-than expression: left < right.
 */

Expression *checkLessThan(Expression *left, Expression *right)
{
    Type t = checkRelational(left, right, "<");
    return new LessThan(left, right, t);
}


/*
 * Function:	checkGreaterThan
 *
 * Description:	Check a greater-than expression: left > right.
 */

Expression *checkGreaterThan(Expression *left, Expression *right)
{
    Type t = checkRelational(left, right, ">");
    return new GreaterThan(left, right, t);
}


/*
 * Function:	checkLessOrEqual
 *
 * Description:	Check a less-than-or-equal expression: left <= right.
 */

Expression *checkLessOrEqual(Expression *left, Expression *right)
{
    Type t = checkRelational(left, right, "<=");
    return new LessOrEqual(left, right, t);
}


/*
 * Function:	checkGreaterOrEqual
 *
 * Description:	Check a greater-than-or-equal expression: left >= right.
 */

Expression *checkGreaterOrEqual(Expression *left, Expression *right)
{
    Type t = checkRelational(left, right, ">=");
    return new GreaterOrEqual(left, right, t);
}


/*
 * Function:	checkEquality
 *
 * Description:	Check an equality expression: the types of both operands
 *		must be compatible, and the result has type int.
 */

static Type
checkEquality(Expression *&left, Expression *&right, const string &op)
{
    const Type &t1 = extend(left, right->type());
    const Type &t2 = extend(right, left->type());
    Type result = error;


    if (t1 != error && t2 != error) {
	if (t1.isCompatibleWith(t2))
	    result = integer;
	else
	    report(invalid_operands, op);
    }

    return result;
}


/*
 * Function:	checkEqual
 *
 * Description:	Check an equality expression: left == right.
 */

Expression *checkEqual(Expression *left, Expression *right)
{
    Type t = checkEquality(left, right, "==");
    return new Equal(left, right, t);
}


/*
 * Function:	checkNotEqual
 *
 * Description:	Check an inequality expression: left != right.
 */

Expression *checkNotEqual(Expression *left, Expression *right)
{
    Type t = checkEquality(left, right, "!=");
    return new NotEqual(left, right, t);
}


/*
 * Function:	checkLogical
 *
 * Description:	Check a logical-or or logical-and expression: the types of
 *		both operands must be predicate types and the result has
 *		type int.
 */

static Type
checkLogical(Expression *&left, Expression *&right, const string &op)
{
    const Type &t1 = extend(left, right->type());
    const Type &t2 = extend(right, left->type());
    Type result = error;


    if (t1 != error && t2 != error) {
	if (t1.isPredicate() && t2.isPredicate())
	    result = integer;
	else
	    report(invalid_operands, op);
    }

    return result;
}


/*
 * Function:	checkLogicalAnd
 *
 * Description:	Check a logical-and expression: left && right.
 */

Expression *checkLogicalAnd(Expression *left, Expression *right)
{
    Type t = checkLogical(left, right, "&&");
    return new LogicalAnd(left, right, t);
}


/*
 * Function:	checkLogicalOr
 *
 * Description:	Check a logical-or expression: left || right.
 */

Expression *checkLogicalOr(Expression *left, Expression *right)
{
    Type t = checkLogical(left, right, "||");
    return new LogicalOr(left, right, t);
}


/*
 * Function:	checkAssignment
 *
 * Description:	Check an assignment statement: the left operand must be an
 *		lvalue and the types of the operands must be compatible.
 */

Statement *checkAssignment(Expression *left, Expression *right)
{
    const Type &t1 = left->type();
    const Type &t2 = convert(right, left->type());


    if (t1 != error && t2 != error) {
	if (!left->lvalue())
	    report(invalid_lvalue);

	else if (!t1.isCompatibleWith(t2))
	    report(invalid_operands, "=");
    }

    return new Assignment(left, right);
}


/*
 * Function:	checkReturn
 *
 * Description:	Check a return statement: the type of the expression must
 *		be compatible with the given type, which should be the
 *		return type of the enclosing function.
 */

void checkReturn(Expression *&expr, const Type &type)
{
    const Type &t = convert(expr, type);


    if (t != error && !t.isCompatibleWith(type))
	report(invalid_return);
}


/*
 * Function:	checkTest
 *
 * Description:	Check if the type of the expression is a legal type in a
 * 		test expression in a while, if-then, if-then-else, or for
 * 		statement: the type must be a predicate type.
 */

void checkTest(Expression *&expr)
{
    const Type &t = promote(expr);

    if (t != error && !t.isPredicate())
	report(invalid_test);
}
