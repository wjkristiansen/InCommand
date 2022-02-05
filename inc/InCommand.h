#pragma once

#include <vector>
#include <string>
#include <map>
#include <set>

enum class ParameterType
{
    Switch,             // Boolean treated as 'true' if present and 'false' if not
    Variable,           // Name/Value pair
    OptionsVariable,    // Name/Value pair with values constrained to a limited set of strings
};

class CInCommandParser
{
    struct Parameter
    {
        Parameter(ParameterType type) :
            Type(type) {}
        ParameterType Type;
        std::string Name;
        std::string Description;
        std::set<std::string> OptionValues; // Used when Type is ParameterType::OptionsVariable

        bool IsPresent = false; // 'true' only if the parameter argument is present in the parsed arguments
        std::string Value; // Used if Type==ParameterType::Variable or Type==ParameterType::OptionsVariable
    };

    std::string m_Description;
    std::string m_Prefix;
    std::map<std::string, Parameter> m_Parameters;
    std::map<std::string, std::unique_ptr<CInCommandParser>> m_Subcommands;
    int m_MaxAnonymousParameters = 0;
    std::vector<std::string> m_AnonymousParameters;

    Parameter& DeclareParameterImpl(const char* name, ParameterType type);
    const Parameter& GetParameterImpl(const char* name, ParameterType type) const;

public:
    CInCommandParser();
    ~CInCommandParser() = default;

    // Processes one or more parameter arguments and returns the number of processed arguments
    // of arguments processed
    int ParseParameterArguments(int argc, const char* argv[]);

    void SetPrefix(const char* prefix);

    CInCommandParser& DeclareSubcommand(const char* name);
    void DeclareAnonymousParameters(int maxAnonymousParameters);

    void DeclareSwitchParameter(const char* name);
    bool GetSwitchValue(const char* name) const;

    void DeclareVariableParameter(const char* name, const char* defaultValue);
    const char* GetVariableValue(const char* name) const;

    void DeclareOptionsVariableParameter(const char* name, int numOptionValues, const char* optionValues[], int defaultOptionIndex = 0);
    const char* GetOptionsVariableValue(const char* name) const;
};