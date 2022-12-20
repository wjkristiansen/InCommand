#pragma once

#include <memory>
#include <vector>
#include <string>
#include <map>
#include <set>
#include <ostream>
#include <optional>

namespace InCommand
{
    //------------------------------------------------------------------------------------------------
    enum class InCommandStatus : int
    {
        Success,
        DuplicateCommand,
        DuplicateOption,
        UnexpectedArgument,
        MissingOptionValue,
        UnknownOption,
        InvalidValue
    };

    //------------------------------------------------------------------------------------------------
    struct InCommandException
    {
        InCommandStatus status;

        InCommandException(InCommandStatus error) :
            status(error) {}
    };

    //------------------------------------------------------------------------------------------------
    struct OptionScanResult
    {
        int NumParameters = 0;
        InCommandStatus Status = InCommandStatus::Success;
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
    inline std::string ToString(const _T &v)
    {
        std::ostringstream ss;
        ss << v;
        return ss.str();
    }

    //------------------------------------------------------------------------------------------------
    template<>
    inline std::string ToString<std::string>(const std::string &s)
    {
        return s;
    }

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

        throw InCommandException(InCommandStatus::InvalidValue);
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
        virtual InCommandStatus SetFromString(const std::string &s) = 0;
        virtual bool HasValue() const = 0;
        virtual std::string ToString() const = 0;
    };

    //------------------------------------------------------------------------------------------------0
    template<class _T>
    class CInCommandTypedValue : public CInCommandValue
    {
        std::optional<_T> m_value;

    public:
        CInCommandTypedValue() = default;
        CInCommandTypedValue(const _T& value) : m_value(value) {}
        operator _T() const { return m_value.value(); }
        CInCommandTypedValue & operator=(const _T & value) { m_value = value; return *this; }
        const _T &Value() const { return m_value.value(); }
        virtual bool HasValue() const final { return m_value.has_value(); }
        virtual InCommandStatus SetFromString(const std::string &s) final
        {
            try
            {
                m_value = FromString<_T>(s);
            }
            catch (const std::invalid_argument& )
            {
                return InCommandStatus::InvalidValue;
            }

            return InCommandStatus::Success;
        }
        virtual std::string ToString() const
        {
            if(!m_value.has_value())
                return "(empty)";

            return InCommand::ToString(m_value.value());
        }
    };

    //------------------------------------------------------------------------------------------------
    template<>
    class CInCommandTypedValue<bool> : public CInCommandValue
    {
        bool m_value = false;

    public:
        CInCommandTypedValue() = default;
        explicit CInCommandTypedValue(bool value) : m_value(value) {}
        operator bool() const { return m_value; }
        CInCommandTypedValue & operator=(bool value) { m_value = value; return *this; }
        const bool Value() const { return m_value; }
        virtual bool HasValue() const final { return true; }
        virtual InCommandStatus SetFromString(const std::string &s) final
        {
            try
            {
                m_value = FromString<bool>(s);
            }
            catch (const std::invalid_argument& )
            {
                return InCommandStatus::InvalidValue;
            }

            return InCommandStatus::Success;
        }
        virtual std::string ToString() const
        {
            return m_value ? "true" : "false";
        }
    };

    //------------------------------------------------------------------------------------------------
    using InCommandString = CInCommandTypedValue<std::string>;
    using InCommandBool = CInCommandTypedValue<bool>;
    using InCommandInt = CInCommandTypedValue<int>;
    using InCommandUInt = CInCommandTypedValue<unsigned int>;

    template<class _T>
    std::ostream& operator<<(std::ostream &s, const CInCommandTypedValue<_T>& v)
    {
        s << v.Value();
        return s;
    }

    //------------------------------------------------------------------------------------------------
    class CArgumentIterator
    {
        int m_index = 0;

    public:
        CArgumentIterator() = default;
        CArgumentIterator(int index) : m_index(index) {}
        int Index() const { return m_index; }
        bool operator==(const CArgumentIterator& o) const { return m_index == o.m_index; }
        bool operator!=(const CArgumentIterator& o) const { return m_index != o.m_index; }
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
            if (argc > 0)
            {
                m_args.resize(argc);
                for (int i = 0; i < argc; ++i)
                    m_args[i] = argv[i];
            }
        }

