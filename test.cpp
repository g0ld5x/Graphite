#include "lexer.h"
#include "parser.h"
#include <iostream>
#include <vector>
#include <sstream>

int main(){
        while(true){
        std::string code;
        std::getline(std::cin, code);
        std::vector<Instruction> parsed = parse(lex(code));

        for(int i=0;i<parsed.size();i++){

            if(parsed[i].type == Instruction::Types::Declare){
                std::cout<<"Decleration of variable " << parsed[i].vardata.name <<"\n";
            }
            else if(parsed[i].type == Instruction::Types::Assign){
                std::cout<<"Assignment of variable " << parsed[i].vardata.name <<"\n";
            }
            for(int k = 0; k < parsed[i].path.size();k++){
                std::cout<<parsed[i].path[k] << "\n";
            }
            
        }
    }
}