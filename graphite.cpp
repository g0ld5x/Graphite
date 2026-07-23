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
    ScopeStack scope;
    initInterpreter(scope);
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

        interpret(parse(lex(input)),scope);
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
    bool debugMode = false;
    for (int i = 1; i < argc; i++)
    {
        std::string currentArg = argv[i];
        
        
            std::ifstream File(currentArg);

            if (!File)
            {
                std::cerr << "Could not open file: " << currentArg << "\n";
                continue;
            }

            std::stringstream buffer;
            buffer << File.rdbuf();

            std::string source = buffer.str();
            auto start = std::chrono::high_resolution_clock::now();
            interpret(parse(lex(source)),scope);
            auto end = std::chrono::high_resolution_clock::now();

                            std::cout << "\n Execution Time: "
                      << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count()
                      << " miliseconds\n";
            
        
    }
}