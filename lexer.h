#ifndef LEXER_H
#define LEXER_H
#include <string>
#include <vector>
#include <variant>


using Value = std::variant<int, double, std::string,bool>;
enum class TokenType
{
    Identifier = 'I', // writeln,write,for,while,
    LParen = '(',     //(
    LCurl = '{',
    //implement theese
    OrOr, // || DONE
    AndAnd,// && DONE
    EqualEqual, // == DONE
    NotEqual, // != DONE
    Bigger, // > DONE
    Smaller, // < DONE
    BiggerEqual, // >= DONE 
    Not, // ! DONE
    SmallerEqual, //<= DONE
    //up
    RCurl = '}',
    LBrac = '[',
    RBrac = ']',
    RParen = ')',     // )
    String = 'S',     // "hi lol"
    Power = '^',
    Remainder = '%',
    Number = 'N', 
    Comma,    // 100 or 12 or 31
    Dot = '.',        // usecase will be choosen depending on the context by the parser
    Plus = '+',       //+
    Minus = '-',      //-
    Multiply = '*',   //*
    Division = '/',   // this --> /
    Semicolon = ';',  //;
    Equals = '=',     //=
    EndOfFile = 'E',  // self-explanatory
    NewLine = '|',
    Error = '!',
    True,
    False       // if an error happens
};

struct Token
{
    TokenType type;
    Value value;
    int line;
    int column;
    std::string errorMessage;
};
std::vector<Token> lex(const std::string &input);

#endif