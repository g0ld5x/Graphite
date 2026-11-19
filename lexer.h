#ifndef LEXER_H
#define LEXER_H
#include <string>
#include <vector>
#include <variant>
#include <cstdint>
#include <memory>
struct ArrayValue;

using ArrayPtr = std::shared_ptr<ArrayValue>;

using Value = std::variant<
    std::monostate,
    int,
    double,
    std::string,
    bool,
    ArrayPtr
>;

struct ArrayValue {
    std::vector<Value> values;
};



using TokenValue = std::variant<int,double,std::string>;

enum class TokenType : std::uint8_t
{
    Identifier = 'I', // writeln,write,for,while,
    LParen = '(',     //(
    LCurl = '{',
    //implement theese
    OrOr, // || DONE
    AndAnd = 127,// && DONE
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
    String = 'S',     // "hello (:"
    Power = '^',
    Remainder = '%',
    Number = 'N', 
    Comma,    // 100 or 12 or 31 etc. etc.
    Dot = '.',        // usecase will be choosen depending on the context by the parser
    Plus = '+',       //+
    Minus = '-',      //-
    Multiply = '*',   //*
    Division = '/',   // this --> /
    Semicolon = ';',  //;
    Equals = '=',     //=
    EndOfFile = 'E',  // self-explanatory
    NewLine = '|',
    TypeEquality, // === or 'isType'
    True,
    False,
    ForDivider // : or 'in'
};

struct Token
{
    TokenValue value;
    TokenType type;
};
std::vector<Token> lex(std::string_view input);

#endif