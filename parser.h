#ifndef PARSER_H
#define PARSER_H

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <variant>
#include "lexer.h"


//this is here because some operators group with left associativity. ex:  2^3^2 is equal to 512 because it groups as 2^(3^2).
// but if you dont implement associavity it groups as (2^3)^2 which gives the result of 64 which is uncorrect

//fn main(int a,string b,bool c,double d)

enum class Associativity
{
    Left,
    Right
};

//these are default VOID commands.
enum class CommandNames{
    Print,
    PrintLn,
    Free
};

enum class VariableTypes{
    Int,
    Double,
    String,
    Bool
};


struct VariableData {
    std::string name;
    Value value;
    bool isConst;
    bool isStrict;
    bool isGlobal;
    VariableTypes vartype;
};
using VariableTable = std::unordered_map<std::string, VariableData>;
using ScopeStack = std::vector<VariableTable>;
struct Instruction
{
    enum class Types{
        If,
        While,
        For,
        Declare,
        Assign,
        FunctionDeclare,
        FunctionCall,
        Command,
        Return
    };
    Types type;
    std::vector<std::string> path;

    VariableData vardata;

    std::vector<Token> expression; //for vars before computing their values at runtime
    std::vector<std::vector<Token>> arguments;

    std::vector<Token> condition;

    //for functions,if,else,while
    std::vector<Instruction> body;
    //only for if
    std::vector<Instruction> elseBody;
    //for functions
    std::string Funcname;

    std::vector<VariableData> locals;

    bool isStrict;
    bool isVoid;
};


struct GFunction
{
    std::string name;

    std::vector<VariableData> locals;

    std::vector<Instruction> body;

    bool isStrict;
    bool isVoid;
};

using FunctionTable = std::unordered_map<std::string, GFunction>;


std::vector<Instruction> parse(std::vector<Token>);


Value Evaluate(
    const std::vector<Token> &tokens,
    int left,
    int right,
    ScopeStack& scope
);
#endif