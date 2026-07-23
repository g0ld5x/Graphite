#include "parser.h"
#include <string>
#include <vector>
#include <memory>
#include <numbers>
#include <format>
#include <algorithm>
#include <variant>
#include "lexer.h"
#include "interpreter.h"
#include <iostream>
#include <numeric>
#include <math.h>
#include <sstream>
#include <climits>
#include <print>

bool variantToBool2(const Value &value)
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

std::string variantToString2(const Value &a)
{
    return std::visit([](const auto &arg) -> std::string
                      {
                        std::ostringstream oss;
                        oss << arg;
                        return oss.str(); }, a);
}

bool isInFunctions(std::string name, FunctionTable functions)
{
    auto it = functions.find(name);
    if (it != functions.end())
    {
        return true;
    }
    else
    {
        return false;
    }
}

std::string join(const std::vector<std::string> &elements, const std::string &delimiter)
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

FunctionTable GlobalFunctionTable;

bool variableExists(std::string name, ScopeStack &scope)
{
    for (int i = scope.size() - 1; i >= 0; i--)
    {
        if (scope[i].find(name) != scope[i].end())
        {
            return true;
        }
    }

    return false;
}

VariableData &getVariable2(std::string name, ScopeStack &scope)
{
    for (int i = scope.size() - 1; i >= 0; i--)
    {
        auto it = scope[i].find(name);

        if (it != scope[i].end())
        {
            return it->second;
        }
    }

    throw std::runtime_error("Unknown variable " + name);
}

