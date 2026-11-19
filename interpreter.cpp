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
#include <stdexcept>

#include <type_traits>
#include <string_view>
#include <variant>
Value tokenToValue2(const TokenValue& tokenValue)
{
    return std::visit([](const auto& value) -> Value {
        using T = std::decay_t<decltype(value)>;

        if constexpr (std::is_same_v<T, std::string_view>) {
            return std::string(value);
        } else {
            return value;
        }
    }, tokenValue);
}

template <class... Ts>
struct overloaded : Ts...
{
    using Ts::operator()...;
};
template <class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

std::string value_to_string(const Value &val);

std::string value_to_string(const Value &val)
{
    return std::visit(overloaded{[](std::monostate) -> std::string
                                 {
                                     return "null";
                                 },
                                 [](int arg) -> std::string
                                 {
                                     return std::to_string(arg);
                                 },
                                 [](double arg) -> std::string
                                 {
                                     std::ostringstream ss;
                                     ss << arg;
                                     return ss.str();
                                 },
                                 [](const std::string &arg) -> std::string
                                 {
                                     return arg;
                                 },
                                 [](bool arg) -> std::string
                                 {
                                     return arg ? "true" : "false";
                                 },
                                 [](const ArrayPtr &arg) -> std::string
                                 {
                                     if (!arg)
                                         return "[]";

                                     std::string result = "[";
                                     const auto &vec = arg->values;

                                     for (size_t i = 0; i < vec.size(); ++i)
                                     {

                                         result += value_to_string(vec[i]);
                                         if (i + 1 < vec.size())
                                         {
                                             result += ", ";
                                         }
                                     }
                                     result += "]";
                                     return result;
                                 }},
                      val);
}

std::vector<Value> &variant_to_array(Value &val)
{

    if (!std::holds_alternative<ArrayPtr>(val))
    {
        throw std::runtime_error("Error: Value is not an Array type.");
    }

    ArrayPtr &arr_ptr = std::get<ArrayPtr>(val);

    if (!arr_ptr)
    {
        throw std::runtime_error("Error: Array pointer is null.");
    }

    return arr_ptr->values;
}
#ifdef _WIN32

#include <windows.h>

void runCommand(
    const std::string &command,
    const std::vector<std::string> &args)
{
    std::string commandLine = "\"" + command + "\"";

    for (const auto &arg : args)
    {
        commandLine += " \"";
        commandLine += arg;
        commandLine += "\"";
    }

    STARTUPINFOA startupInfo{};
    PROCESS_INFORMATION processInfo{};

    startupInfo.cb = sizeof(startupInfo);

    if (!CreateProcessA(
            nullptr,
            commandLine.data(),
            nullptr,
            nullptr,
            FALSE,
            0,
            nullptr,
            nullptr,
            &startupInfo,
            &processInfo))
    {
        throw std::runtime_error("Failed to execute command");
    }

    WaitForSingleObject(processInfo.hProcess, INFINITE);

    CloseHandle(processInfo.hProcess);
    CloseHandle(processInfo.hThread);
}

#else // Linux, macOS, BSD, other POSIX systems

#include <unistd.h>
#include <sys/wait.h>

TokenRange makeTokenRange(const std::vector<Token> &tokens)
{

    if (tokens.empty())
        throw std::runtime_error("Cannot create TokenRange from empty token vector");

    return TokenRange{
        &tokens,
        0,
        tokens.size() - 1};
}

void runCommand(
    const std::string &command,
    const std::vector<std::string> &args)
{
    pid_t pid = fork();

    if (pid < 0)
        throw std::runtime_error("fork() failed");

    if (pid == 0)
    {
        std::vector<char *> argv;

        argv.push_back(const_cast<char *>(command.c_str()));

        for (const auto &arg : args)
        {
            argv.push_back(const_cast<char *>(arg.c_str()));
        }

        argv.push_back(nullptr);

        execvp(command.c_str(), argv.data());

        // execvp only returns if it failed
        _exit(127);
    }

    int status;
    waitpid(pid, &status, 0);
}

#endif

VariableTypes getVarType(const Value &value)
{
    switch (value.index())
    {
    case 0:
        return VariableTypes::Null;
    case 1:
        return VariableTypes::Int;
    case 2:
        return VariableTypes::Double;
    case 3:
        return VariableTypes::String;
    case 4:
        return VariableTypes::Bool;
    case 5:
        return VariableTypes::Array;

    default:
        throw std::runtime_error("Unknown Value type");
    }
}

