#include <iostream>
#include <cstring>
#include <readline/readline.h>
#include <readline/history.h>
#include "lexer.h"
#include "chrono"
#include "parser.h"
#include "interpreter.h"

int main()
{
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

        if(debugMode)
        {
            std::cout << "\n Execution Time: "
                      << std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count()
                      << " nanoseconds\n";
        }

        free(input);
    }

    return 0;
}