#include <sstream>
#include <iomanip>
#include <stack>
#include <assert.h>
#include <iostream>
#include <cstdlib>

#include "InCommand.h"

#include <stdexcept>

namespace InCommand
{

//------------------------------------------------------------------------------------------------
CommandDecl &CommandDecl::AddSubCommand(const std::string &name)
{
    try
    {
        auto [it, inserted] = m_innerCommandDecls.emplace(name, std::make_shared<CommandDecl>(name, m_parser));
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
void CommandParser::EnableAutoHelp(const std::string& optionName, char alias, std::ostream& outputStream)
{
    // Check for conflicts with existing global options
    auto it = m_globalOptionDecls.find(optionName);
    if (it != m_globalOptionDecls.end())
    {
        throw ApiException(ApiError::DuplicateOption, "Auto-help option '" + optionName + "' conflicts with existing global option");
    }
    
    // Check for conflicts with existing alias
    if (alias)
    {
        for (const auto& pair : m_globalOptionDecls)
        {
            if (pair.second->GetAlias() == alias)
            {
                throw ApiException(ApiError::DuplicateOption, "Auto-help alias '" + std::string(1, alias) + "' conflicts with existing global option");
            }
        }
    }
    
    m_autoHelpOptionName = optionName;
    m_autoHelpAlias = alias;
    m_autoHelpOutputStream = &outputStream;
    m_autoHelpEnabled = true;
}

//------------------------------------------------------------------------------------------------
void CommandParser::SetAutoHelpDescription(const std::string& description)
{
    m_autoHelpDescription = description;
}

//------------------------------------------------------------------------------------------------
void CommandParser::DisableAutoHelp()
{
    m_autoHelpEnabled = false;
    m_autoHelpOutputStream = nullptr;
}

//------------------------------------------------------------------------------------------------
OptionDecl& CommandParser::AddGlobalOption(OptionType type, const std::string& name, char alias)
{
    // Parameters cannot be global
    if (type == OptionType::Parameter)
    {
        throw ApiException(ApiError::InvalidOptionType, "Parameters cannot be global options");
    }

    try
    {
        // Register the global option first to check for conflicts
        RegisterGlobalOption(name, alias);
        
        // If this conflicts with auto-help, disable auto-help
        if (m_autoHelpEnabled && 
            (name == m_autoHelpOptionName || (alias != 0 && alias == m_autoHelpAlias)))
        {
            m_autoHelpEnabled = false;
        }
        
        // Store global option in root command block
        auto [it, inserted] = m_globalOptionDecls.emplace(name, std::make_shared<OptionDecl>(type, name));
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
OptionDecl& CommandDecl::AddOption(OptionType type, const std::string& name, char alias)
{
    if (alias != 0)
    {
        // Parameters cannot have aliases - they are positional arguments
        if (type == OptionType::Parameter)
        {
            throw ApiException(ApiError::InvalidOptionType, "Parameters cannot have aliases - use AddOption(type, name) instead");
        }
    }

    // Check for duplicate alias first
    if (m_aliasMap.find(alias) != m_aliasMap.end())
    {
        throw ApiException(ApiError::DuplicateOption, "Alias '" + std::string(1, alias) + "' already exists");
    }

    try
    {
        // Register the local option to check for conflicts with global options
        m_parser->RegisterLocalOption(name, alias);
        
        // All option types go into m_optionDescs for unified storage
        auto [it, inserted] = m_optionDescs.emplace(name, std::make_shared<OptionDecl>(type, name));
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
std::string CommandParser::GetHelpString() const
{
    if (m_commandBlocks.empty())
    {
        throw ApiException(ApiError::OutOfRange, "No command blocks have been parsed yet. Call ParseArgs() first or use GetHelpString(size_t) with a specific index.");
    }
    // Target the rightmost (last) parsed command block
    return GetHelpString(m_commandBlocks.size() - 1);
}

//------------------------------------------------------------------------------------------------
std::string CommandParser::GetHelpString(size_t commandBlockIndex) const
{
    // Require that parsing has occurred first
    if (m_commandBlocks.empty())
    {
        throw ApiException(ApiError::OutOfRange, "No command blocks have been parsed yet. Call ParseArgs() first.");
    }
    
    // Determine which command block to show help for
    const CommandDecl* cmdDesc = nullptr;
    std::string commandPath;
    
    if (commandBlockIndex < m_commandBlocks.size())
    {
        cmdDesc = &m_commandBlocks[commandBlockIndex].GetDecl();
        
        // Build command path up to the specified index
        commandPath = m_rootCommandBlockDesc.GetName();
        for (size_t i = 1; i <= commandBlockIndex; ++i)
        {
            if (i < m_commandBlocks.size())
            {
                commandPath += " " + m_commandBlocks[i].GetDecl().GetName();
            }
        }
    }
    else
    {
        throw ApiException(ApiError::OutOfRange, "Command block index out of range");
    }
    
    std::ostringstream s;
    
    // Add description if available
    if (!cmdDesc->GetDescription().empty())
    {
        s << cmdDesc->GetDescription() << std::endl << std::endl;
    }

    // For the usage line, parse command path and insert [options] after the first command
    std::istringstream pathStream(commandPath);
    std::string firstCommand, remainingPath;
    pathStream >> firstCommand;
    std::getline(pathStream, remainingPath);
    
    // Add usage information with the command path, inserting [options] after the first command
    s << "Usage:" << std::endl << firstCommand << " [options]" << remainingPath << " ";

    // Show global options first (they're available for all commands)
    for (const auto& optionPair : m_globalOptionDecls)
    {
        const OptionDecl& decl = *optionPair.second;
        s << "[--" << decl.GetName();
        if (decl.GetType() == OptionType::Variable)
            s << " <value>";
        s << "] ";
    }

    // Show specific options for this command block
    for (const auto& optionPair : cmdDesc->m_optionDescs)
    {
        const OptionDecl& decl = *optionPair.second;
        s << "[--" << decl.GetName();
        if (decl.GetType() == OptionType::Variable)
            s << " <value>";
        s << "] ";
    }

    // Parameters (positional arguments) in declaration order
    for (const auto& paramDesc : cmdDesc->m_parameterDescs)
    {
        s << "<" << paramDesc.get().GetName() << "> ";
    }

    s << std::endl;

    // Add option details (both global and local options together)
    std::ostringstream optionDetails;
    static const int colwidth = 30;

    // Add global options first
    for (const auto& optionPair : m_globalOptionDecls)
    {
        const OptionDecl& decl = *optionPair.second;
        if (!decl.GetDescription().empty())
        {
            std::string optionName = "  --" + decl.GetName();
            if (decl.GetAlias() != 0)
            {
                optionName += ", -" + std::string(1, decl.GetAlias());
            }
            optionDetails << std::setw(colwidth) << std::left << optionName;
            if (optionName.length() > colwidth)
            {
                optionDetails << std::endl;
                optionDetails << std::setw(colwidth) << ' ';
            }
            optionDetails << decl.GetDescription() << std::endl;
        }
    }

    // Add local option details
    for (const auto& optionPair : cmdDesc->m_optionDescs)
    {
        const OptionDecl& decl = *optionPair.second;
        if (!decl.GetDescription().empty())
        {
            std::string optionName = "  --" + decl.GetName();
            if (decl.GetAlias() != 0)
            {
                optionName += ", -" + std::string(1, decl.GetAlias());
            }
            optionDetails << std::setw(colwidth) << std::left << optionName;
            if (optionName.length() > colwidth)
            {
                optionDetails << std::endl;
                optionDetails << std::setw(colwidth) << ' ';
            }
            optionDetails << decl.GetDescription() << std::endl;
        }
    }

    // Add parameter details
    for (const auto& paramDesc : cmdDesc->m_parameterDescs)
    {
        if (!paramDesc.get().GetDescription().empty())
        {
            std::string paramName = "  <" + paramDesc.get().GetName() + ">";
            optionDetails << std::setw(colwidth) << std::left << paramName;
            if (paramName.length() > colwidth)
            {
                optionDetails << std::endl;
                optionDetails << std::setw(colwidth) << ' ';
            }
            optionDetails << paramDesc.get().GetDescription() << std::endl;
        }
    }

    // Add inner command blocks (subcommands)
    for (const auto& innerPair : cmdDesc->m_innerCommandDecls)
    {
        const CommandDecl& innerDesc = *innerPair.second;
        if (!innerDesc.GetDescription().empty())
        {
            std::string commandName = "  " + innerDesc.GetName();
            optionDetails << std::setw(colwidth) << std::left << commandName;
            if (commandName.length() > colwidth)
            {
                optionDetails << std::endl;
                optionDetails << std::setw(colwidth) << ' ';
            }
            optionDetails << innerDesc.GetDescription() << std::endl;
        }
    }

    // Add the combined option details to the output
    std::string combinedDetails = optionDetails.str();
    if (!combinedDetails.empty())
    {
        s << combinedDetails;
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
    auto it = m_optionSet.find(Option(OptionDecl(OptionType::Undefined, name)));
    return it != m_optionSet.end();
}

//------------------------------------------------------------------------------------------------
const std::string &CommandBlock::GetOptionValue(const std::string &name) const
{
    auto it = m_optionSet.find(Option(OptionDecl(OptionType::Undefined, name)));
    if (it != m_optionSet.end())
    {
        return it->GetValue();
    }
    throw ApiException(ApiError::OptionNotFound, "Option '" + name + "' not set");
}

//------------------------------------------------------------------------------------------------
const std::string &CommandBlock::GetOptionValue(const std::string &name, const std::string &defaultValue) const
{
    auto it = m_optionSet.find(Option(OptionDecl(OptionType::Undefined, name)));
    if (it != m_optionSet.end())
    {
        return it->GetValue();
    }
    return defaultValue;
}

//------------------------------------------------------------------------------------------------
bool CommandBlock::IsParameterSet(const std::string &name) const
{
    auto it = m_optionSet.find(Option(OptionDecl(OptionType::Undefined, name)));
    return it != m_optionSet.end();
}

//------------------------------------------------------------------------------------------------
const std::string &CommandBlock::GetParameterValue(const std::string &name) const
{
    auto it = m_optionSet.find(Option(OptionDecl(OptionType::Undefined, name)));
    if (it != m_optionSet.end())
    {
        return it->GetValue();
    }
    throw ApiException(ApiError::ParameterNotFound, "Parameter '" + name + "' not found");
}

//------------------------------------------------------------------------------------------------
const std::string &CommandBlock::GetParameterValue(const std::string &name, const std::string &defaultValue) const
{
    auto it = m_optionSet.find(Option(OptionDecl(OptionType::Undefined, name)));
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
// Parses command arguments and returns the number of parsed command blocks
size_t CommandParser::ParseArgs(int argc, const char *argv[])
{
    // Clear any previous parse results
    m_commandBlocks.clear();
    m_parsedGlobalOptions.clear();
    m_autoHelpRequested = false;
    
    // Auto-declare help option if enabled and not already declared
    if (m_autoHelpEnabled && !m_autoHelpOptionName.empty())
    {
        auto it = m_globalOptionDecls.find(m_autoHelpOptionName);
        if (it == m_globalOptionDecls.end())
        {
            // Help option not declared yet, auto-declare it
            AddGlobalOption(OptionType::Switch, m_autoHelpOptionName, m_autoHelpAlias)
                .SetDescription(m_autoHelpDescription);
        }
    }
    
    m_commandBlocks.emplace_back(m_rootCommandBlockDesc);
    size_t currentParameterIndex = 0; // Track which parameter we're expecting next

    for (int i = 1; i < argc; ++i)
    {
        CommandBlock &currentBlock = m_commandBlocks.back();
        const CommandDecl &currentDesc = currentBlock.GetDecl();
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
            const OptionDecl *optionDesc = FindGlobalOption(name);

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
                        m_parsedGlobalOptions.insert(std::pair(CommandBlock::Option(*optionDesc), m_commandBlocks.size() - 1));
                    }
                    else
                    {
                        currentBlock.m_optionSet.insert(CommandBlock::Option(*optionDesc));
                    }
                }
                else if (optionDesc->GetType() == OptionType::Variable)
                {
                    if (delimiterPos != std::string::npos)
                    {
                        if (isGlobalOption)
                        {
                            m_parsedGlobalOptions.insert(std::pair(CommandBlock::Option(*optionDesc, value), m_commandBlocks.size() - 1));
                        }
                        else
                        {
                            currentBlock.m_optionSet.insert(CommandBlock::Option(*optionDesc, value));
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
                            m_parsedGlobalOptions.insert(std::pair(CommandBlock::Option(*optionDesc, nextArg), m_commandBlocks.size() - 1));
                        }
                        else
                        {
                            currentBlock.m_optionSet.insert(CommandBlock::Option(*optionDesc, nextArg));
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
            const OptionDecl *firstDesc = FindGlobalOptionByAlias(firstAlias);

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
                            m_parsedGlobalOptions.insert(std::pair(CommandBlock::Option(*firstDesc, value), m_commandBlocks.size() - 1));
                        }
                        else
                        {
                            currentBlock.m_optionSet.insert(CommandBlock::Option(*firstDesc, value));
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
                const OptionDecl *optionDesc = FindGlobalOptionByAlias(alias);
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
                        m_parsedGlobalOptions.insert(std::pair(CommandBlock::Option(*optionDesc), m_commandBlocks.size() - 1));
                    }
                    else
                    {
                        currentBlock.m_optionSet.insert(CommandBlock::Option(*optionDesc));
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
                        m_parsedGlobalOptions.insert(std::pair(CommandBlock::Option(*optionDesc, nextArg), m_commandBlocks.size() - 1));
                    }
                    else
                    {
                        currentBlock.m_optionSet.insert(CommandBlock::Option(*optionDesc, nextArg));
                    }
                }
                continue;
            }
            
            // Multiple characters - only switches can be grouped
            for (char alias : aliases)
            {
                bool isGlobalOptionAlias = false;
                const OptionDecl *optionDesc = FindGlobalOptionByAlias(alias);
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
                    m_parsedGlobalOptions.insert(std::pair(CommandBlock::Option(*optionDesc), m_commandBlocks.size() - 1));
                }
                else
                {
                    currentBlock.m_optionSet.insert(CommandBlock::Option(*optionDesc));
                }
            }
            continue;
        }

        // Check for inner command block
        auto innerIt = currentDesc.m_innerCommandDecls.find(token);
        if (innerIt != currentDesc.m_innerCommandDecls.end())
        {
            m_commandBlocks.emplace_back(*innerIt->second.get());
            currentParameterIndex = 0; // Reset parameter index for new command block
            continue;
        }

        // Otherwise treat as positional parameter
        if (currentParameterIndex < currentDesc.m_parameterDescs.size())
        {
            const OptionDecl &decl = currentDesc.m_parameterDescs[currentParameterIndex].get();
            currentBlock.SetOption(CommandBlock::Option(decl, token));
            currentParameterIndex++;
            continue;
        }
        
        // If we get here, it's an unexpected argument
        throw SyntaxException(SyntaxError::UnexpectedArgument, "Unexpected argument: " + token, token);
    }

    // Check for help after parsing is complete - do this before clearing anything
    // Check if the help option (auto or manually declared) was used
    bool helpWasRequested = IsGlobalOptionSet(m_autoHelpOptionName);
    size_t helpContextIndex = 0;
    
    if (helpWasRequested)
    {
        // Get the context where help was requested
        helpContextIndex = GetGlobalOptionContext(m_autoHelpOptionName);
        
        // Generate help text for the appropriate command block using new simplified method
        std::ostringstream helpStream;
        helpStream << std::endl;
        helpStream << GetHelpString(helpContextIndex);
        
        // Set help state and output help to the configured stream
        m_autoHelpRequested = true;
        if (m_autoHelpOutputStream != nullptr)
        {
            *m_autoHelpOutputStream << helpStream.str();
        }
        
        // Clear command blocks and global options since help was requested
        m_commandBlocks.clear();
        m_parsedGlobalOptions.clear();
        
        // Add an empty command block so we have something to return
        m_commandBlocks.emplace_back(m_rootCommandBlockDesc);
    }

    // Return the number of command blocks that were parsed
    return m_commandBlocks.size();
}

//------------------------------------------------------------------------------------------------
bool CommandParser::IsGlobalOptionSet(const std::string& name) const
{
    // Search through all parsed global options to find one with matching name
    for (const auto &[option, commandBlock] : m_parsedGlobalOptions)
    {
        if (option.GetDecl().GetName() == name)
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
        if (option.GetDecl().GetName() == name)
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
        if (option.GetDecl().GetName() == name)
        {
            return contextIndex;
        }
    }
    throw ApiException(ApiError::OptionNotFound, "Global option '" + name + "' not set");
}

} // namespace InCommand
