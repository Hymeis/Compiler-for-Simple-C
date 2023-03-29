#include "scope.h"

using namespace std;

Scope::Scope(Symbols symbols, Scope* next = nullptr):_symbols(symbols),_next(next)
{

}