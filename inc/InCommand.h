//------------------------------------------------------------------------------------------------
// InCommand
// Copyright (c) 2025 wjkristiansen@hotmail.com
// Licensed under the MIT License. See LICENSE for details.
//------------------------------------------------------------------------------------------------

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
#include <iostream>

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
        InvalidUniqueIdType,
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
    // OptionDecl
    // Describes an option used in a command block.
    class OptionDecl
    {
        OptionType m_type;
        std::string m_name;
        std::string m_description;
        std::vector<std::string> m_domain;
        char m_alias = 0;
        std::any m_id;
        std::function<void(const std::string&)> m_valueBinding; // Callback for type-safe value binding

        friend class CommandDecl;
        friend class CommandParser;
        friend class CommandBlock;

        // Private setter - only CommandDecl should set aliases
        OptionDecl &SetAlias(char alias)
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
        OptionDecl(OptionType type, const std::string &name) :
            m_type(type),
            m_name(name)
        {
        }

        // --- Set Accessors ---
        OptionDecl &SetDescription(const std::string &description)
        {
            m_description = description;
            return *this;
        }

        OptionDecl &SetDomain(const std::vector<std::string> &domain)
        {
            m_domain = domain;
            return *this;
        }

        // --- Value Binding ---
        template<typename T, typename Converter = DefaultConverter<T>>
        OptionDecl& BindTo(T& variable, Converter converter = Converter{})
        {
            // Switch binding specialization (only valid when T is bool)
            if (m_type == OptionType::Switch)
            {
                if constexpr (std::is_same_v<T, bool>)
                {
                    m_valueBinding = [&variable](const std::string & /*str*/)
                        {
                            variable = true; // presence sets to true
                        };
                    return *this;
                }
                else
                {
                    throw ApiException(ApiError::InvalidOptionType,
                        "Switch options can only bind to bool variables.");
                }
            }

            // Variable / Parameter binding (all other supported types)
            if (m_type == OptionType::Variable || m_type == OptionType::Parameter)
            {
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

            throw ApiException(ApiError::InvalidOptionType,
                "Only Variable, Parameter, or Switch(bool) options can bind values.");
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
    // CommandDecl
    // Describes a command block, including command block name and available options.
    class CommandDecl
    {
        std::string m_name;
        std::string m_description;
        std::unordered_map<std::string, std::shared_ptr<OptionDecl>> m_optionDecls;
        std::vector<std::reference_wrapper<const OptionDecl>> m_parameterDecls;
        std::unordered_map<char, std::shared_ptr<OptionDecl>> m_aliasMap;
        std::unordered_map<std::string, std::shared_ptr<CommandDecl>> m_innerCommandDecls;
        std::any m_id;
        CommandParser* m_parser; // Back-reference to owning parser

        const OptionDecl *FindOption(const std::string &name) const
        {
            auto it = m_optionDecls.find(name);
            return it == m_optionDecls.end() ? nullptr : it->second.get();
        }
        const OptionDecl *FindOptionByAlias(const char alias) const
        {
            auto it = m_aliasMap.find(alias);
            return it == m_aliasMap.end() ? nullptr : it->second.get();
        }

        friend class CommandParser;

    public:
        CommandDecl(const std::string &name, CommandParser *parser) :
            m_name(name), m_parser(parser) {}
        
        OptionDecl &AddOption(OptionType type, const std::string &name, char alias = 0);

        CommandDecl &AddSubCommand(const std::string &name);

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
            {
                throw(ApiException(ApiError::UniqueIdNotAssigned, "CommandDecl unique ID not assigned"));
            }

            try
            {
                return std::any_cast<T>(m_id); // throw(std::bad_any_cast)
            }
            catch(const std::bad_any_cast& )
            {
                throw(ApiException(ApiError::InvalidUniqueIdType, "Invalid unique ID type"));
            }
        }

        bool HasUniqueId() const
        {
            return m_id.has_value();
        }
        
        // --- Set Accessors ---
        CommandDecl &SetDescription(const std::string &description)
        {
            m_description = description;
            return *this;
        }

        // --- Get Accessors ---
        const std::string &GetName() const { return m_name; }
        const std::string &GetDescription() const { return m_description; }
    };

    //------------------------------------------------------------------------------------------------
    // Stores a command block parsed from a range of command arguments
    class CommandBlock
    {
        const CommandDecl &m_decl;
        std::unordered_map<std::string, std::string> m_optionMap;

        CommandBlock &SetOption(const OptionDecl &option, const std::string &value = "");

        friend class CommandParser;

    public:
        CommandBlock(const class CommandDecl &decl) : m_decl(decl) {}

        bool IsOptionSet(const std::string &name) const;
        const std::string &GetOptionValue(const std::string &name) const;
        const std::string &GetOptionValue(const std::string &name, const std::string &defaultValue) const;
        const CommandDecl &GetDecl() const { return m_decl; }
    };

    //------------------------------------------------------------------------------------------------
    class CommandParser
    {
        CommandDecl m_rootCommandBlockDesc;
        VariableDelimiter m_variableDelimiter;
        std::vector<CommandBlock> m_commandBlocks;
        std::unordered_map<std::string, std::shared_ptr<OptionDecl>> m_globalOptionDecls;
        std::unordered_map<char, std::shared_ptr<OptionDecl>> m_globalAliasMap;

        // Parsed global options: option -> CommandBlock where set
        std::unordered_map<std::string, std::pair<std::string, size_t>> m_parsedGlobalOptions;
           
        // Global/Local option registry
        enum class OptionScope { Global, Local };
        std::unordered_map<std::string, OptionScope> m_optionRegistry;
        std::unordered_map<char, OptionScope> m_aliasRegistry;

        // Auto-help configuration
        bool m_autoHelpEnabled = false;
        std::string m_autoHelpOptionName;
        char m_autoHelpAlias = 0;
        std::string m_autoHelpDescription;
        std::ostream* m_autoHelpOutputStream;
        
        // Auto-help state
        bool m_autoHelpRequested = false;
     
        char GetDelimiterChar() const;

        // Private help generation methods
        std::string GenerateHelpWithGlobalOptions(const CommandDecl& cmdDesc, const std::string& commandPath) const;

        // Private registry methods
        void RegisterLocalOption(const std::string& name, char alias = 0);
        void RegisterGlobalOption(const std::string& name, char alias = 0);

        const OptionDecl *FindGlobalOption(const std::string &name) const
        {
            auto it = m_globalOptionDecls.find(name);
            return it == m_globalOptionDecls.end() ? nullptr : it->second.get();
        }
        const OptionDecl *FindGlobalOptionByAlias(const char alias) const
        {
            auto it = m_globalAliasMap.find(alias);
            return it == m_globalAliasMap.end() ? nullptr : it->second.get();
        }

        friend class CommandDecl;
        
    public:
        CommandParser(const std::string &appName, VariableDelimiter delimiter = VariableDelimiter::Whitespace) :
            m_rootCommandBlockDesc(appName, this),
            m_variableDelimiter(delimiter),
            m_autoHelpOptionName(""),
            m_autoHelpDescription("Show context-sensitive help information"),
            m_autoHelpOutputStream(nullptr)
        {
        }

        OptionDecl& AddGlobalOption(OptionType type, const std::string& name, char alias = 0);

        CommandDecl& GetAppCommandDecl() { return m_rootCommandBlockDesc; }
        const CommandDecl& GetAppCommandDecl() const { return m_rootCommandBlockDesc; }

        // Auto-help configuration methods
        void EnableAutoHelp(const std::string& optionName, char alias, std::ostream& outputStream = std::cout);
        void SetAutoHelpDescription(const std::string& description);
        void DisableAutoHelp();
        
        // Auto-help status methods
        bool WasAutoHelpRequested() const { return m_autoHelpRequested; }

        size_t ParseArgs(int argc, const char *argv[]);

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
        size_t GetGlobalOptionBlockIndex(const std::string& name) const;
        
        // Help string generation - targets rightmost parsed command block
        std::string GetHelpString() const;
        std::string GetHelpString(size_t commandBlockIndex) const;
    };
}