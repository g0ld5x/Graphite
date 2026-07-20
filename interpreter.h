#ifndef INTERPRETER_H
#define INTERPRETER_H

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <variant>
#include "parser.h"
#include "lexer.h"


void interpret(const std::vector<Instruction>& input);
#endif