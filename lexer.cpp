#include "lexer.h"
#include <variant>
#include <vector>
#include <string>
#include <iostream>
#include <math.h>
inline bool isCharacter(const char & ch)
{
    unsigned char lower = static_cast<unsigned char>(ch) | 0x20;
    return (static_cast<unsigned char>(lower - 'a') < 26) || (ch == '_');
}

std::vector<Token> lex(std::string_view input)
{
    int line = 1;
    int column = 1;

    std::vector<Token> tokens;
    for (size_t i = 0; i < input.size(); i++)
    {
        char current = input[i];
        if (current == '+')
        {
            Token token;
            token.type = TokenType::Plus;
            tokens.emplace_back(std::move(token));
        }
        else if (current == '-')
        {
            Token token;
            token.type = TokenType::Minus;
            tokens.emplace_back(std::move(token));
        }
        else if (current == '/')
        {
            Token token;
            token.type = TokenType::Division;
            tokens.emplace_back(std::move(token));
        }
        else if (current == '*')
        {
            Token token;
            token.type = TokenType::Multiply;
            tokens.emplace_back(std::move(token));
        }
        else if (current == '^')
        {
            Token token;
            token.type = TokenType::Power;
            tokens.emplace_back(std::move(token));
        }
        else if (current == '.')
        {
            Token token;
            token.type = TokenType::Dot;
            tokens.emplace_back(std::move(token));
        }
        else if (current == ',')
        {
            Token token;
            token.type = TokenType::Comma;
            tokens.emplace_back(std::move(token));
        }
        else if (current == '%')
        {
            Token token;
            token.type = TokenType::Remainder;
            tokens.emplace_back(std::move(token));
        }
        else if (current == ')')
        {
            Token token;
            token.type = TokenType::RParen;
            tokens.emplace_back(std::move(token));
        }
        else if (current == '(')
        {
            Token token;
            token.type = TokenType::LParen;
            tokens.emplace_back(std::move(token));
        }
        else if (current == ']')
        {
            Token token;
            token.type = TokenType::RBrac;
            tokens.emplace_back(std::move(token));
        }
        else if (current == '[')
        {
            Token token;
            token.type = TokenType::LBrac;
            tokens.emplace_back(std::move(token));
        }
        else if (current == '}')
        {
            Token token;
            token.type = TokenType::RCurl;
            tokens.emplace_back(std::move(token));
        }
        else if (current == '{')
        {
            Token token;
            token.type = TokenType::LCurl;
            tokens.emplace_back(std::move(token));
        }
        else if (current == '=')
        {
            Token token;
            if (input[i + 1] == '=')
            {
                if(input[i+2] == '='){
                    ++i;
                    ++i;
                    token.type = TokenType::TypeEquality;
                    tokens.emplace_back(std::move(token));
                    continue;
                }
                ++i;
                token.type = TokenType::EqualEqual;
                tokens.emplace_back(std::move(token));
                continue;
            }
            token.type = TokenType::Equals;
            tokens.emplace_back(std::move(token));
        }
        else if (current == '&')
        {
            Token token;
            if (input[i + 1] == '&')
            {
                ++i;
                token.type = TokenType::AndAnd;
                tokens.emplace_back(std::move(token));
                continue;
            }
        }
        else if (current == '|')
        {
            Token token;
            if (input[i + 1] == '|')
            {
                ++i;
                token.type = TokenType::AndAnd;
                tokens.emplace_back(std::move(token));
                continue;
            }
        }
        else if (current == '!')
        {
            Token token;
            if (input[i + 1] == '=')
            {
                ++i;
                token.type = TokenType::NotEqual;
                tokens.emplace_back(std::move(token));
                continue;
            }
            token.type = TokenType::Not;
            tokens.emplace_back(std::move(token));
        }
        else if (current == '<')
        {
            Token token;
            if (input[i + 1] == '=')
            {
                ++i;
                token.type = TokenType::SmallerEqual;
                tokens.emplace_back(std::move(token));
                continue;
            }
            token.type = TokenType::Smaller;
            tokens.emplace_back(std::move(token));
        }
        else if (current == '>')
        {
            Token token;
            if (input[i + 1] == '=')
            {
                ++i;
                token.type = TokenType::BiggerEqual;
                tokens.emplace_back(std::move(token));
                continue;
            }
            token.type = TokenType::Bigger;
            tokens.emplace_back(std::move(token));
        }
        else if (isCharacter(current))
        {
            Token token;
            std::string buffer;

            size_t k = 0;
            while (i + k < input.size() && (isCharacter(input[i + k]) || isdigit(input[i + k])))
            {
                buffer.push_back(input[i + k]);
                ++k;
            }
            if (buffer == "and")
            {
                token.type = TokenType::AndAnd;
            }
            else if (buffer == "is")
            {
                token.type = TokenType::EqualEqual;
            }
            else if(buffer == "isType"){
                token.type = TokenType::TypeEquality;
            }
            else if (buffer == "or")
            {
                token.type = TokenType::OrOr;
            }
            else
            {
                token.type = TokenType::Identifier;
                token.value = buffer;
            }
            tokens.emplace_back(std::move(token));
            i += k - 1;
        }
        else if (current == '"')
        {
            Token token;
            std::string buffer;

            size_t k = 1;

            while (i + k < input.size() && input[i + k] != '"')
            {
                buffer.push_back(input[i + k]);
                ++k;
            }

            if (i + k >= input.size())
            {
                throw std::runtime_error("Error: String never finished.");
            }

            token.type = TokenType::String;
            token.value = buffer;
            tokens.emplace_back(std::move(token));

            i += k;
        }
        else if (current == '\n')
        {
            Token token;
            token.type = TokenType::NewLine;
            line++;
            column = 0;
            tokens.emplace_back(std::move(token));
        }
        else if (current == ';')
        {
            Token token;
            token.type = TokenType::Semicolon;
            tokens.emplace_back(std::move(token));
        }
        else if (current == '\'')
        {
            Token token;
            std::string buffer;

            size_t k = 1;

            while (i + k < input.size() && input[i + k] != '\'')
            {
                buffer.push_back(input[i + k]);
                ++k;
            }

            if (i + k >= input.size())
            {
                throw std::runtime_error("Error: Char never finished.");
            }

            token.type = TokenType::String;
            token.value = buffer;
            tokens.emplace_back(std::move(token));

            i += k;
        }

        else if (isdigit(current))
        {
            Token token;
            token.type = TokenType::Number;

            double value = 0;

            size_t k = 0;

            while (i + k < input.size() && isdigit(input[i + k]))
            {
                value = value * 10 + (input[i + k] - '0');
                ++k;
            }

            if (i + k < input.size() && input[i + k] == '.')
            {
                k++;

                double place = 0.1;

                while (i + k < input.size() && isdigit(input[i + k]))
                {
                    value += (input[i + k] - '0') * place;
                    place *= 0.1;
                    ++k;
                }
            }

            token.value = value;
            tokens.emplace_back(std::move(token));

            i += k - 1;
        }

        column++;
    }
    Token token;
    token.type = TokenType::EndOfFile;
    tokens.emplace_back(std::move(token));
    return tokens;
}