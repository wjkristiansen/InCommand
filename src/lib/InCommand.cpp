#include <sstream>
#include <iomanip>

#include "InCommand.h"

namespace InCommand
{
    static std::string OptionUsageString(const COption *pOption)
    {
        //------------------------------------------------------------------------------------------------
        std::ostringstream s;
        if (pOption->Type() == OptionType::Input)
        {
            s << "<" << pOption->Name() << ">";
        }
        else
        {
            if (pOption->ShortKey())
            {
                s << '-' << pOption->ShortKey() << ", ";
            }
            s << "--" << pOption->Name();
            if (pOption->Type() == OptionType::Switch)
                s << " <value>";
        }

        return s.str();
    }

    //------------------------------------------------------------------------------------------------
    CCommand::CCommand(const char* name, const char* description, int scopeId) :
        m_ScopeId(scopeId),
        m_Description(description ? description : "")
    {
        if (name)
            m_Name = name;
    }

    //------------------------------------------------------------------------------------------------
    CCommand* CCommand::DeclareCommand(const char* name, const char* description, int scopeId)
    {
        auto it = m_InnerCommands.find(name);
        if (it != m_InnerCommands.end())
            throw Exception(Status::DuplicateCommand);

        auto result = m_InnerCommands.emplace(name, std::make_shared<CCommand>(name, description, scopeId));
        result.first->second->m_pOuterCommand = this;
        return result.first->second.get();
    }

    //------------------------------------------------------------------------------------------------
    std::string CCommand::CommandScopeString() const
    {
        std::ostringstream s;
        if (m_pOuterCommand)
        {
            s << m_pOuterCommand->CommandScopeString();
            s << " ";
        }

        s << m_Name;

        return s.str();
    }

    //------------------------------------------------------------------------------------------------
    std::string CCommand::UsageString() const
    {
        std::ostringstream s;
        s << m_Description << std::endl;
        s << std::endl;
        s << "USAGE:" << std::endl;
        s << std::endl;
        static const int colwidth = 30;

        std::string commandScopeString = CommandScopeString();

        if (!m_Options.empty())
        {
            s << "  " + commandScopeString;

            // Option arguments first
            for (auto& nko : m_InputOptions)
            {
                s << " <" << nko->Name() << ">";
            }

            s << " [<options>]" << std::endl;
        }

        if (!m_InnerCommands.empty())
        {
            s << "  " + commandScopeString;

            s << " [<command> [<options>]]" << std::endl;
        }

        s << std::endl;

        if (!m_InnerCommands.empty())
        {
            s << "COMMANDS" << std::endl;
            s << std::endl;

            for (auto subIt : m_InnerCommands)
            {
                s << std::setw(colwidth) << std::left << "  " + subIt.second->Name();
                s << subIt.second->m_Description << std::endl;
            };
        }

        if (!m_Options.empty())
        {
            // Input options first
            for (auto& inputOption : m_InputOptions)
            {
                s << " <" << inputOption->Name() << ">";
            }
        }

        s << std::endl;

        s << std::endl;

        if (!m_Options.empty())
        {
            // Option first
            s << "OPTIONS" << std::endl;
            s << std::endl;

            // Option details
            for (auto& inputOption : m_InputOptions)
            {
                s << std::setw(colwidth) << std::left << "  " + OptionUsageString(inputOption.get());
                s << inputOption->Description() << std::endl;
            }

            // Keyed-options details
            for (auto& ko : m_Options)
            {
                auto type = ko.second->Type();
                if (type == OptionType::Input)
                    continue; // Already dumped
                s << std::setw(colwidth) << std::left << "  " + OptionUsageString(ko.second.get());
                if (OptionUsageString(ko.second.get()).length() + 4 > colwidth)
                {
                    s << std::endl;
                    s << std::setw(colwidth) << ' ';
                }
                s << ko.second->Description() << std::endl;
            }

            s << std::endl;
        }

        return s.str();
    }

    //------------------------------------------------------------------------------------------------
    const COption* CCommand::DeclareBoolSwitchOption(Bool& boundValue, const char* name, const char *description, char shortKey)
    {
        auto it = m_Options.find(name);
        if (it != m_Options.end())
            throw Exception(Status::DuplicateOption);

        auto insert = m_Options.emplace(name, std::make_shared<CBoolOption>(boundValue, name, description, shortKey));
        if (shortKey)
            m_ShortOptions.insert(std::make_pair(shortKey, insert.first->second));
        return insert.first->second.get();
    }

    //------------------------------------------------------------------------------------------------
    const COption* CCommand::DeclareSwitchOption(Value& boundValue, const char* name, const char *description, char shortKey)
    {
        auto it = m_Options.find(name);
        if (it != m_Options.end())
            throw Exception(Status::DuplicateOption);

        auto insert = m_Options.emplace(name, std::make_shared<CSwitchOption>(boundValue, name, description, shortKey));
        if (shortKey)
            m_ShortOptions.insert(std::make_pair(shortKey, insert.first->second));
        return insert.first->second.get();
    }

