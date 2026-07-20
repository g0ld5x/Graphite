#include "lexer.h"
#include "parser.h"
#include <iostream>
#include <numeric>
#include <vector>
#include <sstream>

std::string variantToString2(const Value &a)
{
    return std::visit([](const auto &arg) -> std::string
                      {
                        std::ostringstream oss;
                        oss << arg;
                        return oss.str(); }, a);
}

int main(){
        while(true){
        std::string code;
        std::getline(std::cin, code);
        std::vector<Instruction> parsed = parse(lex(code));

        for(int i=0;i<parsed.size();i++){
            std::cout << parsed[i].path.size() << "\n";
            std::cout << "Intsruction number " <<i<<"\n";
            if(parsed[i].type == Instruction::Types::Declare){
                std::cout<<"Decleration of variable " << parsed[i].vardata.name <<"\n";
            }
            else if(parsed[i].type == Instruction::Types::FunctionCall){
            
                std::cout<<"Function with path " <<  std::accumulate(parsed[i].path.begin(), parsed[i].path.end(), std::string("")) << " has been called \n";
            }
            else if(parsed[i].type == Instruction::Types::Assign){
                std::cout<<"Assignment of variable " << parsed[i].vardata.name <<"\n";
            }
            for(int k = 0; k < parsed[i].arguments.size();k++){
                
                std::cout << "arg size = " << parsed[i].arguments.size()<<"\n";
            
            for (size_t j = 0; j < parsed[i].arguments[k].size(); j++)
            {
                std::cout<< "arg " << k << ": "<<variantToString2(parsed[i].arguments[k][j].value) << "\n";
                
            }
            
            
        }
    }
}
}