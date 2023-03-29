/*
 * File:	generator.cpp
 *
 * Description:	This file contains the public and member function
 *		definitions for the code generator for Simple C.
 *
 *		Extra functionality:
 *		- putting all the global declarations at the end
 */

# include <vector>
# include <cassert>
# include <iostream>
# include "generator.h"
# include "machine.h"
# include "Tree.h"
# include <map>
# include <iterator>
# include "string.h"
# include "tokens.h"

using namespace std;

static int offset;
static string funcname;
static string suffix(Expression *expr);
static ostream &operator <<(ostream &ostr, Expression *expr);

static Register *rax = new Register("%rax", "%eax", "%al");
static Register *rbx = new Register("%rbx", "%ebx", "%bl");
static Register *rcx = new Register("%rcx", "%ecx", "%cl");
static Register *rdx = new Register("%rdx", "%edx", "%dl");
static Register *rsi = new Register("%rsi", "%esi", "%sil");
static Register *rdi = new Register("%rdi", "%edi", "%dil");
static Register *r8 = new Register("%r8", "%r8d", "%r8b");
static Register *r9 = new Register("%r9", "%r9d", "%r9b");
static Register *r10 = new Register("%r10", "%r10d", "%r10b");
static Register *r11 = new Register("%r11", "%r11d", "%r11b");
static Register *r12 = new Register("%r12", "%r12d", "%r12b");
static Register *r13 = new Register("%r13", "%r13d", "%r13b");
static Register *r14 = new Register("%r14", "%r14d", "%r14b");
static Register *r15 = new Register("%r15", "%r15d", "%r15b");

static vector<Register *> parameters = {rdi, rsi, rdx, rcx, r8, r9};
static vector<Register *> registers = {r11, r10, r9, r8, rcx, rdx, rsi, rdi, rax};
static map<string, Label> stringMap;


static void assign(Expression *expr, Register *reg)
{
    if (expr != nullptr) {
        if (expr->_register != nullptr) {
            expr->_register->_node = nullptr;
        }
        expr->_register = reg;
    }
    if (reg != nullptr) {
        if (reg->_node != nullptr) {
            reg->_node->_register = nullptr;
        }
        reg->_node = expr;
    }
}


static void load(Expression *expr, Register *reg)
{
    if (reg->_node != expr) {
        if (reg->_node != nullptr) {
            unsigned size = reg->_node->type().size();
            offset -= size;
            reg->_node->_offset = offset;
            cout << "\tmov" << suffix(reg->_node);
            cout << reg->name(size) << ", ";
            cout << offset << "(%rbp)" << endl;
        }
        if (expr != nullptr) {
            unsigned size = expr->type().size();
            cout << "\tmov" << suffix(expr) << expr;
            cout << ", " << reg->name(size) << endl;
        }
        assign(expr, reg);
    }
}


static Register *getreg()
{
    for (auto reg : registers) {
        if (reg->_node == nullptr)
            return reg;
    }    
    load(nullptr, registers[0]);
    return registers[0];
}


/*
 * Function:	suffix (private)
 *
 * Description:	Return the suffix for an opcode based on the given size.
 */

static string suffix(unsigned long size)
{
    return size == 1 ? "b\t" : (size == 4 ? "l\t" : "q\t");
}


/*
 * Function:	suffix (private)
 *
 * Description:	Return the suffix for an opcode based on the size of the
 *		given expression.
 */

static string suffix(Expression *expr)
{
    return suffix(expr->type().size());
}


/*
 * Function:	align (private)
 *
 * Description:	Return the number of bytes necessary to align the given
 *		offset on the stack.
 */

static int align(int offset)
{
    if (offset % STACK_ALIGNMENT == 0)
	return 0;

    return STACK_ALIGNMENT - (abs(offset) % STACK_ALIGNMENT);
}


/*
 * Function:	operator << (private)
 *
 * Description:	Convenience function for writing the operand of an
 *		expression using the output stream operator.
 */

static ostream &operator <<(ostream &ostr, Expression *expr)
{
    if (expr->_register != nullptr)
	return ostr << expr->_register;

    expr->operand(ostr);
    return ostr;
}


/*
 * Function:	Expression::operand
 *
 * Description:	Write an expression as an operand to the specified stream.
 */