std::string osType()
{
#ifdef _WIN32
    return "windows";
#elif defined(__APPLE__)
    return "macos";
#elif defined(__linux__)
    return "linux";
#elif defined(__FreeBSD__)
    return "freebsd";
#elif defined(__OpenBSD__)
    return "openbsd";
#elif defined(__NetBSD__)
    return "netbsd";
#else
    return "unknown";
#endif
}

std::size_t utf8Length(const std::string &str)
{
    std::size_t count = 0;

    for (unsigned char c : str)
    {
        if ((c & 0xC0) != 0x80)
        {
            count++;
        }
    }

    return count;
}

std::size_t utf8Offset(const std::string &str, std::size_t index)
{
    std::size_t current = 0;

    for (std::size_t i = 0; i < str.size(); i++)
    {
        unsigned char c = str[i];

        if ((c & 0xC0) != 0x80)
        {
            if (current == index)
                return i;

            current++;
        }
    }

    throw std::out_of_range("UTF-8 index out of range");
}

Value indexVariant(const Value &value, std::size_t index)
{
    return std::visit([index](const auto &v) -> Value
                      {
        using T = std::decay_t<decltype(v)>;

        if constexpr (std::is_same_v<T, ArrayPtr>) {
            if (!v) {
                throw std::runtime_error("Cannot index a null array");
            }

            return v->values.at(index);
        }
        else if constexpr (std::is_same_v<T, std::string>) {
            std::size_t offset = utf8Offset(v, index);
            unsigned char c = v[offset];

            std::size_t charLength =
                (c < 0x80) ? 1 :
                (c < 0xE0) ? 2 :
                (c < 0xF0) ? 3 : 4;

            return v.substr(offset, charLength);
        }
        else {
            throw std::runtime_error("Value is not indexable");
        } }, value);
}

int variantToInt2(const Value &value)
{
    if (const auto *p = std::get_if<int>(&value))
        return *p;

    if (const auto *p = std::get_if<double>(&value))
        return static_cast<int>(*p);

    if (const auto *p = std::get_if<bool>(&value))
        return *p ? 1 : 0;

    if (const auto *p = std::get_if<std::string>(&value))
    {
        try
        {
            return std::stoi(*p);
        }
        catch (...)
        {
            return 0;
        }
    }

    return 0;
}

std::size_t variantLength(const Value &value)
{
    return std::visit([](const auto &v) -> std::size_t
                      {
        using T = std::decay_t<decltype(v)>;

        if constexpr (std::is_same_v<T, std::string>) {
            return utf8Length(v);
        }
        else if constexpr (std::is_same_v<T, ArrayPtr>) {
            if (!v) {
                throw std::runtime_error("Cannot get length of a null array");
            }

            return v->values.size();
        }
        else {
            throw std::runtime_error("Value does not have a length");
        } }, value);
}
std::string getFileName(const std::string &path)
{
    size_t lastSlash = path.find_last_of("/\\");

    if (lastSlash == std::string::npos)
    {
        return path;
    }
    return path.substr(lastSlash + 1);
}
namespace fs = std::filesystem;

