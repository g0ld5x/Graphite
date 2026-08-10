#include "parser.h"
#include "interpreter.h"
#include <string>
#include <vector>
#include <memory>
#include <numbers>
#include <format>
#include <algorithm>
#include <variant>
#include "lexer.h"
#include <iostream>
#include <math.h>
#include <cmath>
#include <sstream>
#include <climits>
// here we goo



std::string joinpath(
    const std::vector<std::string> &elements,
    const std::string &delimiter)
{
    std::string result;

    for (const auto &element : elements)
    {
        if (element.empty())
            continue;

        if (!result.empty())
            result += delimiter;

        result += element;
    }

    return result;
}
bool hold_same_type(const Value &v1, const Value &v2)
{
    // if the indices are equal, the active types are the same
    return v1.index() == v2.index();
}

bool stringToBool(const std::string &str)
{
    if (str == "true")
        return true;
    else if (str == "false")
        return false;
    return false;
}

double variantToDouble(const Value &value)
{
    if (const auto *p = std::get_if<double>(&value))
        return *p;

    if (const auto *p = std::get_if<int>(&value))
        return static_cast<double>(*p);

    if (const auto *p = std::get_if<bool>(&value))
        return *p ? 1.0 : 0.0;

    if (const auto *p = std::get_if<std::string>(&value))
    {
        try
        {
            return std::stod(*p);
        }
        catch (...)
        {
            return 0.0;
        }
    }

    return 0.0;
}

int variantToInt(const Value &value)
{
    if (const auto *p = std::get_if<int>(&value))
        return *p;

    if (const auto *p = std::get_if<double>(&value))
        return static_cast<int>(*p);

    if (const auto *p = std::get_if<bool>(&value))
        return *p ? 1 : 0;

    if (const auto *p = std::get_if<std::string>(&value))
    {
        try
        {
            return std::stoi(*p);
        }
        catch (...)
        {
            return 0;
        }
    }

    return 0;
}
VariableTypes getType(const Value &value)
{
    return std::visit([](auto &&arg) -> VariableTypes
                      {
                          using T = std::decay_t<decltype(arg)>;

                          if constexpr (std::is_same_v<T, int>)
                              return VariableTypes::Int;
                          else if constexpr (std::is_same_v<T, double>)
                              return VariableTypes::Double;
                          else if constexpr (std::is_same_v<T, bool>)
                              return VariableTypes::Bool;
                          else if constexpr (std::is_same_v<T, std::string>)
                              return VariableTypes::String;
                          else if constexpr (std::is_same_v<T, std::monostate>)
                              return VariableTypes::Null;

                          else if constexpr (std::is_same_v<T, ArrayPtr>)
                              return VariableTypes::Array;
                      },
                      value);
}

int precedence(TokenType type)
{
    switch (type)
    {

    case TokenType::Equals:
        return 0;

    case TokenType::OrOr:
        return 1;

    case TokenType::AndAnd:
        return 2;

    case TokenType::EqualEqual:
    case TokenType::NotEqual:
    case TokenType::TypeEquality:
        return 3;

    case TokenType::Bigger:
    case TokenType::BiggerEqual:
    case TokenType::Smaller:
    case TokenType::SmallerEqual:
        return 4;

    case TokenType::Plus:
    case TokenType::Minus:
        return 5;

    case TokenType::Multiply:
    case TokenType::Division:
    case TokenType::Remainder:
        return 6;

    case TokenType::Not:

        return 7;

    case TokenType::Power:
        return 8;

    default:
        return -1;
    }
}

std::string variantToString(const Value &a)
{
    return std::visit([](const auto &arg) -> std::string
                      {
                          using T = std::decay_t<decltype(arg)>;

                          if constexpr (std::is_same_v<T, std::monostate>)
                          {
                              return "null";
                          }
                          else if constexpr (std::is_same_v<T, std::shared_ptr<ArrayValue>>)
                          {
                              return arrayToString(arg);
                          }
                          else
                          {
                              std::ostringstream oss;
                              oss << arg;
                              return oss.str();
                          }
                      },
                      a);
}

bool isUnary(const std::vector<Token> &tokens, int index)
{
    TokenType t = tokens[index].type;

    switch (t)
    {
    case TokenType::Not:
        return true;

    case TokenType::Plus:
    case TokenType::Minus:
        break;

    default:
        return false;
    }

    if (index == 0)
        return true;

    TokenType prev = tokens[index - 1].type;

    return prev == TokenType::LParen ||
           prev == TokenType::Comma ||
           precedence(prev) != -1;
}

