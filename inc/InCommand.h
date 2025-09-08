#pragma once

#include <memory>
#include <vector>
#include <string>
#include <map>
#include <set>
#include <deque>
#include <unordered_set>
#include <unordered_map>
#include <ostream>
#include <sstream>
#include <optional>
#include <any>
#include <functional>
#include <charconv>
#include <type_traits>

namespace InCommand
{
    //------------------------------------------------------------------------------------------------
    // API Misuse Errors - thrown when developers use the InCommand API incorrectly
    enum class ApiError
    {
        None,
        OutOfMemory,
        DuplicateCommandBlock,
        DuplicateOption,
        UniqueIdNotAssigned,
        OptionNotFound,
        ParameterNotFound,
        InvalidOptionType,
        OutOfRange,
    };

    //------------------------------------------------------------------------------------------------
    // Command Line Syntax Errors - thrown when end users provide invalid command line syntax
    enum class SyntaxError
    {
        None,
        UnknownOption,
        MissingVariableValue,
        UnexpectedArgument,
        TooManyParameters,
        InvalidValue,
        OptionNotSet,
        InvalidAlias
    };

    //------------------------------------------------------------------------------------------------
    // Variable assignment delimiters for packed option=value syntax
    enum class VariableDelimiter
    {
        Whitespace,  // Traditional whitespace-separated format only (--name value)
        Equals,      // Enable --name=value and -n=value formats
        Colon        // Enable --name:value and -n:value formats
    };

    //------------------------------------------------------------------------------------------------
    // Exception for API misuse by developers
    class ApiException
    {
        ApiError m_error;
        std::string m_message;

    public:
        ApiException(ApiError error, const std::string& message = "") : 
            m_error(error), m_message(message) {}

        ApiError GetError() const { return m_error; }
        const std::string& GetMessage() const { return m_message; }
    };

    //------------------------------------------------------------------------------------------------
    // Exception for command line syntax errors by end users
    class SyntaxException
    {
        SyntaxError m_error;
        std::string m_message;
        std::string m_token; // The problematic token from command line

    public:
        SyntaxException(SyntaxError error, const std::string& message = "", const std::string& token = "") : 
            m_error(error), m_message(message), m_token(token) {}

        SyntaxError GetError() const { return m_error; }
        const std::string& GetMessage() const { return m_message; }
        const std::string& GetToken() const { return m_token; }
    };

    //------------------------------------------------------------------------------------------------
    enum class OptionType
    {
        Undefined,
        Switch,
        Variable,
        Parameter,
    };

    //------------------------------------------------------------------------------------------------
    // Default conversion functor for fundamental types using std::from_chars
    template<typename T>
    struct DefaultConverter
    {
        T operator()(const std::string& str) const
        {
            if constexpr (std::is_same_v<T, std::string>)
            {
                return str;
            }
            else if constexpr (std::is_same_v<T, char>)
            {
                if (str.empty())
                    throw SyntaxException(SyntaxError::InvalidValue, "Empty string cannot be converted to char", str);
                return str[0];
            }
            else if constexpr (std::is_same_v<T, bool>)
            {
                // Handle boolean conversions
                if (str == "true" || str == "1" || str == "yes" || str == "on")
                    return true;
                else if (str == "false" || str == "0" || str == "no" || str == "off")
                    return false;
                else
                    throw SyntaxException(SyntaxError::InvalidValue, "Invalid boolean value (expected true/false, 1/0, yes/no, on/off)", str);
            }
            else if constexpr (std::is_arithmetic_v<T>)
            {
                T result;
                auto [ptr, ec] = std::from_chars(str.data(), str.data() + str.size(), result);
                if (ec != std::errc{} || ptr != str.data() + str.size())
                {
                    throw SyntaxException(SyntaxError::InvalidValue, "Invalid value for numeric type", str);
                }
                return result;
            }
            else
            {
                static_assert(!sizeof(T), "Type not supported by DefaultConverter. Please provide a custom converter.");
            }
        }
    };

    //------------------------------------------------------------------------------------------------
    // OptionDesc
    // Describes an option used in a command block.
    class OptionDesc
    {
        OptionType m_type;
        std::string m_name;
        std::string m_description;
        std::vector<std::string> m_domain;
        char m_alias = 0;
        std::any m_id;
        std::function<void(const std::string&)> m_valueBinding; // Callback for type-safe value binding

        friend class CommandBlockDesc;
        friend class CommandParser;
        friend class Option;

        // Private setter - only CommandBlockDesc should set aliases
        OptionDesc &SetAlias(char alias)
        {
            m_alias = alias;
            return *this;
        }

        // Check if this option has a value binding
        bool HasValueBinding() const { return static_cast<bool>(m_valueBinding); }

        // Apply the value binding (called internally by the parser)
        void ApplyValueBinding(const std::string& value) const
        {
            if (m_valueBinding)
            {
                m_valueBinding(value);
            }
        }

