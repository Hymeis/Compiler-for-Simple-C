# include <iostream>
# include "tokens.h"
# include "lexer.h"

using namespace std;

int lookahead;

void translationUnit();
void globalDeclaration();
void globalDeclaratorList();
void globalDeclarator();
void functionOrGlobal();
void pointers();
void specifier();
void functionDefinition();
void parameters();
void parameterList();
void parameter();
void declarations();
void declaration();
void declaratorList();
void declarator();
void statements();
void statement();
void assignment();
void expression();
// Levels of expression
void A();
void B();
void C();
void D();
void E();
void F();
void G();
void H();
// 
void expressionList();

void error() {
	report("Error!",yytext);
	exit(1);
}


void match(int t) {
	if (lookahead == t) {
		lookahead = yylex();
	}
	else {
		error();
	}
}




int main()
{
	while ((lookahead = yylex()) != Done) {
		translationUnit();
	}
}

/*
ε
| global-declaration translation-unit
| function-definition translation-unit
*/
void translationUnit() {
	functionOrGlobal();
	translationUnit();
}

void globalDeclaration() { // specifier global-declarator-list ;
	specifier();
	globalDeclaratorList();
	match(SEMICOLON);
}


/**
global-declarator | global-declarator , global-declarator-list
*/
void globalDeclaratorList() {
	globalDeclarator();
	if (lookahead == COMMA) {
		match(COMMA);
		globalDeclaratorList();
	}
}


/**
pointers id
| pointers id ( )
| pointers id [ num ]
*/
void globalDeclarator() {
	pointers();
	match(ID);
	if (lookahead == '(') { // ()
		match('(');
		match(')');
	}
	else if (lookahead == '['){ // []
		match('[');
		match(NUM);
		match(']');
	}
	else {
		// do nothing
	}
}

void remainingDecls() {
	if (lookahead == ';') {
		match(';');
	}
	else { // ,
		match(COMMA);
		globalDeclarator();
		remainingDecls();
	}
}

void fogHelper() {
	if (lookahead == '(') {
		match('(');
		if (lookahead == ')') {
			match(')');
			remainingDecls();
		}
		else {
			parameters();
			match(')');
			match('{');
			declarations();
			statements();
			match('}');
		}
	}
	else if (lookahead == '[') {
		match('[');
		match(NUM);
		match(']');
		remainingDecls();
	}
	else { // remaining decl's --> ; or ,
		remainingDecls();
	}
}

void functionOrGlobal() {
	specifier();
	pointers();
	match(ID);
	fogHelper();
}



/*
ε | * pointers
*/
void pointers() {
	if (lookahead == '*') {
		match('*');
		pointers();
	}

}


/*
int | char | long | void
*/
void specifier() {
	if (lookahead == INT) {
		match(INT);
	}
	else if(lookahead == CHAR) {
		match(CHAR);
	}
	else if (lookahead == LONG) {
		match(LONG);
	}
	else { // void
		match(VOID);
	}
}


/**
specifier pointers id ( parameters ) { declarations statements }
*/
void functionDefinition() {
	specifier();
	pointers();
	match(ID);
	match('(');
	parameters();
	match(')');
	match('{');
	declarations();
	statements();
	match('}');
}


/*
void | parameter-list
*/
void parameters() {
	if (lookahead == VOID) {
		match(VOID);
		if (lookahead != ')') { // void is the specifier i.e part of the parameter --> void pointers id [, parametersList]+
			pointers();
			match(ID);
			if (lookahead == ',') {
				match(',');
				parameterList();
			}
		}
	}
	else {
		parameterList();
	}
}


/*
parameter | parameter , parameter-list
*/
void parameterList() {
	parameter();
	if (lookahead == COMMA) {
		match(COMMA);
		parameterList();
	}
}


/*
specifier pointers id
*/
void parameter() {
	specifier();
	pointers();
	match(ID);
}


/*
ε | declaration declarations
*/
void declarations() {
	if (lookahead == INT || lookahead == CHAR || lookahead == LONG || lookahead == VOID) {
		declaration();
		declarations();
	}
}


/*
specifier declarator-list ;
*/
void declaration() {  // Checked
	specifier();
	declaratorList();
	match(';');
}


/*
declarator | declarator , declarator-list
*/
void declaratorList() {  // Checked
	declarator();
	if (lookahead == COMMA) {
		match(COMMA);
		declaratorList();
	}
}


/*
pointers id | pointers id [ num ]
*/
void declarator() {
	pointers();
	match(ID);
	if (lookahead == '[') {
		match('[');
		match(NUM);
		match(']');
	}
}


/*
ε | statement statements
*/
void statements() {
	if (lookahead != '}') { // if empty, statements always ends with }
		statement();
		statements();
	}
}


