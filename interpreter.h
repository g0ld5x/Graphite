#ifndef INTERPRETER_H
#define INTERPRETER_H

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <variant>
#include "parser.h"
#include "lexer.h"

void initInterpreter(); //used to run things once without recursion to init the interpreter.

bool interpret(const std::vector<Instruction>& input);
#endif