bool isInVariables(
    const Token& tok,
    const VariableTable& variables)
{
    return variables.find(variantToString(tok.value))
           != variables.end();
}

VariableData *getVariable(std::string name, ScopeStack &scopes)
{
    for (int i = scopes.size() - 1; i >= 0; i--)
    {
        auto it = scopes[i].find(name);

        if (it != scopes[i].end())
        {
            return &it->second;
        }
    }

    return nullptr;
}

bool isInVector(const std::vector<std::string> &vec, const std::string &target)
{
    return std::find(vec.begin(), vec.end(), target) != vec.end();
}

Associativity associativity(TokenType type)
{
    switch (type)
    {
    case TokenType::Power:
    case TokenType::Equals:
        return Associativity::Right;

    default:
        return Associativity::Left;
    }
}
bool variantToBool(const Value &value)
{
    if (const bool *val_ptr = std::get_if<bool>(&value))
    {
        return *val_ptr;
    }
    else
    {
        std::cout << "bad";
        return false;
        // handle error here
    }
}

Value ExecuteFunction(
    const std::string& name,
    const std::vector<TokenRange>& arguments,
    FunctionTable& ftable,
    ScopeStack& scopeStack)
{
    auto it = ftable.find(name);

    if (it == ftable.end())
    {
        std::cerr << "Error: function " << name
                  << " does not exist\n";
        return Value{};
    }

    const GFunction& targetFunc = it->second;

    if (arguments.size() != targetFunc.locals.size())
    {
        std::cerr << "Error: function " << name
                  << " expected "
                  << targetFunc.locals.size()
                  << " arguments but got "
                  << arguments.size()
                  << "\n";

        return Value{};
    }

    VariableTable bufferTable;

    for (size_t k = 0; k < arguments.size(); k++)
    {
        VariableData var = targetFunc.locals[k];

        var.value = Evaluate(
            arguments[k],
            scopeStack,
            ftable
        );

        bufferTable[var.name] = std::move(var);
    }

    scopeStack.push_back(std::move(bufferTable));

    const ExecutionResult& result =
        interpret(targetFunc.body, scopeStack, ftable);

    scopeStack.pop_back();

    if (result.didReturn)
    {
        return result.returnValue;
    }

    return Value{};
}

std::string arrayToString(const std::shared_ptr<ArrayValue> &array)
{
    std::string result = "[";

    for (size_t i = 0; i < array->values.size(); i++)
    {
        result += variantToString(array->values[i]);

        if (i != array->values.size() - 1)
            result += ",";
    }

    result += "]";

    return result;
}