void initInterpreter(ScopeStack &scope)
{
    scope.push_back(VariableTable{}); // global scope
    scope[0]["pi"] = {
        "pi",
        M_PI,
        true, // isConst
        true, // isStrict
        true,
        VariableTypes::Double};

    scope[0]["e"] = {
        "e",
        M_E,
        true, // isConst
        true, // isStrict
        true,
        VariableTypes::Double};

    scope[0]["true"] = {
        "true",
        true,
        true, // isConst
        true, // isStrict
        true,
        VariableTypes::Bool};

    scope[0]["false"] = {
        "false",
        false,
        true, // isConst
        true, // isStrict
        true,
        VariableTypes::Bool};
}
ExecutionResult interpret(const std::vector<Instruction> &input, ScopeStack &scope)
{
    bool returned = false;
    for (size_t i = 0; i < input.size(); i++)
    {
        Instruction instr = input[i];
        if (instr.type == Instruction::Types::FunctionDeclare)
        {
            GFunction &func = GlobalFunctionTable[instr.Funcname];

            func.body = instr.body;
            func.isStrict = instr.isStrict;
            func.name = instr.Funcname;
            func.locals = instr.locals;
            func.isVoid = true;
        }
        else if (instr.type == Instruction::Types::Space)
        {
            interpret(instr.body, scope);
        }
        else if (instr.type == Instruction::Types::Return)
        {

            ExecutionResult exec;
            exec.didReturn = true;

            for (const auto &token : instr.expression)
            {
            }
            exec.returnValue = Evaluate(
                instr.expression,
                0,
                instr.expression.size() - 1,
                scope,
                GlobalFunctionTable);

            return exec;
        }
        else if (instr.type == Instruction::Types::Declare)
        {

            Value value = Evaluate(
                instr.expression,
                0,
                instr.expression.size() - 1,
                scope,
                GlobalFunctionTable);

            if (std::holds_alternative<int>(value))
            {
                instr.vardata.vartype = VariableTypes::Int;
            }
            else if (std::holds_alternative<std::string>(value))
            {
                instr.vardata.vartype = VariableTypes::String;
            }
            else if (std::holds_alternative<bool>(value))
            {
                instr.vardata.vartype = VariableTypes::Bool;
            }
            else if (std::holds_alternative<double>(value))
            {
                instr.vardata.vartype = VariableTypes::Double;
            }

            instr.vardata.value = value;

            if (instr.vardata.isGlobal)
            {

                scope.front()[instr.vardata.name] = instr.vardata;
            }
            else
            {

                scope.back()[instr.vardata.name] = instr.vardata;
            }
        }
        else if (instr.type == Instruction::Types::Assign)
        { // for things like var x = 32;
            VariableData &target = getVariable2(instr.vardata.name, scope);
            if (target.isConst)
            {
                std::cerr << "Cant change the value of a constant. \n";
            }
            else
            {

                Value value = Evaluate(instr.expression, 0, instr.expression.size() - 1, scope, GlobalFunctionTable);
                if (target.isStrict)
                {
                    if (std::holds_alternative<int>(value) && instr.vardata.vartype == VariableTypes::Int)
                    {
                        target.value = value;
                    }
                    else if (std::holds_alternative<std::string>(value) && instr.vardata.vartype == VariableTypes::String)
                    {
                        target.value = value;
                    }
                    else if (std::holds_alternative<double>(value) && instr.vardata.vartype == VariableTypes::Double)
                    {
                        target.value = value;
                    }
                    else if (std::holds_alternative<bool>(value) && instr.vardata.vartype == VariableTypes::Bool)
                    {
                        target.value = value;
                    }
                    else
                    {
                        std::cerr << "Cannot change the type of a strict variable";
                    }
                }
                else
                {
                    getVariable2(instr.vardata.name, scope).value = value;
                }
            }
            continue;
        }
        else if (instr.type == Instruction::Types::If)
        {
            scope.push_back(VariableTable{});

            if (variantToBool2(Evaluate(instr.condition, 0, instr.condition.size() - 1, scope, GlobalFunctionTable)))
            {
                ExecutionResult result = interpret(instr.body, scope);

                if (result.didReturn == true)
                {
                    scope.pop_back();
                    return result;
                }
            }
            else if (!instr.elseBody.empty())
            {
                ExecutionResult result = interpret(instr.elseBody, scope);

                if (result.didReturn)
                {
                    scope.pop_back();
                    return result;
                }
            }

            scope.pop_back();
        }
        else if (instr.type == Instruction::Types::While)
        {
            while (variantToBool2(Evaluate(
                instr.condition,
                0,
                instr.condition.size() - 1,
                scope,
                GlobalFunctionTable)))
            {
                ExecutionResult result = interpret(instr.body, scope);

                if (result.didReturn)
                    return result;
            }
        }

        else if (instr.type == Instruction::Types::FunctionCall)
        {

            std::string path = join(instr.path, ".");

            if (path == "Int")
            { // for casting Int(varname);
                VariableData &target = getVariable2(variantToString2(instr.arguments[0][0].value), scope);
                if (target.isStrict)
                {
                    std::cerr << "Cannot cast strict variables";
                }
                else
                {
                    if (target.vartype != VariableTypes::Int)
                    {
                        target.value = std::stoi(variantToString2(target.value));
                    }
                    target.vartype = VariableTypes::Int;
                }
            }
            else if (path == "Double")
            { // for casting Int(varname);
                VariableData &target = getVariable2(variantToString2(instr.arguments[0][0].value), scope);
                if (target.isStrict)
                {
                    std::cerr << "Cannot cast strict variables";
                }
                else
                {
                    if (target.vartype != VariableTypes::Double)
                    {
                        target.value = std::stod(variantToString2(target.value));
                    }
                    target.vartype = VariableTypes::Double;
                }
            }
            else if (path == "Terminal.IO.print")
            {
                for (size_t k = 0; k < instr.arguments.size(); k++)
                {
                    std::cout << variantToString2(Evaluate(instr.arguments[k], 0, instr.arguments[k].size() - 1, scope, GlobalFunctionTable));
                }
            }
            else if (path == "Terminal.IO.println")
            {
                for (size_t k = 0; k < instr.arguments.size(); k++)
                {
                    std::cout << variantToString2(Evaluate(instr.arguments[k], 0, instr.arguments[k].size() - 1, scope, GlobalFunctionTable));
                    std::cout << "\n";
                }
            }
            else if (path == "Terminal.IO.input")
            {

                if (instr.arguments.size() == 0)
                {
                    std::string buffer = "";
                    std::getline(std::cin, buffer);
                }
                else
                {
                    VariableData &target = getVariable2(variantToString2(instr.arguments[0][0].value), scope);
                    if (!target.isConst)
                    {
                        if (target.vartype == VariableTypes::String || !target.isStrict)
                        {
                            std::string buffer = "";
                            std::getline(std::cin, buffer);

                            target.value = buffer;

                            target.vartype = VariableTypes::String;
                        }
                        else
                        {
                            std::cerr << "Cannot change the type of a strict variable.";
                        }
                    }
                    else
                    {
                        std::cerr << "Cannot change the value of a constant.";
                    }
                }
            }
            else if (path == "Terminal.clear")
            {
                if (instr.arguments.size() != 0)
                {
                    std::cerr << "Argument Overflow! Function 'clear' expected 0 arguments, got " << instr.arguments.size() << ". \n";
                }
                else
                {
                    std::cout << "\033[2J\033[1;1H" << std::flush;
                }
            }
            else if (path == "free")
            {

                if (instr.arguments.size() == 0)
                {
                    std::cerr << "Function 'free' requires atleast 1 argument";
                }
                else
                {
                    for (int k = 0; k < instr.arguments.size(); k++)
                    {
                        scope.back().erase(variantToString2(instr.arguments[k][0].value));
                    }
                }
            }
            else if (isInFunctions(path, GlobalFunctionTable))
            {
                GFunction &targetFunc = GlobalFunctionTable[path];
                size_t count = std::min(instr.arguments.size(), targetFunc.locals.size());
                if (instr.arguments.size() != targetFunc.locals.size())
                {
                    std::cerr << "Error: function " << path
                              << " expected "
                              << targetFunc.locals.size()
                              << " arguments but got "
                              << instr.arguments.size()
                              << "\n";
                    ExecutionResult result;
                    result.didReturn = false;
                    result.returnValue = nullptr;
                    return result;
                }
                for (int k = 0; k < count; k++)
                {
                    targetFunc.locals[k].value = Evaluate(instr.arguments[k], 0, instr.arguments[k].size() - 1, scope, GlobalFunctionTable);
                }
                VariableTable bufferTable;

                for (const auto &var : targetFunc.locals)
                {

                    bufferTable[var.name] = var;
                }
                scope.push_back(bufferTable);
                ExecutionResult Return = interpret(targetFunc.body, scope);

                scope.pop_back();
                if (Return.didReturn)
                {
                    return Return;
                }
            }
            else
            {
                std::cerr << "Unknown function '" << path << "' \n";
            }
        }
    }
    return ExecutionResult{};
}