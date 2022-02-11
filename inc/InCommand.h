#pragma once

#include <memory>
#include <vector>
#include <string>
#include <map>
#include <set>
#include <ostream>

namespace InCommand
{
    //------------------------------------------------------------------------------------------------
    enum class InCommandResult
    {
        Success,
        DuplicateCommand,
        DuplicateOption,
        UnexpectedArgument,
        NotEnoughArguments,
        UnknownOption,
        InvalidValueString,
    };

    //------------------------------------------------------------------------------------------------
    struct InCommandException
    {
        InCommandResult e;

        InCommandException(InCommandResult error) :
            e(error) {}
    };

    //------------------------------------------------------------------------------------------------
    enum class OptionType
    {
        Parameter,          // A string option not associated with a declared key
        Switch,             // Boolean treated as 'true' if present and 'false' if not
        Variable,           // Name/Value pair
    };

    //------------------------------------------------------------------------------------------------
    template<typename _T>
    inline _T FromString(const std::string &s)
    {
        return _T();
    }

    //------------------------------------------------------------------------------------------------
    template<>
    inline bool FromString<bool>(const std::string &s)
    {
        if(s == "true" || s == "on" || s == "1")
            return true;
        if(s == "false" || s == "off" || s == "0")
            return true;

        throw InCommandException(InCommandResult::InvalidValueString);
    }

    //------------------------------------------------------------------------------------------------
    template<>
    inline int FromString<int>(const std::string &s)
    {
        return std::stoi(s); // may throw on conversion error
    }

    //------------------------------------------------------------------------------------------------
    template<>
    inline unsigned int FromString<unsigned int>(const std::string &s)
    {
        return (unsigned int) std::stoul(s); // may throw on conversion error
    }

    //------------------------------------------------------------------------------------------------
    template<>
    inline std::string FromString<std::string>(const std::string &s)
    {
        return s;
    }

    //------------------------------------------------------------------------------------------------
    class CInCommandValue
    {
    public:
        virtual InCommandResult SetFromString(const std::string &s) = 0;
    };

    //------------------------------------------------------------------------------------------------
    template<class _T>
    class CInCommandTypedValue : public CInCommandValue
    {
        _T m_value = _T();

    public:
        CInCommandTypedValue() = default;
        explicit CInCommandTypedValue(const _T &value) : m_value(value) {}
        operator _T() const { return m_value; }
        CInCommandTypedValue& operator=(const _T &value) { m_value = value; return *this; }
        const _T &Get() const { return m_value; }
        virtual InCommandResult SetFromString(const std::string &s) final
        {
            try
            {
                m_value = FromString<_T>(s);
            }
            catch (const std::invalid_argument& )
            {
                return InCommandResult::InvalidValueString;
            }

            return InCommandResult::Success;
        }
    };

    using InCommandString = CInCommandTypedValue<std::string>;
    using InCommandBool = CInCommandTypedValue<bool>;
    using InCommandInt = CInCommandTypedValue<int>;
    using InCommandUInt = CInCommandTypedValue<unsigned int>;

    //------------------------------------------------------------------------------------------------
    class CArgumentIterator
    {
        int m_index = 0;

    public:
        CArgumentIterator() = default;
        CArgumentIterator(int index) : m_index(index) {}
        int GetIndex() const { return m_index; }
        bool operator==(const CArgumentIterator& o) { return m_index == o.m_index; }
        bool operator!=(const CArgumentIterator& o) { return m_index != o.m_index; }
        CArgumentIterator& operator++() { ++m_index; return *this; }
        CArgumentIterator operator++(int) { CArgumentIterator old(*this); ++m_index; return old; }
    };

    //------------------------------------------------------------------------------------------------
    class CArgumentList
    {
        std::vector<std::string> m_args;

    public:
        CArgumentList(int argc, const char* argv[])
        {
            m_args.resize(argc);
            for(int i = 0; i < argc; ++i)
                m_args[i] = argv[i];
        }

        CArgumentIterator Begin() const { return CArgumentIterator(0); }
        CArgumentIterator End() const { return CArgumentIterator(int(m_args.size())); }
        int Size() const { return int(m_args.size()); }
        const std::string &At(const CArgumentIterator& it) const
        {
            return m_args[it.GetIndex()];
        }
    };

    //------------------------------------------------------------------------------------------------
    class COption
    {
    protected:
        std::string m_name;
        std::string m_description;
        char m_shortKey = 0;

        COption(const char* name, const char* description) :
            m_name(name)
        {
            if (description)
                m_description = description;
        }

    public:
        virtual ~COption() = default;

        virtual OptionType Type() const = 0;

        // Returns the index of the first unparsed argument
        virtual InCommandResult ParseArgs(const CArgumentList &args, CArgumentIterator &it) const = 0;