void Expression::operand(ostream &ostr) const
{
    assert(_offset != 0);
    ostr << _offset << "(%rbp)";
}


/*
 * Function:	Identifier::operand
 *
 * Description:	Write an identifier as an operand to the specified stream.
 */

void Identifier::operand(ostream &ostr) const
{
    if (_symbol->_offset == 0)
	ostr << global_prefix << _symbol->name() << global_suffix;
    else
	ostr << _symbol->_offset << "(%rbp)";
}


/*
 * Function:	Number::operand
 *
 * Description:	Write a number as an operand to the specified stream.
 */

void Number::operand(ostream &ostr) const
{
    ostr << "$" << _value;
}


/*
 * Function:	String::operand
 *
 * Description:	Write a label as an operand to the specified stream.
 */
void String::operand(ostream &ostr) const {
    string key = _value;
    if (stringMap.count(key) == 0) {
        Label label;
        stringMap[key] = label;
        ostr << label;
    }
    else {
        Label label = stringMap[key];
        ostr << label;
    }
    
}


/*
 * Function:	Call::generate
 *
 * Description:	Generate code for a function call expression.
 *
 *		On a 64-bit platform, the stack needs to be aligned on a
 *		16-byte boundary.  So, if the stack will not be aligned
 *		after pushing any arguments, we first adjust the stack
 *		pointer.
 *
 *		Since all arguments are 8-bytes wide, we could simply do:
 *
 *		    if (args.size() > 6 && args.size() % 2 != 0)
 *			subq $8, %rsp
 */

void Call::generate()
{
    unsigned numBytes;


    /* Generate code for the arguments first. */ 

    numBytes = 0;

    for (int i = _args.size() - 1; i >= 0; i --)
	_args[i]->generate();


    /* Adjust the stack if necessary */

    if (_args.size() > NUM_PARAM_REGS) {
	numBytes = align((_args.size() - NUM_PARAM_REGS) * SIZEOF_PARAM);

	if (numBytes > 0)
	    cout << "\tsubq\t$" << numBytes << ", %rsp" << endl;
    }


    /* Move the arguments into the correct registers or memory locations. */

    for (int i = _args.size() - 1; i >= 0; i --) {
	if (i >= NUM_PARAM_REGS) {
	    numBytes += SIZEOF_PARAM;
	    load(_args[i], rax);
	    cout << "\tpushq\t%rax" << endl;

	} else
	    load(_args[i], parameters[i]);

	assign(_args[i], nullptr);
    }


    /* Call the function and then reclaim the stack space.  Technically, we
       only need to assign the number of floating point arguments passed in
       vector registers to %eax if the function being called takes a
       variable number of arguments.  But, it never hurts. */

    for (auto reg : registers)
	load(nullptr, reg);

    if (_id->type().parameters() == nullptr)
	cout << "\tmovl\t$0, %eax" << endl;

    cout << "\tcall\t" << global_prefix << _id->name() << endl;

    if (numBytes > 0)
	cout << "\taddq\t$" << numBytes << ", %rsp" << endl;

    assign(this, rax);
}


/*
 * Function:	Block::generate
 *
 * Description:	Generate code for this block, which simply means we
 *		generate code for each statement within the block.
 */

void Block::generate()
{
    for (auto stmt : _stmts) {
	stmt->generate();

	for (auto reg : registers)
	    assert(reg->_node == nullptr);
    }
}


/*
 * Function:	Simple::generate
 *
 * Description:	Generate code for a simple (expression) statement, which
 *		means simply generating code for the expression.
 */

void Simple::generate()
{
    _expr->generate();
    assign(_expr, nullptr);
}


/*
 * Function:	Function::generate
 *
 * Description:	Generate code for this function, which entails allocating
 *		space for local variables, then emitting our prologue, the
 *		body of the function, and the epilogue.
 */

