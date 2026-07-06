#include "lexer.h"
#include "parser.h"
#include <iostream>
#include <vector>
#include <sstream>

int main(){
    while(true){
        std::string code;
        std::getline(std::cin, code);
        std::vector<Token> lexeroutput = lex(code);

for (const auto& token : lexeroutput)

{
std::cout
<< "type = " << static_cast<int>(token.type)
<< " index = " << token.value.index()
<< " value = ";

std::visit([](const auto& x)
{
std::cout << x;
}, token.value);

std::cout << '\n';
}
        Value result = Evaluate(lexeroutput,0,lexeroutput.size()-2);
        std::visit([](const auto& value) {
            std::cout << value << std::endl;
        }, result);
        }
    }