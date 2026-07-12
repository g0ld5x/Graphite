#include "lexer.h"
#include <string>
#include <vector>
#include <algorithm>
#include <iterator>

/*     std::vector<Token> tokens; <-- example in creating a token and then appending it to tokens

    Token t;
    t.type = TokenType::Identifier;
    t.value = "writeln";

    tokens.push_back(t);

    return tokens;*/

bool isLetter(char c)
{
    return (c >= 'A' && c <= 'Z') ||
           (c >= 'a' && c <= 'z') ||
           (c == '_'); // important for identifiers
}
std::vector<Token> lex(const std::string &input)
{
    int line = 0;
    int column = 0;

    std::vector<Token> tokens;
    std::string buffer = "";
    std::string slashBuffer = "";
    bool stringMode = false;
    for (int i = 0; i < input.size(); i++)
    {
        column++;
        char c = input[i];
        if (!stringMode)
        {
            if (isspace(c) && c != '\n')
                continue;

            Token token;

            if (c == '+')
            {
                token.line = line;
                token.column = column;
                token.type = TokenType::Plus;
                tokens.push_back(token);
            }
            else if (c == '(')
            {
                token.line = line;
                token.column = column;
                token.type = TokenType::LParen;
                tokens.push_back(token);
            }

            else if (c == '{')
            {
                token.line = line;
                token.column = column;
                token.type = TokenType::LCurl;
                tokens.push_back(token);
            }
            else if (c == '}')
            {
                token.line = line;
                token.column = column;
                token.type = TokenType::RCurl;
                tokens.push_back(token);
            }

            else if (c == '[')
            {
                token.line = line;
                token.column = column;
                token.type = TokenType::LBrac;
                tokens.push_back(token);
            }
            else if (c == ']')
            {
                token.line = line;
                token.column = column;
                token.type = TokenType::RBrac;
                tokens.push_back(token);
            }
            else if (c == '\n')
            {
                token.line = line;
                token.column = column;
                token.type = TokenType::NewLine;
                tokens.push_back(token);
                column = 0;
                line++;
            }
            else if (c == ')')
            {
                token.line = line;
                token.column = column;
                token.type = TokenType::RParen;
                tokens.push_back(token);
            }
            else if (c == '-')
            {
                token.line = line;
                token.column = column;
                token.type = TokenType::Minus;
                tokens.push_back(token);
            }
            else if (c == '*')
            {
                token.line = line;
                token.column = column;
                token.type = TokenType::Multiply;
                tokens.push_back(token);
            }
            else if (c == '/')
            {
                if (i + 1 < input.size() && input[i + 1] == '/')
                {
                    i += 2;
                    while (i < input.size() && input[i] != '\n')
                    {
                        i++;
                    }
                    if (i < input.size() && input[i] == '\n')
                    {
                        line++;
                        column = 1;
                    }
                }
                else
                {
                    token.line = line;
                    token.column = column;
                    token.type = TokenType::Division;
                    tokens.push_back(token);
                    column++;
                }
            }
            else if (c == '!')
            {
                if (i + 1 < input.size() && input[i + 1] == '=')
                {
                    token.line = line;
                    token.column = column;
                    token.type = TokenType::NotEqual;
                    tokens.push_back(token);
                    i++;
                }
                else
                {
                    token.line = line;
                    token.column = column;
                    token.type = TokenType::Not;
                    tokens.push_back(token);
                }
            }
            else if (c == '>')
            {
                if (i + 1 < input.size() && input[i + 1] == '=')
                {
                    token.line = line;
                    token.column = column;
                    token.type = TokenType::BiggerEqual;
                    tokens.push_back(token);
                    i++;
                }
                else
                {
                    token.line = line;
                    token.column = column;
                    token.type = TokenType::Bigger;
                    tokens.push_back(token);
                }
            }
            else if (c == '<')
            {
                if (i + 1 < input.size() && input[i + 1] == '=')
                {
                    token.line = line;
                    token.column = column;
                    token.type = TokenType::SmallerEqual;
                    tokens.push_back(token);
                    i++;
                }
                else
                {
                    token.line = line;
                    token.column = column;
                    token.type = TokenType::Smaller;
                    tokens.push_back(token);
                }
            }
            else if (c == '=')
            {
                if (i + 1 < input.size() && input[i + 1] == '=')
                {
                    token.line = line;
                    token.column = column;
                    token.type = TokenType::EqualEqual;
                    tokens.push_back(token);
                    i++;
                }
                else
                {
                    token.line = line;
                    token.column = column;
                    token.type = TokenType::Equals;
                    tokens.push_back(token);
                }
            }
            else if (c == ';')
            {
                token.line = line;
                token.column = column;
                token.type = TokenType::Semicolon;
                tokens.push_back(token);
            }
            else if (c == ',')
            {
                token.line = line;
                token.column = column;
                token.type = TokenType::Comma;
                tokens.push_back(token);
            }
            else if (c == '.')
            {
                token.line = line;
                token.column = column;
                token.type = TokenType::Dot;
                tokens.push_back(token);
            }
            else if (c == '&' && i + 1 < input.size() && input[i + 1] == '&')
            {
                Token token;
                token.line = line;
                token.column = column;
                token.type = TokenType::AndAnd;
                tokens.push_back(token);

                i++;
                continue;
            }
            else if (c == '|' && i + 1 < input.size() && input[i + 1] == '|')
            {
                Token token;
                token.line = line;
                token.column = column;
                token.type = TokenType::OrOr;
                tokens.push_back(token);

                i++;
                continue;
            }
            else if (c == '"')
            {
                stringMode = !stringMode;
            }
            else if (c == '^')
            {
                token.line = line;
                token.column = column;
                token.type = TokenType::Power;
                tokens.push_back(token);
            }
            else if (c == '%')
            {
                token.line = line;
                token.column = column;
                token.type = TokenType::Remainder;
                tokens.push_back(token);
            }
            else if (std::isdigit(c))
            {
                std::string number;

                while (i < input.size() && std::isdigit(input[i]))
                {
                    number += input[i];
                    i++;
                }

                // Decimal part
                if (i < input.size() && input[i] == '.')
                {
                    number += '.';
                    i++;

                    while (i < input.size() && std::isdigit(input[i]))
                    {
                        number += input[i];
                        i++;
                    }
                }

                i--;

                Token token;
                token.type = TokenType::Number;

                if (number.find('.') != std::string::npos)
                {
                    token.value = std::stod(number);
                }
                else
                {
                    token.value = std::stoi(number);
                }

                tokens.push_back(token);
            }
            else if (isLetter(c) || c == '_')
            {
                std::string value = "";
                while (i < input.size() &&
                       (isLetter(input[i]) || isdigit(input[i]) || input[i] == '_'))
                {
                    value += input[i];
                    i++;
                }

                i--;
                if(value == "true"){
                    Token token;
                    token.value = true;
                    token.type = TokenType::True;
                    tokens.push_back(token);
                }
                if(value == "false"){
                    Token token;
                    token.value = false;
                    token.type = TokenType::False;
                    tokens.push_back(token);
                }
                if (value == "not")
                {
                    Token token;
                    token.type = TokenType::Not;
                    tokens.push_back(token);
                }
                else if (value == "and")
                {
                    Token token;
                    token.type = TokenType::AndAnd;
                    tokens.push_back(token);
                }
                else if (value == "or")
                {
                    Token token;
                    token.type = TokenType::OrOr;
                    tokens.push_back(token);
                }
                else if (value == "is")
                {
                    Token token;
                    token.type = TokenType::EqualEqual;
                    tokens.push_back(token);
                }
                else
                {
                    Token token;
                    token.type = TokenType::Identifier;
                    token.value = value;
                    tokens.push_back(token);
                }
            }
        }
        else
        {
            Token token;
            // this is string mode
            if (c == '"')
            {
                stringMode = !stringMode;
                token.type = TokenType::String;
                token.value = buffer;
                tokens.push_back(token);
                buffer.clear();
            }
            else
            {
                buffer += c;
            }
        }
    }
    if (stringMode)
    {
        Token token;
        token.type = TokenType::Error;
        token.line = line;
        token.column = column;
        token.errorMessage = "String not closed!";
        tokens.push_back(token);
    }
    Token token;
    token.type = TokenType::EndOfFile;
    tokens.push_back(token);
    return tokens;
}