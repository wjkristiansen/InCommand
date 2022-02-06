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
        DuplicateOption,
        UnexpectedArgument,
        NotEnoughArguments,
        UnknownOption,
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
    enum class OptionType
    {
        NonKeyed,           // A string option not associated with a declared key
        Switch,             // Boolean treated as 'true' if present and 'false' if not
        Variable,           // Name/Value pair
    };

    //------------------------------------------------------------------------------------------------
    class COption
    {
    protected:
        bool m_IsPresent = false;
        std::string m_name;
        std::string m_description;

        COption(const char* name, const char* description = nullptr) :
            m_name(name)
        {
            if (description)
                m_description = description;
        }

    public:
        virtual ~COption() = default;

        virtual OptionType GetType() const = 0;
        virtual const char* GetValueAsString() const = 0;

        // Returns the index of the first unparsed argument
        virtual int ParseArgs(int arg, int argc, const char* argv[]) = 0;

        bool IsPresent() const { return m_IsPresent; }
        const char* GetName() const { return m_name.c_str(); }
        const char* GetDescripotion() const { return m_description.c_str(); }
    };

    //------------------------------------------------------------------------------------------------
    class CNonKeyedOption : public COption
    {
        std::string m_value;

    public:
        CNonKeyedOption(const char* name, const char* description = nullptr) :
            COption(name, description)
        {}

        virtual OptionType GetType() const final { return OptionType::NonKeyed; }
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
    class CSwitchOption : public COption
    {
    public:
        CSwitchOption(const char* name, const char* description = nullptr) :
            COption(name, description)
        {}

        virtual OptionType GetType() const final { return OptionType::Switch; }
        virtual int ParseArgs(int arg, int argc, [[maybe_unused]] const char* argv[]) final
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
    class CVariableOption : public COption
    {
        std::string m_value;
        std::set<std::string> m_domain;

    public:
        CVariableOption(const char* name, const char* defaultValue, const char* description = nullptr) :
            COption(name, description),
            m_value(defaultValue)
        {}

        CVariableOption(const char* name, int domainSize, const char* domain[], int defaultIndex = 0, const char* description = nullptr) :
            COption(name, description),
            m_value(domain[defaultIndex])
        {
            for (int i = 0; i < domainSize; ++i)
                m_domain.insert(domain[i]);
        }

        virtual OptionType GetType() const final { return OptionType::Variable; }
        virtual int ParseArgs(int arg, int argc, const char* argv[]) final
        {
            if (argc - arg < 2)
                throw InCommandException(InCommandError::NotEnoughArguments, argv[arg], arg);

            ++arg;

            if (!m_domain.empty())
            {
                // See if the given value is a valid 
                // domain value.
                auto vit = m_domain.find(argv[arg]);
                if (vit == m_domain.end())
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
        std::map<std::string, std::unique_ptr<COption>> m_Options;
        std::map<std::string, std::unique_ptr<CCommandScope>> m_Subcommands;
        std::vector<std::unique_ptr<CNonKeyedOption>> m_NonKeyedOptions;
        int m_NumPresentNonKeyed = 0;

    public:
        CCommandScope();
        ~CCommandScope() = default;
        CCommandScope(CCommandScope&& o) = delete;

        // Processes one or more option arguments and returns the number of processed arguments
        // of arguments processed.
        // Returns the index of the first unparsed argument.
        int ParseArguments(int arg, int argc, const char* argv[]);

        void SetPrefix(const char* prefix);

        CCommandScope& DeclareSubcommand(const char* name);
        const COption& DeclareNonKeyedOption(const char* name);
        const COption& DeclareSwitchOption(const char* name);
        const COption& DeclareVariableOption(const char* name, const char* defaultValue);
        const COption& DeclareVariableOption(const char* name, int domainSize, const char* domain[], int defaultIndex = 0);

        const COption& GetOption(const char* name) const;
    };
}