    public:
        // Constructors
        OptionDesc(OptionType type, const std::string &name) :
            m_type(type),
            m_name(name)
        {
        }

        // --- Unique ID ---
        template<typename T>
        void SetUniqueId(T id)
        {
            m_id = id;
        }

        template<typename T>
        T GetUniqueId() const
        {
            if (!HasUniqueId())
                throw(ApiException(ApiError::UniqueIdNotAssigned, "OptionDesc unique ID not assigned"));
            return std::any_cast<T>(m_id);
        }

        bool HasUniqueId() const
        {
            return m_id.has_value();
        }

        // --- Set Accessors ---
        OptionDesc &SetDescription(const std::string &description)
        {
            m_description = description;
            return *this;
        }

        OptionDesc &SetDomain(const std::vector<std::string> &domain)
        {
            m_domain = domain;
            return *this;
        }

        // --- Value Binding ---
        template<typename T, typename Converter = DefaultConverter<T>>
        OptionDesc& BindTo(T& variable, Converter converter = Converter{})
        {
            // Only Variable and Parameter types can have bound values
            if (m_type != OptionType::Variable && m_type != OptionType::Parameter)
            {
                throw ApiException(ApiError::InvalidOptionType, 
                    "Only Variable and Parameter options can bind values. Switch options are boolean flags.");
            }

            m_valueBinding = [&variable, converter](const std::string &str)
                {
                    try
                    {
                        variable = converter(str);
                    }
                    catch (const SyntaxException &)
                    {
                        throw; // Re-throw syntax exceptions as-is
                    }
                    catch (const std::exception &e)
                    {
                        throw SyntaxException(SyntaxError::InvalidValue, 
                            std::string("Conversion failed: ") + e.what(), str);
                    }
                };
            
            return *this;
        }

        // --- Get Accessors ---
        OptionType GetType() const { return m_type; }
        const std::string &GetName() const { return m_name; }
        const std::string &GetDescription() const { return m_description; }
        const std::vector<std::string> &GetDomain() const { return m_domain; }
        char GetAlias() const { return m_alias; }

        // Hash method
        size_t Hash() const
        {
            return std::hash<std::string>{}(m_name);
        }
    };

    //------------------------------------------------------------------------------------------------
    // Forward declaration
    class CommandParser;

    //------------------------------------------------------------------------------------------------
    // CommandBlockDesc
    // Describes a command block, including command block name and available options.
    class CommandBlockDesc
    {
        std::string m_name;
        std::string m_description;
        std::unordered_map<std::string, std::shared_ptr<OptionDesc>> m_optionDescs;
        std::vector<std::reference_wrapper<const OptionDesc>> m_parameterDescs;
        std::unordered_map<char, std::shared_ptr<OptionDesc>> m_aliasMap;
        std::unordered_map<std::string, std::shared_ptr<CommandBlockDesc>> m_innerCommandBlockDescs;
        std::any m_id;
        CommandParser* m_parser; // Back-reference to owning parser

        std::string SimpleUsageStringWithPath(const std::string& pathPrefix) const;
        std::string SimpleUsageStringWithPath(const std::string& pathPrefix, bool parentHasOptions) const;

        const OptionDesc *FindOption(const std::string &name) const
        {
            auto it = m_optionDescs.find(name);
            return it == m_optionDescs.end() ? nullptr : it->second.get();
        }
        const OptionDesc *FindOptionByAlias(const char alias) const
        {
            auto it = m_aliasMap.find(alias);
            return it == m_aliasMap.end() ? nullptr : it->second.get();
        }

        friend class CommandParser;

    public:
        CommandBlockDesc(const std::string &name, CommandParser *parser) :
            m_name(name), m_parser(parser) {}
        
        OptionDesc &DeclareOption(OptionType type, const std::string &name, char alias = 0);

        CommandBlockDesc &DeclareSubCommandBlock(const std::string &name);

        // --- Unique ID ---
        template<typename T>
        void SetUniqueId(T id)
        {
            m_id = id;
        }

        template<typename T>
        T GetUniqueId() const
        {
            if (!HasUniqueId())
                throw(ApiException(ApiError::UniqueIdNotAssigned, "CommandBlockDesc unique ID not assigned"));
            return std::any_cast<T>(m_id);
        }

        bool HasUniqueId() const
        {
            return m_id.has_value();
        }
        
        // --- Set Accessors ---
        CommandBlockDesc &SetDescription(const std::string &description)
        {
            m_description = description;
            return *this;
        }

        // --- Get Accessors ---
        const std::string &GetName() const { return m_name; }
        const std::string &GetDescription() const { return m_description; }
        
        // --- Usage String Helpers ---
        std::string SimpleUsageString() const;
        std::string OptionDetailsString() const;
        std::string GetHelpString() const;
        std::string GetHelpStringWithPath(const std::string& commandPath) const;
    };

