#ifndef INTERPRETER_H
#define INTERPRETER_H

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <variant>
#include "parser.h"
#include "lexer.h"

 struct ExecutionResult{
    bool didReturn;
    Value returnValue;
};

void initInterpreter(ScopeStack & scope); //used to run things once without recursion to init the interpreter.

ExecutionResult interpret(const std::vector<Instruction>& input,ScopeStack & scopeStack);
#endif