void Function::generate()
{
    int param_offset;
    unsigned size;
    Parameters *params;
    Symbols symbols;


    /* Assign offsets to the parameters and local variables. */

    param_offset = 2 * SIZEOF_REG;
    offset = param_offset;
    allocate(offset);


    /* Generate our prologue. */

    funcname = _id->name();
    cout << global_prefix << funcname << ":" << endl;
    cout << "\tpushq\t%rbp" << endl;
    cout << "\tmovq\t%rsp, %rbp" << endl;
    cout << "\tmovl\t$" << funcname << ".size, %eax" << endl;
    cout << "\tsubq\t%rax, %rsp" << endl;


    /* Spill any parameters. */

    params = _id->type().parameters();
    symbols = _body->declarations()->symbols();

    for (unsigned i = 0; i < NUM_PARAM_REGS; i ++)
	if (i < params->size()) {
	    size = symbols[i]->type().size();
	    cout << "\tmov" << suffix(size) << parameters[i]->name(size);
	    cout << ", " << symbols[i]->_offset << "(%rbp)" << endl;
	} else
	    break;


    /* Generate the body of this function. */

    _body->generate();

    /* Generate our epilogue. */

    cout << endl << global_prefix << funcname << ".exit:" << endl;
    cout << "\tmovq\t%rbp, %rsp" << endl;
    cout << "\tpopq\t%rbp" << endl;
    cout << "\tret" << endl << endl;

    offset -= align(offset - param_offset);
    cout << "\t.set\t" << funcname << ".size, " << -offset << endl;
    cout << "\t.globl\t" << global_prefix << funcname << endl << endl;
}


/*
 * Function:	generateGlobals
 *
 * Description:	Generate code for any global variable declarations.
 */

void generateGlobals(Scope *scope)
{
    const Symbols &symbols = scope->symbols();

    for (auto symbol : symbols) {
        if (!symbol->type().isFunction()) {
            cout << "\t.comm\t" << global_prefix << symbol->name() << ", ";
            cout << symbol->type().size() << endl;
        }
    }
    cout << "\t.data" << endl;
    map<std::string, Label>::iterator it;
    for (it = stringMap.begin(); it != stringMap.end(); ++it) {
        cout << it->second << ":\t" << ".asciz\t" << "\"" << escapeString(it->first) << "\"" << endl;
    }
}


/*
 * Function:	Assignment::generate
 *
 * Description:	Generate code for an assignment statement.
 *
 */

void Assignment::generate()
{
    cout << "######ASSIGNMENT######" << endl;
    Expression *pointer;
    _right->generate();     // generate right
    if (_left->isDereference(pointer)) {
        pointer->generate();
        load(pointer, getreg());
        load(_right, getreg());     // load right into a register
        cout << "\tmov" << suffix(_right) << _right << ", (" << pointer << ")" << endl;      //move right into left
        // Unassign all registers
        assign(pointer, nullptr);
        assign(_right, nullptr);
    }
    else {
        //_left->generate();
        load(_right, getreg());     // load right into a register
        cout << "\tmov" << suffix(_right) << _right << ", " << _left << endl;      //move right into left
        // Unassign all registers
        //assign(_left, nullptr);
        assign(_right, nullptr);
    }
    cout << "######END ASSIGNMENT######" << endl;
}


/*
 * Function:	Add::generate
 *
 * Description:	Generate code for an addition.
 *
 */

void Add::generate()
{
    cout << "######ADD######" << endl;
    // Generate left and right
    _left->generate();
    _right->generate();
    // Load left if not in register
    if (_left->_register == nullptr) {
        load(_left, getreg());
    }
    // Run the operation
    cout << "\tadd" << suffix(_left);
    cout << _right << ", " << _left << endl;
    // Unassign right register
    assign(_right, nullptr);
    // Assign left register to the expression
    assign(this, _left->_register);
    cout << "######END ADD######" << endl;
}


/*
 * Function:	Subtract::generate
 *
 * Description:	Generate code for an subtraction.
 *
 */
void Subtract::generate()
{
    cout << "######SUBTRACT######" << endl;
    // Generate left and right
    _left->generate();
    _right->generate();
    // Load left if not in register
    if (_left->_register == nullptr) {
        load(_left, getreg());
    }
    // Run the operation
    cout << "\tsub" << suffix(_left);
    cout << _right << ", " << _left << endl;
    // Unassign right register
    assign(_right, nullptr);
    // Assign left register to the expression
    assign(this, _left->_register);
    cout << "######END SUBTRACT######" << endl;
}


/*
 * Function:	Add::generate
 *
 * Description:	Generate code for a multiplication.
 *
 */
