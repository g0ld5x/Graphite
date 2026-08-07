#include "../lexer.h"
#include <vector>
#include <string>
#include "parseHeader.h"
#include <sstream>

std::string variantToString3(const Value &a)
{
    return std::visit([](const auto &arg) -> std::string
    {
        using T = std::decay_t<decltype(arg)>;

        if constexpr (std::is_same_v<T, std::monostate>)
        {
            return "null";
        }
        else
        {
            std::ostringstream oss;
            oss << arg;
            return oss.str();
        }

    }, a);
}
ExportList parseHeader(const std::vector<Token> & input){
    ExportList exports;
    for (size_t i = 0; i < input.size(); i++)
    {
        const Token & current = input[i];
        if(current.type == TokenType::Identifier &&  variantToString3(current.value) == "export"){
            ++i;
            while(input[i].type == TokenType::NewLine){
                ++i;
            }
            if(input[i].type == TokenType::LCurl){
                ++i;
                while(input[i].type != TokenType::RCurl){
                    if(input[i].type ==  TokenType::String){
                        exports.paths.emplace_back(variantToString3(input[i].value));
                    }
                    ++i;
                }
            }
        }
    }
    return exports;
}