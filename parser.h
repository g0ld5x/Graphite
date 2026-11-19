#ifndef PARSER_H
#define PARSER_H

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <variant>
#include "lexer.h"
#include <type_traits>
#include <utility>


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
    Bool,
    Array,
    Null,
    AllAccept
};



struct VariableData {
    std::string name;
    Value value;
    bool isConst;
    bool isStrict;
    bool isGlobal;
    VariableTypes vartype;
};

struct TokenRange
{
    const std::vector<Token>* tokens;
    size_t start;
    size_t end;

    const Token& operator[](size_t index) const
    {
        return (*tokens)[start + index];
    }

    size_t size() const
    {
        if (tokens == nullptr || start > end)
            return 0;

        return end - start + 1;
    }

    TokenRange subrange(size_t newStart, size_t newEnd) const
    {
        return {
            tokens,
            start + newStart,
            start + newEnd
        };
    }
};
using VariableTable = std::unordered_map<std::string, VariableData>;
using ScopeStack = std::vector<VariableTable>;



struct Instruction;

using InstructionList = std::vector<Instruction>;

//now here i actually seperate the instructions instead of putting all their values and types into one instruction struct with a type. Now i seperate them into seperate structs and combine them with std::variant. I didnt choose virtual functions because they are lame. its 4:41 rn

struct IfInstruction {
    std::vector<Token> condition;
    InstructionList body;
    InstructionList elseBody;
};

struct WhileInstruction {
    std::vector<Token> condition;
    InstructionList body;
};

struct ForInstruction {
    std::vector<Token> iterateable;
    InstructionList body;
    std::string object;
};

struct DeclareInstruction {
    VariableData vardata;
    std::vector<Token> expression;
};

struct AssignInstruction {
    std::vector<std::string> path;
    VariableData vardata;
    std::vector<Token> expression;
    std::vector<std::vector<Token>> indexExpressions;
};

struct FunctionDeclareInstruction {
    std::string Funcname;
    std::vector<VariableData> locals;
    InstructionList body;
    bool isStrict;
    bool isVoid;
};

struct FunctionCallInstruction {
    std::vector<std::string> path;
    std::string Funcname;
    std::vector<std::vector<Token>> arguments;
};

struct CommandInstruction {
    std::string object;
    std::vector<TokenRange> arguments;
};

struct ReturnInstruction {
    std::vector<Token> expression;
};

struct UseInstruction {
    std::vector<std::string> importPath;
};

struct SpaceInstruction {

    std::vector<Instruction> body;
};
struct BreakInstruction {};
struct ContinueInstruction {};
struct EmptyInstruction {};
using InstructionData = std::variant<
    EmptyInstruction,
    IfInstruction,
    WhileInstruction,
    ForInstruction,
    DeclareInstruction,
    AssignInstruction,
    FunctionDeclareInstruction,
    FunctionCallInstruction,
    CommandInstruction,
    ReturnInstruction,
    SpaceInstruction,
    UseInstruction,
    BreakInstruction,
    ContinueInstruction
>;

struct Instruction {
    InstructionData data;

    Instruction()
        : data(EmptyInstruction{}) {}

    template<
        typename T,
        typename = std::enable_if_t<
            !std::is_same_v<std::decay_t<T>, Instruction>
        >
    >
    Instruction(T&& value)
        : data(std::forward<T>(value)) {}
};


template<typename T>
T* getInstructionData(Instruction* instruction) {
    if (!instruction)
        return nullptr;

    return std::get_if<T>(&instruction->data);
}

template<typename T>
const T* getInstructionData(const Instruction* instruction) {
    if (!instruction)
        return nullptr;

    return std::get_if<T>(&instruction->data);
}

template<typename T>
bool isInstruction(const Instruction* instruction) {
    return getInstructionData<T>(instruction) != nullptr;
}

struct GFunction
{
    std::string name;

    std::vector<VariableData> locals;

    std::vector<Instruction> body;

    bool isStrict;
    bool isVoid;
};

using FunctionTable = std::unordered_map<std::string, GFunction>;


std::vector<Instruction> parse(const std::vector<Token> & input,std::vector<std::string> & path);

Value ExecuteFunction(const std::string& name,
                   const std::vector<Value>& args);

Value Evaluate(
    const TokenRange& tokens,
    ScopeStack& scope,
    FunctionTable& functionTable);
#endif