void Multiply::generate()
{
    cout << "######MULTIPLY######" << endl;
    // Generate left and right
    _left->generate();
    _right->generate();
    // Load left if not in register
    if (_left->_register == nullptr) {
        load(_left, getreg());
    }
    // Run the operation
    cout << "\timul" << suffix(_left);
    cout << _right << ", " << _left << endl;
    // Unassign right register
    assign(_right, nullptr);
    // Assign left register to the expression
    assign(this, _left->_register);
    cout << "######END MULTIPLY######" << endl;
}


/*
 * Function:	Divide::generate
 *
 * Description:	Generate code for the integer result of a division.
 *
 */
void Divide::generate()
{  
    cout << "######DIVIDE######" << endl;
    // Generate left and right
    _left->generate();
    _right->generate();
    // Load left into rax
    load(_left, rax);
    // Unload rdx
    load(nullptr, rdx);
    // Load right into rcx
    load(_right, rcx);
    // Sign extend rax into rdx
        // cltd if result is size 4, else use cqto
    if (_left->type().size() == 4) {
        cout << "cltd" << endl;
    }
    else {
        cout << "cqto" << endl;
    }
    // idiv right with correct suffix
    cout << "\tidiv" << suffix(_left) << _right->_register << endl;
    // Unassign left and right register
    assign(_left, nullptr);
    assign(_right, nullptr);
    // Assign this to rax
    assign(this, rax);
    cout << "######END DIVIDE######" << endl;
}


/*
 * Function:	Remainder::generate
 *
 * Description:	Generate code for the remainder of a division.
 *
 */
void Remainder::generate()
{  
    cout << "######REMAINDER######" << endl;
    // Generate left and right
    _left->generate();
    _right->generate();
    // Load left into rax
    load(_left, rax);
    // Unload rdx
    load(nullptr, rdx);
    // Load right into rcx
    load(_right, rcx);
    // Sign extend rax into rdx
        // cltd if result is size 4, else use cqto
    if (_left->type().size() == 4) {
        cout << "cltd" << endl;
    }
    else {
        cout << "cqto" << endl;
    }
    // idiv right with correct suffix
    cout << "\tidiv" << suffix(_left) << _right->_register << endl;
    // Unassign left and right register
    assign(_left, nullptr);
    assign(_right, nullptr);
    // Assign this to rdx
    assign(this, rdx);
    cout << "######END REMAINDER######" << endl;
}


/*
 * Function:	Lessthan::generate
 *
 * Description:	Generate code for <.
 *
 */
void LessThan::generate() {
    cout << "######LESS THAN######" << endl;
    // Generate left and right
    compare(_left, _right);
    // Store result of condition code in byte register
    cout << "\tsetl\t" << this->_register->byte() << endl;
    // Move zero-extend byte to long
    cout << "\tmovzbl\t" << this->_register->byte() << ", " << this->_register->name(4) << endl;
    cout << "######END LESS THAN######" << endl;
}


/*
 * Function:	GreaterThan::generate
 *
 * Description:	Generate code for >.
 *
 */
void GreaterThan::generate() {
    cout << "######GREATER THAN######" << endl;
    compare(_left, _right);
    // Store result of condition code in byte register
    cout << "\tsetg\t" << this->_register->byte() << endl;
    // Move zero-extend byte to long
    cout << "\tmovzbl\t" << this->_register->byte() << ", " << this->_register->name(4) << endl;
    cout << "######END GREATER THAN######" << endl;
}


/*
 * Function:	LessOrEqual::generate
 *
 * Description:	Generate code for <=.
 *
 */
void LessOrEqual::generate() {
    cout << "######LEQ######" << endl;
    compare(_left, _right);
    // Store result of condition code in byte register
    cout << "\tsetle\t" << this->_register->byte() << endl;
    // Move zero-extend byte to long
    cout << "\tmovzbl\t" << this->_register->byte() << ", " << this->_register->name(4) << endl;
    cout << "######END LEQ######" << endl;
}


/*
 * Function:	GreaterOrEqual::generate
 *
 * Description:	Generate code for >=.
 *
 */