std::vector<Token> resolveFile(const std::string &input)
{
    std::vector<Token> tokens;

    auto processFile = [&](const fs::path &path)
    {
        std::ifstream inputFile(path);

        if (!inputFile.is_open())
        {
            std::cerr << "Error: Could not open the file: "
                      << path << std::endl;
            return;
        }


        std::string source{
            std::istreambuf_iterator<char>(inputFile),
            std::istreambuf_iterator<char>()
        };

        auto fileTokens = lex(std::string_view( source));

        tokens.insert(
            tokens.end(),
            fileTokens.begin(),
            fileTokens.end());
    };

    if (fs::is_regular_file(input))
    {
        processFile(input);
    }
    else if (fs::is_directory(input))
    {
        std::vector<std::string> includedInExport;
        for (const auto &entry : fs::directory_iterator(input))
        {
            if (getFileName(entry.path()).compare(getFileName(entry.path()).size() - 4, 4, ".grh") == 0)
            {
                //.grh is the extension for header files btw.
                std::ifstream inputFile(entry.path());
                std::string source;
                std::string line;
                while (std::getline(inputFile, line))
                {
                    source += line;
                }

                ExportList parsed = parseHeader(lex(source));
                fs::path filePath = getFileName(entry.path());

                fs::path fullPath = fs::absolute(filePath);

                fs::path HeaderfolderName = fullPath.parent_path().filename();
                for (size_t i = 0; i < parsed.paths.size(); i++)
                {
                    // since headers must be at the same path as the .gh files this should work.
                    // the users will write only the relative file name, so i have to somehow get their paths relative to the file that is being ran.
                    if (parsed.paths[i] != "")
                    {
                        includedInExport.push_back(parsed.paths[i]);
                    }
                }
            }
        }

        for (const auto &entry : fs::directory_iterator(input))
        {
            if (std::find(includedInExport.begin(), includedInExport.end(), getFileName(entry.path())) != includedInExport.end())
            {
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

int charCode(const std::string &str)
{
    if (str.empty())
        return -1;
    return static_cast<unsigned char>(str[0]);
}

std::string charFromCode(int code)
{
    if (code < 0 || code > 127)
        return "";

    return std::string(1, static_cast<char>(code));
}

bool doesVariableExist(const std::string &name, ScopeStack &scopes)
{
    // std::cerr << "Name: " << name << "\n";
    // std::cerr << "Checking variable: [" << name << "]\n";

    for (auto it = scopes.rbegin(); it != scopes.rend(); ++it)
    {
        // std::cerr << "Scope contains " << it->size() << " variables\n";

        if (it->find(name) != it->end())
        {
            // std::cerr << "FOUND\n";
            return true;
        }
    }

    // std::cerr << "NOT FOUND\n";
    return false;
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
                          } },
                      a);
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
ExecutionResult interpret(const std::vector<Instruction> &input, ScopeStack &scope, FunctionTable &GlobalFunctionTable)
{
    bool dependenciesEncountered = false;
    bool returned = false;
    for (size_t i = 0; i < input.size(); i++)
    {
        Instruction instr = input[i];
        if (std::holds_alternative<FunctionDeclareInstruction>(instr.data))
        {
            auto& function = std::get<FunctionDeclareInstruction>(instr.data);
            GFunction &func = GlobalFunctionTable[function.Funcname];
            
            func.body = function.body;
            func.isStrict = function.isStrict;
            func.name = function.Funcname;
            func.locals = function.locals;
            func.isVoid = true;
        }
        else if (std::holds_alternative<UseInstruction>(instr.data))
        {

           auto& use = std::get<UseInstruction>(instr.data);
            for (size_t i = 0; i < use.importPath.size(); i++)
            {
                std::vector<std::string> a;
                ScopeStack importScope;
                interpret(parse(resolveFile(use.importPath[i]), a), importScope, GlobalFunctionTable);
            }
        }
        else if (std::holds_alternative<SpaceInstruction>(instr.data))
        {
            auto& space = std::get<SpaceInstruction>(instr.data);
            ExecutionResult result = interpret(space.body, scope, GlobalFunctionTable);

            if (result.didReturn || result.didBreak || result.didContinue)
                return result;
        }
        else if (std::holds_alternative<ReturnInstruction>(instr.data))
        {
            auto& retrn = std::get<ReturnInstruction>(instr.data);
            ExecutionResult exec;
            exec.didReturn = true;
            exec.returnValue = Evaluate(
                makeTokenRange(retrn.expression),
                scope,
                GlobalFunctionTable);

            return exec;
        }
        else if (std::holds_alternative<DeclareInstruction>(instr.data))
        {
            auto& declare = std::get<DeclareInstruction>(instr.data);
            Value value = Evaluate(
                makeTokenRange(declare.expression),
                scope,
                GlobalFunctionTable);

            if (std::holds_alternative<int>(value))
            {
                declare.vardata.vartype = VariableTypes::Int;
            }
            else if (std::holds_alternative<std::string>(value))
            {
                declare.vardata.vartype = VariableTypes::String;
            }
            else if (std::holds_alternative<bool>(value))
            {
                declare.vardata.vartype = VariableTypes::Bool;
            }
            else if (std::holds_alternative<double>(value))
            {
                declare.vardata.vartype = VariableTypes::Double;
            }

            declare.vardata.value = value;

            if (declare.vardata.isGlobal)
            {

                scope.front()[declare.vardata.name] = std::move(declare.vardata);
            }
            else
            {

                scope.back()[declare.vardata.name] = std::move(declare.vardata);
            }
        }
        else if (std::holds_alternative<AssignInstruction>(instr.data))
        {
            auto& assign = std::get<AssignInstruction>(instr.data);
            VariableData &target = getVariable2(assign.vardata.name, scope);
            if (target.isConst)
            {
                std::cerr << "Cant change the value of a constant. \n";
            }
            else
            {

                Value value = Evaluate(makeTokenRange(assign.expression), scope, GlobalFunctionTable);
                if (!assign.indexExpressions.empty())
                {
                    if (!assign.indexExpressions.empty())
                    {
                        std::vector<std::size_t> indices;

                        // Evaluate each index expression.
                        for (const auto &indexExpression : assign.indexExpressions)
                        {
                            Value indexValue = Evaluate(
                                makeTokenRange(indexExpression),
                                scope,
                                GlobalFunctionTable);

                            int index = variantToInt2(indexValue);

                            if (index < 0)
                                throw std::runtime_error("Array index cannot be negative");

                            indices.push_back(static_cast<std::size_t>(index));
                        }

                        // Start at the variable's value.
                        Value *current = &target.value;

                        // Traverse all arrays except the final one.
                        for (std::size_t i = 0; i + 1 < indices.size(); ++i)
                        {
                            auto *array = std::get_if<std::shared_ptr<ArrayValue>>(current);

                            if (!array || !*array)
                                throw std::runtime_error("Cannot index a non-array value");

                            current = &(*array)->values.at(indices[i]);
                        }

                        // Modify the final indexed element.
                        std::size_t finalIndex = indices.back();

                        if (auto *array = std::get_if<std::shared_ptr<ArrayValue>>(current))
                        {
                            if (!*array)
                                throw std::runtime_error("Cannot index a null array");

                            (*array)->values.at(finalIndex) = value;
                        }
                        else if (auto *str = std::get_if<std::string>(current))
                        {
                            auto *replacement = std::get_if<std::string>(&value);

                            if (!replacement)
                                throw std::runtime_error("String index assignment requires a string");

                            str->replace(finalIndex, 1, *replacement);
                        }
                        else
                        {
                            throw std::runtime_error("Cannot assign to an indexed non-array value");
                        }
                    }
                }
                else
                {
                    if (target.isStrict)
                    {
                        if (std::holds_alternative<int>(value) && assign.vardata.vartype == VariableTypes::Int)
                        {
                            target.value = std::move(value);
                        }
                        else if (std::holds_alternative<std::string>(value) && assign.vardata.vartype == VariableTypes::String)
                        {
                            target.value = std::move(value);
                        }
                        else if (std::holds_alternative<double>(value) && assign.vardata.vartype == VariableTypes::Double)
                        {
                            target.value = std::move(value);
                        }
                        else if (std::holds_alternative<bool>(value) && assign.vardata.vartype == VariableTypes::Bool)
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
                        getVariable2(assign.vardata.name, scope).value = value;
                    }
                }
            }
            continue;
        }
        else if (std::holds_alternative<ForInstruction>(instr.data))
        {
            auto& forst = std::get<ForInstruction>(instr.data);
            Value iterateable = Evaluate(makeTokenRange(forst.iterateable), scope, GlobalFunctionTable);

            std::size_t length = variantLength(iterateable);

            if (length == 0)
            {
                // skip the lop
            }
            else
            {
                scope.back()[forst.object] = {forst.object, indexVariant(iterateable, 0), true, true, false, VariableTypes::AllAccept};

                auto &toModify = getVariable2(forst.object, scope);

                for (std::size_t index = 0; index < length; ++index)
                {
                    toModify.value = indexVariant(iterateable, index);

                    ExecutionResult result = interpret(
                        forst.body,
                        scope,
                        GlobalFunctionTable);

                    if (result.didReturn)
                        return result;

                    if (result.didBreak)
                        break;
                    if (result.didContinue)
                        continue;
                }
            }
        }
        else if (std::holds_alternative<IfInstruction>(instr.data))
        {
            auto& ifst = std::get<IfInstruction>(instr.data);
            scope.push_back(VariableTable{});

            if (variantToBool2(Evaluate(
                    makeTokenRange(ifst.condition),
                    scope,
                    GlobalFunctionTable)))
            {
                ExecutionResult result = interpret(
                    ifst.body, scope, GlobalFunctionTable);

                if (result.didReturn || result.didBreak || result.didContinue)
                {
                    scope.pop_back();
                    return result;
                }
            }
            else if (!ifst.elseBody.empty())
            {
                ExecutionResult result = interpret(
                    ifst.elseBody, scope, GlobalFunctionTable);

                if (result.didReturn || result.didBreak || result.didContinue)
                {
                    scope.pop_back();
                    return result;
                }
            }

            scope.pop_back();
        }
        else if (std::holds_alternative<WhileInstruction>(instr.data))
        {
            auto& whilest = std::get<WhileInstruction>(instr.data);
            while (variantToBool2(Evaluate(
                makeTokenRange(whilest.condition),
                scope,
                GlobalFunctionTable)))
            {
                ExecutionResult result = interpret(
                    whilest.body,
                    scope,
                    GlobalFunctionTable);

                if (result.didReturn)
                    return result;

                if (result.didBreak)
                    break;
                if (result.didContinue)
                    continue;
            }
        }
        else if (std::holds_alternative<BreakInstruction>(instr.data))
        {

            ExecutionResult exec;
            exec.didBreak = true;
            return exec;
        }

        else if (std::holds_alternative<ContinueInstruction>(instr.data))
        {

            ExecutionResult exec;
            exec.didContinue = true;
            return exec;
        }
        else if (std::holds_alternative<FunctionCallInstruction>(instr.data))
        {
            auto& fcall = std::get<FunctionCallInstruction>(instr.data);
            std::string path = join(fcall.path, ".");
            if (path == "Int")
            { // for casting Int(varname);
                VariableData &target = getVariable2(variantToString2(tokenToValue2(fcall.arguments[0][0].value)), scope);
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
                VariableData &target = getVariable2(variantToString2(tokenToValue2(fcall.arguments[0][0].value)), scope);
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
            else if (path == "append")
            {
                Value target = Evaluate(makeTokenRange(fcall.arguments[0]), scope, GlobalFunctionTable);

                Value element = Evaluate(makeTokenRange(fcall.arguments[1]), scope, GlobalFunctionTable);

                if (std::holds_alternative<ArrayPtr>(target))
                {
                    ArrayPtr arr_ptr = std::get<ArrayPtr>(target);
                    if (arr_ptr)
                    {

                        arr_ptr->values.push_back(element);
                    }
                }
                else
                {
                    throw std::runtime_error("Error: Target is not an array.");
                }
            }

            else if (path == "appendByName")
            {
                Value nameValue = Evaluate(
                    makeTokenRange(fcall.arguments[0]),
                    scope,
                    GlobalFunctionTable);

                std::string name = variantToString2(nameValue);

                VariableData &target = getVariable2(name, scope);

                Value element = Evaluate(
                    makeTokenRange(fcall.arguments[1]),
                    scope,
                    GlobalFunctionTable);

                if (std::holds_alternative<ArrayPtr>(target.value))
                {
                    ArrayPtr arr_ptr = std::get<ArrayPtr>(target.value);

                    if (arr_ptr)
                    {
                        arr_ptr->values.push_back(element);
                    }
                    else
                    {
                        throw std::runtime_error("Error: Target array is null.");
                    }
                }
                else
                {
                    throw std::runtime_error("Error: Target is not an array.");
                }
            }
            else if (path == "__varExists")
            {
                VariableData &target = getVariable2(
                    variantToString2(tokenToValue2(fcall.arguments[1][0].value)),
                    scope);

                VariableData &argument = getVariable2(
                    variantToString2( tokenToValue2(fcall.arguments[0][0].value)),
                    scope);

                target.value = doesVariableExist(
                    variantToString2(argument.value),
                    scope);
            }
            else if (path == "__native.getOsType")
            {
                VariableData &target = getVariable2(variantToString2( tokenToValue2(fcall.arguments[0][0].value)), scope);
                target.value = osType();
            }
            else if (path == "__exec")
            {

                auto tokenized = lex(variantToString2(Evaluate(makeTokenRange(fcall.arguments[0]), scope, GlobalFunctionTable)));
                std::vector<std::string> evalPath;
                auto parsed = parse(tokenized, evalPath);
                ExecutionResult exec = interpret(
                    parsed, scope, GlobalFunctionTable);
                if (exec.didReturn)
                {
                    return exec;
                }
            }
            else if (path == "Terminal.IO.print")
            {
                for (size_t k = 0; k < fcall.arguments.size(); k++)
                {
                    std::cout << value_to_string(Evaluate(makeTokenRange(fcall.arguments[k]), scope, GlobalFunctionTable));
                }
            }
            else if (path == "os")
            {
                if (fcall.arguments.empty())
                    throw std::runtime_error("os requires at least one argument");

                std::string command = variantToString2(
                    Evaluate(
                        makeTokenRange(fcall.arguments[0]),
                        scope,
                        GlobalFunctionTable));

                std::vector<std::string> args;

                for (size_t i = 1; i < fcall.arguments.size(); ++i)
                {
                    args.push_back(
                        variantToString2(
                            Evaluate(
                                makeTokenRange(fcall.arguments[i]),
                                scope,
                                GlobalFunctionTable)));
                }

                runCommand(command, args);
            }
            else if (path == "size")
            {
                VariableData &source =
                    getVariable2(
                        variantToString2( tokenToValue2(fcall.arguments[0][0].value)),
                        scope);

                VariableData &target =
                    getVariable2(
                        variantToString2( tokenToValue2(fcall.arguments[1][0].value)),
                        scope);

                Value &value = source.value;

                target.value = static_cast<int>(variantLength(value));
            }

            else if (path == "chrti")
            {
                VariableData &target = getVariable2(variantToString2( tokenToValue2(fcall.arguments[0][0].value)), scope);
                target.value = charCode(variantToString2(Evaluate(makeTokenRange(fcall.arguments[1]), scope, GlobalFunctionTable)));
            }
            else if (path == "itchr")
            {
                VariableData &target = getVariable2(variantToString2( tokenToValue2(fcall.arguments[0][0].value)), scope);
                target.value = charFromCode(variantToInt2(Evaluate(makeTokenRange(fcall.arguments[1]), scope, GlobalFunctionTable)));
            }
            else if (path == "Terminal.IO.println")
            {
                for (size_t k = 0; k < fcall.arguments.size(); k++)
                {
                    std::cout << variantToString2(Evaluate(makeTokenRange(fcall.arguments[k]), scope, GlobalFunctionTable));
                    std::cout << "\n";
                }
            }
            else if (path == "Terminal.IO.input")
            {

                if (fcall.arguments.size() == 0)
                {
                    std::string buffer = "";
                    std::getline(std::cin, buffer);
                }
                else
                {
                    VariableData &target = getVariable2(variantToString2( tokenToValue2(fcall.arguments[0][0].value)), scope);
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
                if (fcall.arguments.size() != 0)
                {
                    std::cerr << "Argument Overflow! Function 'clear' expected 0 arguments, got " << fcall.arguments.size() << ". \n";
                }
                else
                {
                    std::cout << "\033[2J\033[1;1H" << std::flush;
                }
            }
            else if (path == "free")
            {

                if (fcall.arguments.size() == 0)
                {
                    std::cerr << "Function 'free' requires atleast 1 argument";
                }
                else
                {
                    for (int k = 0; k < fcall.arguments.size(); k++)
                    {
                        scope.back().erase(variantToString2(tokenToValue2(fcall.arguments[k][0].value)));
                    }
                }
            }
            else if (auto funcIt = GlobalFunctionTable.find(path);
                     funcIt != GlobalFunctionTable.end())
            {
                GFunction &targetFunc = funcIt->second;

                if (fcall.arguments.size() != targetFunc.locals.size())
                {
                    std::cerr << "Error: function " << path
                              << " expected "
                              << targetFunc.locals.size()
                              << " arguments but got "
                              << fcall.arguments.size()
                              << "\n";

                    ExecutionResult result;
                    result.didReturn = false;
                    result.returnValue = std::monostate();
                    return result;
                }

                // Evaluate arguments
                for (size_t k = 0; k < fcall.arguments.size(); k++)
                {
                    targetFunc.locals[k].value =
                        Evaluate(
                            makeTokenRange(fcall.arguments[k]),
                            scope,
                            GlobalFunctionTable);
                }

                // Create new scope directly
                scope.emplace_back();
                VariableTable &bufferTable = scope.back();

                bufferTable.reserve(targetFunc.locals.size());

                for (const auto &var : targetFunc.locals)
                {
                    bufferTable.emplace(var.name, var);
                }

                ExecutionResult Return = interpret(targetFunc.body, scope, GlobalFunctionTable);

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