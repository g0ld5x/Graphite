#include <sstream>
#include <fstream>
#include <iostream>
#include "lexer.h"
#include "parser.h"
#include "interpreter.h"
int main(int argc, char* argv[])
{
    initInterpreter();
    for (int i = 1; i < argc; i++)
    {
        std::string currentArg = argv[i];

        if (currentArg.rfind("--", 0) == 0)
        {
            if (currentArg == "--help")
            {
                // help
            }
        }
        else
        {
            std::ifstream File(currentArg);

            if (!File)
            {
                std::cerr << "Could not open file: " << currentArg << "\n";
                continue;
            }

            std::stringstream buffer;
            buffer << File.rdbuf();

            std::string source = buffer.str();
            interpret(parse(lex(source)));
        }
    }
}