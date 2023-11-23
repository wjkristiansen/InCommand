#pragma once

#include <memory>
#include <vector>
#include <string>
#include <map>
#include <set>
#include <ostream>
#include <sstream>
#include <optional>

namespace InCommand
{
    //------------------------------------------------------------------------------------------------
    enum class Status : int
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
    struct Exception
    {
        Status status;

        Exception(Status error) :
            status(error) {}
    };

    //------------------------------------------------------------------------------------------------
    enum class ParameterType
    {
        Input,
        Option,
        Bool
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

        throw Exception(Status::InvalidValue);
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
    class Value
    {
    public:
        virtual Status SetFromString(const std::string &s) = 0;
        virtual bool HasValue() const = 0;
        virtual std::string ToString() const = 0;
    };

    //------------------------------------------------------------------------------------------------
    class Domain
    {
        std::set<std::string> m_values;

    public:
        Domain() = default;

        template<class _T>
        Domain(int NumValues, const _T* pValues)
        {
            for (int i = 0; i < NumValues; ++i)
            {
                m_values.emplace(InCommand::ToString(pValues[i]));
            }
        }

        template<class _T>
        Domain(const _T& v0, const _T& v1)
        {
            m_values.emplace(InCommand::ToString(v0));
            m_values.emplace(InCommand::ToString(v1));
        }

        template<class _T>
        Domain(const _T& v0, const _T& v1, const _T& v2, const _T& v3)
        {
            m_values.emplace(InCommand::ToString(v0));
            m_values.emplace(InCommand::ToString(v1));
            m_values.emplace(InCommand::ToString(v2));
            m_values.emplace(InCommand::ToString(v3));
        }

        template<class _T>
        Domain(const _T& v0, const _T& v1, const _T& v2, const _T& v3, const _T& v4)
        {
            m_values.emplace(InCommand::ToString(v0));
            m_values.emplace(InCommand::ToString(v1));
            m_values.emplace(InCommand::ToString(v2));
            m_values.emplace(InCommand::ToString(v3));
            m_values.emplace(InCommand::ToString(v4));
        }

        template<class _T>
        Domain(const _T& v0, const _T& v1, const _T& v2, const _T& v3, const _T& v4, const _T& v5)
        {
            m_values.emplace(InCommand::ToString(v0));
            m_values.emplace(InCommand::ToString(v1));
            m_values.emplace(InCommand::ToString(v2));
            m_values.emplace(InCommand::ToString(v3));
            m_values.emplace(InCommand::ToString(v4));
            m_values.emplace(InCommand::ToString(v5));
        }

        template<class _T>
        Domain(const _T& v0, const _T& v1, const _T& v2, const _T& v3, const _T& v4, const _T& v5, const _T& v6)
        {
            m_values.emplace(InCommand::ToString(v0));
            m_values.emplace(InCommand::ToString(v1));
            m_values.emplace(InCommand::ToString(v2));
            m_values.emplace(InCommand::ToString(v3));
            m_values.emplace(InCommand::ToString(v4));
            m_values.emplace(InCommand::ToString(v5));
            m_values.emplace(InCommand::ToString(v6));
        }

        template<class _T>
        Domain(const _T& v0, const _T& v1, const _T& v2, const _T& v3, const _T& v4, const _T& v5, const _T& v6, const _T& v7)
        {
            m_values.emplace(InCommand::ToString(v0));
            m_values.emplace(InCommand::ToString(v1));
            m_values.emplace(InCommand::ToString(v2));
            m_values.emplace(InCommand::ToString(v3));
            m_values.emplace(InCommand::ToString(v4));
            m_values.emplace(InCommand::ToString(v5));
            m_values.emplace(InCommand::ToString(v6));
            m_values.emplace(InCommand::ToString(v7));
        }

        bool HasValues() const { return !m_values.empty(); }

        template<class _T>
        bool ContainsValue(const _T& value) const
        {
            std::string valueString = InCommand::ToString(value);
            auto it = m_values.find(valueString);
            return it != m_values.end();
        }
    };

