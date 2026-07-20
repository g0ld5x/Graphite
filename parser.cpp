#include "parser.h"
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

Value Evaluate(const std::vector<Token> &tokens, int left, int right, VariableTable variables)
{
    // some basic predefined variables to test the evaluator and also provide ease for simple calculations.
    variables["pi"] = {
        "pi",
        M_PI,
        true, // isConst
        true, // isStrict
        VariableTypes::Double};

    variables["e"] = {
        "e",
        M_E,
        true, // isConst
        true, // isStrict
        VariableTypes::Double};

    variables["true"] = {
        "true",
        true,
        true, // isConst
        true, // isStrict
        VariableTypes::Bool};

    variables["false"] = {
        "false",
        false,
        true, // isConst
        true, // isStrict
        VariableTypes::Bool};
    // Invalid range
    if (left > right)
        throw std::runtime_error("Invalid expression.");

    // Single value
    if (left == right)
    {
        if (tokens[left].type == TokenType::Identifier)
        {
            auto name = variantToString(tokens[left].value);

            auto it = variables.find(name);
            if (it == variables.end())
                throw std::runtime_error("Undefined variable: " + name);

            return it->second.value;
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
        Value rhs = Evaluate(tokens, left + 1, right, variables);

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

    Value lhs = Evaluate(tokens, left, split - 1, variables);
    Value rhs = Evaluate(tokens, split + 1, right, variables);
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

std::vector<Instruction> parse(std::vector<Token> input)
{
    std::vector<Instruction> instructions;

    for (size_t i = 0; i < input.size(); i++)
    {
        bool strictMode = false;
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
                    instr.Funcname = variantToString(input[i + 1].value);
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
                        }
                        else if (input[i].type == TokenType::Identifier)
                        {
                            instr.locals[variantToString(input[i].value)];
                        }
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

                instr.body = parse(bodyTokens);

                instructions.push_back(instr);

                continue;
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

                    instr.body = parse(bodyTokens);
                }
                if (variantToString(input[i + 1].value) == "else")
                {

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

                        instr.elseBody = parse(elsebodyTokens);
                    }
                }
                instructions.push_back(instr);
            }
            else if (value == "strict")
            {
                strictMode = true;
            }
            else if (value == "var")
            {
                int paranDepth = 0;
                bool canContinue = true;
                instr.vardata.isConst = false;
                instr.vardata.isStrict = strictMode;
                strictMode = false;
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
                break;
            }

            else if (value == "const")
            {
                int paranDepth = 0;
                bool canContinue = true;
                instr.vardata.isConst = true;
                instr.vardata.isStrict = strictMode;
                strictMode = false;
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
                break;
            }
            else if (i + 1 < input.size() && input[i + 1].type == TokenType::Equals)
            {

                instr.vardata.name = variantToString(input[i].value);
                instr.type = Instruction::Types::Assign;
                while (i + 2 < input.size() && (input[i + 2].type != TokenType::NewLine && input[i + 2].type != TokenType::Semicolon))
                {
                    instr.expression.push_back(input[i + 2]);
                    i++;
                }
                instructions.push_back(instr);
                break;
            }
            else
            {
                instr.type = Instruction::Types::FunctionCall;
                while (i < input.size() && input[i].type == TokenType::Identifier)
                {
                    instr.path.push_back(variantToString(input[i].value));
                    if (i + 1 < input.size() && input[i + 1].type == TokenType::Dot)
                    {
                        i += 2;
                        continue;
                    }
                    else if (i + 1 < input.size() && input[i + 1].type == TokenType::LParen)
                    {
                        int argCount = 0;
                        int paranDepth = 0;
                        paranDepth++;
                        while (i + 1 < input.size() && paranDepth != 0)
                        {
                            i++;
                            if (input[i + 1].type == TokenType::RParen)
                            {
                                paranDepth--;
                                if (paranDepth != 0)
                                {
                                    if (instr.arguments.size() <= argCount)
                                    {
                                        instr.arguments.resize(argCount + 1);
                                    }

                                    instr.arguments[argCount].push_back(input[i + 1]);
                                }
                            }
                            else if (input[i + 1].type == TokenType::Comma)
                            {
                                argCount++;
                            }
                            else if (input[i + 1].type == TokenType::LParen)
                            {
                                paranDepth++;
                                if (paranDepth != 1)
                                {
                                    if (instr.arguments.size() <= argCount)
                                    {
                                        instr.arguments.resize(argCount + 1);
                                    }

                                    instr.arguments[argCount].push_back(input[i + 1]);
                                }
                            }
                            else
                            {
                                // this is to ensure that arguments vector is large enough
                                if (instr.arguments.size() <= argCount)
                                {
                                    instr.arguments.resize(argCount + 1);
                                }

                                // now it is safe to push back
                                instr.arguments[argCount].push_back(input[i + 1]);
                            }
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