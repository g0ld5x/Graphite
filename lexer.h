#ifndef LEXER_H
#define LEXER_H
#include <string>
#include <vector>
#include <variant>
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
enum class TokenType
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
    TypeEquality,
    True,
    False
};

struct Token
{
    TokenType type;
    Value value;
    int line;
    int column;
    std::string errorMessage;
};
std::vector<Token> lex(std::string_view input);

#endif