void GreaterOrEqual::generate() {
    cout << "######GEQ######" << endl;
    compare(_left, _right);
    // Store result of condition code in byte register
    cout << "\tsetge\t" << this->_register->byte() << endl;
    // Move zero-extend byte to long
    cout << "\tmovzbl\t" << this->_register->byte() << ", " << this->_register->name(4) << endl;
    cout << "######END GEQ######" << endl;
}


/*
 * Function:	Equal::generate
 *
 * Description:	Generate code for ==.
 *
 */
void Equal::generate() {
    cout << "######EQ######" << endl;
    compare(_left, _right);
    // Store result of condition code in byte register
    cout << "\tsete\t" << this->_register->byte() << endl;
    // Move zero-extend byte to long
    cout << "\tmovzbl\t" << this->_register->byte() << ", " << this->_register->name(4) << endl;
    cout << "######END EQ######" << endl;
}


/*
 * Function:	NotEqual::generate
 *
 * Description:	Generate code for !=.
 *
 */
void NotEqual::generate() {
    cout << "######NEQ######" << endl;
    compare(_left, _right);
    // Store result of condition code in byte register
    cout << "\tsetne\t" << this->_register->byte() << endl;
    // Move zero-extend byte to long
    cout << "\tmovzbl\t" << this->_register->byte() << ", " << this->_register->name(4) << endl;
    cout << "######END NEQ######" << endl;
}


/*
 * Function:	Not::generate
 *
 * Description:	Generate code for !.
 *
 */
void Not::generate() {
    cout << "######NOT######" << endl;
    // Generate expression
    _expr->generate();
    // Load expression
    if (_expr->_register == nullptr) {
        load(_expr, getreg());
    }
    // Compare 0 and expression
    cout << "\tcmp" << suffix(_expr) << "$0, " << _expr << endl;
    cout << "\tsete\t" << _expr->_register->byte() << endl;
    cout << "\tmovzbl\t" << _expr->_register->byte() << ", " << _expr->_register << endl;
    // Assign this to a register
    assign(this, _expr->_register);
    // Unassign expression
    assign(_expr, nullptr);
    cout << "######END NOT######" << endl;
}


/*
 * Function:	Negate::generate
 *
 * Description:	Generate code for -.
 *
 */
void Negate::generate() {
    cout << "######NEGATE######" << endl;
    // Generate expression
    _expr->generate();
    // Load expression
    load(_expr, getreg());
    cout << "\tneg" << suffix(_expr) << _expr->_register << endl;
    // Unassign expression
    assign(_expr, nullptr);
    // Assign this to a register
    assign(this, getreg());
    cout << "######END NEGATE######" << endl;
}


/*
 * Function:	While::generate
 *
 * Description:	Generate code for while.
 *
 */
void While::generate() {
    cout << "######WHILE######" << endl;
    Label loop, exit;
    cout << loop << ":" << endl;
    _expr->test(exit, false);
    _stmt->generate();
    cout << "\tjmp\t" << loop << endl;
    cout << exit << ":" << endl; 
    cout << "######END WHILE######" << endl;
}


/*
 * Function:	Address::generate
 *
 * Description:	Generate code for while.
 *
 */
void Address::generate() {
    cout << "######ADDRESS######" << endl;
    Expression *pointer;

    if (_expr->isDereference(pointer)) {
        pointer->generate();
        if (pointer->_register == nullptr) {
            load(pointer, getreg());
        }
        assign(this, pointer->_register);
    } else {
        assign(this, getreg());
        cout << "\tleaq\t" << _expr << ", " << this << endl;
    }
    cout << "######END ADDRESS######" << endl;
}


/*
 * Function:	Dereference::generate
 *
 * Description:	Generate code for &.
 *
 */
void Dereference::generate() {
    cout << "######DEREFERENCE######" << endl;
    _expr->generate();
    if (_expr->_register == nullptr) {
        load(_expr, getreg());
    }
    cout << "\tmov" << suffix(_expr) << "(" << _expr << ")" << ", " << _expr << endl;
    assign(this, _expr->_register);
    cout << "######END DEREFERENCE######" << endl;
}


/*
 * Function:	Return::generate
 *
 * Description:	Generate code for return statement.
 *
 */
void Return::generate() {
    cout << "######RETURN######" << endl;
    _expr->generate();
    load(_expr, rax);
    cout << "\tjmp\t" << funcname << ".exit" << endl;
    assign(_expr, nullptr);
    cout << "######END RETURN######" << endl;
}