Value Evaluate(
    const TokenRange& tokens,
    ScopeStack& scope,
    FunctionTable& functionTable)
{
    if (tokens.size() == 0)
        throw std::runtime_error("Cannot evaluate empty expression");


    if (tokens[0].type == TokenType::Identifier)
    {
        int parenIndex = -1;

        // Find opening parenthesis after function path
        for (size_t i = 0; i < tokens.size(); ++i)
        {
            if (tokens[i].type == TokenType::LParen)
            {
                parenIndex = static_cast<int>(i);
                break;
            }

            if (tokens[i].type != TokenType::Identifier &&
                tokens[i].type != TokenType::Dot)
            {
                break;
            }
        }

        if (parenIndex != -1)
        {
            int depth = 1;
            size_t closing = parenIndex;

            while (++closing < tokens.size())
            {
                if (tokens[closing].type == TokenType::LParen)
                    depth++;

                else if (tokens[closing].type == TokenType::RParen)
                {
                    depth--;

                    if (depth == 0)
                        break;
                }
            }

            if (closing == tokens.size())
                throw std::runtime_error("Unclosed function call");

            if (closing == tokens.size() - 1)
            {

                std::vector<TokenRange> bufferArg;

                int argDepth = 1;
                size_t argStart = parenIndex + 1;

                for (size_t i = parenIndex + 1;
                     i < closing;
                     ++i)
                {
                    const Token& currentToken = tokens[i];

                    if (currentToken.type == TokenType::LParen)
                    {
                        argDepth++;
                    }
                    else if (currentToken.type == TokenType::RParen)
                    {
                        argDepth--;

                        if (argDepth == 0)
                            break;
                    }
                    else if (currentToken.type == TokenType::Comma &&
                             argDepth == 1)
                    {
                        // Only add non-empty argument
                        if (argStart < i)
                        {
                            bufferArg.push_back(
                                tokens.subrange(
                                    argStart,
                                    i - 1));
                        }

                        argStart = i + 1;
                    }
                }

                //last argument
                if (argStart < closing)
                {
                    bufferArg.push_back(
                        tokens.subrange(
                            argStart,
                            closing - 1));
                }



                std::string funcPath;

                for (int i = 0; i < parenIndex; ++i)
                {
                    if (tokens[i].type == TokenType::Identifier)
                    {
                        if (!funcPath.empty())
                            funcPath += '.';

                        funcPath += variantToString(tokens[i].value);
                    }
                }

                return ExecuteFunction(
                    std::move(funcPath),
                    bufferArg,
                    functionTable,
                    scope);
            }
        }
    }

    if (tokens.size() == 1)
    {
        if (tokens[0].type == TokenType::Identifier)
        {
            VariableData* variable = getVariable(
                variantToString(tokens[0].value),
                scope);

            if (variable == nullptr)
            {
                throw std::runtime_error(
                    "Unknown identifier in eval " +
                    variantToString(tokens[0].value));
            }

            return variable->value;
        }

        return tokens[0].value;
    }



    TokenRange range = tokens;

    while (range.size() >= 2 &&
           range[0].type == TokenType::LParen &&
           range[range.size() - 1].type == TokenType::RParen)
    {
        int depth = 0;
        bool wraps = true;

        for (size_t i = 0; i < range.size(); ++i)
        {
            if (range[i].type == TokenType::LParen)
                depth++;

            else if (range[i].type == TokenType::RParen)
                depth--;

            if (depth == 0 && i != range.size() - 1)
            {
                wraps = false;
                break;
            }
        }

        if (!wraps)
            break;

        range = range.subrange(
            1,
            range.size() - 2);
    }


    if (range[0].type == TokenType::LBrac)
    {
        int depth = 1;
        size_t closing = 0;

        while (++closing < range.size())
        {
            if (range[closing].type == TokenType::LBrac)
                depth++;

            else if (range[closing].type == TokenType::RBrac)
            {
                depth--;

                if (depth == 0)
                    break;
            }
        }

        if (closing == range.size() - 1)
        {
            auto array = std::make_shared<ArrayValue>();

            size_t start = 1;
            int elementDepth = 0;

            for (size_t i = start; i < closing; ++i)
            {
                if (range[i].type == TokenType::LParen ||
                    range[i].type == TokenType::LBrac)
                {
                    elementDepth++;
                }
                else if (range[i].type == TokenType::RParen ||
                         range[i].type == TokenType::RBrac)
                {
                    elementDepth--;
                }

                if (range[i].type == TokenType::Comma &&
                    elementDepth == 0)
                {
                    array->values.push_back(
                        Evaluate(
                            range.subrange(start, i - 1),
                            scope,
                            functionTable));

                    start = i + 1;
                }
            }

            //last element
            if (start < closing)
            {
                array->values.push_back(
                    Evaluate(
                        range.subrange(start, closing - 1),
                        scope,
                        functionTable));
            }

            return array;
        }
    }


    if (isUnary(*range.tokens, range.start))
    {
        Value rhs = Evaluate(
            range.subrange(1, range.size() - 1),
            scope,
            functionTable);

        switch (range[0].type)
        {
        case TokenType::Not:
            return !variantToBool(rhs);

        case TokenType::Minus:
            return -variantToDouble(rhs);

        case TokenType::Plus:
            return variantToDouble(rhs);

        default:
            break;
        }
    }

    int split = -1;
    int lowestPrec = INT_MAX;
    int depth = 0;

    for (size_t i = 0; i < range.size(); ++i)
    {
        if (range[i].type == TokenType::LParen)
        {
            depth++;
            continue;
        }

        if (range[i].type == TokenType::RParen)
        {
            depth--;
            continue;
        }

        if (depth != 0)
            continue;

        // Ignore unary operators
        if (isUnary(*range.tokens, range.start + i))
            continue;

        int prec = precedence(range[i].type);

        if (prec == -1)
            continue;

        if (prec < lowestPrec)
        {
            lowestPrec = prec;
            split = static_cast<int>(i);
        }
        else if (prec == lowestPrec &&
                 associativity(range[i].type) == Associativity::Left)
        {
            split = static_cast<int>(i);
        }
    }



    if (range[range.size() - 1].type == TokenType::RBrac)
    {
        int depth = 0;
        int open = -1;

        for (int i = static_cast<int>(range.size()) - 1;
             i >= 0;
             --i)
        {
            if (range[i].type == TokenType::RBrac)
                depth++;

            else if (range[i].type == TokenType::LBrac)
            {
                depth--;

                if (depth == 0)
                {
                    open = i;
                    break;
                }
            }
        }

if (open != -1)
{
    Value containerValue = Evaluate(
        range.subrange(0, open - 1),
        scope,
        functionTable);

    Value indexValue = Evaluate(
        range.subrange(open + 1, range.size() - 2),
        scope,
        functionTable);

    int index = variantToInt(indexValue);

    if (std::holds_alternative<std::shared_ptr<ArrayValue>>(containerValue))
    {
        auto array =
            std::get<std::shared_ptr<ArrayValue>>(containerValue);

        if (index < 0 ||
            index >= static_cast<int>(array->values.size()))
        {
            throw std::runtime_error(
                "Array index out of bounds");
        }

        return array->values[index];
    }

    if (std::holds_alternative<std::string>(containerValue))
    {
        const std::string &str =
            std::get<std::string>(containerValue);

        if (index < 0 ||
            index >= static_cast<int>(str.size()))
        {
            throw std::runtime_error(
                "String index out of bounds");
        }

        return std::string(1, str[index]);
    }

    throw std::runtime_error(
        "Cannot index non-array or string value");
}
    }



    if (split == -1)
        throw std::runtime_error("No operator found.");


    Value lhs = Evaluate(
        range.subrange(0, split - 1),
        scope,
        functionTable);

    Value rhs = Evaluate(
        range.subrange(split + 1, range.size() - 1),
        scope,
        functionTable);



    switch (range[split].type)
    {
    case TokenType::Plus:
        if (std::holds_alternative<std::string>(lhs) ||
            std::holds_alternative<std::string>(rhs))
        {
            return variantToString(lhs) +
                   variantToString(rhs);
        }

        return variantToDouble(lhs) +
               variantToDouble(rhs);

    case TokenType::Remainder:
        return std::fmod(
            variantToDouble(lhs),
            variantToDouble(rhs));

    case TokenType::Minus:
        return variantToDouble(lhs) -
               variantToDouble(rhs);

    case TokenType::Multiply:

        if (std::holds_alternative<std::string>(lhs) &&
            std::holds_alternative<int>(rhs))
        {
            const std::string& str =
                std::get<std::string>(lhs);

            int count = std::get<int>(rhs);

            std::string buffer;
            buffer.reserve(str.size() * count);

            for (int i = 0; i < count; ++i)
                buffer += str;

            return buffer;
        }

        if (std::holds_alternative<int>(lhs) &&
            std::holds_alternative<std::string>(rhs))
        {
            const std::string& str =
                std::get<std::string>(rhs);

            int count = std::get<int>(lhs);

            std::string buffer;
            buffer.reserve(str.size() * count);

            for (int i = 0; i < count; ++i)
                buffer += str;

            return buffer;
        }

        return variantToDouble(lhs) *
               variantToDouble(rhs);

    case TokenType::Division:
        return variantToDouble(lhs) /
               variantToDouble(rhs);

    case TokenType::Power:
        return std::pow(
            variantToDouble(lhs),
            variantToDouble(rhs));

    case TokenType::EqualEqual:
    {
        bool lhsNumeric =
            std::holds_alternative<int>(lhs) ||
            std::holds_alternative<double>(lhs);

        bool rhsNumeric =
            std::holds_alternative<int>(rhs) ||
            std::holds_alternative<double>(rhs);

        if (lhsNumeric && rhsNumeric)
            return variantToDouble(lhs) ==
                   variantToDouble(rhs);

        return lhs == rhs;
    }

    case TokenType::TypeEquality:
        return getType(lhs) == getType(rhs);

    case TokenType::NotEqual:
    {
        bool lhsNumeric =
            std::holds_alternative<int>(lhs) ||
            std::holds_alternative<double>(lhs);

        bool rhsNumeric =
            std::holds_alternative<int>(rhs) ||
            std::holds_alternative<double>(rhs);

        if (lhsNumeric && rhsNumeric)
            return variantToDouble(lhs) !=
                   variantToDouble(rhs);

        return lhs != rhs;
    }

    case TokenType::Bigger:
    {
        bool lhsNumeric =
            std::holds_alternative<int>(lhs) ||
            std::holds_alternative<double>(lhs);

        bool rhsNumeric =
            std::holds_alternative<int>(rhs) ||
            std::holds_alternative<double>(rhs);

        if (lhsNumeric && rhsNumeric)
            return variantToDouble(lhs) >
                   variantToDouble(rhs);

        return lhs > rhs;
    }

    case TokenType::Smaller:
    {
        bool lhsNumeric =
            std::holds_alternative<int>(lhs) ||
            std::holds_alternative<double>(lhs);

        bool rhsNumeric =
            std::holds_alternative<int>(rhs) ||
            std::holds_alternative<double>(rhs);

        if (lhsNumeric && rhsNumeric)
            return variantToDouble(lhs) <
                   variantToDouble(rhs);

        return lhs < rhs;
    }

    case TokenType::BiggerEqual:
    {
        bool lhsNumeric =
            std::holds_alternative<int>(lhs) ||
            std::holds_alternative<double>(lhs);

        bool rhsNumeric =
            std::holds_alternative<int>(rhs) ||
            std::holds_alternative<double>(rhs);

        if (lhsNumeric && rhsNumeric)
            return variantToDouble(lhs) >=
                   variantToDouble(rhs);

        return lhs >= rhs;
    }

    case TokenType::SmallerEqual:
    {
        bool lhsNumeric =
            std::holds_alternative<int>(lhs) ||
            std::holds_alternative<double>(lhs);

        bool rhsNumeric =
            std::holds_alternative<int>(rhs) ||
            std::holds_alternative<double>(rhs);

        if (lhsNumeric && rhsNumeric)
            return variantToDouble(lhs) <=
                   variantToDouble(rhs);

        return lhs <= rhs;
    }

    case TokenType::AndAnd:
        return variantToBool(lhs) &&
               variantToBool(rhs);

    case TokenType::OrOr:
        return variantToBool(lhs) ||
               variantToBool(rhs);

    default:
        throw std::runtime_error("Unknown operator.");
    }
}

