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


bool stringToBool(const std::string &str)
{
    if (str == "true")
        return true;
    else if (str == "false")
        return false;
    return false;
}

VariableTable variables;


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

bool isUnary(const std::vector<Token>& tokens, int index)
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

std::string variantToString(const Value &a)
{
    return std::visit([](const auto &arg) -> std::string
                      {
                        std::ostringstream oss;
                        oss << arg;
                        return oss.str(); }, a);
}

bool isInVariables(Token tok)
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
bool variantToBool(const Value &value){
    if (const bool* val_ptr = std::get_if<bool>(&value)) {
        return *val_ptr;
    } else {
        return stringToBool(variantToString(value));
    }
}


Value Evaluate(const std::vector<Token>& tokens, int left, int right)
{
        variables["pi"] = {
        "pi",
        M_PI,
        true,   // isConst
        true,   // isStrict
        VariableTypes::Double
    };

    variables["e"] = {
        "e",
        M_E,
        true,   // isConst
        true,   // isStrict
        VariableTypes::Double
    };

        variables["true"] = {
        "true",
        true,
        true,   // isConst
        true,   // isStrict
        VariableTypes::Bool
    };

        variables["false"] = {
        "false",
        false,
        true,   // isConst
        true,   // isStrict
        VariableTypes::Bool
    };
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
        Value rhs = Evaluate(tokens, left + 1, right);

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

    Value lhs = Evaluate(tokens, left, split - 1);
    Value rhs = Evaluate(tokens, split + 1, right);
    switch (tokens[split].type)
    {
    
        case TokenType::Plus:
            return variantToDouble(lhs) + variantToDouble(rhs);

        case TokenType::Minus:
            return variantToDouble(lhs) - variantToDouble(rhs);

        case TokenType::Multiply:
            return variantToDouble(lhs) * variantToDouble(rhs);

        case TokenType::Division:
            return variantToDouble(lhs) / variantToDouble(rhs);

        case TokenType::Power:
            return std::pow(variantToDouble(lhs),variantToDouble(rhs));

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
    bool strictMode = false;
    Instruction instr;
    bool buildingInstr = false;
    int start = 0;
    int end = 3;
    bool argMode = false;
    std::vector<Token> buffer;
    bool declareMode = false;
    for (size_t i = 0; i < input.size(); i++)
    {
        const Token &tok = input[i];
        if (!argMode)
        {
            if (declareMode)
            {
                if (tok.type == TokenType::String ||
                    tok.type == TokenType::Number ||
                    (tok.type == TokenType::Identifier &&
                     (variantToString(tok.value) == "true" ||
                      variantToString(tok.value) == "false")))
                {
                    auto &var = variables[instr.vardata.name];

                    var.name = instr.vardata.name;
                    var.isConst = instr.vardata.isConst;
                    var.isStrict = instr.vardata.isStrict;

                    switch (tok.type)
                    {
                    case TokenType::String:
                        var.vartype = VariableTypes::String;
                        var.value = tok.value;
                        break;

                    case TokenType::Number:
                        if (std::holds_alternative<int>(tok.value))
                            var.vartype = VariableTypes::Int;
                        else
                            var.vartype = VariableTypes::Double;

                        var.value = tok.value;
                        break;

                    case TokenType::Identifier:
                    {
                        std::string value = variantToString(tok.value);

                        if (value == "true")
                        {
                            var.vartype = VariableTypes::Bool;
                            var.value = true;
                        }
                        else if (value == "false")
                        {
                            var.vartype = VariableTypes::Bool;
                            var.value = false;
                        }

                        break;
                    }

                    default:
                        break;
                    }

                    declareMode = false;
                }
            }
            if (tok.type == TokenType::Identifier)
            {
                std::string name = std::get<std::string>(tok.value);
                instr = Instruction{};
                buildingInstr = true;
                // this is for assignment   ex: x = 32 objective rn is to check if it is strict, if it is, do not allow type chancing. I FINALLY DID ITT
                if (isInVariables(input[i]) && !variables[variantToString(input[i].value)].isConst)
                {
                    if (i + 2 >= input.size())
                    {
                        // Error: incomplete declaration do error handling here later
                    }
                    else if (input[i + 1].type != TokenType::Equals)
                    {
                        // Expected = do error handling here
                    }
                    else if (input[i + 2].type == TokenType::String || input[i + 2].type == TokenType::Number || (input[i + 2].type == TokenType::Identifier && variantToString(input[i + 2].value) == "true") || (input[i + 2].type == TokenType::Identifier && variantToString(input[i + 2].value) == "false"))
                    {
                        auto &var = variables[variantToString(input[i].value)];
                        if (variables[variantToString(input[i].value)].isStrict == true)
                        {
                            std::cout << "strict \n";
                            switch (input[i + 2].type)
                            {
                            case TokenType::String:
                                if (variables[variantToString(input[i].value)].vartype == VariableTypes::String)
                                    var.value = input[i + 2].value;
                                instr.type = Instruction::Type::Assign;
                                break;
                            case TokenType::Number:
                                if (std::holds_alternative<int>(input[i + 2].value) && variables[variantToString(input[i].value)].vartype == VariableTypes::Int)
                                    var.value = input[i + 2].value;
                                instr.type = Instruction::Type::Assign;
                                break;
                            case TokenType::Identifier:
                                if (std::holds_alternative<bool>(input[i + 2].value) && variables[variantToString(input[i].value)].vartype == VariableTypes::Bool)
                                    var.value = stringToBool(variantToString(input[i + 2].value));
                                instr.type = Instruction::Type::Assign;
                                break;
                            }
                        }
                        else if (variables[variantToString(input[i].value)].isStrict == false)
                        {
                            instr.type = Instruction::Type::Assign;
                            std::cout << "not strict \n";
                            var.value = input[i + 2].value;
                        }
                    }
                }
                if (name == "strict")
                {
                    strictMode = true;
                }
                if (name == "var")
                {
                    if (i + 2 >= input.size())
                    {
                        // Error: incomplete declaration do error handling here later
                    }
                    else if (input[i + 1].type != TokenType::Identifier)
                    {
                        // Error: expected variable name do error handling here lter
                    }
                    else if (input[i + 2].type != TokenType::Equals)
                    {
                        // Error: expected '=' do error handling here later
                    }
                    else
                    {
                        auto &var = variables[instr.vardata.name];
                        instr.vardata.isConst = false;
                        instr.vardata.isStrict = strictMode;
                        var.isConst = false;
                        var.isStrict = strictMode;
                        std::cout << variantToString(input[i + 1].value);
                        instr.type = Instruction::Type::Declare;
                        instr.vardata.name = variantToString(input[i + 1].value);
                        declareMode = true;
                        strictMode = false;
                        // here i skip the identifier and '=' since they are already processed. its NOT 5 am rn ):
                        i += 2;
                    }
                }
                else if (name == "const")
                {
                    if (i + 2 >= input.size())
                    {
                        // Error: incomplete declaration, do error handling here later
                    }
                    else if (input[i + 1].type != TokenType::Identifier)
                    {
                        // Error: expected variable name, do error handling here lter
                    }
                    else if (input[i + 2].type != TokenType::Equals)
                    {
                        // Error: expected '=' do error handling here later
                    }
                    else
                    {
                        auto &var = variables[instr.vardata.name];
                        instr.vardata.isConst = true;
                        instr.vardata.isStrict = strictMode;
                        var.isConst = true;
                        var.isStrict = strictMode;
                        std::cout << variantToString(input[i + 1].value);
                        instr.type = Instruction::Type::Declare;
                        instr.vardata.name = variantToString(input[i + 1].value);
                        declareMode = true;
                        strictMode = false;
                        // here i skip the identifier and '=' since they are already processed. its NOT 5 am rn ):
                        i += 2;
                    }
                }
                else if (name == "writeln")
                {
                    instr.type = Instruction::Type::BuiltIn;
                    instr.builtin = CommandNames::WriteLn;
                }
                else if (name == "free")
                { // used to free variables from memory
                    instr.type = Instruction::Type::BuiltIn;
                    instr.builtin = CommandNames::Free;
                }
                else if (name == "write")
                {
                    instr.type = Instruction::Type::BuiltIn;
                    instr.builtin = CommandNames::Write;
                }
                else
                {
                    instr.type = Instruction::Type::NonDefined;
                    instructions.push_back(instr);
                }
            }
            else if (tok.type == TokenType::LParen)
            {
                argMode = true;
                start = tok.column;
            }
        }
        else
        {
            if (tok.type == TokenType::Comma)
            {
                continue;
            }
            else if (tok.type == TokenType::RParen)
            {
                argMode = false;
                
                if (buildingInstr)
                {
                    instructions.push_back(instr);
                    buildingInstr = false;
                }
            }
            else if (tok.type == TokenType::Identifier)
            {
                auto it = variables.find(variantToString(tok.value));
                std::cout << "start:" << start << "\n";
                std::cout << "end: "<< end << "\n";
                if (it != variables.end())
                {
                    instr.args.push_back(it->second.value);
                }
                else
                {
                    // handle undefined identifier here
                    instr.args.push_back(tok.value);
                    continue;
                }
            }
            else
            {
                instr.args.push_back(tok.value);
            }
        }
    }

    return instructions;
}