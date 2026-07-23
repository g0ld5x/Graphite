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
#include <sstream>
#include <climits>
// here we goo

// integrate unordered map plsss!!!!! i did bro no worries (me)

std::string joinpath(const std::vector<std::string> &elements, const std::string &delimiter)
{
    if (elements.empty())
        return "";

    // Calculate total length to allocate memory at once
    size_t total_length = 0;
    for (const auto &s : elements)
        total_length += s.length();
    total_length += delimiter.length() * (elements.size() - 1);

    std::string result;
    result.reserve(total_length);

    // Join elements
    result += elements[0];
    for (size_t i = 1; i < elements.size(); ++i)
    {
        result += delimiter + elements[i];
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

int precedence(TokenType type)
{
    switch (type)
    {
    // Assignment
    case TokenType::Equals:
        return 0;

    // Logical OR
    case TokenType::OrOr:
        return 1;

    // Logical AND
    case TokenType::AndAnd:
        return 2;

    // Equality
    case TokenType::EqualEqual:
    case TokenType::NotEqual:
        return 3;

    // Comparison
    case TokenType::Bigger:
    case TokenType::BiggerEqual:
    case TokenType::Smaller:
    case TokenType::SmallerEqual:
        return 4;

    // Addition / Subtraction
    case TokenType::Plus:
    case TokenType::Minus:
        return 5;

    // Multiplication / Division / Modulo
    case TokenType::Multiply:
    case TokenType::Division:
    case TokenType::Remainder:
        return 6;

    // Unary operators
    case TokenType::Not:
        // case TokenType::UnaryPlus:
        // case TokenType::UnaryMinus:
        return 7;

    // Exponentiation
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
                        std::ostringstream oss;
                        oss << arg;
                        return oss.str(); }, a);
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

bool isInVariables(Token tok, VariableTable variables)
{
    auto it = variables.find(variantToString(tok.value));
    if (it != variables.end())
    {
        return true;
    }
    else
    {
        return false;
    }
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
    const std::string &name,
    const std::vector<std::vector<Token>> &arguments,
    FunctionTable &ftable,
    ScopeStack &scopeStack)
{
    auto it = ftable.find(name);

    if (it == ftable.end())
    {
        std::cerr << "Error: function " << name << " does not exist\n";
        return Value{};
    }

    GFunction &targetFunc = it->second;

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

        var.value =
            Evaluate(arguments[k],
                     0,
                     arguments[k].size() - 1,
                     scopeStack, ftable);

        bufferTable[var.name] = var;
    }

    scopeStack.push_back(bufferTable);

    ExecutionResult result = interpret(targetFunc.body, scopeStack);

    scopeStack.pop_back();

    if (result.didReturn)
    {
        return result.returnValue;
    }

    return Value{};
}

Value Evaluate(const std::vector<Token> &tokens, int left, int right, ScopeStack &scope, FunctionTable &functionTable)
{
    if (tokens.empty())
    {
        throw std::runtime_error("Cannot evaluate empty expression");
    }

    if (left < 0 || right >= (int)tokens.size() || left > right)
    {
        throw std::runtime_error("Invalid evaluation range");
    }
    if (tokens[left].type == TokenType::Identifier)
    {
        int parenIndex = -1;

        // Find the opening parenthesis after the function path
        for (int i = left; i <= right; i++)
        {
            if (tokens[i].type == TokenType::LParen)
            {
                parenIndex = i;
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
            int closing = parenIndex;

            while (++closing <= right)
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

            if (closing == right)
            {
                std::vector<std::vector<Token>> bufferArg;
                std::vector<Token> current;

                int argDepth = 1;

                for (int i = parenIndex + 1; i <= right; i++)
                {
                    Token currentToken = tokens[i];

                    if (currentToken.type == TokenType::LParen)
                    {
                        argDepth++;
                        current.push_back(currentToken);
                    }
                    else if (currentToken.type == TokenType::RParen)
                    {
                        argDepth--;

                        if (argDepth == 0)
                        {
                            if (!current.empty())
                                bufferArg.push_back(current);

                            break;
                        }

                        current.push_back(currentToken);
                    }
                    else if (currentToken.type == TokenType::Comma &&
                             argDepth == 1)
                    {
                        bufferArg.push_back(current);
                        current.clear();
                    }
                    else
                    {
                        current.push_back(currentToken);
                    }
                }

                std::vector<std::string> funcPath;

                for (int i = left; i < parenIndex; i++)
                {
                    if (tokens[i].type == TokenType::Identifier)
                    {
                        funcPath.push_back(
                            variantToString(tokens[i].value));
                    }
                }

                return ExecuteFunction(
                    joinpath(funcPath, "."),
                    bufferArg,
                    functionTable,
                    scope);
            }
        }
    }
    // some basic predefined variables to test the evaluator and also provide ease for simple calculations.

    // Invalid range
    if (left > right)
        throw std::runtime_error("Invalid expression.");

    // Single value
    if (left == right)
    {
        if (tokens[left].type == TokenType::Identifier)
        {
            VariableData *variable = getVariable(
                variantToString(tokens[left].value),
                scope);

            if (variable == nullptr)
            {
                throw std::runtime_error(
                    "Unknown identifier in eval " + variantToString(tokens[left].value));
            }

            return variable->value;
        }

        return tokens[left].value;
    }

    // Strip outer parentheses
    while (tokens[left].type == TokenType::LParen &&
           tokens[right].type == TokenType::RParen)
    {
        int depth = 0;
        bool wraps = true;

        for (int i = left; i <= right; i++)
        {
            if (tokens[i].type == TokenType::LParen)
                depth++;

            else if (tokens[i].type == TokenType::RParen)
                depth--;

            if (depth == 0 && i != right)
            {
                wraps = false;
                break;
            }
        }

        if (!wraps)
            break;

        left++;
        right--;
    }

    // Prefix unary operators
    if (isUnary(tokens, left))
    {
        Value rhs = Evaluate(tokens, left + 1, right, scope, functionTable);

        switch (tokens[left].type)
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

    // Find split operator
    int split = -1;
    int lowestPrec = INT_MAX;
    int depth = 0;

    for (int i = left; i <= right; i++)
    {
        if (tokens[i].type == TokenType::LParen)
        {
            depth++;
            continue;
        }

        if (tokens[i].type == TokenType::RParen)
        {
            depth--;
            continue;
        }

        if (depth != 0)
            continue;

        // Ignore unary operators here
        if (isUnary(tokens, i))
            continue;

        int prec = precedence(tokens[i].type);

        if (prec == -1)
            continue;

        if (prec < lowestPrec)
        {
            lowestPrec = prec;
            split = i;
        }
        else if (prec == lowestPrec &&
                 associativity(tokens[i].type) == Associativity::Left)
        {
            split = i;
        }
    }

    if (split == -1)
        throw std::runtime_error("No operator found.");

    Value lhs = Evaluate(tokens, left, split - 1, scope, functionTable);
    Value rhs = Evaluate(tokens, split + 1, right, scope, functionTable);
    switch (tokens[split].type)
    {

    case TokenType::Plus:
        if (std::holds_alternative<std::string>(lhs) && std::holds_alternative<std::string>(rhs) || (std::holds_alternative<std::string>(lhs) || std::holds_alternative<std::string>(rhs)))
        {
            return variantToString(lhs).append(variantToString(rhs));
        }
        else
        {
            return variantToDouble(lhs) + variantToDouble(rhs);
        }

    case TokenType::Minus:
        return variantToDouble(lhs) - variantToDouble(rhs);

    case TokenType::Multiply:
        if (std::holds_alternative<std::string>(lhs) &&
            std::holds_alternative<int>(rhs))
        {
            const std::string &str = std::get<std::string>(lhs);
            int count = std::get<int>(rhs);

            std::string buffer;
            buffer.reserve(str.size() * count);

            for (int i = 0; i < count; ++i)
            {
                buffer += str;
            }

            return buffer;
        }
        else if (std::holds_alternative<int>(lhs) &&
                 std::holds_alternative<std::string>(rhs))
        {
            const std::string &str = std::get<std::string>(rhs);
            int count = std::get<int>(lhs);

            std::string buffer;
            buffer.reserve(str.size() * count);

            for (int i = 0; i < count; ++i)
            {
                buffer += str;
            }

            return buffer;
        }
        else
        {
            return variantToDouble(lhs) * variantToDouble(rhs);
        }

    case TokenType::Division:
        return variantToDouble(lhs) / variantToDouble(rhs);

    case TokenType::Power:
        return std::pow(variantToDouble(lhs), variantToDouble(rhs));

    case TokenType::EqualEqual:
    {
        bool lhsNumeric =
            std::holds_alternative<int>(lhs) ||
            std::holds_alternative<double>(lhs);

        bool rhsNumeric =
            std::holds_alternative<int>(rhs) ||
            std::holds_alternative<double>(rhs);

        if (lhsNumeric && rhsNumeric)
            return variantToDouble(lhs) == variantToDouble(rhs);

        return lhs == rhs;
    }
    case TokenType::NotEqual:
    {
        bool lhsNumeric =
            std::holds_alternative<int>(lhs) ||
            std::holds_alternative<double>(lhs);

        bool rhsNumeric =
            std::holds_alternative<int>(rhs) ||
            std::holds_alternative<double>(rhs);

        if (lhsNumeric && rhsNumeric)
            return variantToDouble(lhs) != variantToDouble(rhs);

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
            return variantToDouble(lhs) > variantToDouble(rhs);

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
            return variantToDouble(lhs) < variantToDouble(rhs);

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
            return variantToDouble(lhs) >= variantToDouble(rhs);

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
            return variantToDouble(lhs) <= variantToDouble(rhs);

        return lhs <= rhs;
    }

    case TokenType::AndAnd:
        return variantToBool(lhs) && variantToBool(rhs);

    case TokenType::OrOr:
        return variantToBool(lhs) || variantToBool(rhs);
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
            if (value == "fn")
            {
                instr.type = Instruction::Types::FunctionDeclare;
                instr.isStrict = strictMode;

                if (i + 1 < input.size() && input[i + 1].type == TokenType::Identifier)
                {
                    instr.Funcname = path.empty()
                                         ? variantToString(input[i + 1].value)
                                         : joinpath(path, ".") + "." + variantToString(input[i + 1].value);

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

                instr.body = parse(bodyTokens, path);

                instructions.push_back(instr);

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
                            bodyTokens.push_back(input[i]);
                        }
                    }

                    instr.body = parse(bodyTokens, path);
                }

                instructions.push_back(instr);
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
                if (i + 1 < input.size() && input[i + 1].type == TokenType::Identifier && variantToString(input[i + 1].value) == "else")
                {
                    while (i + 1 < input.size() && input[i + 1].type == TokenType::NewLine)
                    {
                        i++;
                    }
                    if (i + 2 < input.size() && input[i + 2].type == TokenType::LCurl)
                    {
                        curlyDepth++;
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
                            {
                                elsebodyTokens.push_back(input[i]);
                            }
                        }

                        instr.elseBody = parse(elsebodyTokens, path);
                    }
                }
                instructions.push_back(instr);
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

                path.push_back(name);

                instr.body = parse(bodyTokens, path);

                path.pop_back();

                instructions.push_back(instr);

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
                while (i + 3 < input.size() && (input[i + 3].type != TokenType::NewLine && input[i + 3].type != TokenType::Semicolon))
                {
                    instr.expression.push_back(input[i + 3]);
                    i++;
                }
                instructions.push_back(instr);
            }
            else if (value == "return")
            {
                instr.type = Instruction::Types::Return;

                i++;

                while (input[i].type != TokenType::Semicolon &&
                       input[i].type != TokenType::NewLine)
                {
                    instr.expression.push_back(input[i]);
                    i++;
                }

                instructions.push_back(instr);
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
                while (i + 3 < input.size() && (input[i + 3].type != TokenType::NewLine && input[i + 3].type != TokenType::Semicolon))
                {
                    instr.expression.push_back(input[i + 3]);
                    i++;
                }
                instructions.push_back(instr);
            }
            else if (i + 1 < input.size() && input[i + 1].type == TokenType::Equals)
            {
                instr.vardata.name = variantToString(input[i].value);
                instr.type = Instruction::Types::Assign;

                int j = i + 2;

                while (j < input.size() &&
                       input[j].type != TokenType::NewLine &&
                       input[j].type != TokenType::Semicolon)
                {
                    instr.expression.push_back(input[j]);
                    j++;
                }

                i = j; // skip the whole expression

                instructions.push_back(instr);
                continue;
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

                        // Remove empty argument caused by ()
                        if (instr.arguments.size() == 1 && instr.arguments[0].empty())
                        {
                            instr.arguments.clear();
                        }
                    }

                    break;
                }
                instructions.push_back(instr);
            }
        }
    }
    return instructions;
}