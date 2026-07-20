#include "lexer.h"
#include "parser.h"
#include "interpreter.h"
#include <vector>
#include <iostream>
#include <string>

int main(){
        while(true){
        std::string code;
        std::getline(std::cin, code);
        std::vector<Instruction> parsed = parse(lex(code));
        interpret(parsed);
    }
}