/*
{ declarations statements }
| return expression ;
| while ( expression ) statement
| for ( assignment ; expression ; assignment ) statement
| if ( expression ) statement
| if ( expression ) statement else statement
| assignment ;
*/
void statement() {
	if (lookahead == '{') { // { declarations statements }
		match('{');
		declarations();
		statements();
		match('}');
	}
	else if (lookahead == RETURN) { // return expression ;
		match(RETURN);
		expression();
		match(';');
	}
	else if (lookahead == WHILE) { // while ( expression ) statement
		match(WHILE);
		match('(');
		expression();
		match(')');
		statement();
	}
	else if (lookahead == FOR) { // for ( assignment ; expression ; assignment ) statement
		match(FOR);
		match('(');
		assignment();
		match(';');
		expression();
		match(';');
		assignment();
		match(')');
		statement();
	}
	else if (lookahead == IF) { // if ( expression ) statement
		match(IF);
		match('(');
		expression();
		match(')');
		statement();
		if (lookahead == ELSE) { // else statement
			match(ELSE);
			statement();
		}
	}
	else { // assignment ;
		assignment();
		match(';');
	}
}


/*
expression = expression | expression
*/
void assignment() {
	expression();
	if (lookahead == ASSIGN) {
		match(ASSIGN);
		expression();
	}
}


/*
expression || expression
| expression && expression
| expression == expression
| expression != expression
| expression <= expression
| expression >= expression
| expression < expression
| expression > expression
| expression + expression
| expression - expression
| expression * expression
| expression / expression
| expression % expression
| ! expression
| - expression
| & expression
| * expression
| sizeof expression
| expression [ expression ]
| id ( expression-list )
| id ( )
| id
| num
| string
| character
| ( expression )


LEVELS OF PRECEDENCE

[] 
& * ! - sizeof    //right-associative prefixes
* / % 
+ - 
< > <= >= 
== != 
&& 
|| 
*/

/**
exp-->A: ||
*/
void expression() {
	A();
	while (lookahead == OR) {
		match(OR);
		A();
		cout << "or" << endl;
	}
}


/*
A-->B: &&
*/
void A() {
	B();
	while(lookahead == AND) {
		match(AND);
		B();
		cout << "and" << endl;
	}
}

/*
b-->c: == !=
*/
void B() {
	C();
	while (true) {
		if (lookahead == EQL) {
			match(lookahead);
			C();
			cout << "eql" << endl;
		}
		else if (lookahead == NEQ) {
			match(lookahead);
			C();
			cout << "neq" << endl;
		}
		else {
			break;
		}
	}
}

/*
C-->D: < > <= >=
*/
void C() {
	D();
	while (true) {
		if (lookahead == LEQ) {
			match(lookahead);
			D();
			cout << "leq" << endl;
		}
		else if (lookahead == GEQ) {
			match(lookahead);
			D();
			cout << "geq" << endl;
		}
		else if (lookahead == LT) {
			match(lookahead);
			D();
			cout << "ltn" << endl;
		}
		else if (lookahead == GT) {
			match(lookahead);
			D();
			cout << "gtn" << endl;
		}
		else {
			break;
		}
	}
}


/*
D-->E: + -
*/
void D() {
	E();
	while (true) {
		if (lookahead == ADD) {
			match(lookahead);
			E();
			cout << "add" << endl;
		}
		else if (lookahead == SUB) {
			match(lookahead);
			E();
			cout << "sub" << endl;
		}
		else {
			break;
		}
	}
}


/*
E-->F: * / %
*/
void E() {
	F();
	while (true) {
		if (lookahead == MUL) {
			match(lookahead);
			F();
			cout << "mul" << endl;
		}
		else if (lookahead == DIV) {
			match(lookahead);
			F();
			cout << "div" << endl;
		}
		else if (lookahead == MOD) {
			match(lookahead);
			F();
			cout << "rem" << endl;
		}
		else {
			break;
		}
	}
}


/*
F-->G: & * ! - sizeof // prefixes
*/
void F() {
	if (lookahead == '&') {
		match('&');
		F();
		cout << "addr" << endl;
	}
	else if (lookahead == '*') {
		match('*');
		F();
		cout << "deref" << endl;
	}
	else if (lookahead == NOT) {
		match(NOT);
		F();
		cout << "not" << endl;
	}
	else if (lookahead == '-') {
		match('-');
		F();
		cout << "neg" << endl;
	}
	else if (lookahead == SIZEOF){ // sizeof
		match(SIZEOF);
		F();
		cout << "sizeof" << endl;
	}
	else {
		G();
	}
}


/*
G ::= G[expression] | H
*/
void G() {
	H();
	while(lookahead == '[') {
		match('[');
		expression();
		match(']');
		cout << "index" << endl;
	}
}

void H() {
	if (lookahead == ID) {
		match(ID);
		if (lookahead == '(') {
			match('(');
			if (lookahead != ')') {
				expressionList();
			}
			match(')');
		}
		else {
			// do nothing
		}
	}
	else if (lookahead == NUM) {
		match(NUM);
	}
	else if (lookahead == STRING) {
		match(STRING);
	}
	else if (lookahead == CHARACTER){
		match(CHARACTER);
	}
	else if (lookahead == '('){ // ( exp )
		match('(');
		expression();
		match(')');
	}
}


/*
expression | expression , expression-list
*/
void expressionList() {
	expression();
	if (lookahead == COMMA) {
		match(COMMA);
		expressionList();
	}
}
