#ifndef CHECKER_H
#define CHECKER_H

#include "scope.h"
#include "lexer.h"
#include <forward_list>

void openScope();
void closeScope();
void defineFunction(const std::string &name, const Type &type);
void declareFunction(const std::string &name, const Type &type);
void declareVariable(const std::string &name, const Type &type);
void checkIdentifier(const std::string &name);


#endif /* CHECKER_H */