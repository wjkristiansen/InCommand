#pragma once

#include <vector>
#include <string>
#include <map>
#include <set>

//------------------------------------------------------------------------------------------------
enum class ParameterType
{
    Switch,             // Boolean treated as 'true' if present and 'false' if not
    Variable,           // Name/Value pair
    OptionsVariable,    // Name/Value pair with values constrained to a limited set of strings
};

//------------------------------------------------------------------------------------------------
class CParameter
{
protected:
    bool m_IsPresent = false;
    std::string m_name;
    std::string m_description;

    CParameter(const char* name, const char* description = nullptr) :
        m_name(name)
    {
        if (description)
            m_description = description;
    }

public:
    virtual ~CParameter() = default;

    virtual ParameterType GetType() const = 0;
    virtual const char *GetValueAsString() const = 0;
    virtual int ParseArgs(int argc, const char* argv[]) = 0;

    bool IsPresent() const { return m_IsPresent; }
    const char *GetName() const { return m_name.c_str(); }
    const char *GetDescripotion() const { return m_description.c_str(); }
};

//------------------------------------------------------------------------------------------------
class CSwitchParameter : public CParameter
{
public:
    CSwitchParameter(const char* name, const char* description = nullptr) :
        CParameter(name, description)
    {}

    virtual ParameterType GetType() const final { return ParameterType::Switch; }
    virtual int ParseArgs(int argc, const char* argv[]) final
    {
        m_IsPresent = true;
        return 1;
    }
    virtual const char* GetValueAsString() const final
    {
        return IsPresent() ? "on" : "off";
    }
};

//------------------------------------------------------------------------------------------------
class CVariableParameter : public CParameter
{
    std::string m_value;

public:
    CVariableParameter(const char* name, const char *defaultValue, const char* description = nullptr) :
        CParameter(name, description),
        m_value(defaultValue)
    {}

    virtual ParameterType GetType() const final { return ParameterType::Variable; }
    virtual int ParseArgs(int argc, const char* argv[]) final
    {
        if (argc < 2)
            throw(std::exception("missing argument"));

        m_IsPresent = true;
        m_value = argv[1];
        return 2;
    }
    virtual const char* GetValueAsString() const final { return m_value.c_str(); }
};

//------------------------------------------------------------------------------------------------
class COptionsVariableParameter : public CParameter
{
    std::string m_value;
    std::set<std::string> m_options;

public:
    COptionsVariableParameter(const char* name, int numOptions, const char* options[], int defaultOption = 0, const char* description = nullptr) :
        CParameter(name, description),
        m_value(options[defaultOption])
    {
        for(int i = 0; i < numOptions; ++i)
            m_options.insert(options[i]);
    }

    virtual ParameterType GetType() const final { return ParameterType::OptionsVariable; }
    virtual int ParseArgs(int argc, const char* argv[]) final
    {
        if (argc < 2)
            throw(std::exception("missing argument"));

        // See if the given value is a valid 
        // option value.
        auto vit = m_options.find(argv[1]);
        if (vit == m_options.end())
            throw(std::exception("Invalid option value"));

        m_IsPresent = true;
        m_value = argv[1];
        return 2;
    }
    virtual const char* GetValueAsString() const final { return m_value.c_str(); }
};

//------------------------------------------------------------------------------------------------
class CInCommandParser
{
    std::string m_Description;
    std::string m_Prefix;
    std::map<std::string, std::unique_ptr<CParameter>> m_Parameters;
    std::map<std::string, std::unique_ptr<CInCommandParser>> m_Subcommands;
    int m_MaxAnonymousParameters = 0;
    std::vector<std::string> m_AnonymousParameters;

public:
    CInCommandParser();
    ~CInCommandParser() = default;

    // Processes one or more parameter arguments and returns the number of processed arguments
    // of arguments processed
    int ParseParameterArguments(int argc, const char* argv[]);

    void SetPrefix(const char* prefix);

    CInCommandParser& DeclareSubcommand(const char* name);
    const CParameter& DeclareAnonymousParameters(int maxAnonymousParameters);
    const CParameter& DeclareSwitchParameter(const char* name);
    const CParameter& DeclareVariableParameter(const char* name, const char* defaultValue);
    const CParameter& DeclareOptionsVariableParameter(const char* name, int numOptionValues, const char* optionValues[], int defaultOptionIndex = 0);

    const CParameter& GetParameter(const char* name) const;
};