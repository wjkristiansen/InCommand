#pragma once

#include <vector>
#include <string>
#include <map>
#include <set>

namespace InCommand
{
    enum class InCommandError
    {
        DuplicateCommand,
        DuplicateKeyedParameter,
        UnexpectedArgument,
        NotEnoughArguments,
        UnknownParameter,
        InvalidVariableValue,
    };

    struct InCommandException
    {
        InCommandError e;
        std::string s;
        int arg;

        InCommandException(InCommandError error, const char* str, int argIndex) :
            e(error),
            s(str),
            arg(argIndex) {}
    };

    //------------------------------------------------------------------------------------------------
    enum class ParameterType
    {
        NonKeyed,           // A string parameter not associated with a declared key
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
        virtual const char* GetValueAsString() const = 0;

        // Returns the index of the first unparsed argument
        virtual int ParseArgs(int arg, int argc, const char* argv[]) = 0;

        bool IsPresent() const { return m_IsPresent; }
        const char* GetName() const { return m_name.c_str(); }
        const char* GetDescripotion() const { return m_description.c_str(); }
    };

    //------------------------------------------------------------------------------------------------
    class CNonKeyedParameter : public CParameter
    {
        std::string m_value;

    public:
        CNonKeyedParameter(const char* name, const char* description = nullptr) :
            CParameter(name, description)
        {}

        virtual ParameterType GetType() const final { return ParameterType::NonKeyed; }
        virtual int ParseArgs(int arg, int argc, const char* argv[]) final
        {
            if (arg >= argc)
                return arg;

            m_IsPresent = true;
            m_value = argv[arg];
            return arg + 1;
        }
        virtual const char* GetValueAsString() const final { return m_value.c_str(); }
    };

    //------------------------------------------------------------------------------------------------
    class CSwitchParameter : public CParameter
    {
    public:
        CSwitchParameter(const char* name, const char* description = nullptr) :
            CParameter(name, description)
        {}

        virtual ParameterType GetType() const final { return ParameterType::Switch; }
        virtual int ParseArgs(int arg, int argc, const char* argv[]) final
        {
            if (arg >= argc)
                return arg;

            m_IsPresent = true;
            return arg + 1;
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
        std::set<std::string> m_options;

    public:
        CVariableParameter(const char* name, const char* defaultValue, const char* description = nullptr) :
            CParameter(name, description),
            m_value(defaultValue)
        {}

        CVariableParameter(const char* name, int numOptions, const char* options[], int defaultOption = 0, const char* description = nullptr) :
            CParameter(name, description),
            m_value(options[defaultOption])
        {
            for (int i = 0; i < numOptions; ++i)
                m_options.insert(options[i]);
        }

        virtual ParameterType GetType() const final { return ParameterType::OptionsVariable; }
        virtual int ParseArgs(int arg, int argc, const char* argv[]) final
        {
            if (argc - arg < 2)
                throw InCommandException(InCommandError::NotEnoughArguments, argv[arg], arg);

            ++arg;

            if (!m_options.empty())
            {
                // See if the given value is a valid 
                // option value.
                auto vit = m_options.find(argv[arg]);
                if (vit == m_options.end())
                    throw InCommandException(InCommandError::InvalidVariableValue, argv[arg], arg);
            }

            m_IsPresent = true;
            m_value = argv[arg];
            return arg + 1;
        }
        virtual const char* GetValueAsString() const final { return m_value.c_str(); }
    };

    //------------------------------------------------------------------------------------------------
    class CCommandScope
    {
        std::string m_Description;
        std::string m_Prefix;
        std::map<std::string, std::unique_ptr<CParameter>> m_Parameters;
        std::map<std::string, std::unique_ptr<CCommandScope>> m_Subcommands;
        std::vector<std::unique_ptr<CNonKeyedParameter>> m_NonKeyedParameters;
        int m_NumPresentNonKeyed = 0;

    public:
        CCommandScope();
        ~CCommandScope() = default;
        CCommandScope(CCommandScope&& o) = delete;

        // Processes one or more parameter arguments and returns the number of processed arguments
        // of arguments processed.
        // Returns the index of the first unparsed argument.
        int ParseParameterArguments(int arg, int argc, const char* argv[]);

        void SetPrefix(const char* prefix);

        CCommandScope& DeclareSubcommand(const char* name);
        const CParameter& DeclareNonKeyedParameter(const char* name);
        const CParameter& DeclareSwitchParameter(const char* name);
        const CParameter& DeclareVariableParameter(const char* name, const char* defaultValue);
        const CParameter& DeclareOptionsVariableParameter(const char* name, int numOptionValues, const char* optionValues[], int defaultOptionIndex = 0);

        const CParameter& GetParameter(const char* name) const;
    };
}