    //------------------------------------------------------------------------------------------------0
    template<class _T>
    class CTypedValue : public Value
    {
        std::optional<_T> m_value;

    public:
        CTypedValue() = default;
        CTypedValue(const _T& value) : m_value(value) {}
        operator _T() const { return m_value.value(); }
        CTypedValue & operator=(const _T & value) { m_value = value; return *this; }
        const _T &Value() const { return m_value.value(); }
        virtual bool HasValue() const final { return m_value.has_value(); }
        virtual Status SetFromString(const std::string &s) final
        {
            try
            {
                m_value = FromString<_T>(s);
            }
            catch (const std::invalid_argument& )
            {
                return Status::InvalidValue;
            }

            return Status::Success;
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
    class CTypedValue<bool> : public Value
    {
        bool m_value = false;

    public:
        CTypedValue() = default;
        explicit CTypedValue(bool value) : m_value(value) {}
        operator bool() const { return m_value; }
        CTypedValue & operator=(bool value) { m_value = value; return *this; }
        const bool Value() const { return m_value; }
        virtual bool HasValue() const final { return true; }
        virtual Status SetFromString(const std::string &s) final
        {
            try
            {
                m_value = FromString<bool>(s);
            }
            catch (const std::invalid_argument& )
            {
                return Status::InvalidValue;
            }

            return Status::Success;
        }
        virtual std::string ToString() const
        {
            return m_value ? "true" : "false";
        }
    };

    //------------------------------------------------------------------------------------------------
    using String = CTypedValue<std::string>;
    using Bool = CTypedValue<bool>;
    using Int = CTypedValue<int>;
    using UInt = CTypedValue<unsigned int>;

    template<class _T>
    std::ostream& operator<<(std::ostream &s, const CTypedValue<_T>& v)
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
    class CParameter
    {
    protected:
        std::string m_name;
        std::string m_description;
        char m_shortKey = 0;

        CParameter(const char* name, const char* description) :
            m_name(name)
        {
            if (description)
                m_description = description;
        }

    public:
        virtual ~CParameter() = default;

        virtual ParameterType Type() const = 0;

        // Returns the index of the first unparsed argument
        virtual Status ParseArgs(const CArgumentList &args, CArgumentIterator &it) const = 0;

        const std::string &Name() const { return m_name; }
        const std::string &Description() const { return m_description; }
        char ShortKey() const { return m_shortKey; }
    };

    //------------------------------------------------------------------------------------------------
    class CInputParameter : public CParameter
    {
        Value &m_value;

    public:
        CInputParameter(Value &value, const char* name, const char* description) :
            CParameter(name, description),
            m_value(value)
        {}

        virtual ParameterType Type() const final { return ParameterType::Input; }
        virtual Status ParseArgs(const CArgumentList& args, CArgumentIterator& it) const final
        {
            if (it == args.End())
                return Status::Success;

            auto result = m_value.SetFromString(args.At(it));
            if (result != Status::Success)
                return result;

            ++it;
            return Status::Success;
        }
    };

    //------------------------------------------------------------------------------------------------
    class CBoolParameter : public CParameter
    {
        Bool& m_value;

    public:
        CBoolParameter(Bool &value, const char* name, const char* description, char shortKey = 0) :
            CParameter(name, description),
            m_value(value)
        {
            m_shortKey = shortKey;
        }

        virtual ParameterType Type() const final { return ParameterType::Bool; }
        virtual Status ParseArgs(const CArgumentList& args, CArgumentIterator& it) const final
        {
            if (it == args.End())
                return Status::Success;

            m_value = true; // Present means true
            ++it;

            return Status::Success;
        }
    };

    //------------------------------------------------------------------------------------------------
    class COptionParameter : public CParameter
    {
        Value &m_value;
        Domain m_domain;

    public:
        COptionParameter(Value &value, const char* name, const char* description, char shortKey = 0) :
            CParameter(name, description),
            m_value(value)
        {
            m_shortKey = shortKey;
        }

        COptionParameter(Value& value, const char* name, const Domain &domain, const char* description, char shortKey = 0) :
            CParameter(name, description),
            m_value(value),
            m_domain(domain)
        {
            m_shortKey = shortKey;
        }

        virtual ParameterType Type() const final { return ParameterType::Option; }
        virtual Status ParseArgs(const CArgumentList& args, CArgumentIterator& it) const final
        {
            ++it;

            if (it == args.End())
                return Status::MissingOptionValue;

            if (m_domain.HasValues())
            {
                // See if the given value is a valid 
                // domain value.
                if( !m_domain.ContainsValue(args.At(it)))
                    return Status::InvalidValue;
            }

            auto result = m_value.SetFromString(args.At(it));
            if (result != Status::Success)
                return result;

            ++it;
            return Status::Success;
        }
    };

    //------------------------------------------------------------------------------------------------
    class CCommand
    {
        int m_ScopeId = 0;
        std::string m_Name;
        std::string m_Description;
        std::map<std::string, std::shared_ptr<CParameter>> m_Parameters;
        std::map<char, std::shared_ptr<CParameter>> m_ShortOptions;
        std::map<std::string, std::shared_ptr<CCommand>> m_Subcommands;
        std::vector<std::shared_ptr<CParameter>> m_InputParameters;
        CCommand* m_pSuperScope = nullptr;

        CCommand* ReadCommandArguments(class CCommandReader *pReader);
        Status ReadParameterArguments(class CCommandReader *pReader) const;

    public:
        CCommand(const char* name, const char* description, int scopeId = 0);
        CCommand(CCommand&& o) = delete;
        ~CCommand() = default;

        CCommand* DeclareCommand(const char* name, const char* description, int scopeId = 0);
        const CParameter* DeclareInputParameter(Value &value, const char* name, const char* description);
        const CParameter* DeclareBoolParameter(Bool &value, const char* name, const char* description, char shortKey = 0);
        const CParameter* DeclareOptionParameter(Value& value, const char* name, const char* description, char shortKey = 0);
        const CParameter* DeclareOptionParameter(Value& value, const char* name, int domainSize, const char* domain[], const char* description, char shortKey = 0);
        const CParameter* DeclareOptionParameter(Value& value, const char* name, const Domain& domain, const char* description, char shortKey = 0);

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
        CCommand m_DefaultCtx;
        CArgumentList m_ArgList;
        CArgumentIterator m_ArgIt;
        CCommand* m_pActiveCtx = nullptr;
        Status m_LastStatus = Status::Success;
        size_t m_ParametersRead = 0;

        friend class CCommand;

    public:
        CCommandReader(const char* appName, const char *defaultDescription, int argc, const char* argv[]);

        void Reset(int argc, const char* argv[])
        {
            m_ArgList = CArgumentList(argc, argv);
            m_ArgIt = m_ArgList.Begin();
            m_ParametersRead = 0;
            m_LastStatus = Status::Success;
        }

        // Returns the default command context pointer, used for declaring subcommands.
        CCommand* DefaultCommand() { return &m_DefaultCtx; }

        // Returns the active command context pointer, or nullptr if the active command
        // has not been fetched.
        CCommand* ActiveCommand() { return m_pActiveCtx; }

        // Reads the command arguments from the argument list
        // and returns the matching command context.
        // Afterward, the argument iterator index points to
        // the first option argument in the argument list.
        // Allows the application to delay-declare command options.
        CCommand* ReadCommandArguments();

        // Reads the parameter values from the argument list, setting the bound values.
        // Requires the active command be set by calling ReadActiveCommand.
        // Reads the active command if ReadCommand was not previously called.
        Status ReadParameterArguments();

        std::string LastErrorString() const;
        Status LastStatus() const { return m_LastStatus; }
        int ReadIndex() const { return m_ArgIt.Index(); }
    };
}