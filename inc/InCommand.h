#pragma once

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
        InvalidVariableValue,
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
        NonKeyed,           // A string option not associated with a declared key
        Switch,             // Boolean treated as 'true' if present and 'false' if not
        Variable,           // Name/Value pair
    };

    //------------------------------------------------------------------------------------------------
    template<class _T>
    class CInCommandType
    {
        _T m_Value = _T();

    public:
        CInCommandType() = default;
        explicit CInCommandType(const _T &value) : m_Value(value) {}
        operator _T() const { return m_Value; }
        CInCommandType& operator=(const _T &value) { m_Value = value; return *this; }
        const _T &Get() const { return m_Value; }
    };

    using InCommandString = CInCommandType<std::string>;
    using InCommandBool = CInCommandType<bool>;

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
        int m_argc;
        const char** m_argv;

    public:
        CArgumentList(int argc, const char* argv[]) :
            m_argc(argc),
            m_argv(argv)
        {}

        CArgumentIterator Begin() const { return CArgumentIterator(0); }
        CArgumentIterator End() const { return CArgumentIterator(m_argc); }
        int Size() const { return m_argc; }
        const char* At(const CArgumentIterator& it) const
        {
            return m_argv[it.GetIndex()];
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
        const char GetShortKey() const { return m_shortKey; }
    };

    //------------------------------------------------------------------------------------------------
    class CNonKeyedOption : public COption
    {
        InCommandString& m_value;

    public:
        CNonKeyedOption(InCommandString &value, const char* name, const char* description) :
            m_value(value),
            COption(name, description)
        {}

        virtual OptionType Type() const final { return OptionType::NonKeyed; }
        virtual InCommandResult ParseArgs(const CArgumentList& args, CArgumentIterator& it) const final
        {
            if (it == args.End())
                return InCommandResult::Success;

            m_value = args.At(it);
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
            m_value(value),
            COption(name, description)
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
        InCommandString& m_value;

        std::set<std::string> m_domain;

    public:
        CVariableOption(InCommandString &value, const char* name, const char* description, char shortKey = 0) :
            m_value(value),
            COption(name, description)
        {
            m_shortKey = shortKey;
        }

        CVariableOption(InCommandString& value, const char* name, int domainSize, const char* domain[], const char* description, char shortKey = 0) :
            m_value(value),
            COption(name, description)
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
                    return InCommandResult::InvalidVariableValue;
            }

            m_value = args.At(it);
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
        std::vector<std::shared_ptr<COption>> m_NonKeyedOptions;
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
        const COption& DeclareNonKeyedOption(InCommandString &value, const char* name, const char* description);
        const COption& DeclareSwitchOption(InCommandBool &value, const char* name, const char* description, char shortKey = 0);
        const COption& DeclareVariableOption(InCommandString& value, const char* name, const char* description, char shortKey = 0);
        const COption& DeclareVariableOption(InCommandString& value, const char* name, int domainSize, const char* domain[], const char* description, char shortKey = 0);

        std::string CommandChainString() const;
        std::string UsageString() const;

        const COption& GetOption(const char* name) const;

        // Useful for switch/case using command scope id
        const std::string& Name() const { return m_Name; }
        int GetScopeId() const { return m_ScopeId; }
    };
}