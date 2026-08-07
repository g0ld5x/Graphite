#include "parser.h"
#include <string>
#include <vector>
#include <algorithm>
#include <variant>
#include "lexer.h"
#include "interpreter.h"
#include <iostream>
#include <math.h>
#include <sstream>
#include <fstream>
#include <filesystem>
#include <algorithm>
#include "Header_LPI/parseHeader.h"
#include <cstdlib>
#include <string>

void runCommand(const std::string& command)
{
    std::system(command.c_str());
}
std::string getFileName(const std::string& path) {
    size_t lastSlash = path.find_last_of("/\\");
    
    if (lastSlash == std::string::npos) {
        return path;
    }
    return path.substr(lastSlash + 1);
}
namespace fs = std::filesystem;

std::vector<Token> resolveFile(const std::string& input)
{
    std::vector<Token> tokens;

    auto processFile = [&](const fs::path& path)
    {
        std::ifstream inputFile(path);

        if (!inputFile.is_open())
        {
            std::cerr << "Error: Could not open the file: " 
                      << path << std::endl;
            return;
        }

        std::string source;
        std::string line;

        while (std::getline(inputFile, line))
        {
            source += line + "\n";
        }

        auto fileTokens = lex(source);

        tokens.insert(
            tokens.end(),
            fileTokens.begin(),
            fileTokens.end()
        );
    };


    if (fs::is_regular_file(input))
    {
        processFile(input);
    }
    else if (fs::is_directory(input))
    {
        std::vector<std::string> includedInExport;
        for (const auto& entry : fs::directory_iterator(input))
        {
            if(getFileName(entry.path()).ends_with(".grh")){
                //.grh is the extension for header files btw.
                std::ifstream inputFile(entry.path());
                std::string source;
                std::string line;
                while(std::getline(inputFile,line)){
                    source += line;
                }

                ExportList parsed = parseHeader(lex(source));
                fs::path filePath = getFileName(entry.path()); 
    
                fs::path fullPath = fs::absolute(filePath);
                
                fs::path HeaderfolderName = fullPath.parent_path().filename();
                for (size_t i = 0; i < parsed.paths.size(); i++)
                { 
                    //since headers must be at the same path as the .gh files this should work.
                    //the users will write only the relative file name, so i have to somehow get their paths relative to the file that is being ran.
                    if(parsed.paths[i] != ""){
                        includedInExport.push_back(parsed.paths[i]);
                    }
                }
                
                
                
            }
        }

        for(const auto & entry : fs::directory_iterator(input)){
            if (std::find(includedInExport.begin(), includedInExport.end(), getFileName(entry.path())) != includedInExport.end()) {
                processFile(entry);
        }
    }
}
    else
    {
        std::cerr << "Error: Invalid import path: " 
                  << input << std::endl;
    }

    return tokens;
}

bool variantToBool2(const Value &value)
{
    if (const bool *val_ptr = std::get_if<bool>(&value))
    {
        return *val_ptr;
    }
    else
    {
        std::cout << "bad";
        return false;
        // handle error here
    }
}

std::string variantToString2(const Value &a)
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

bool isInFunctions(std::string name, FunctionTable functions)
{
    auto it = functions.find(name);
    if (it != functions.end())
    {
        return true;
    }
    else
    {
        return false;
    }
}

std::string join(const std::vector<std::string> &elements, const std::string &delimiter)
{
    if (elements.empty())
        return "";


    size_t total_length = 0;
    for (const auto &s : elements)
        total_length += s.length();
    total_length += delimiter.length() * (elements.size() - 1);

    std::string result;
    result.reserve(total_length);

    // Join elements
    result += elements[0];
    for (size_t i = 1; i < elements.size(); ++i)
    {
        result += delimiter + elements[i];
    }

    return result;
}


bool variableExists(std::string name, ScopeStack &scope)
{
    for (int i = scope.size() - 1; i >= 0; i--)
    {
        if (scope[i].find(name) != scope[i].end())
        {
            return true;
        }
    }

    return false;
}

VariableData &getVariable2(std::string name, ScopeStack &scope)
{
    for (int i = scope.size() - 1; i >= 0; i--)
    {
        auto it = scope[i].find(name);

        if (it != scope[i].end())
        {
            return it->second;
        }
    }

    throw std::runtime_error("Unknown variable " + name);
}

