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
#include <type_traits>
#include <string_view>


// here we goo

Value tokenToValue(const TokenValue& tokenValue)
{
    return std::visit([](const auto& value) -> Value {
        using T = std::decay_t<decltype(value)>;

        if constexpr (std::is_same_v<T, std::string_view>) {
            return std::string(value);
        } else {
            return value;
        }
    }, tokenValue);
}

std::string arrayToString(const std::shared_ptr<ArrayValue> &array);



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
                              return VariableTypes::Array; },
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
                          } },
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
    const Token &tok,
    const VariableTable &variables)
{
    return variables.find(variantToString(tokenToValue(tok.value))) != variables.end();
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
    const std::vector<TokenRange> &arguments,
    FunctionTable &ftable,
    ScopeStack &scopeStack)
{
    auto it = ftable.find(name);

    if (it == ftable.end())
    {
        std::cerr << "Error: function " << name
                  << " does not exist\n";
        return Value{};
    }

    const GFunction &targetFunc = it->second;

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
            ftable);

        bufferTable[var.name] = std::move(var);
    }

    scopeStack.push_back(std::move(bufferTable));

    const ExecutionResult &result =
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
    const TokenRange &tokens,
    ScopeStack &scope,
    FunctionTable &functionTable)
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
                int bracketDepth = 0;
                size_t argStart = parenIndex + 1;

                for (size_t i = parenIndex + 1;
                     i < closing;
                     ++i)
                {
                    const Token &currentToken = tokens[i];

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
                        else if (currentToken.type == TokenType::LBrac)
                        {
                            bracketDepth++;
                        }
                        else if (currentToken.type == TokenType::RBrac)
                        {
                            bracketDepth--;
                        }
                    else if (currentToken.type == TokenType::Comma && argDepth == 1 && bracketDepth == 0)
                    {
                        if (argStart < i)
                        {
                            bufferArg.push_back(
                                tokens.subrange(argStart, i - 1));
                        }

                        argStart = i + 1;
                    }
                }

                // last argument
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

                        funcPath += variantToString( tokenToValue(tokens[i].value));
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
            VariableData *variable = getVariable(
                variantToString(tokenToValue(tokens[0].value)),
                scope);

            if (variable == nullptr)
            {
                throw std::runtime_error(
                    "Unknown identifier in eval " +
                    variantToString(tokenToValue(tokens[0].value)));
            }

            return variable->value;
        }

        return tokenToValue(tokens[0].value);
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

    if (range.size() > 0 &&
        range[0].type == TokenType::LBrac)
    {
        int depth = 1;
        size_t closing = range.size();
        bool foundClosing = false;

        // Find the matching closing bracket.
        for (size_t i = 1; i < range.size(); ++i)
        {
            if (range[i].type == TokenType::LBrac)
                depth++;

            else if (range[i].type == TokenType::RBrac)
            {
                depth--;

                if (depth == 0)
                {
                    closing = i;
                    foundClosing = true;
                    break;
                }
            }
        }

        if (!foundClosing)
            throw std::runtime_error("Missing closing ']'");

        // Ignore trailing newlines after the array.
        size_t last = range.size();

        while (last > closing + 1 &&
               range[last - 1].type == TokenType::NewLine)
        {
            last--;
        }

        // The array must occupy the complete expression.
        if (last == closing + 1)
        {
            auto array = std::make_shared<ArrayValue>();

            size_t start = 1;
            int elementDepth = 0;

            for (size_t i = start; i < closing; ++i)
            {
                if (range[i].type == TokenType::LParen ||
                    range[i].type == TokenType::LBrac ||
                    range[i].type == TokenType::LCurl)
                {
                    elementDepth++;
                }
                else if (range[i].type == TokenType::RParen ||
                         range[i].type == TokenType::RBrac ||
                         range[i].type == TokenType::RCurl)
                {
                    elementDepth--;
                }

                if (range[i].type == TokenType::Comma &&
                    elementDepth == 0)
                {
                    // Element range uses an exclusive end here.
                    size_t elementStart = start;
                    size_t elementEnd = i;

                    // Remove leading newlines.
                    while (elementStart < elementEnd &&
                           range[elementStart].type == TokenType::NewLine)
                    {
                        elementStart++;
                    }

                    // Remove trailing newlines.
                    while (elementEnd > elementStart &&
                           range[elementEnd - 1].type == TokenType::NewLine)
                    {
                        elementEnd--;
                    }

                    if (elementStart < elementEnd)
                    {
                        array->values.push_back(
                            Evaluate(
                                range.subrange(
                                    elementStart,
                                    elementEnd - 1),
                                scope,
                                functionTable));
                    }

                    start = i + 1;
                }
            }

            // Handle the final element.
            size_t elementStart = start;
            size_t elementEnd = closing;

            while (elementStart < elementEnd &&
                   range[elementStart].type == TokenType::NewLine)
            {
                elementStart++;
            }

            while (elementEnd > elementStart &&
                   range[elementEnd - 1].type == TokenType::NewLine)
            {
                elementEnd--;
            }

            if (elementStart < elementEnd)
            {
                array->values.push_back(
                    Evaluate(
                        range.subrange(
                            elementStart,
                            elementEnd - 1),
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

        if (open > 0)
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
        else if (std::holds_alternative<int>(lhs) && std::holds_alternative<int>(rhs))
        {
            return variantToInt(lhs) + variantToInt(rhs);
        }

        return variantToDouble(lhs) +
               variantToDouble(rhs);

    case TokenType::Remainder:
        return std::fmod(
            variantToDouble(lhs),
            variantToDouble(rhs));

    case TokenType::Minus:
        if (std::holds_alternative<int>(lhs) && std::holds_alternative<int>(rhs))
        {
            return variantToInt(lhs) + variantToInt(rhs);
        }
        else
        {
            return variantToDouble(lhs) -
                   variantToDouble(rhs);
        }

    case TokenType::Multiply:

        if (std::holds_alternative<std::string>(lhs) &&
            std::holds_alternative<int>(rhs))
        {
            const std::string &str =
                std::get<std::string>(lhs);

            int count = std::get<int>(rhs);

            std::string buffer;
            buffer.reserve(str.size() * count);

            for (int i = 0; i < count; ++i)
                buffer += str;

            return buffer;
        }

        else if (std::holds_alternative<int>(lhs) &&
                 std::holds_alternative<std::string>(rhs))
        {
            const std::string &str =
                std::get<std::string>(rhs);

            int count = std::get<int>(lhs);

            std::string buffer;
            buffer.reserve(str.size() * count);

            for (int i = 0; i < count; ++i)
                buffer += str;

            return buffer;
        }
        else if (std::holds_alternative<int>(lhs) && std::holds_alternative<int>(rhs))
        {
            return variantToInt(lhs) * variantToInt(rhs);
        }
        else
        {

            return variantToDouble(lhs) *
                   variantToDouble(rhs);
        }

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
        std::string value = variantToString(tokenToValue(tok.value));
        if (tok.type == TokenType::Identifier)
        { // could be a function,if,for,while,fn,var,const,strict or a function call like Terminal.IO.print("hello world!");
            if (value == "use")
            {
                
                auto& use = instr.data.template emplace<UseInstruction>();
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
                    use.importPath.push_back(variantToString(tokenToValue(input[i].value)));
                }
                else
                {

                    i++; // skip {

                    while (i < input.size() && input[i].type != TokenType::RCurl)
                    {
                        if (input[i].type == TokenType::String)
                        {
                            use.importPath.push_back(
                                variantToString( tokenToValue(input[i].value)));
                        }

                        i++;
                    }
                }
                instructions.push_back(std::move(use));
            }

            else if (value == "fn")
            {
                auto& fndeclare = instr.data.template emplace<FunctionDeclareInstruction>();
                fndeclare.isStrict = strictMode;

                if (i + 1 < input.size() && input[i + 1].type == TokenType::Identifier)
                {
                    std::string functionName = variantToString( tokenToValue(input[i + 1].value));

                    if (!path.empty())
                    {
                        std::string prefix = joinpath(path, ".");

                        functionName = prefix + "." + functionName;
                    }
                    fndeclare.Funcname = functionName;

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

                        if (fndeclare.locals.size() <= slot)
                            fndeclare.locals.resize(slot + 1);

                        std::string name = variantToString( tokenToValue(input[i].value));
                        if (name == "string")
                            fndeclare.locals[slot].vartype = VariableTypes::String;
                        else if (name == "int")
                            fndeclare.locals[slot].vartype = VariableTypes::Int;
                        else if (name == "bool")
                            fndeclare.locals[slot].vartype = VariableTypes::Bool;
                        else if (name == "double")
                           fndeclare.locals[slot].vartype = VariableTypes::Double;
                        else if (name == "value[]")
                            fndeclare.locals[slot].vartype = VariableTypes::Array;
                        else
                            fndeclare.locals[slot].name = name;
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
                auto parsedBody = parse(bodyTokens, empty);
                fndeclare.body = std::move(parsedBody);

                instructions.push_back(std::move(fndeclare));

                continue;
            }

            else if (value == "break")
            {
                instructions.push_back(instr.data.template emplace<BreakInstruction>());
            }
            else if (value == "continue")
            {
                instructions.push_back(instr.data.template emplace<ContinueInstruction>());
            }
            else if (value == "for")
            { // for(fruit in fruits) here fruits is the iterateable so it will be stored as a vector of tokens, because it can be a function returning an array or things like that. The object can be stored as a string.
                auto & forstm = instr.data.template emplace<ForInstruction>();
                int curlyDepth = 0;
                // parse the for object
                ++i;
                ++i;
                if (input[i + 1].type == TokenType::ForDivider)
                {
                    
                    forstm.object = variantToString( tokenToValue(input[i].value));
                    i++;
                }
                else
                {
                    std::cerr << "Wrong for loop syntax";
                }
                int paranDepth = 1;
                while (i + 1 < input.size() && paranDepth != 0)
                {
                    ++i;
                    if (input[i].type == TokenType::RParen)
                    {
                        paranDepth--;
                    }
                    else if (input[i].type == TokenType::LParen)
                    {
                        paranDepth++;
                    }
                    if (paranDepth > 0)
                    {
                        forstm.iterateable.push_back(input[i]);
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

                    forstm.body = parse(bodyTokens, path);
                }
                instructions.push_back(std::move(forstm));
            }

            else if (value == "while")
            {
                auto & whlstm = instr.data.template emplace<WhileInstruction>();

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
                            whlstm.condition.push_back(input[i]);
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

                    whlstm.body = parse(bodyTokens, path);
                }

                instructions.push_back(std::move(whlstm));
            }
            else if (value == "if")
            {
                
                auto & ifstm = instr.data.template emplace<IfInstruction>();

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
                            ifstm.condition.push_back(input[i]);
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

                    ifstm.body = parse(bodyTokens, path);
                }
                while (i + 1 < input.size() &&
                       input[i + 1].type == TokenType::NewLine)
                {
                    i++;
                }

                if (i + 1 < input.size() &&
                    input[i + 1].type == TokenType::Identifier &&
                    variantToString( tokenToValue(input[i + 1].value)) == "else")
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

                        ifstm.elseBody = parse(elsebodyTokens, path);
                    }
                }
                instructions.push_back(std::move(ifstm));
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
                auto & spcstm = instr.data.template emplace<SpaceInstruction>();
                std::string name;

                if (i + 1 < input.size() && input[i + 1].type == TokenType::Identifier)
                {
                    name = variantToString( tokenToValue(input[i + 1].value));
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
                spcstm.body = parse(bodyTokens, path);

                path.pop_back();

                instructions.push_back(std::move(spcstm));

                continue;
            }

            else if (value == "var")
            {
                auto & varstm = instr.data.template emplace<DeclareInstruction>();
                int paranDepth = 0;
                bool canContinue = true;
            
                
                varstm.vardata.isConst = false;
                varstm.vardata.isStrict = strictMode;
                varstm.vardata.isGlobal = globalMode;
                strictMode = false;
                globalMode = false;

                if (i + 1 < input.size() && input[i + 1].type == TokenType::Identifier)
                {
                    varstm.vardata.name = variantToString( tokenToValue(input[i + 1].value));
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
                int expressionDepth = 0;

                while (i + 3 < input.size())
                {
                    const Token &token = input[i + 3];

                    if (token.type == TokenType::EndOfFile)
                        break;

                    if (expressionDepth == 0 &&
                        (token.type == TokenType::NewLine ||
                         token.type == TokenType::Semicolon))
                    {
                        break;
                    }

                    if (token.type == TokenType::LParen ||
                        token.type == TokenType::LBrac ||
                        token.type == TokenType::LCurl)
                    {
                        expressionDepth++;
                    }
                    else if (token.type == TokenType::RParen ||
                             token.type == TokenType::RBrac ||
                             token.type == TokenType::RCurl)
                    {
                        expressionDepth--;
                    }

                    varstm.expression.push_back(token);
                    i++;
                }
                instructions.push_back(std::move(varstm));
            }
            else if (value == "return")
            {
                auto & retstm = instr.data.template emplace<ReturnInstruction>();
                i++;

                while (input[i].type != TokenType::Semicolon && input[i].type != TokenType::NewLine && input[i].type != TokenType::EndOfFile)
                {
                    retstm.expression.push_back(input[i]);
                    i++;
                }

                instructions.push_back(std::move(retstm));
            }
            else if (value == "const")
            {
                auto & varstm = instr.data.template emplace<DeclareInstruction>();
                int paranDepth = 0;
                bool canContinue = true;
                
                varstm.vardata.isConst = true;
                varstm.vardata.isStrict = strictMode;
                varstm.vardata.isGlobal = globalMode;
                strictMode = false;
                globalMode = false;
                if (i + 1 < input.size() && input[i + 1].type == TokenType::Identifier)
                {
                    varstm.vardata.name = variantToString( tokenToValue(input[i + 1].value));
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
                    varstm.expression.push_back(input[i + 3]);
                    i++;
                }
                instructions.push_back(std::move(varstm));
            }

            else if (i + 1 < input.size() &&
                     input[i + 1].type == TokenType::Equals)
            {
                auto & assignstm = instr.data.template emplace<AssignInstruction>();
                assignstm.vardata.name = variantToString( tokenToValue(input[i].value));

                int j = i + 2;
                int depth = 0;

                while (j < input.size())
                {
                    auto type = input[j].type;

                    if (type == TokenType::LBrac ||
                        type == TokenType::LParen ||
                        type == TokenType::LCurl)
                    {
                        depth++;
                    }
                    else if (type == TokenType::RBrac ||
                             type == TokenType::RParen ||
                             type == TokenType::RCurl)
                    {
                        depth--;
                    }

                    if (depth == 0 &&
                        (type == TokenType::NewLine ||
                         type == TokenType::Semicolon ||
                         type == TokenType::EndOfFile))
                    {
                        break;
                    }
                    assignstm.expression.push_back(input[j]);
                    j++;
                }

                i = j;

                instructions.push_back(std::move(assignstm));
                continue;
            }
            else if (i + 1 < input.size() &&
                     input[i + 1].type == TokenType::LBrac)
            {
                auto & assignstm = instr.data.template emplace<AssignInstruction>();
                
                assignstm.vardata.name = variantToString( tokenToValue(input[i].value));

                // Collect consecutive indexing operations.
                std::size_t j = i + 1;

                while (j < input.size() &&
                       input[j].type == TokenType::LBrac)
                {
                    j++; // Skip '['.

                    int bracketDepth = 1;
                    std::vector<Token> indexTokens;

                    while (j < input.size() && bracketDepth > 0)
                    {
                        if (input[j].type == TokenType::LBrac)
                        {
                            bracketDepth++;
                        }
                        else if (input[j].type == TokenType::RBrac)
                        {
                            bracketDepth--;
                        }

                        if (bracketDepth > 0 &&
                            input[j].type != TokenType::NewLine)
                        {
                            indexTokens.push_back(input[j]);
                        }

                        j++;
                    }

                    if (bracketDepth != 0)
                        throw std::runtime_error("Missing closing ']'");

                    if (indexTokens.empty())
                        throw std::runtime_error("Empty index expression");
                    assignstm.indexExpressions.push_back(
                        std::move(indexTokens));
                }

                // j points to the token after the final ']'.
                if (j < input.size() &&
                    input[j].type == TokenType::Equals)
                {
                    std::size_t a = j + 1;
                    int depth = 0;

                    while (a < input.size())
                    {
                        auto type = input[a].type;

                        if (type == TokenType::LBrac ||
                            type == TokenType::LParen ||
                            type == TokenType::LCurl)
                        {
                            depth++;
                        }
                        else if (type == TokenType::RBrac ||
                                 type == TokenType::RParen ||
                                 type == TokenType::RCurl)
                        {
                            depth--;
                        }

                        if (depth == 0 &&
                            (type == TokenType::NewLine ||
                             type == TokenType::Semicolon ||
                             type == TokenType::EndOfFile))
                        {
                            break;
                        }

                        assignstm.expression.push_back(input[a]);
                        a++;
                    }

                    i = a;

                    instructions.push_back(std::move(assignstm));
                    continue;
                }
                else
                {
                    i = j;
                }
            }

            else if (
                (i + 1 < input.size() &&
                 (input[i + 1].type == TokenType::LParen ||
                  input[i + 1].type == TokenType::Dot)))
            {
                auto & fncall = instr.data.template emplace<FunctionCallInstruction>();
                fncall.path = path;

                while (i < input.size() && input[i].type == TokenType::Identifier)
                {
                    fncall.path.push_back(variantToString( tokenToValue(input[i].value)));

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
                                    if (fncall.arguments.size() <= argCount)
                                        fncall.arguments.resize(argCount + 1);

                                    fncall.arguments[argCount].push_back(current);
                                }
                            }

                            else if (current.type == TokenType::RParen)
                            {
                                paranDepth--;

                                // store nested ')' but not the function's closing ')'
                                if (paranDepth > 0)
                                {
                                    if (fncall.arguments.size() <= argCount)
                                        fncall.arguments.resize(argCount + 1);

                                    fncall.arguments[argCount].push_back(current);
                                }
                            }

                            else if (current.type == TokenType::Comma && paranDepth == 1)
                            {
                                argCount++;
                            }

                            else
                            {
                                if (fncall.arguments.size() <= argCount)
                                    fncall.arguments.resize(argCount + 1);

                                fncall.arguments[argCount].push_back(current);
                            }
                        }

                        if (fncall.arguments.size() == 1 && fncall.arguments[0].empty())
                        {
                            fncall.arguments.clear();
                        }
                    }

                    break;
                }

                instructions.push_back(std::move(fncall));
            }
        }
    }
    return instructions;
}