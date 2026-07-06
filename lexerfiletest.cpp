#include "lexer.h"
#include <iostream>
#include <fstream>
#include <string>

int main()
{
    while (true)
    {
        std::string input = "";
        std::getline(std::cin, input);
        std::ifstream file(input);

        if (!file.is_open())
        {
            std::cerr << "Could not open the file." << std::endl;
            return 1;
        }
        std::string code;
        std::string line;

        // 3. Loop through the file line by line
        while (std::getline(file, line))
        {
            code += line + "\n";
        }
        std::vector<Token> result = lex(code);
        for (int i = 0; i < std::size(result); i++)
        {
            if (result[i].type == TokenType::Identifier)
            {
                std::cout << "IDENTIFIER[";
                std::visit([](auto &&v)
                           { std::cout << v; }, result[i].value);
                std::cout << "] ";
            }

            if (result[i].type == TokenType::Dot)
            {
                std::cout << "DOT  ";
            }
            if (result[i].type == TokenType::LParen)
            {
                std::cout << "LPAREN  ";
            }
            if (result[i].type == TokenType::RParen)
            {
                std::cout << "RPAREN  ";
            }
            if (result[i].type == TokenType::Semicolon)
            {
                std::cout << "SEMICOLON  ";
            }
            if (result[i].type == TokenType::Division)
            {
                std::cout << "DIVISION  ";
            }
            if (result[i].type == TokenType::Multiply)
            {
                std::cout << "MULTIPLY  ";
            }
            if (result[i].type == TokenType::Plus)
            {
                std::cout << "PLUS  ";
            }
            if (result[i].type == TokenType::Minus)
            {
                std::cout << "MINUS  ";
            }
            if (result[i].type == TokenType::Equals)
            {
                std::cout << "EQUALS  ";
            }
            if (result[i].type == TokenType::String)
            {
                std::cout << "STRING[";
                std::visit([](auto &&v)
                           { std::cout << v; }, result[i].value);
                std::cout << "] ";
            }
            if (result[i].type == TokenType::Number)
            {
                std::cout << "NUMBER[";
                std::visit([](auto &&v)
                           { std::cout << v; }, result[i].value);
                std::cout << "] ";
            }
            if (result[i].type == TokenType::Error)
            {
                std::cout << "ERROR[" << result[i].errorMessage << "]  ";
            }
            if (result[i].type == TokenType::EndOfFile)
            {
                std::cout << "END_OF_FILE  \n";
            }
        }
        file.close();
    }
    return 0;
}
