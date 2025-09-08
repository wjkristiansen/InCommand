#include <sstream>
#include <iomanip>
#include <stack>
#include <assert.h>

#include "InCommand.h"

#include <stdexcept>

namespace InCommand
{

//------------------------------------------------------------------------------------------------
CommandBlockDesc &CommandBlockDesc::DeclareSubCommandBlock(const std::string &name)
{
    try
    {
        auto [it, inserted] = m_innerCommandBlockDescs.emplace(name, std::make_shared<CommandBlockDesc>(name, m_parser));
        if (!inserted)
        {
            throw ApiException(ApiError::DuplicateCommandBlock, "Command block '" + name + "' already exists");
        }
        
        return *(it->second.get());
    }
    catch (const std::bad_alloc &)
    {
        throw ApiException(ApiError::OutOfMemory, "Out of memory creating command block");
    }
}

//------------------------------------------------------------------------------------------------
void CommandParser::RegisterLocalOption(const std::string& name, char alias)
{
    // Check for existing global declarations
    auto it = m_optionRegistry.find(name);
    if (it != m_optionRegistry.end() && it->second == OptionScope::Global)
    {
        throw ApiException(ApiError::DuplicateOption, "Local option '" + name + "' conflicts with global option");
    }
    if (alias)
    {
        auto aliasIt = m_aliasRegistry.find(alias);
        if (aliasIt != m_aliasRegistry.end() && aliasIt->second == OptionScope::Global)
        {
            throw ApiException(ApiError::DuplicateOption, "Local option alias '" + std::string(1, alias) + "' conflicts with global option");
        }
    }
    
    // Register as local (only if not already registered as local)
    if (m_optionRegistry.find(name) == m_optionRegistry.end())
    {
        m_optionRegistry[name] = OptionScope::Local;
    }
    if (alias && m_aliasRegistry.find(alias) == m_aliasRegistry.end())
    {
        m_aliasRegistry[alias] = OptionScope::Local;
    }
}

//------------------------------------------------------------------------------------------------
void CommandParser::RegisterGlobalOption(const std::string& name, char alias)
{
    // Check for existing local declarations
    auto it = m_optionRegistry.find(name);
    if (it != m_optionRegistry.end() && it->second == OptionScope::Local)
    {
        throw ApiException(ApiError::DuplicateOption, "Global option '" + name + "' conflicts with local option");
    }
    if (alias)
    {
        auto aliasIt = m_aliasRegistry.find(alias);
        if (aliasIt != m_aliasRegistry.end() && aliasIt->second == OptionScope::Local)
        {
            throw ApiException(ApiError::DuplicateOption, "Global option alias '" + std::string(1, alias) + "' conflicts with local option");
        }
    }
    
    // Register as global
    m_optionRegistry[name] = OptionScope::Global;
    if (alias)
    {
        m_aliasRegistry[alias] = OptionScope::Global;
    }
}

//------------------------------------------------------------------------------------------------
OptionDesc& CommandParser::DeclareGlobalOption(OptionType type, const std::string& name, char alias)
{
    // Parameters cannot be global
    if (type == OptionType::Parameter)
    {
        throw ApiException(ApiError::InvalidOptionType, "Parameters cannot be global options");
    }

    try
    {
        // Store global option in root command block
        auto [it, inserted] = m_globalOptionDescs.emplace(name, std::make_shared<OptionDesc>(type, name));
        if (!inserted)
        {
            throw ApiException(ApiError::DuplicateOption, "Global option '" + name + "' already exists");
        }

        if (alias != 0)
        {
            // Set the alias and register it
            it->second->SetAlias(alias);
            m_globalAliasMap[alias] = it->second;
        }

        return *(it->second.get());
    }
    catch (const std::bad_alloc &)
    {
        throw ApiException(ApiError::OutOfMemory, "Out of memory creating global option");
    }
}

//------------------------------------------------------------------------------------------------
OptionDesc& CommandBlockDesc::DeclareOption(OptionType type, const std::string& name, char alias)
{
    if (alias != 0)
    {
        // Parameters cannot have aliases - they are positional arguments
        if (type == OptionType::Parameter)
        {
            throw ApiException(ApiError::InvalidOptionType, "Parameters cannot have aliases - use DeclareOption(type, name) instead");
        }
    }

    // Check for duplicate alias first
    if (m_aliasMap.find(alias) != m_aliasMap.end())
    {
        throw ApiException(ApiError::DuplicateOption, "Alias '" + std::string(1, alias) + "' already exists");
    }

    try
    {
        // All option types go into m_optionDescs for unified storage
        auto [it, inserted] = m_optionDescs.emplace(name, std::make_shared<OptionDesc>(type, name));
        if (!inserted)
        {
            throw ApiException(ApiError::DuplicateOption, "Option '" + name + "' already exists");
        }

        if (type == OptionType::Parameter)
        {
            // Parameters also get added to the ordered parameter list
            m_parameterDescs.emplace_back(std::cref(*(it->second)));
        }
        else if (alias != 0)
        {
            // Only Switches and Variables can have aliases
            it->second->SetAlias(alias);
            m_aliasMap[alias] = it->second;
        }

        return *(it->second.get());
    }
    catch (const std::bad_alloc &)
    {
        throw ApiException(ApiError::OutOfMemory);
    }
}

//------------------------------------------------------------------------------------------------
std::string CommandBlockDesc::SimpleUsageString() const
{
    return SimpleUsageStringWithPath(GetName(), false);
}

//------------------------------------------------------------------------------------------------
std::string CommandBlockDesc::SimpleUsageStringWithPath(const std::string& pathPrefix) const
{
    return SimpleUsageStringWithPath(pathPrefix, false);
}

//------------------------------------------------------------------------------------------------
std::string CommandBlockDesc::SimpleUsageStringWithPath(const std::string& pathPrefix, bool parentHasOptions) const
{
    std::ostringstream s;

    // Check if this command block has non-parameter options (switches or variables)
    bool thisHasOptions = !m_optionDescs.empty();

    // Recursively generate usage for all inner command blocks with full path
    for (const auto &innerPair : m_innerCommandBlockDescs)
    {
        // Build the new path with [options] inserted after each command that has options
        std::string newPath = pathPrefix;
        if (thisHasOptions)
        {
            newPath += " [options]";
        }
        newPath += " " + innerPair.first;
        
        s << innerPair.second->SimpleUsageStringWithPath(newPath, false);
    }

    // Build usage line for this command block
    s << pathPrefix;
    
    // Add [options] if parent has options (this is for when we're showing this command's own usage)
    if (parentHasOptions)
    {
        s << " [options]";
    }
    
    s << " ";

    // Show specific options for this command block
    // Options (switches and variables only - parameters are positional, not named)
    for (const auto &optionPair : m_optionDescs)
    {
        const OptionDesc &desc = *optionPair.second;
        s << "[--" << desc.GetName();
        if (desc.GetType() == OptionType::Variable)
            s << " <value>";
        s << "] ";
    }

    // Parameters (positional arguments) in declaration order
    for (const auto &paramDesc : m_parameterDescs)
    {
        s << "<" << paramDesc.get().GetName() << "> ";
    }

    // Inner command blocks (only show if this is a leaf node with subcommands)
    if (!m_innerCommandBlockDescs.empty())
    {
        for (const auto &innerPair : m_innerCommandBlockDescs)
        {
            s << "[" << innerPair.first << "] ";
        }
    }

    s << std::endl;

    return s.str();
}

//------------------------------------------------------------------------------------------------
std::string CommandBlockDesc::OptionDetailsString() const
{
    std::ostringstream s;
    static const int colwidth = 30;

    // List all non-parameter options with descriptions
    for (const auto &optionPair : m_optionDescs)
    {
        const OptionDesc &desc = *optionPair.second;
        if (!desc.GetDescription().empty())
        {
            std::string optionName = "  --" + desc.GetName();
            if (desc.GetAlias() != 0)
            {
                optionName += ", -" + std::string(1, desc.GetAlias());
            }
            s << std::setw(colwidth) << std::left << optionName;
            if (optionName.length() > colwidth)
            {
                s << std::endl;
                s << std::setw(colwidth) << ' ';
            }
            s << desc.GetDescription() << std::endl;
        }
    }

    // List all parameters in order
    for (const auto &paramDesc : m_parameterDescs)
    {
        if (!paramDesc.get().GetDescription().empty())
        {
            std::string paramName = "  <" + paramDesc.get().GetName() + ">";
            s << std::setw(colwidth) << std::left << paramName;
            if (paramName.length() > colwidth)
            {
                s << std::endl;
                s << std::setw(colwidth) << ' ';
            }
            s << paramDesc.get().GetDescription() << std::endl;
        }
    }

    // List inner command blocks
    for (const auto &innerPair : m_innerCommandBlockDescs)
    {
        const CommandBlockDesc &innerDesc = *innerPair.second;
        if (!innerDesc.GetDescription().empty())
        {
            std::string commandName = "  " + innerDesc.GetName();
            s << std::setw(colwidth) << std::left << commandName;
            if (commandName.length() > colwidth)
            {
                s << std::endl;
                s << std::setw(colwidth) << ' ';
            }
            s << innerDesc.GetDescription() << std::endl;
        }
    }

    s << std::endl;

    return s.str();
}

//------------------------------------------------------------------------------------------------
std::string CommandBlockDesc::GetHelpString() const
{
    std::ostringstream s;

    // Start with description if available
    if (!m_description.empty())
    {
        s << m_description << std::endl << std::endl;
    }

    // Add usage information
    s << "Usage:" << std::endl << SimpleUsageString();

    // Add option details
    std::string details = OptionDetailsString();
    if (!details.empty())
    {
        s << details;
    }

    return s.str();
}

//------------------------------------------------------------------------------------------------
std::string CommandBlockDesc::GetHelpStringWithPath(const std::string& commandPath) const
{
    std::ostringstream s;

    // Start with description if available
    if (!m_description.empty())
    {
        s << m_description << std::endl << std::endl;
    }

    // Parse the command path and rebuild it with proper [options] placement
    // For now, we'll assume the parent commands have options and insert [options] after the first command
    std::istringstream pathStream(commandPath);
    std::string firstCommand, remainingPath;
    pathStream >> firstCommand;
    std::getline(pathStream, remainingPath);
    
    // Add usage information with the command path, inserting [options] after the first command
    s << "Usage:" << std::endl << firstCommand << " [options]" << remainingPath << " ";

    // Show specific options for this command block
    // Options (switches and variables only - parameters are positional, not named)
    for (const auto &optionPair : m_optionDescs)
    {
        const OptionDesc &desc = *optionPair.second;
        s << "[--" << desc.GetName();
        if (desc.GetType() == OptionType::Variable)
            s << " <value>";
        s << "] ";
    }

    // Parameters (positional arguments) in declaration order
    for (const auto &paramDesc : m_parameterDescs)
    {
        s << "<" << paramDesc.get().GetName() << "> ";
    }

    s << std::endl;

    // Add option details
    std::string details = OptionDetailsString();
    if (!details.empty())
    {
        s << details;
    }

    return s.str();
}

//------------------------------------------------------------------------------------------------
CommandBlock &CommandBlock::SetOption(const Option &option)
{
    m_optionSet.insert(option);
    
    return *this;
}

//------------------------------------------------------------------------------------------------
bool CommandBlock::IsOptionSet(const std::string &name) const
{
    auto it = m_optionSet.find(Option(OptionDesc(OptionType::Undefined, name)));
    return it != m_optionSet.end();
}

//------------------------------------------------------------------------------------------------
const std::string &CommandBlock::GetOptionValue(const std::string &name) const
{
    auto it = m_optionSet.find(Option(OptionDesc(OptionType::Undefined, name)));
    if (it != m_optionSet.end())
    {
        return it->GetValue();
    }
    throw ApiException(ApiError::OptionNotFound, "Option '" + name + "' not set");
}

//------------------------------------------------------------------------------------------------
const std::string &CommandBlock::GetOptionValue(const std::string &name, const std::string &defaultValue) const
{
    auto it = m_optionSet.find(Option(OptionDesc(OptionType::Undefined, name)));
    if (it != m_optionSet.end())
    {
        return it->GetValue();
    }
    return defaultValue;
}

//------------------------------------------------------------------------------------------------
bool CommandBlock::IsParameterSet(const std::string &name) const
{
    auto it = m_optionSet.find(Option(OptionDesc(OptionType::Undefined, name)));
    return it != m_optionSet.end();
}

//------------------------------------------------------------------------------------------------
const std::string &CommandBlock::GetParameterValue(const std::string &name) const
{
    auto it = m_optionSet.find(Option(OptionDesc(OptionType::Undefined, name)));
    if (it != m_optionSet.end())
    {
        return it->GetValue();
    }
    throw ApiException(ApiError::ParameterNotFound, "Parameter '" + name + "' not found");
}

//------------------------------------------------------------------------------------------------
const std::string &CommandBlock::GetParameterValue(const std::string &name, const std::string &defaultValue) const
{
    auto it = m_optionSet.find(Option(OptionDesc(OptionType::Undefined, name)));
    if (it != m_optionSet.end())
    {
        return it->GetValue();
    }
    return defaultValue;
}

//------------------------------------------------------------------------------------------------
char CommandParser::GetDelimiterChar() const
{
    switch (m_variableDelimiter)
    {
        case VariableDelimiter::Equals: return '=';
        case VariableDelimiter::Colon: return ':';
        case VariableDelimiter::Whitespace:
        default: return '\0';  // No delimiter - whitespace-separated tokens
    }
}

//------------------------------------------------------------------------------------------------
// Parses command arguments and returns the innermost command block
const CommandBlock &CommandParser::ParseArgs(int argc, const char *argv[])
{
    m_commandBlocks.emplace_back(m_rootCommandBlockDesc);
    size_t currentParameterIndex = 0; // Track which parameter we're expecting next

    for (int i = 1; i < argc; ++i)
    {
        CommandBlock &currentBlock = m_commandBlocks.back();
        const CommandBlockDesc &currentDesc = currentBlock.GetDesc();
        std::string token = argv[i];

        // Check for long option (--name)
        if (token.rfind("--", 0) == 0)
        {
            std::string name = token.substr(2);
            std::string value;
            
            // Check if the option contains a delimiter (--name=value when delimiter is configured)
            size_t delimiterPos = std::string::npos;
            char delimiterChar = GetDelimiterChar();
            
            if (delimiterChar != '\0')
            {
                delimiterPos = name.find(delimiterChar);
            }
            
            if (delimiterPos != std::string::npos)
            {
                value = name.substr(delimiterPos + 1);
                name = name.substr(0, delimiterPos);
            }

            bool isGlobalOption = false;
            const OptionDesc *optionDesc = FindGlobalOption(name);

            if (optionDesc)
            {
                isGlobalOption = true;
            }
            else
            {
                optionDesc = currentDesc.FindOption(name);
            }

            if (optionDesc)
            {
                if (optionDesc->GetType() == OptionType::Switch)
                {
                    if (delimiterPos != std::string::npos)
                    {
                        throw SyntaxException(SyntaxError::InvalidValue, "Switch options cannot have values. Option --" + name + " is a switch", "--" + name + delimiterChar + value);
                    }

                    if (isGlobalOption)
                    {
                        m_parsedGlobalOptions.insert(std::pair(Option(*optionDesc), m_commandBlocks.size() - 1));
                    }
                    else
                    {
                        currentBlock.m_optionSet.insert(Option(*optionDesc));
                    }
                }
                else if (optionDesc->GetType() == OptionType::Variable)
                {
                    if (delimiterPos != std::string::npos)
                    {
                        if (isGlobalOption)
                        {
                            m_parsedGlobalOptions.insert(std::pair(Option(*optionDesc, value), m_commandBlocks.size() - 1));
                        }
                        else
                        {
                            currentBlock.m_optionSet.insert(Option(*optionDesc, value));
                        }
                    }
                    else
                    {
                        // Value should be in next argument (traditional format)
                        if (i + 1 >= argc)
                        {
                            throw SyntaxException(SyntaxError::MissingVariableValue, "Missing value for option --" + name, "--" + name);
                        }
                        
                        // Check if the next argument looks like an option (starts with -)
                        std::string nextArg = argv[++i];
                        if (nextArg.rfind("-", 0) == 0)
                        {
                            throw SyntaxException(SyntaxError::MissingVariableValue, "Missing value for option --" + name, "--" + name);
                        }
                        
                        if (isGlobalOption)
                        {
                            m_parsedGlobalOptions.insert(std::pair(Option(*optionDesc, nextArg), m_commandBlocks.size() - 1));
                        }
                        else
                        {
                            currentBlock.m_optionSet.insert(Option(*optionDesc, nextArg));
                        }
                    }
                }
                continue;
            }
            
            // Unknown option
            throw SyntaxException(SyntaxError::UnknownOption, "Unknown option --" + name, "--" + name);
        }

        // Check for short option(s) (-x or -xyz for grouped switches)
        if (token.rfind("-", 0) == 0 && token.length() > 1 && token[1] != '-')
        {
            std::string aliases = token.substr(1);
            
            // Check for single character with possible delimiter and value (-o=value or -o:value)
            char firstAlias = aliases[0];

            bool isGlobalOption = false;
            const OptionDesc *firstDesc = FindGlobalOptionByAlias(firstAlias);

            if (firstDesc)
            {
                isGlobalOption = true;
            }
            else
            {
                firstDesc = currentDesc.FindOptionByAlias(firstAlias);
            }
            
            if (firstDesc != nullptr && aliases.length() > 1)
            {
                // Check if second character is a delimiter
                char possibleDelimiter = aliases[1];
                char delimiterChar = GetDelimiterChar();
                if (delimiterChar != '\0' && possibleDelimiter == delimiterChar)
                {
                    if (firstDesc->GetType() == OptionType::Switch)
                    {
                        throw SyntaxException(SyntaxError::InvalidValue, "Switch options cannot have values. Option -" + std::string(1, firstAlias) + " is a switch", "-" + std::string(1, firstAlias) + possibleDelimiter + aliases.substr(2));
                    }
                    else if (firstDesc->GetType() == OptionType::Variable)
                    {
                        std::string value = aliases.substr(2);
                        if (isGlobalOption)
                        {
                            m_parsedGlobalOptions.insert(std::pair(Option(*firstDesc, value), m_commandBlocks.size() - 1));
                        }
                        else
                        {
                            currentBlock.m_optionSet.insert(Option(*firstDesc, value));
                        }
                        continue;
                    }
                }
            }
            
            // Single character alias without delimiter (could be switch or variable)
            if (aliases.length() == 1)
            {
                char alias = aliases[0];
                bool isGlobalOptionAlias = false;
                const OptionDesc *optionDesc = FindGlobalOptionByAlias(alias);
                if (optionDesc)
                {
                    isGlobalOptionAlias = true;
                }
                else
                {
                    optionDesc = currentDesc.FindOptionByAlias(alias);
                }
                
                if (optionDesc == nullptr)
                {
                    throw SyntaxException(SyntaxError::UnknownOption, "Unknown option -" + std::string(1, alias), "-" + std::string(1, alias));
                }
                
                if (optionDesc->GetType() == OptionType::Switch)
                {
                    if (isGlobalOptionAlias)
                    {
                        m_parsedGlobalOptions.insert(std::pair(Option(*optionDesc), m_commandBlocks.size() - 1));
                    }
                    else
                    {
                        currentBlock.m_optionSet.insert(Option(*optionDesc));
                    }
                }
                else if (optionDesc->GetType() == OptionType::Variable)
                {
                    if (i + 1 >= argc)
                    {
                        throw SyntaxException(SyntaxError::MissingVariableValue, "Missing value for option -" + std::string(1, alias), "-" + std::string(1, alias));
                    }
                    
                    // Check if the next argument looks like an option (starts with -)
                    std::string nextArg = argv[++i];
                    if (nextArg.rfind("-", 0) == 0)
                    {
                        throw SyntaxException(SyntaxError::MissingVariableValue, "Missing value for option -" + std::string(1, alias), "-" + std::string(1, alias));
                    }

                    if (isGlobalOptionAlias)
                    {
                        m_parsedGlobalOptions.insert(std::pair(Option(*optionDesc, nextArg), m_commandBlocks.size() - 1));
                    }
                    else
                    {
                        currentBlock.m_optionSet.insert(Option(*optionDesc, nextArg));
                    }
                }
                continue;
            }
            
            // Multiple characters - only switches can be grouped
            for (char alias : aliases)
            {
                bool isGlobalOptionAlias = false;
                const OptionDesc *optionDesc = FindGlobalOptionByAlias(alias);
                if (optionDesc)
                {
                    isGlobalOptionAlias = true;
                }
                else
                {
                    optionDesc = currentDesc.FindOptionByAlias(alias);
                }
                if (optionDesc == nullptr)
                {
                    throw SyntaxException(SyntaxError::UnknownOption, "Unknown option -" + std::string(1, alias), "-" + std::string(1, alias));
                }
                
                if (optionDesc->GetType() != OptionType::Switch)
                {
                    throw SyntaxException(SyntaxError::InvalidAlias, "Only switch options can be grouped. Option -" + std::string(1, alias) + " is not a switch", "-" + std::string(1, alias));
                }

                if (isGlobalOptionAlias)
                {
                    m_parsedGlobalOptions.insert(std::pair(Option(*optionDesc), m_commandBlocks.size() - 1));
                }
                else
                {
                    currentBlock.m_optionSet.insert(Option(*optionDesc));
                }
            }
            continue;
        }

        // Check for inner command block
        auto innerIt = currentDesc.m_innerCommandBlockDescs.find(token);
        if (innerIt != currentDesc.m_innerCommandBlockDescs.end())
        {
            m_commandBlocks.emplace_back(*innerIt->second.get());
            currentParameterIndex = 0; // Reset parameter index for new command block
            continue;
        }

        // Otherwise treat as positional parameter
        if (currentParameterIndex < currentDesc.m_parameterDescs.size())
        {
            const OptionDesc &desc = currentDesc.m_parameterDescs[currentParameterIndex].get();
            currentBlock.SetOption(Option(desc, token));
            currentParameterIndex++;
            continue;
        }
        
        // If we get here, it's an unexpected argument
        throw SyntaxException(SyntaxError::UnexpectedArgument, "Unexpected argument: " + token, token);
    }

    // Return the last command block that was actually parsed
    return m_commandBlocks.back();
}

//------------------------------------------------------------------------------------------------
bool CommandParser::IsGlobalOptionSet(const std::string& name) const
{
    // Search through all parsed global options to find one with matching name
    for (const auto &[option, commandBlock] : m_parsedGlobalOptions)
    {
        if (option.GetDesc().GetName() == name)
        {
            return true;
        }
    }
    return false;
}

//------------------------------------------------------------------------------------------------
const std::string& CommandParser::GetGlobalOptionValue(const std::string& name) const
{
    // Search through all parsed global options to find one with matching name
    for (const auto &[option, commandBlock] : m_parsedGlobalOptions)
    {
        if (option.GetDesc().GetName() == name)
        {
            return option.GetValue();
        }
    }
    throw ApiException(ApiError::OptionNotFound, "Global option '" + name + "' not set");
}

//------------------------------------------------------------------------------------------------
size_t CommandParser::GetGlobalOptionContext(const std::string& name) const
{
    // Search through all parsed global options to find one with matching name
    for (const auto& [option, contextIndex] : m_parsedGlobalOptions)
    {
        if (option.GetDesc().GetName() == name)
        {
            return contextIndex;
        }
    }
    throw ApiException(ApiError::OptionNotFound, "Global option '" + name + "' not set");
}

} // namespace InCommand
