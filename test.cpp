#include "lexer.h"
#include "parser.h"
#include <iostream>
#include <vector>
#include <sstream>

    std::string variantToString2(const Value& a)
{
    return std::visit([](const auto &arg) -> std::string
                      {
                        std::ostringstream oss;
                        oss << arg;
                        return oss.str(); }, a);
}

int main()
{

    while (true)
    {
        std::string code;
        std::getline(std::cin, code);

        if (code == "exit")
            break;
        if (code.empty())
            continue;

        auto lexresult = lex(code);
        auto parseresult = parse(lexresult);

        for (size_t i = 0; i < parseresult.size(); i++)
        {
            const auto &instr = parseresult[i];

            if (instr.type == Instruction::Type::BuiltIn)
            {
                std::cout << "BuiltIn function: ";

                if (instr.builtin == CommandNames::WriteLn)
                {
                    std::cout << "WriteLn args: ";

                    for (size_t k = 0; k < instr.args.size(); k++)
                    {
                        std::visit([](const auto &arg)
                                   { std::cout << arg << "-"; }, instr.args[k]);
                    }
                    std::cout << "  arg count:" << std::size(parseresult[i].args);
                    std::cout << "\n";
                }
                else if (instr.builtin == CommandNames::Write)
                {
                    std::cout << "Write args: ";

                    for (size_t k = 0; k < instr.args.size(); k++)
                    {
                        std::visit([](const auto &arg)
                                   { std::cout << arg << "-"; }, instr.args[k]);
                    }

                    std::cout << "  arg count:" << std::size(parseresult[i].args);

                    std::cout << "\n";
                }
            }
            else if (instr.type == Instruction::Type::NonDefined)
            {
                std::cout << "Non defined instruction \n";
            }else if (instr.type == Instruction::Type::Declare){
                std::cout << "Declaretion of variable " << parseresult[i].vardata.name << " with value of "<< variantToString2(parseresult[i].vardata.value);
                if(instr.vardata.isConst) std::cout << " (Constant)";
                else if(instr.vardata.isConst == false) std::cout << " (Variable)";
            }
        }
    }
}