/*
 * Function:	Cast::generate
 *
 * Description:	Generate code when casting happens.
 *
 */
void Cast::generate() {
    cout << "######CAST######" << endl;
    Register *reg;
    unsigned source, target;
    source = _expr->type().size();
    target = _type.size();
    _expr->generate();
    if (source >= target) { // do nothing
        load(_expr, getreg());
        assign(this, _expr->_register);
    }
    else {
        load(_expr, getreg());
        if (source == 1 && target == 4) {
            cout << "\tmovsbl\t" << _expr << ", " << _expr->_register->name(target) << endl;
        }
        else if (source == 1 && target == 8) {
            cout << "\tmovsbq\t" << _expr << ", " << _expr->_register->name(target) << endl;
        }
        else { // source == 4 && target == 8
            assert(source == 4 && target == 8);
            cout << "\tmovslq\t" << _expr << ", " << _expr->_register->name(target) << endl;
        }
        reg = _expr->_register;
        assign(_expr, nullptr);
        assign(this, reg);
    }
    cout << "######END CAST######" << endl;
}


/*
 * Function:	LogicalAnd::generate
 *
 * Description:	Generate code for &&.
 *
 */
void LogicalAnd::generate() {
    cout << "######AND######" << endl;
    Label correct, exit;
    _left->test(exit, false);
    _right->test(exit, false);
    assign(this, getreg());
    cout << "\tmovl\t" << "$1, " << this->_register << endl;
    cout << "\tjmp\t" << correct << endl;
    cout << exit << ":" << endl;
    cout << "\tmovl\t" << "$0, " << this->_register << endl;
    cout << correct << ":" << endl;
    cout << "######END AND######" << endl;
}


/*
 * Function:	LogicalOr::generate
 *
 * Description:	Generate code for ||.
 *
 */
void LogicalOr::generate() {
    cout << "######OR######" << endl;
    Label incorrect, exit;
    _left->test(exit, true);
    _right->test(exit, true);
    assign(this, getreg());
    cout << "\tmovl\t" << "$0, " << this->_register << endl;
    cout << "\tjmp\t" << incorrect << endl;
    cout << exit << ":" << endl;
    cout << "\tmovl\t" << "$1, " << this->_register << endl;
    cout << incorrect << ":" << endl;
    cout << "######END OR######" << endl;
}


/*
 * Function:	For::generate
 *
 * Description:	Generate code for for loop.
 *
 */
void For::generate() {
    cout << "######FOR######" << endl;
    Label loop, exit;
    _init->generate();
    cout << loop << ":" << endl;
    _expr->test(exit, false);
    _stmt->generate();
    _incr->generate();
    cout << "\tjmp\t" << loop << endl;
    cout << exit << ":" << endl; 
    cout << "######END FOR######" << endl;
}


/*
 * Function:	If::generate
 *
 * Description:	Generate code for if statement.
 *
 */
void If::generate() {
    cout << "######IF######" << endl;
    Label skip, exit;
    _expr->test(skip, false);
    _thenStmt->generate();
    
    if (_elseStmt != nullptr) {
        cout << "\tjmp\t" << exit << endl;
        cout << skip << ":" << endl;
        _elseStmt->generate();
        cout << exit << ":" << endl;
    }
    else {
        cout << skip << ":" << endl;
    }
    
    cout << "######END IF######" << endl;
}


/*
 * Function:	Expression::test
 *
 * Description:	Simplify code for conditional expressions
 *
 */
void Expression::test(const Label &label, bool ifTrue) {
    generate();
    if (_register == nullptr) {
        load(this, getreg());
    }
    cout << "\tcmp" << suffix(this) << "$0, " << this << endl;
    cout << (ifTrue ? "\tjne\t" : "\tje\t") << label << endl;
    assign(this, nullptr);
}


void Expression::compare(Expression *_left, Expression *_right) {
    // Generate left and right
    _left->generate();
    _right->generate();
    // Load left
    load(_left, getreg());
    // Compare left and right
    cout << "\tcmp" << suffix(_left) << _right << ", " << _left->_register << endl;
    // Unassign left and right
    assign(_left, nullptr);
    assign(_right, nullptr);
    // Assign this to a register
    assign(this, getreg());
}