    //------------------------------------------------------------------------------------------------
    const COption* CCommand::DeclareSwitchOption(Value& boundValue, const char* name, int domainSize, const char* domainValueStrings[], const char *description, char shortKey)
    {
        auto it = m_Options.find(name);
        if (it != m_Options.end())
            throw Exception(Status::DuplicateOption);

        Domain domain(domainSize, domainValueStrings);
        auto insert = m_Options.emplace(name, std::make_shared<CSwitchOption>(boundValue, name, domain, description, shortKey));
        if (shortKey)
            m_ShortOptions.insert(std::make_pair(shortKey, insert.first->second));
        return insert.first->second.get();
    }

    //------------------------------------------------------------------------------------------------
    const COption* CCommand::DeclareSwitchOption(Value& boundValue, const char* name, const Domain &domain, const char* description, char shortKey)
    {
        auto it = m_Options.find(name);
        if (it != m_Options.end())
            throw Exception(Status::DuplicateOption);

        auto insert = m_Options.emplace(name, std::make_shared<CSwitchOption>(boundValue, name, domain, description, shortKey));
        if (shortKey)
            m_ShortOptions.insert(std::make_pair(shortKey, insert.first->second));
        return insert.first->second.get();
    }

    //------------------------------------------------------------------------------------------------
    const COption* CCommand::DeclareInputOption(Value& boundValue, const char* name, const char* description)
    {
        auto it = m_Options.find(name);
        if (it != m_Options.end())
            throw Exception(Status::DuplicateOption);


        std::shared_ptr<CInputOption> pOption = std::make_shared<CInputOption>(boundValue, name, description);

        // Add the option to the declared options map
        m_Options.insert(std::make_pair(name, pOption));

        // Add to the array of input options
        m_InputOptions.push_back(pOption);
        return m_InputOptions.back().get();
    }

    //------------------------------------------------------------------------------------------------
    CCommandReader::CCommandReader(const char* appName, const char *defaultDescription) :
        CCommand(appName, defaultDescription)
    {
    }

    //------------------------------------------------------------------------------------------------
    CCommand *CCommandReader::PreReadCommandArguments(int argc, const char* argv[])
    {
        m_ArgList = CArgumentList(argc, argv);
        m_ArgIt = m_ArgList.Begin();

        CCommand *pCommand = this;

        ++m_ArgIt;
        while (m_ArgIt != m_ArgList.End())
        {
            auto subIt = pCommand->m_InnerCommands.find(m_ArgList.At(m_ArgIt));
            if (subIt == pCommand->m_InnerCommands.end())
                break;
            pCommand = subIt->second.get();
            ++m_ArgIt;
        }

        return pCommand;
    }

    //------------------------------------------------------------------------------------------------
    Status CCommandReader::ReadOptionArguments(const CCommand *pCommand)
    {
        Status result = Status::Success;

        if (m_ArgIt == m_ArgList.End())
        {
            return result;
        }

        // Parse the options
        while(m_ArgIt != m_ArgList.End())
        {
            if (m_ArgList.At(m_ArgIt)[0] == '-')
            {
                const COption* pOption = nullptr;

                if (m_ArgList.At(m_ArgIt)[1] == '-')
                {
                    // Long-form option
                    auto optIt = pCommand->m_Options.find(m_ArgList.At(m_ArgIt).substr(2));
                    if (optIt == pCommand->m_Options.end() || optIt->second->Type() == OptionType::Input)
                    {
                        result = Status::UnknownOption;
                        return result;
                    }
                    pOption = optIt->second.get();
                }
                else
                {
                    auto optIt = pCommand->m_ShortOptions.find(m_ArgList.At(m_ArgIt)[1]);
                    if (optIt == pCommand->m_ShortOptions.end())
                    {
                        result = Status::UnknownOption;
                        return result;
                    }
                    pOption = optIt->second.get();
                }

                if (pOption)
                {
                    result = pOption->ParseArgs(m_ArgList, m_ArgIt);
                    if (result != Status::Success)
                        return result;
                }
            }
            else
            {
                // Assume input option
                if (m_InputOptionArgsRead == pCommand->m_InputOptions.size())
                {
                    result = Status::UnexpectedArgument;
                    return result;
                }

                result = pCommand->m_InputOptions[m_InputOptionArgsRead]->ParseArgs(m_ArgList, m_ArgIt);
                if (result != Status::Success)
                    return result;
                m_InputOptionArgsRead++;
            }
        }

        return result;
    }

    //------------------------------------------------------------------------------------------------
    Status CCommandReader::ReadArguments(int argc, const char* argv[])
    {
        CCommand *pCommand = PreReadCommandArguments(argc, argv);

        m_InputOptionArgsRead = 0;
        m_LastStatus = Status::Success;

        return ReadOptionArguments(pCommand);
    }

    //------------------------------------------------------------------------------------------------
    std::string CCommandReader::LastErrorString() const
    {
        std::ostringstream s;

        switch (m_LastStatus)
        {
        case Status::Success:
            s << "Success";
            break;

        case Status::UnexpectedArgument:
            s << "Unexpected Argument: \"" << m_ArgList.At(m_ArgIt) << "\"";
            break;

        case Status::InvalidValue:
            s << "Invalid Value: \"" << m_ArgList.At(m_ArgIt) << "\"";
            break;

        case Status::MissingOptionValue:
            s << "Missing Option Value";
            break;

        case Status::UnknownOption:
            s << "Unknown Option: \"" << m_ArgList.At(m_ArgIt) << "\"";
            break;

        default:
            s << "Unknown Error";
            break;
        }

        return s.str();

    }

}