        const std::string &Name() const { return m_name; }
        const std::string &Description() const { return m_description; }
        char GetShortKey() const { return m_shortKey; }
    };

    //------------------------------------------------------------------------------------------------
    class CParameterOption : public COption
    {
        CInCommandValue &m_value;

    public:
        CParameterOption(CInCommandValue &value, const char* name, const char* description) :
            COption(name, description),
            m_value(value)
        {}

        virtual OptionType Type() const final { return OptionType::Parameter; }
        virtual InCommandResult ParseArgs(const CArgumentList& args, CArgumentIterator& it) const final
        {
            if (it == args.End())
                return InCommandResult::Success;

            auto result = m_value.SetFromString(args.At(it));
            if (result != InCommandResult::Success)
                return result;

            ++it;
            return InCommandResult::Success;
        }
    };

    //------------------------------------------------------------------------------------------------
    class CSwitchOption : public COption
    {
        InCommandBool& m_value;

    public:
        CSwitchOption(InCommandBool &value, const char* name, const char* description, char shortKey = 0) :
            COption(name, description),
            m_value(value)
        {
            m_shortKey = shortKey;
        }

        virtual OptionType Type() const final { return OptionType::Switch; }
        virtual InCommandResult ParseArgs(const CArgumentList& args, CArgumentIterator& it) const final
        {
            if (it == args.End())
                return InCommandResult::Success;

            m_value = true; // Present means true
            ++it;

            return InCommandResult::Success;
        }
    };

    //------------------------------------------------------------------------------------------------
    class CVariableOption : public COption
    {
        CInCommandValue &m_value;

        std::set<std::string> m_domain;

    public:
        CVariableOption(CInCommandValue &value, const char* name, const char* description, char shortKey = 0) :
            COption(name, description),
            m_value(value)
        {
            m_shortKey = shortKey;
        }

        CVariableOption(CInCommandValue &value, const char* name, int domainSize, const char* domain[], const char* description, char shortKey = 0) :
            COption(name, description),
            m_value(value)
        {
            m_shortKey = shortKey;

            for (int i = 0; i < domainSize; ++i)
                m_domain.insert(domain[i]);
        }

        virtual OptionType Type() const final { return OptionType::Variable; }
        virtual InCommandResult ParseArgs(const CArgumentList& args, CArgumentIterator& it) const final
        {
            ++it;

            if (it == args.End())
                return InCommandResult::NotEnoughArguments;

            if (!m_domain.empty())
            {
                // See if the given value is a valid 
                // domain value.
                auto vit = m_domain.find(args.At(it));
                if (vit == m_domain.end())
                    return InCommandResult::InvalidValueString;
            }

            m_value.SetFromString(args.At(it));
            ++it;
            return InCommandResult::Success;
        }
    };

    //------------------------------------------------------------------------------------------------
    class CCommandScope
    {
        int m_ScopeId = 0;
        std::string m_Name;
        std::string m_Description;
        std::map<std::string, std::shared_ptr<COption>> m_Options;
        std::map<char, std::shared_ptr<COption>> m_ShortOptions;
        std::map<std::string, std::shared_ptr<CCommandScope>> m_Subcommands;
        std::vector<std::shared_ptr<COption>> m_ParameterOptions;
        CCommandScope* m_pSuperScope = nullptr;

    public:
        CCommandScope(const char *name, const char *description, int scopeId = 0);
        ~CCommandScope() = default;
        CCommandScope(CCommandScope&& o) = delete;

        // Parses the command argument and set the active command scope.
        // Returns the index of the first argument after the command arguments. 
        // Sets the active subcommand scope.
        CCommandScope &ScanCommandArgs(const CArgumentList &args, CArgumentIterator &it);

        // Processes one or more option arguments and returns the number of processed arguments
        // of arguments processed.
        // Returns the index of the first unparsed argument.
        // Default value for index is 1 since typically the first argument is the app name.
        InCommandResult ScanOptionArgs(const CArgumentList& args, CArgumentIterator& it) const;

        CCommandScope& DeclareSubcommand(const char* name, const char* description, int scopeId = 0);
        const COption& DeclareParameterOption(CInCommandValue &value, const char* name, const char* description);
        const COption& DeclareSwitchOption(InCommandBool &value, const char* name, const char* description, char shortKey = 0);
        const COption& DeclareVariableOption(CInCommandValue& value, const char* name, const char* description, char shortKey = 0);
        const COption& DeclareVariableOption(CInCommandValue& value, const char* name, int domainSize, const char* domain[], const char* description, char shortKey = 0);

        std::string CommandChainString() const;
        std::string UsageString() const;

        const COption& GetOption(const char* name) const;

        // Useful for switch/case using command scope id
        const std::string& Name() const { return m_Name; }
        int GetScopeId() const { return m_ScopeId; }
    };
}