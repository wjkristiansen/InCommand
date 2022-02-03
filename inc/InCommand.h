#pragma once

#include <vector>
#include <string>
#include <set>
#include <memory>

//enum class ArgumentType
//{
//    Switch,     // Boolean treated as 'true' if present and 'false' if not
//    Variable,   // Name/Value pair
//    Options,    // Name/Value pair with values constrained to a limited set of strings
//    Command,    // A command that can influence handling of subsequent arguments
//};
//
//struct ArgumentDesc
//{
//    std::string Name;
//    ArgumentType Type;
//    std::string Description;
//    std::vector <std::string> Options; // Used when Type is ArgumentType::Options
//};
//
//inline bool operator<(const ArgumentDesc&a, const ArgumentDesc&b)
//{
//    return a.Name < b.Name;
//}

class CInCommandParser
{
    //std::set<ArgumentDesc> m_pParameters;

public:
    CInCommandParser();
    virtual ~CInCommandParser() = default;

    void ParseArguments(int argc, const char *argv[]);

    // Processes one or more arguments and returns the index of
    // the last processed argument.
    virtual int ProcessArgument(int arg, const char* argv[]);
};