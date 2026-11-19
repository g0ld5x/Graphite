#include <chrono>
#include <iostream>
#include <cstring>
#include "lexer.h"
#include "parser.h"
#include "interpreter.h"
#include <sstream>
#include <fstream>
#include <sys/resource.h>

void printMemoryUsage()
{
    struct rusage usage{};

    if (getrusage(RUSAGE_SELF, &usage) == 0)
    {
        std::cout << "Peak RAM usage: "
                  << usage.ru_maxrss << " KB\n";
    }
}
int main(int argc, char *argv[])
{
    std::vector<std::string> path;
    
    ScopeStack scope;
    FunctionTable ftable;
    initInterpreter(scope);
    if (argc == 1)
    { // repl mode
        std::cout << "Graphite version-0.1 \n";
        bool debugMode = false;
        std::string input ="";
        while (true)
        {
            
            std::getline(std::cin,input);
            if (std::strcmp(input.c_str(), "--debug") == 0)
            {
                debugMode = !debugMode;
                std::cout << "Debug mode: "
                          << (debugMode ? "ON" : "OFF")
                          << "\n";
                continue;
            }
            else if (std::strcmp(input.c_str(), "--quit") == 0)
            {
                break;
            }

            auto start = std::chrono::high_resolution_clock::now();

            interpret(parse(lex(input), path), scope, ftable);
            auto end = std::chrono::high_resolution_clock::now();
            std::cout << "\n";
            if (debugMode)
            {
                std::cout << "\n Execution Time: "
                          << std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count()
                          << " nanoseconds\n";
            }
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
        interpret(parse(lex(source), path), scope, ftable);
        auto end = std::chrono::high_resolution_clock::now();
        

        std::cout << "\n Execution Time: "
                  << std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count()
                  << " nanoseconds\n";
        printf("Instruction: %zu bytes\n",
        sizeof(Instruction));

        std::cout << "Token: " << sizeof(Token) << '\n';
    std::cout << "Value: " << sizeof(Value) << '\n';
    std::cout << "String: " << sizeof(std::string) << '\n';
    std::cout << "ArrayPtr: " << sizeof(ArrayPtr) << '\n';
    std::cout << "TokenValue:" << sizeof(TokenValue) << '\n';
    std::cout << "TokenValue:" << sizeof(ArrayValue) << '\n';


    

        printMemoryUsage();
    }
}