    //------------------------------------------------------------------------------------------------
    class Option
    {
        const OptionDesc &m_desc;
        std::string m_value; // Used when OptionDesc type is Variable or Parameter

    public:
        // --- Constructors ---
        Option(const OptionDesc &desc) :
            m_desc(desc)
        {
            // Apply value binding if one exists
            if (desc.HasValueBinding())
            {
                desc.ApplyValueBinding("");
            }
        }

        Option(const OptionDesc &desc, const std::string &value) :
            m_desc(desc),
            m_value(value)
        {
            // Apply value binding if one exists
            if (desc.HasValueBinding())
            {
                desc.ApplyValueBinding(value);
            }
        }

        // --- Get Accessors ---
        const OptionDesc &GetDesc() const { return m_desc; }
        const std::string &GetValue() const { return m_value; }
    };

    //------------------------------------------------------------------------------------------------
    struct OptionHasher
    {
        size_t operator()(const Option &option) const
        {
            return option.GetDesc().Hash();
        }
    };
    //------------------------------------------------------------------------------------------------
    struct OptionEqual
    {
        bool operator()(const Option &a, const Option &b) const
        {
            return a.GetDesc().GetName() == b.GetDesc().GetName();
        }
    };

    //------------------------------------------------------------------------------------------------
    // Stores a command block parsed from a range of command arguments
    class CommandBlock
    {
        const CommandBlockDesc &m_desc;
        std::unordered_set<Option, OptionHasher, OptionEqual> m_optionSet;

        CommandBlock &SetOption(const Option &option);

        friend class CommandParser;

    public:
        CommandBlock(const class CommandBlockDesc &desc) : m_desc(desc) {}

        bool IsOptionSet(const std::string &name) const;
        const std::string &GetOptionValue(const std::string &name) const;
        const std::string &GetOptionValue(const std::string &name, const std::string &defaultValue) const;
        bool IsParameterSet(const std::string &name) const;
        const std::string &GetParameterValue(const std::string &name) const;
        const std::string &GetParameterValue(const std::string &name, const std::string &defaultValue) const;
        const CommandBlockDesc &GetDesc() const { return m_desc; }
    };

    //------------------------------------------------------------------------------------------------
    class CommandParser
    {
        CommandBlockDesc m_rootCommandBlockDesc;
        VariableDelimiter m_variableDelimiter;
        std::vector<CommandBlock> m_commandBlocks;
        std::unordered_map<std::string, std::shared_ptr<OptionDesc>> m_globalOptionDescs;
        std::unordered_map<char, std::shared_ptr<OptionDesc>> m_globalAliasMap;

        // Parsed global options: option -> CommandBlock where set
        std::unordered_map<Option, size_t, OptionHasher, OptionEqual> m_parsedGlobalOptions;
           
        // Global/Local option registry
        enum class OptionScope { Global, Local };
        std::unordered_map<std::string, OptionScope> m_optionRegistry;
        std::unordered_map<char, OptionScope> m_aliasRegistry;
     
        char GetDelimiterChar() const;

        // Private registry methods
        void RegisterLocalOption(const std::string& name, char alias = 0);
        void RegisterGlobalOption(const std::string& name, char alias = 0);

        const OptionDesc *FindGlobalOption(const std::string &name) const
        {
            auto it = m_globalOptionDescs.find(name);
            return it == m_globalOptionDescs.end() ? nullptr : it->second.get();
        }
        const OptionDesc *FindGlobalOptionByAlias(const char alias) const
        {
            auto it = m_globalAliasMap.find(alias);
            return it == m_globalAliasMap.end() ? nullptr : it->second.get();
        }

    public:
        CommandParser(const std::string &appName, VariableDelimiter delimiter = VariableDelimiter::Whitespace) :
            m_rootCommandBlockDesc(appName, this),
            m_variableDelimiter(delimiter)
        {
        }

        OptionDesc& DeclareGlobalOption(OptionType type, const std::string& name, char alias = 0);

        CommandBlockDesc& GetRootCommandBlockDesc() { return m_rootCommandBlockDesc; }

        const CommandBlock &ParseArgs(int argc, const char *argv[]);

        size_t GetNumCommandBlocks() const { return m_commandBlocks.size(); }
        const CommandBlock& GetCommandBlock(size_t index) const
        {
            if (index >= m_commandBlocks.size())
                throw ApiException(ApiError::OutOfRange, "Parsed command block index out of range");
            return m_commandBlocks[index];
        }

        // Global option accessors
        bool IsGlobalOptionSet(const std::string& name) const;
        const std::string& GetGlobalOptionValue(const std::string& name) const;
        size_t GetGlobalOptionContext(const std::string& name) const;
        
        std::string SimpleUsageString() const { return m_rootCommandBlockDesc.SimpleUsageString(); }
        std::string OptionDetailsString() const { return m_rootCommandBlockDesc.OptionDetailsString(); }
        std::string GetHelpString() const { return m_rootCommandBlockDesc.GetHelpString(); }
    };
}