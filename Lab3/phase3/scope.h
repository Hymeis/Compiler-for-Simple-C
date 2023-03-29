#ifndef SCOPE_H
#define SCOPE_H

#include <string>
#include <vector>
#include "symbol.h"
#include <iostream>
#include <algorithm>

typedef std::string string;
typedef std::vector<class Symbol*> Symbols; // One scope
//typedef std::forward_list<Symbols> Scopes;

using namespace std;

class Scope {
    Symbols _symbols; // a vector of pointers to current symbols/scope
    Scope* _next; // a pointer to the next set of symbols/scope
public:
    Scope();
    Scope(Symbols symbols, Scope* next);
    void insert(Symbol *symbol) {_symbols.push_back(symbol);}
    void remove(const string &name) {
        _symbols.erase(remove_if(_symbols.begin(), _symbols.end(), [&](Symbol* symbol) {return symbol->getName() == name;}));
    }

    /*
     * Return the first symbol pointer with the same name within the current scope. If there is no such symbol, return nullptr.
     */
    Symbol *find(const string &name) const {
        auto iter = find_if(_symbols.begin(), _symbols.end(), [&](Symbol* symbol) {return symbol->getName() == name;});
        return iter == _symbols.end() ? nullptr : *iter;
    }
    Symbol *lookup(const string &name) const {
        Symbol* curr = this->find(name);
        if (curr != nullptr) return curr;
        else if (_next == nullptr) return nullptr;
        else return _next->lookup(name);
    }
    Scope *next() {
        return _next;
    }
};



#endif /* SCOPE_H */