        CArgumentIterator Begin() const { return CArgumentIterator(0); }
        CArgumentIterator End() const { return CArgumentIterator(int(m_args.size())); }
        int Size() const { return int(m_args.size()); }
        const std::string &At(const CArgumentIterator& it) const
        {
            return m_args[it.Index()];
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
        virtual InCommandStatus ParseArgs(const CArgumentList &args, CArgumentIterator &it) const = 0;

        const std::string &Name() const { return m_name; }
        const std::string &Description() const { return m_description; }
        char ShortKey() const { return m_shortKey; }
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
        virtual InCommandStatus ParseArgs(const CArgumentList& args, CArgumentIterator& it) const final
        {
            if (it == args.End())
                return InCommandStatus::Success;

            auto result = m_value.SetFromString(args.At(it));
            if (result != InCommandStatus::Success)
                return result;

            ++it;
            return InCommandStatus::Success;
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
        virtual InCommandStatus ParseArgs(const CArgumentList& args, CArgumentIterator& it) const final
        {
            if (it == args.End())
                return InCommandStatus::Success;

            m_value = true; // Present means true
            ++it;

            return InCommandStatus::Success;
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

        CVariableOption(CInCommandValue& value, const char* name, int domainSize, const CInCommandValue *pDomainValues, const char* description, char shortKey = 0) :
            COption(name, description),
            m_value(value)
        {
            m_shortKey = shortKey;

            for (int i = 0; i < domainSize; ++i)
                m_domain.insert(pDomainValues[i].ToString());
        }

        virtual OptionType Type() const final { return OptionType::Variable; }
        virtual InCommandStatus ParseArgs(const CArgumentList& args, CArgumentIterator& it) const final
        {
            ++it;

            if (it == args.End())
                return InCommandStatus::MissingOptionValue;

            if (!m_domain.empty())
            {
                // See if the given value is a valid 
                // domain value.
                auto vit = m_domain.find(args.At(it));
                if (vit == m_domain.end())
                    return InCommandStatus::InvalidValue;
            }

            auto result = m_value.SetFromString(args.At(it));
            if (result != InCommandStatus::Success)
                return result;

            ++it;
            return InCommandStatus::Success;
        }
    };

    //------------------------------------------------------------------------------------------------
    class CCommandCtx
    {
        int m_ScopeId = 0;
        std::string m_Name;
        std::string m_Description;
        std::map<std::string, std::shared_ptr<COption>> m_Options;
        std::map<char, std::shared_ptr<COption>> m_ShortOptions;
        std::map<std::string, std::shared_ptr<CCommandCtx>> m_Subcommands;
        std::vector<std::shared_ptr<COption>> m_ParameterOptions;
        CCommandCtx* m_pSuperScope = nullptr;

        CCommandCtx* FetchCommandCtx(class CCommandReader *pReader);
        InCommandStatus FetchOptions(class CCommandReader *pReader) const;

    public:
        CCommandCtx(const char* name, const char* description, int scopeId = 0);
        CCommandCtx(CCommandCtx&& o) = delete;
        ~CCommandCtx() = default;

        CCommandCtx* DeclareCommandCtx(const char* name, const char* description, int scopeId = 0);
        const COption* DeclareParameterOption(CInCommandValue &value, const char* name, const char* description);
        const COption* DeclareSwitchOption(InCommandBool &value, const char* name, const char* description, char shortKey = 0);
        const COption* DeclareVariableOption(CInCommandValue& value, const char* name, const char* description, char shortKey = 0);
        const COption* DeclareVariableOption(CInCommandValue& value, const char* name, int domainSize, const char* domain[], const char* description, char shortKey = 0);
        const COption* DeclareVariableOption(CInCommandValue& value, const char* name, int domainSize, const CInCommandValue *pDomainValues, const char* description, char shortKey = 0);

        std::string CommandChainString() const;

        friend class CCommandReader;

        std::string UsageString() const;

        // Useful for switch/case using command scope id
        const std::string& Name() const { return m_Name; }
        int Id() const { return m_ScopeId; }
    };

    //------------------------------------------------------------------------------------------------
    class CCommandReader
    {
        CCommandCtx m_DefaultCtx;
        CArgumentList m_ArgList;
        CArgumentIterator m_ArgIt;
        CCommandCtx* m_pActiveCtx = nullptr;
        InCommandStatus m_LastStatus = InCommandStatus::Success;
        size_t m_ParametersRead = 0;

        friend class CCommandCtx;

    public:
        CCommandReader(const char* appName, const char *defaultDescription, int argc, const char* argv[]);

        void Reset(int argc, const char* argv[])
        {
            m_ArgList = CArgumentList(argc, argv);
            m_ArgIt = m_ArgList.Begin();
            m_ParametersRead = 0;
            m_LastStatus = InCommandStatus::Success;
        }

        // Returns the default command context pointer, used for declaring subcommands.
        CCommandCtx* DefaultCommandCtx() { return &m_DefaultCtx; }

        // Returns the active command context pointer, or nullptr if the active command
        // has not been fetched.
        CCommandCtx* ActiveCommandCtx() { return m_pActiveCtx; }

        // Reads the command context chain from the argument list
        // and returns the active command context.
        // Afterward, the argument iterator index points to
        // the first option argument in the argument list.
        // Allows the application to delay-declare command options.
        CCommandCtx* ReadCommandCtx();

        // Reads the command options from the argument list, setting the bound values.
        // Requires the active command be set by calling ReadActiveCommand.
        // Reads the active command if ReadCommand was not previously called.
        InCommandStatus ReadOptions();

        std::string LastErrorString() const;
        InCommandStatus LastStatus() const { return m_LastStatus; }
        int ReadIndex() const { return m_ArgIt.Index(); }
    };
}