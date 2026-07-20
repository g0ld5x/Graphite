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

VariableTable GlobalTable;
FunctionTable GlobalFunctionTable;
void interpret(const std::vector<Instruction> &input)
{
    for (size_t i = 0; i < input.size(); i++)
    {
        Instruction instr = input[i];
        if (instr.type == Instruction::Types::FunctionDeclare)
        {
            Function &func = GlobalFunctionTable[instr.Funcname];

            func.body = instr.body;
            func.isStrict = instr.isStrict;
            func.name = instr.Funcname;
            func.locals = instr.locals;
            func.isVoid = true;
            continue;
        }
        else if (instr.type == Instruction::Types::Declare)
        { // for things like var x = 32;
            instr.vardata.value = Evaluate(instr.expression, 0, instr.expression.size() - 1, GlobalTable);
            GlobalTable[instr.vardata.name] = instr.vardata;
        }
        else if (instr.type == Instruction::Types::Assign)
        { // for things like var x = 32;
            instr.vardata = GlobalTable[instr.vardata.name];
            if (instr.vardata.isConst)
            {
                std::cerr << "Cant change the value of a constant. \n";
            }
            else
            {
                GlobalTable[instr.vardata.name].value = Evaluate(instr.expression, 0, instr.expression.size() - 1, GlobalTable);
            }
        }
        else if (instr.type == Instruction::Types::If)
        {
            if (variantToBool2(Evaluate(instr.condition, 0, instr.condition.size() - 1, GlobalTable)) == true)
            {
                interpret(instr.body);
            }
            else if (instr.elseBody.size() != 0)
            {
                interpret(instr.elseBody);
            }
        }
        if (instr.type == Instruction::Types::FunctionCall)
        {
            std::cout << "Path size: " << instr.path.size() << "\n";
            
            if (instr.path[0] == "Terminal")
            {
                if (instr.path[1] == "IO")
                {
                    if (instr.path[2] == "input")
                    {
                        std::string buffer = "";
                        std::getline(std::cin, buffer);
                        GlobalTable[variantToString2(instr.arguments[0][0].value)].value = buffer;
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
                            std::cout << variantToString2(Evaluate(instr.arguments[k], 0, instr.arguments[k].size() - 1, GlobalTable));
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
                            std::cout << variantToString2(Evaluate(instr.arguments[k], 0, instr.arguments[k].size() - 1, GlobalTable));
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
                        std::cerr << "Argument Overflow! Function 'clear' is a void, expected 0 arguments, got " << instr.arguments.size() << ". \n";
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
                interpret(GlobalFunctionTable[instr.path[0]].body);
            }
            else
            {
                std::cerr << "Unknown identifier " << instr.path[0] << "\n";
            }
        }
    }
}