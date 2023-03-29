# include "Tree.h"
# include "generator.h"
# include <iostream>

using namespace std;

static ostream &operator<<(ostream &ostr, Expression *expr);


/*
 * Function:	Block::generate
 *
 * Description:	Generate each statement in te block's vector of Statement pointers.
 */
void Block::generate() {
    for (auto stmt : _stmts) {
        stmt->generate();
    }
}


/*
 * Function:	Simple::generate
 *
 * Description:	Generate code for a Simple object
 */
void Simple::generate() {
    _expr->generate();
}


/*
 * Function:	Function::generate
 *
 * Description:	Generate code for a function, including prologue, body, and epilogue
 */
void Function::generate() {
    int offsetCounter = 0;
    for (auto symbol : _body->declarations()->symbols()) {
        offsetCounter -= symbol->type().size();
        symbol->_offset = offsetCounter;
    }
    while (offsetCounter % 16 != 0) {
        offsetCounter--;
    }
    cout << _id->name() << ":";
    // Generate the function prologue
    cout << "pushq %rbp" << endl;
    cout << "movq %rsp, %rbp" << endl;
    cout << "subq $" << -offsetCounter << ", %rsp" << endl; // subq $<offsetCounter>, %rsp
        // Spill parameters -- assume parameters().size() <= 6
    string registers[6] = {"%edi", "%esi", "%edx", "%ecx", "%r8d", "%r9d"};
    int size = _id->type().parameters()->size();
    int idx = 0;
    for (auto symbol : _body->declarations()->symbols()) {
        if (idx < size) {
            cout << "movl " << registers[idx++] << ", " << symbol->_offset << "(%rbp)" << endl;
        }
        else break;
    }
    // Generate the function body
    _body->generate();
    // Generate the function epilogue
    cout << "movq %rbp, %rsp" << endl;
    cout << "popq %rbp" << endl;
    cout << "ret" << endl;
    // Output the .globl directive
    cout << ".globl " << _id->name() << endl; 
}


/*
 * Function:	Assignment::generate
 *
 * Description:	Generate code for assignments.
 */
void Assignment::generate() {
    cout << "movl " << _right <<", " << _left << endl;
}


/*
 * Function:	Call::generate
 *
 * Description:	Generate code for function calls.
 */
void Call::generate() {
    string registers[6] = {"%edi", "%esi", "%edx", "%ecx", "%r8d", "%r9d"};
    int i = 0;
    for (auto arg : _args) {
        cout << "movl " << arg << ", " << registers[i++] << endl;
    }
    cout << "call " << _id->name() << endl;
}


static ostream &operator<<(ostream &ostr, Expression *expr) {
    expr->operand(ostr);
    return ostr;
}


void Expression::operand(ostream& ostr) const {}


void Number::operand(ostream& ostr) const {
    ostr << "$" << _value;
}


void Identifier::operand(ostream& ostr) const {
    int offset = _symbol->_offset;
    if (offset == 0) { // Identifier is global
        ostr << _symbol->name();
    }
    else {
        ostr << offset << "(%rbp)";
    }
}


/**
* Generate code for global variables
*/
void generateGlobals(Scope *scope) {
    for (auto symbol : scope->symbols()) {
        Type t = symbol->type();
        if (!t.isFunction()) {
            cout << ".comm " << symbol->name() << ", " << t.size() << endl;
        }
    }
}