std::vector<Instruction> parse(const std::vector<Token> &input, std::vector<std::string> &path)
{
    std::vector<Instruction> instructions;
    bool strictMode = false;
    bool globalMode = false;
    for (size_t i = 0; i < input.size(); i++)
    {

        Instruction instr;

        Token tok = input[i];
        std::string value = variantToString(tok.value);
        if (tok.type == TokenType::Identifier)
        { // could be a function,if,for,while,fn,var,const,strict or a function call like Terminal.IO.print("hello world!");
            if (value == "use")
            {
                instr.type = Instruction::Types::Use;
                i++;
                while (input[i].type == TokenType::NewLine)
                {
                    i++;
                }
                if (input[i].type != TokenType::LCurl && input[i].type != TokenType::String)
                {
                    std::cerr << "expected { or a string after use";
                }

                if (input[i].type == TokenType::String)
                {
                    instr.importPath.push_back(variantToString(input[i].value));
                }
                else
                {

                    i++; // skip {

                    while (i < input.size() && input[i].type != TokenType::RCurl)
                    {
                        if (input[i].type == TokenType::String)
                        {
                            instr.importPath.push_back(
                                variantToString(input[i].value));
                        }

                        i++;
                    }
                }
                instructions.push_back(std::move(instr));
            }

            else if (value == "fn")
            {
                instr.type = Instruction::Types::FunctionDeclare;
                instr.isStrict = strictMode;

                if (i + 1 < input.size() && input[i + 1].type == TokenType::Identifier)
                {
                    std::string functionName = variantToString(input[i + 1].value);

                    if (!path.empty())
                    {
                        std::string prefix = joinpath(path, ".");

                        functionName = prefix + "." + functionName;
                    }
                    instr.Funcname = functionName;

                    i++;
                }
                else
                {
                    continue;
                }

                if (i + 1 < input.size() && input[i + 1].type == TokenType::LParen)
                {
                    i++;

                    int paranDepth = 1;
                    int slot = 0;

                    while (i + 1 < input.size() && paranDepth != 0)
                    {
                        i++;

                        if (input[i].type == TokenType::LParen)
                        {
                            paranDepth++;
                            continue;
                        }

                        if (input[i].type == TokenType::RParen)
                        {
                            paranDepth--;
                            continue;
                        }

                        if (input[i].type == TokenType::Comma)
                        {
                            slot++;
                            continue;
                        }

                        if (input[i].type != TokenType::Identifier)
                            continue;

                        if (instr.locals.size() <= slot)
                            instr.locals.resize(slot + 1);

                        std::string name = variantToString(input[i].value);

                        if (name == "string")
                            instr.locals[slot].vartype = VariableTypes::String;
                        else if (name == "int")
                            instr.locals[slot].vartype = VariableTypes::Int;
                        else if (name == "bool")
                            instr.locals[slot].vartype = VariableTypes::Bool;
                        else if (name == "double")
                            instr.locals[slot].vartype = VariableTypes::Double;
                        else if (name == "value[]")
                            instr.locals[slot].vartype = VariableTypes::Array;
                        else
                            instr.locals[slot].name = name;
                    }
                }

                while (i < input.size() && input[i].type != TokenType::LCurl)
                {
                    i++;
                }

                if (i >= input.size())
                {
                    continue;
                }

                i++;

                std::vector<Token> bodyTokens;

                int curlyDepth = 1;

                while (i < input.size() && curlyDepth > 0)
                {
                    if (input[i].type == TokenType::LCurl)
                    {
                        curlyDepth++;
                    }
                    else if (input[i].type == TokenType::RCurl)
                    {
                        curlyDepth--;
                    }

                    if (curlyDepth > 0)
                    {
                        bodyTokens.push_back(input[i]);
                    }

                    i++;
                }
                std::vector<std::string> empty = {};
                instr.body = parse(bodyTokens, empty);

                instructions.push_back(std::move(instr));

                continue;
            }
            else if (value == "while")
            {
                instr.type = Instruction::Types::While;

                int paranDepth = 0;
                int curlyDepth = 0;

                // Parse condition
                if (i + 1 < input.size() && input[i + 1].type == TokenType::LParen)
                {
                    paranDepth++;
                    i++;

                    while (i + 1 < input.size() && paranDepth != 0)
                    {
                        i++;

                        if (input[i].type == TokenType::LParen)
                        {
                            paranDepth++;
                        }
                        else if (input[i].type == TokenType::RParen)
                        {
                            paranDepth--;

                            if (paranDepth == 0)
                                break;
                        }

                        if (paranDepth != 0)
                        {
                            instr.condition.push_back(input[i]);
                        }
                    }
                }

                // Ignore newlines between condition and {
                while (i + 1 < input.size() && input[i + 1].type == TokenType::NewLine)
                {
                    i++;
                }

                // Parse body
                if (i + 1 < input.size() && input[i + 1].type == TokenType::LCurl)
                {
                    curlyDepth++;
                    i++;

                    std::vector<Token> bodyTokens;

                    while (i + 1 < input.size() && curlyDepth != 0)
                    {
                        i++;

                        if (input[i].type == TokenType::LCurl)
                        {
                            curlyDepth++;
                        }
                        else if (input[i].type == TokenType::RCurl)
                        {
                            curlyDepth--;

                            if (curlyDepth == 0)
                                break;
                        }

                        if (curlyDepth != 0)
                        {
                            bodyTokens.push_back(std::move(input[i]));
                        }
                    }

                    instr.body = parse(bodyTokens, path);
                }

                instructions.push_back(std::move(instr));
            }
            else if (value == "if")
            {
                instr.type = Instruction::Types::If;

                int paranDepth = 0;
                int curlyDepth = 0;

                // Parse condition
                if (i + 1 < input.size() && input[i + 1].type == TokenType::LParen)
                {
                    paranDepth++;
                    i++;

                    while (i + 1 < input.size() && paranDepth != 0)
                    {
                        i++;

                        if (input[i].type == TokenType::LParen)
                        {
                            paranDepth++;
                        }
                        else if (input[i].type == TokenType::RParen)
                        {
                            paranDepth--;
                            if (paranDepth == 0)
                                break;
                        }

                        if (paranDepth != 0)
                        {
                            instr.condition.push_back(input[i]);
                        }
                    }
                }
                while (i + 1 < input.size() && input[i + 1].type == TokenType::NewLine)
                {
                    i++;
                }
                if (i + 1 < input.size() && input[i + 1].type == TokenType::LCurl)
                {
                    curlyDepth++;
                    i++;

                    std::vector<Token> bodyTokens;

                    while (i + 1 < input.size() && curlyDepth != 0)
                    {
                        i++;

                        if (input[i].type == TokenType::LCurl)
                        {
                            curlyDepth++;
                        }
                        else if (input[i].type == TokenType::RCurl)
                        {
                            curlyDepth--;
                            if (curlyDepth == 0)
                                break;
                        }

                        if (curlyDepth != 0)
                        {
                            bodyTokens.push_back(input[i]);
                        }
                    }

                    instr.body = parse(bodyTokens, path);
                }
                 while (i + 1 < input.size() &&
       input[i + 1].type == TokenType::NewLine)
{
    i++;
}

if (i + 1 < input.size() &&
    input[i + 1].type == TokenType::Identifier &&
    variantToString(input[i + 1].value) == "else")
{
    i++; // move to else

    while (i + 1 < input.size() &&
           input[i + 1].type == TokenType::NewLine)
    {
        i++;
    }

    if (i + 1 < input.size() &&
        input[i + 1].type == TokenType::LCurl)
    {
        curlyDepth = 1;
        i++;

        std::vector<Token> elsebodyTokens;

        while (i + 1 < input.size() && curlyDepth != 0)
        {
            i++;

            if (input[i].type == TokenType::LCurl)
            {
                curlyDepth++;
            }
            else if (input[i].type == TokenType::RCurl)
            {
                curlyDepth--;

                if (curlyDepth == 0)
                    break;
            }

            if (curlyDepth != 0)
                elsebodyTokens.push_back(input[i]);
        }

        instr.elseBody = parse(elsebodyTokens, path);
    }
}
                instructions.push_back(std::move(instr));
            }
            else if (value == "strict")
            {
                strictMode = true;
            }
            else if (value == "global")
            {
                globalMode = true;
            }

            else if (value == "space")
            {
                instr.type = Instruction::Types::Space;
                std::string name;

                if (i + 1 < input.size() && input[i + 1].type == TokenType::Identifier)
                {
                    name = variantToString(input[i + 1].value);
                    i++;
                }
                else
                {
                    std::cerr << "Error: expected space name\n";
                    continue;
                }

                while (i < input.size() && input[i].type != TokenType::LCurl)
                {
                    i++;
                }

                if (i >= input.size())
                {
                    continue;
                }

                i++; // skip {

                std::vector<Token> bodyTokens;

                int curlyDepth = 1;

                while (i < input.size() && curlyDepth > 0)
                {
                    if (input[i].type == TokenType::LCurl)
                    {
                        curlyDepth++;
                    }
                    else if (input[i].type == TokenType::RCurl)
                    {
                        curlyDepth--;
                    }

                    if (curlyDepth > 0)
                    {
                        bodyTokens.push_back(input[i]);
                    }

                    i++;
                }

                path.push_back(std::move(name));

                instr.body = parse(bodyTokens, path);

                path.pop_back();

                instructions.push_back(std::move(instr));

                continue;
            }

            else if (value == "var")
            {
                int paranDepth = 0;
                bool canContinue = true;
                instr.vardata.isConst = false;
                instr.vardata.isStrict = strictMode;
                instr.vardata.isGlobal = globalMode;
                strictMode = false;
                globalMode = false;
                instr.type = Instruction::Types::Declare;
                if (i + 1 < input.size() && input[i + 1].type == TokenType::Identifier)
                {
                    instr.vardata.name = variantToString(input[i + 1].value);
                }
                else
                {
                    canContinue = false;
                }
                if (i + 2 < input.size() && input[i + 2].type == TokenType::Equals && canContinue)
                {
                }
                else
                {
                    canContinue = false;
                }
                while (i + 3 < input.size() && (input[i + 3].type != TokenType::NewLine && input[i + 3].type != TokenType::Semicolon && input[i + 3].type != TokenType::EndOfFile))
                {
                    instr.expression.push_back(input[i + 3]);
                    i++;
                }
                instructions.push_back(std::move(instr));
            }
            else if (value == "return")
            {
                instr.type = Instruction::Types::Return;

                i++;

                while (input[i].type != TokenType::Semicolon && input[i].type != TokenType::NewLine && input[i + 3].type != TokenType::EndOfFile)
                {
                    instr.expression.push_back(input[i]);
                    i++;
                }

                instructions.push_back(std::move(instr));
            }
            else if (value == "const")
            {
                int paranDepth = 0;
                bool canContinue = true;
                instr.vardata.isConst = true;
                instr.vardata.isStrict = strictMode;
                instr.vardata.isGlobal = globalMode;
                strictMode = false;
                globalMode = false;
                instr.type = Instruction::Types::Declare;
                if (i + 1 < input.size() && input[i + 1].type == TokenType::Identifier)
                {
                    instr.vardata.name = variantToString(input[i + 1].value);
                }
                else
                {
                    canContinue = false;
                }
                if (i + 2 < input.size() && input[i + 2].type == TokenType::Equals && canContinue)
                {
                }
                else
                {
                    canContinue = false;
                }
                while (i + 3 < input.size() && (input[i + 3].type != TokenType::NewLine && input[i + 3].type != TokenType::Semicolon && input[i + 3].type != TokenType::EndOfFile))
                {
                    instr.expression.push_back(input[i + 3]);
                    i++;
                }
                instructions.push_back(std::move(instr));
            }
            else if (i + 1 < input.size() && input[i + 1].type == TokenType::Equals)
            {
                instr.vardata.name = variantToString(input[i].value);
                instr.type = Instruction::Types::Assign;

                int j = i + 2;

                while (j < input.size() && input[j].type != TokenType::NewLine && input[j].type != TokenType::Semicolon && input[i + 3].type != TokenType::EndOfFile)
                {
                    instr.expression.push_back(input[j]);
                    j++;
                }

                i = j; // skip the whole expression

                instructions.push_back(std::move(instr));
                continue;
            }
            else if (i + 1 < input.size() &&
                     input[i + 1].type == TokenType::LBrac)
            {
                instr.vardata.name = variantToString(input[i].value);
                instr.type = Instruction::Types::Assign;

                int j = i + 2;

                
                while (j < input.size() &&
                       input[j].type != TokenType::RBrac)
                {
                    instr.indexExpression.push_back(input[j]);
                    j++;
                }

                if (j >= input.size())
                    throw std::runtime_error("Missing closing ']'");

                
                i = j;

                
                if (j + 1 < input.size() &&
                    input[j + 1].type == TokenType::Equals)
                {
                    int a = j + 2;

                    // Read assignment expression
                    while (a < input.size() && input[a].type != TokenType::NewLine && input[a].type != TokenType::Semicolon && input[i + 3].type != TokenType::EndOfFile)
                    {
                        instr.expression.push_back(input[a]);
                        a++;
                    }

                    i = a;

                    instructions.push_back(std::move(instr));
                    continue;
                }
            }
            else if (
                (i + 1 < input.size() &&
                 (input[i + 1].type == TokenType::LParen ||
                  input[i + 1].type == TokenType::Dot)))
            {
                instr.type = Instruction::Types::FunctionCall;
                instr.path = path;

                while (i < input.size() && input[i].type == TokenType::Identifier)
                {
                    instr.path.push_back(variantToString(input[i].value));

                    if (i + 1 < input.size() && input[i + 1].type == TokenType::Dot)
                    {
                        i += 2;

                        if (i >= input.size() || input[i].type != TokenType::Identifier)
                        {
                            std::cerr << "Invalid function path\n";
                            break;
                        }

                        continue;
                    }

                    else if (i + 1 < input.size() && input[i + 1].type == TokenType::LParen)
                    {
                        i++; // move to '('

                        int argCount = 0;
                        int paranDepth = 1;

                        while (i + 1 < input.size() && paranDepth != 0)
                        {
                            i++;

                            Token current = input[i];

                            if (current.type == TokenType::LParen)
                            {
                                paranDepth++;

                                // store nested '('
                                if (paranDepth > 1)
                                {
                                    if (instr.arguments.size() <= argCount)
                                        instr.arguments.resize(argCount + 1);

                                    instr.arguments[argCount].push_back(current);
                                }
                            }

                            else if (current.type == TokenType::RParen)
                            {
                                paranDepth--;

                                // store nested ')' but not the function's closing ')'
                                if (paranDepth > 0)
                                {
                                    if (instr.arguments.size() <= argCount)
                                        instr.arguments.resize(argCount + 1);

                                    instr.arguments[argCount].push_back(current);
                                }
                            }

                            else if (current.type == TokenType::Comma && paranDepth == 1)
                            {
                                argCount++;
                            }

                            else
                            {
                                if (instr.arguments.size() <= argCount)
                                    instr.arguments.resize(argCount + 1);

                                instr.arguments[argCount].push_back(current);
                            }
                        }

                        
                        if (instr.arguments.size() == 1 && instr.arguments[0].empty())
                        {
                            instr.arguments.clear();
                        }
                    }

                    break;
                }

                instructions.push_back(std::move(instr));
            }
        }
    }
    return instructions;
}