void initInterpreter(ScopeStack &scope)
{
    scope.push_back(VariableTable{}); // global scope

    scope[0]["true"] = {
        "true",
        true,
        true, // isConst
        true, // isStrict
        true,
        VariableTypes::Bool};

    scope[0]["false"] = {
        "false",
        false,
        true, // isConst
        true, // isStrict
        true,
        VariableTypes::Bool};
}
ExecutionResult interpret(const std::vector<Instruction> &input, ScopeStack &scope,FunctionTable & GlobalFunctionTable)
{
    bool dependenciesEncountered = false;
    bool returned = false;
    for (size_t i = 0; i < input.size(); i++)
    {
        Instruction instr = input[i];
        if (instr.type == Instruction::Types::FunctionDeclare)
        {
            GFunction &func = GlobalFunctionTable[instr.Funcname];

            func.body = instr.body;
            func.isStrict = instr.isStrict;
            func.name = instr.Funcname;
            func.locals = instr.locals;
            func.isVoid = true;
        }
        else if (instr.type == Instruction::Types::Use)
        {
            
            
            if(dependenciesEncountered){
                std::cerr << "Error: keyword use can only be used once per file.";
            }
            if(i != 0){
                std::cerr << "Error: Dependencies must be listed at the top of the code!";
            }
            
            else{
                //normal route
                for (size_t i = 0; i < instr.importPath.size(); i++)
                {
                    std::vector<std::string> a;
                    ScopeStack importScope;
                    interpret(parse(resolveFile(instr.importPath[i]), a),importScope,GlobalFunctionTable);
                }
                
            }
            dependenciesEncountered = true;
        }
        else if (instr.type == Instruction::Types::Space)
        {
            interpret(instr.body, scope,GlobalFunctionTable);
        }
        else if (instr.type == Instruction::Types::Return)
        {

            ExecutionResult exec;
            exec.didReturn = true;
            exec.returnValue = Evaluate(
                instr.expression,
                0,
                instr.expression.size() - 1,
                scope,
                GlobalFunctionTable);

            return exec;
        }
        else if (instr.type == Instruction::Types::Declare)
        {

            Value value = Evaluate(
                instr.expression,
                0,
                instr.expression.size() - 1,
                scope,
                GlobalFunctionTable);

            if (std::holds_alternative<int>(value))
            {
                instr.vardata.vartype = VariableTypes::Int;
            }
            else if (std::holds_alternative<std::string>(value))
            {
                instr.vardata.vartype = VariableTypes::String;
            }
            else if (std::holds_alternative<bool>(value))
            {
                instr.vardata.vartype = VariableTypes::Bool;
            }
            else if (std::holds_alternative<double>(value))
            {
                instr.vardata.vartype = VariableTypes::Double;
            }

            instr.vardata.value = value;

            if (instr.vardata.isGlobal)
            {

                scope.front()[instr.vardata.name] = std::move(instr.vardata);
            }
            else
            {

                scope.back()[instr.vardata.name] = std::move(instr.vardata);
            }
        }
        else if (instr.type == Instruction::Types::Assign)
        { // for things like var x = 32;
            VariableData &target = getVariable2(instr.vardata.name, scope);
            if (target.isConst)
            {
                std::cerr << "Cant change the value of a constant. \n";
            }
            else
            {

                Value value = Evaluate(instr.expression, 0, instr.expression.size() - 1, scope, GlobalFunctionTable);
                if (target.isStrict)
                {
                    if (std::holds_alternative<int>(value) && instr.vardata.vartype == VariableTypes::Int)
                    {
                        target.value = std::move(value);
                    }
                    else if (std::holds_alternative<std::string>(value) && instr.vardata.vartype == VariableTypes::String)
                    {
                        target.value = std::move(value);
                    }
                    else if (std::holds_alternative<double>(value) && instr.vardata.vartype == VariableTypes::Double)
                    {
                        target.value = std::move(value);
                    }
                    else if (std::holds_alternative<bool>(value) && instr.vardata.vartype == VariableTypes::Bool)
                    {
                        target.value = std::move(value);
                    }
                    else
                    {
                        std::cerr << "Cannot change the type of a strict variable";
                    }
                }
                else
                {
                    getVariable2(instr.vardata.name, scope).value = value;
                }
            }
            continue;
        }
        else if (instr.type == Instruction::Types::If)
        {
            scope.push_back(VariableTable{});

            if (variantToBool2(Evaluate(instr.condition, 0, instr.condition.size() - 1, scope, GlobalFunctionTable)))
            {
                ExecutionResult result = interpret(instr.body, scope,GlobalFunctionTable);

                if (result.didReturn == true)
                {
                    scope.pop_back();
                    return result;
                }
            }
            else if (!instr.elseBody.empty())
            {
                ExecutionResult result = interpret(instr.elseBody, scope,GlobalFunctionTable);

                if (result.didReturn)
                {
                    scope.pop_back();
                    return result;
                }
            }

            scope.pop_back();
        }
        else if (instr.type == Instruction::Types::While)
        {
            while (variantToBool2(Evaluate(
                instr.condition,
                0,
                instr.condition.size() - 1,
                scope,
                GlobalFunctionTable)))
            {
                ExecutionResult result = interpret(instr.body, scope,GlobalFunctionTable);

                if (result.didReturn)
                    return result;
            }
        }

        else if (instr.type == Instruction::Types::FunctionCall)
        {

            std::string path = join(instr.path, ".");

            if (path == "Int")
            { // for casting Int(varname);
                VariableData &target = getVariable2(variantToString2(instr.arguments[0][0].value), scope);
                if (target.isStrict)
                {
                    std::cerr << "Cannot cast strict variables";
                }
                else
                {
                    if (target.vartype != VariableTypes::Int)
                    {
                        target.value = std::stoi(variantToString2(target.value));
                    }
                    target.vartype = VariableTypes::Int;
                }
            }
            else if (path == "Double")
            { // for casting Int(varname);
                VariableData &target = getVariable2(variantToString2(instr.arguments[0][0].value), scope);
                if (target.isStrict)
                {
                    std::cerr << "Cannot cast strict variables";
                }
                else
                {
                    if (target.vartype != VariableTypes::Double)
                    {
                        target.value = std::stod(variantToString2(target.value));
                    }
                    target.vartype = VariableTypes::Double;
                }
            }
            else if (path == "Terminal.IO.print")
            {
                for (size_t k = 0; k < instr.arguments.size(); k++)
                {
                    std::cout << variantToString2(Evaluate(instr.arguments[k], 0, instr.arguments[k].size() - 1, scope, GlobalFunctionTable));
                }
            }
            else if(path == "os"){
                runCommand(variantToString2(Evaluate(instr.arguments[0], 0, instr.arguments[0].size() - 1, scope, GlobalFunctionTable)));
            }
            else if (path == "Terminal.IO.println")
            {
                for (size_t k = 0; k < instr.arguments.size(); k++)
                {
                    std::cout << variantToString2(Evaluate(instr.arguments[k], 0, instr.arguments[k].size() - 1, scope, GlobalFunctionTable));
                    std::cout << "\n";
                }
            }
            else if (path == "Terminal.IO.input")
            {

                if (instr.arguments.size() == 0)
                {
                    std::string buffer = "";
                    std::getline(std::cin, buffer);
                }
                else
                {
                    VariableData &target = getVariable2(variantToString2(instr.arguments[0][0].value), scope);
                    if (!target.isConst)
                    {
                        if (target.vartype == VariableTypes::String || !target.isStrict)
                        {
                            std::string buffer = "";
                            std::getline(std::cin, buffer);

                            target.value = buffer;

                            target.vartype = VariableTypes::String;
                        }
                        else
                        {
                            std::cerr << "Cannot change the type of a strict variable.";
                        }
                    }
                    else
                    {
                        std::cerr << "Cannot change the value of a constant.";
                    }
                }
            }
            else if (path == "Terminal.clear")
            {
                if (instr.arguments.size() != 0)
                {
                    std::cerr << "Argument Overflow! Function 'clear' expected 0 arguments, got " << instr.arguments.size() << ". \n";
                }
                else
                {
                    std::cout << "\033[2J\033[1;1H" << std::flush;
                }
            }
            else if (path == "free")
            {

                if (instr.arguments.size() == 0)
                {
                    std::cerr << "Function 'free' requires atleast 1 argument";
                }
                else
                {
                    for (int k = 0; k < instr.arguments.size(); k++)
                    {
                        scope.back().erase(variantToString2(instr.arguments[k][0].value));
                    }
                }
            }
else if (auto funcIt = GlobalFunctionTable.find(path);
         funcIt != GlobalFunctionTable.end())
{
    GFunction& targetFunc = funcIt->second;

    if (instr.arguments.size() != targetFunc.locals.size())
    {
        std::cerr << "Error: function " << path
                  << " expected "
                  << targetFunc.locals.size()
                  << " arguments but got "
                  << instr.arguments.size()
                  << "\n";

        ExecutionResult result;
        result.didReturn = false;
        result.returnValue = std::monostate();
        return result;
    }

    // Evaluate arguments
    for (size_t k = 0; k < instr.arguments.size(); k++)
    {
        targetFunc.locals[k].value =
            Evaluate(
                instr.arguments[k],
                0,
                instr.arguments[k].size() - 1,
                scope,
                GlobalFunctionTable
            );
    }

    // Create new scope directly
    scope.emplace_back();
    VariableTable& bufferTable = scope.back();

    bufferTable.reserve(targetFunc.locals.size());

    for (const auto& var : targetFunc.locals)
    {
        bufferTable.emplace(var.name, var);
    }

    ExecutionResult Return = interpret(targetFunc.body, scope,GlobalFunctionTable);

    scope.pop_back();

    if (Return.didReturn)
    {
        return Return;
    }
}
            else
            {
                std::cerr << "Unknown function '" << path << "' \n";
            }
        }
    }
    return ExecutionResult{};
}