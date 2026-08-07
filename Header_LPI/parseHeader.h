#ifndef PARSEHEADER_H
#define PARSEHEADER_H
#include "../lexer.h"
#include <vector>
#include <string>

struct ExportList{
    std::vector<std::string> paths;
};

ExportList parseHeader(const std::vector<Token> & input);

#endif