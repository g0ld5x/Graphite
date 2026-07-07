#ifndef PARSER_H
#define PARSER_H

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <variant>
#include "lexer.h"



//this parser has no ast tree, it basically solves an expression part by part until there is no precedence inequality left. then the interpreter can execute it in a straight line and still be correct. 

//there is no tree structure, everything is in a straight line so for example its kinda like this for writeln(1+1):
//Program-Command(writeln,Int(32))
//or for this a = 32;
//Declare(Name(a),Int(32))
//another example c = a+2 connecting with previous example
//becomes: Declare(Name(c),Int(34))
//and then if you do c=3 that becomes:
//Assign()
//basically the parsers job is to "simplify" the lexers input as much as possible.


//this is here because some operators group with left associativity. ex:  2^3^2 is equal to 512 because it groupes as 2^(3^2). but if you dont implement associavity it groupes as (2^3)^2 which gives the result of 64 which is uncorrect
enum class Associativity
{
    Left,
    Right
};

//these are default VOID commands.
enum class CommandNames{
    WriteLn,
    Write,
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
    VariableTypes vartype;
};
using VariableTable = std::unordered_map<std::string, VariableData>;


struct Instruction {
        enum class Type {
        BuiltIn,
        UserDefined,
        NonDefined,
        If,
        While,
        For,
        Declare,
        Assign
    }; 
    Type type;
    bool isVoid;
    VariableTypes returnType;
    CommandNames builtin;     // valid if BuiltIn
    VariableData vardata;

    std::vector<Instruction> body; //for if conditions that require runtime variables, for and while loops
    std::vector<Token> condition; //for if conditions that require runtime variables, for and while loops

    std::vector<Value> args;
};

struct Function { //for usermade functions
    std::string name;
    bool isVoid;
    bool isStrict; //does the function allow returns with type inferring, ex:  strict fn hi(){ if(condition){return "hi";} return 2;} will give an error. but fn hello(){if(condition){return "hi"} return 3;} will not.
    VariableTypes returnType;
    Value returnVal;
    std::vector<Instruction> instructions;
};
std::vector<Instruction> parse(std::vector<Token>);

Value Evaluate(const std::vector<Token>& tokens, int left, int right);
#endif