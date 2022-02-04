#pragma once

#include <vector>
#include <string>
#include <map>
#include <set>

enum class ArgumentType
{
    Switch,     // Boolean treated as 'true' if present and 'false' if not
    Variable,   // Name/Value pair
    Options,    // Name/Value pair with values constrained to a limited set of strings
    Command,    // A command that can influence handling of subsequent arguments
};

class CInCommandParser
{
    struct Argument
    {
        Argument(ArgumentType type) :
            Type(type) {}
        ArgumentType Type;
        std::string Description;
        std::set<std::string> OptionValues; // Used when Type is ArgumentType::Options

        bool IsPresent = false; // 'true' if the argment is present in the parsed arguments
        std::string Value; // Used if Type==ArgumentType::Variable
    };

    std::map<std::string, Argument> m_Arguments;

    Argument& AddArgument(const char* name, ArgumentType type);

public:
    CInCommandParser();
    virtual ~CInCommandParser() = default;

    // Processes one or more arguments and returns the number
    // of arguments processed
    virtual int ParseArguments(int argc, const char *argv[]);

    void AddSwitchArgument(const char* name);
    bool GetSwitchValue(const char* name) const;

    void AddVariableArgument(const char* name, const char* defaultValue);
    const char* GetVariableValue(const char* name) const;

    void AddOptionsArgument(const char* name, int numOptionValues, const char* optionValues[], int defaultOptionIndex = 0);
    const char* GetOptionsValue(const char* name) const;
};