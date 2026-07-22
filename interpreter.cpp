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

ScopeStack scopeStack;
FunctionTable GlobalFunctionTable;

bool variableExists(std::string name)
{
    for (int i = scopeStack.size() - 1; i >= 0; i--)
    {
        if (scopeStack[i].find(name) != scopeStack[i].end())
        {
            return true;
        }
    }

    return false;
}

VariableData &getVariable(std::string name)
{
    for (int i = scopeStack.size() - 1; i >= 0; i--)
    {
        auto it = scopeStack[i].find(name);

        if (it != scopeStack[i].end())
        {
            return it->second;
        }
    }

    throw std::runtime_error("Unknown variable " + name);
}

void initInterpreter()
{
    scopeStack.push_back(VariableTable{}); // global scope
    scopeStack[0]["pi"] = {
        "pi",
        M_PI,
        true, // isConst
        true, // isStrict
        true,
        VariableTypes::Double};

    scopeStack[0]["e"] = {
        "e",
        M_E,
        true, // isConst
        true, // isStrict
        true,
        VariableTypes::Double};

    scopeStack[0]["true"] = {
        "true",
        true,
        true, // isConst
        true, // isStrict
        true,
        VariableTypes::Bool};

    scopeStack[0]["false"] = {
        "false",
        false,
        true, // isConst
        true, // isStrict
        true,
        VariableTypes::Bool};
}
bool interpret(const std::vector<Instruction> &input)
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
        else if (instr.type == Instruction::Types::Return)
        {
            return true;
        }
        else if (instr.type == Instruction::Types::Declare)
        { // for things like var x = 32;
            Value value = Evaluate(instr.expression, 0, instr.expression.size() - 1, scopeStack);
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
                scopeStack.front()[instr.vardata.name] = instr.vardata;}
            else
            {
                scopeStack.back()[instr.vardata.name] = instr.vardata;
            }
        }
        else if (instr.type == Instruction::Types::Assign)
        { // for things like var x = 32;
            VariableData &target = getVariable(instr.vardata.name);
            if (target.isConst)
            {
                std::cerr << "Cant change the value of a constant. \n";
            }
            else
            {

                Value value = Evaluate(instr.expression, 0, instr.expression.size() - 1, scopeStack);
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
                    getVariable(instr.vardata.name).value = value;
                }
            }
        }
                else if (instr.type == Instruction::Types::If)
        {
            scopeStack.push_back(VariableTable{});

            if (variantToBool2(Evaluate(instr.condition, 0, instr.condition.size() - 1, scopeStack)))
            {
                bool result = interpret(instr.body);

                if (result)
                {
                    scopeStack.pop_back();
                    return true;
                }
            }
            else if (!instr.elseBody.empty())
            {
                bool result = interpret(instr.elseBody);

                if (result)
                {
                    scopeStack.pop_back();
                    return true;
                }
            }

            scopeStack.pop_back();
        }
        else if (instr.type == Instruction::Types::While)
        {
            scopeStack.push_back(VariableTable{});
            while (variantToBool2(Evaluate(instr.condition, 0, instr.condition.size() - 1, scopeStack)) == true)
            {
                bool result = interpret(instr.body);

                if (result)
                {
                    scopeStack.pop_back();
                    return true;
                }
            }
            scopeStack.pop_back();
        }
        if (instr.type == Instruction::Types::FunctionCall)
        {

            if (instr.path[0] == "Int")
            { // for casting Int(varname);
                VariableData &target = getVariable(variantToString2(instr.arguments[0][0].value));
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
            else if (instr.path[0] == "Double")
            { // for casting Int(varname);
                VariableData &target = getVariable(variantToString2(instr.arguments[0][0].value));
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
            else if (instr.path[0] == "free")
            {
                if (instr.arguments.size() == 0)
                {
                    std::cerr << "Function 'free' requires atleast 1 argument";
                }
                for (int k = 0; k < instr.arguments.size(); k++)
                {
                    scopeStack.back().erase(variantToString2(instr.arguments[k][0].value));
                }
            }
            else if (instr.path[0] == "Terminal")
            {
                if (instr.path[1] == "IO")
                {
                    if (instr.path[2] == "input")
                    {
                        if (instr.arguments.size() == 0)
                        {
                            std::string buffer = "";
                            std::getline(std::cin, buffer);
                        }
                        else
                        {
                            VariableData &target = getVariable(variantToString2(instr.arguments[0][0].value));
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
                    else if (instr.path[2] == "print")
                    {
                        /* for debugging
                            try{
                                std::cout <<"size: " <<instr.arguments.size();
                                std::cout << "subsize: " << instr.arguments[0].size() << "\n";
                                std::cout <<"Start: " <<variantToString2(instr.arguments[0][0].value) << "  End: " << variantToString2(instr.arguments[0][instr.arguments[0].size()-1].value) << "\n";
                            }catch(int a){
                                std::cout <<"Error here";
                            }
                                */
                        for (size_t k = 0; k < instr.arguments.size(); k++)
                        {
                            std::cout << variantToString2(Evaluate(instr.arguments[k], 0, instr.arguments[k].size() - 1, scopeStack));
                        }
                    }
                    else if (instr.path[2] == "println")
                    {
                        /* for debugging
                            try{
                                std::cout <<"size: " <<instr.arguments.size();
                                std::cout << "subsize: " << instr.arguments[0].size() << "\n";
                                std::cout <<"Start: " <<variantToString2(instr.arguments[0][0].value) << "  End: " << variantToString2(instr.arguments[0][instr.arguments[0].size()-1].value) << "\n";
                            }catch(int a){
                                std::cout <<"Error here";
                            }
                                */
                        for (size_t k = 0; k < instr.arguments.size(); k++)
                        {
                            std::cout << variantToString2(Evaluate(instr.arguments[k], 0, instr.arguments[k].size() - 1, scopeStack));
                        }
                        std::cout << "\n";
                    }
                    else
                    {
                        std::cerr << "Unknown identifier " << instr.path[2] << "\n";
                    }
                }
                else if (instr.path[1] == "clear")
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
                else
                {
                    std::cerr << "Unknown identifier " << instr.path[1] << "\n";
                }
            }
            else if (isInFunctions(instr.path[0], GlobalFunctionTable))
            {
                auto targetFunc = GlobalFunctionTable[instr.path[0]];
                size_t count = std::min(instr.arguments.size(), targetFunc.locals.size());
                if (instr.arguments.size() != targetFunc.locals.size())
{
                std::cout << "Error: function " << instr.path[0]
                        << " expected "
                        << targetFunc.locals.size()
                        << " arguments but got "
                        << instr.arguments.size()
                        << "\n";

                return true;
            }
                for(int k = 0; k < count;k++){
                    targetFunc.locals[k].value = Evaluate(instr.arguments[k],0,instr.arguments[k].size()-1,scopeStack);
                }
                VariableTable bufferTable;
                
                for (const auto& var : targetFunc.locals)
                {
                    
                    bufferTable[var.name] = var;
                }
                scopeStack.push_back(bufferTable);
                bool didReturn = interpret(targetFunc.body);

                scopeStack.pop_back();

                if (didReturn)
                {
                    return true;
                }
            }
            else
            {
                std::cerr << "Unknown identifier " << instr.path[0] << "\n";
            }
        }
    }
    return false;
}