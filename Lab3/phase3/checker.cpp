#include "checker.h"
#include "tokens.h"
#include <iostream>
#include <cstdlib>


using namespace std; 

forward_list<Scope> scopes; // linkedlist of scopes
const string E1 = "redefinition of '%s'";
const string E2 = "conflicting types for '%s'";
const string E3 = "redeclaration of '%s'";
const string E4 = "'%s' undeclared";
const string E5 = "'%s' has type void";

void init() {
    
}

void openScope(){
    //cout << "open scope" << endl;
    Symbols symbols;
    Scope* next = scopes.empty() ? nullptr : &scopes.front();
    Scope scope(symbols, next);
    scopes.emplace_front(scope);
}

void closeScope(){
    //cout << "close scope" << endl;
    scopes.pop_front();
}

/**
 * Check for redefinition (type's parameters is not a nullptr)
 * Check for conflicting declarations (types must be identical)
 * Always insert newest definition (in GLOBAL scope)
 *  -- the newest function definition always replaces any prev definition or declaration
 * We can define a function without any declaration.
 */
void defineFunction(const std::string &name, const Type &type){
    //cout << name << ": " << type << endl;
    Scope* secondLevel = &scopes.front();
    Scope* global = secondLevel->next();
    Symbol* symbol = global->find(name);
    // while (curr->lookup() != nullptr) {
        // if name is the same:
            // if type is the same:
                // if parameters' types are the same, in order"
                    // report ---RE-DEF'---
            // else report ---CONFLICTING DECL'---
    //}
    if (symbol != nullptr) {
        if (symbol->getType() == type) { /* everything is the same, or previous is a declaration */
            if (symbol->getType().getParameters()) { // is not a declaration
                report(E1, name);  // redefinition of functions
            }
        }
        else {
            report(E2, name); // conflicting types
        }
        global->remove(name);
    }
    global->insert(new Symbol(name, type));
}

/**
 *  Functions are always declared globally.
 *  Check for conflicting declarations (types must be identical)
 *  
 */
void declareFunction(const std::string &name, const Type &type){
    //cout << name << ": " << type << endl;
    Scope* global = &scopes.front();
    Symbol* symbol = global->find(name);
    if (symbol == nullptr) {
        global->insert(new Symbol(name, type));
    }
    else if (symbol->getType() != type) {
        report(E2, name); // conflicting type (for declaring functions)
    }
}


void declareVariable(const std::string &name, const Type &type){
    //cout << name << ": " << type << endl;
    if (type.getSpecifier() == VOID && type.getIndirection() == 0) {
        report(E5, name); // illegal void declaration
    }
    Scope* current = &scopes.front();
    Symbol* symbol = current->find(name);
    if (symbol == nullptr) {
        current->insert(new Symbol(name, type));
    }
    else if (current->next() != nullptr) {
        report(E3, name); // redeclaration
    }
    else if (symbol->getType() != type) {
        report(E2, name); // conflicting type
    }
}

void checkIdentifier(const std::string &name){
    Scope* current = &scopes.front();
    Symbol* symbol = current->lookup(name);
    if (symbol == nullptr) {
        report(E4, name);
    }
}