#include "parser.h"
#include "lexer.h"
#include "interpreter.h"
#include <iostream>
#include <string>
#include <chrono>
int main(){

    while(true){
        
        std::string code = "";
        std::getline(std::cin,code);
        auto start = std::chrono::high_resolution_clock::now();
        auto program = parse(lex(code));
        std::cout << "Program size: " << program.size() << "\n";

for(size_t i = 0; i < program.size(); i++)
{
    std::cout << "Instruction " << i 
              << " type: " 
              << static_cast<int>(program[i].type)
              << " path size: "
              << program[i].path.size()
              << " body size: "
              << program[i].body.size()
              << "\n";

    for(auto& p : program[i].path)
    {
        std::cout << "  path: " << p << "\n";
    }
}
        interpret(program);
        auto end = std::chrono::high_resolution_clock::now();

        auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);

        std::cout << "Execution took: " << duration.count() << " ns\n";
    }
    return 0;
}