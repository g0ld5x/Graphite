#include <iostream>
#include <cstring>
#include <readline/readline.h>
#include <readline/history.h>
#include "lexer.h"
#include "chrono"
#include "parser.h"
#include "interpreter.h"
#include <sstream>
#include <fstream>
int main(int argc, char* argv[])
{
    initInterpreter();
    if(argc == 1){ //repl mode
    bool debugMode = false;
    while (true)
    {
        char* input = readline("graphite-<< ");

        if (!input)
            break;

        if (*input)
            add_history(input);

        if (strcmp(input, "--debug") == 0)
        {
            debugMode = !debugMode;
            std::cout << "Debug mode: " 
                      << (debugMode ? "ON" : "OFF") 
                      << "\n";

            free(input);
            continue;
        }else if(strcmp(input,"--quit") == 0){
            break;
        }

        auto start = std::chrono::high_resolution_clock::now();

        interpret(parse(lex(input)));
        auto end = std::chrono::high_resolution_clock::now();
        std::cout << "\n";
        if(debugMode)
        {
            std::cout << "\n Execution Time: "
                      << std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count()
                      << " nanoseconds\n";
        }

        free(input);
    }
    }
    for (int i = 1; i < argc; i++)
    {
        std::string currentArg = argv[i];

        if (currentArg.rfind("--", 0) == 0)
        {
            if (currentArg == "--help")
            {
                std::cout << "Provide the file name to interpret it.";
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