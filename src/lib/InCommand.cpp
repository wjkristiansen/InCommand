#include <sstream>
#include <iomanip>

#include "InCommand.h"

namespace InCommand
{
    static std::string OptionUsageString(const COption *pOption)
    {
        //------------------------------------------------------------------------------------------------
        std::ostringstream s;
        if (pOption->Type() == OptionType::Parameter)
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
            if (pOption->Type() == OptionType::Variable)
                s << " <value>";
        }

        return s.str();
    }

    //------------------------------------------------------------------------------------------------
    CCommandScope::CCommandScope(const char* name, const char* description, int scopeId) :
        m_ScopeId(scopeId),
        m_Description(description ? description : "")
    {
        if (name)
            m_Name = name;
    }

    //------------------------------------------------------------------------------------------------
    CCommandScope &CCommandScope::ScanCommandArgs(const CArgumentList& args, CArgumentIterator& it)
    {
        CCommandScope *pScope = this;

        if (it != args.End())
        {
            auto subIt = m_Subcommands.find(args.At(it));
            if (subIt != m_Subcommands.end())
            {
                ++it;
                pScope = &(subIt->second->ScanCommandArgs(args, it));
            }
        }

        return *pScope;
    }

    //------------------------------------------------------------------------------------------------
    OptionScanResult CCommandScope::ScanOptionArgs(const CArgumentList& args, CArgumentIterator& it) const
    {
        OptionScanResult result;

        if (it == args.End())
        {
            return result;
        }

        // Parse the options
        while(it != args.End())
        {
            if (args.At(it)[0] == '-')
            {
                const COption* pOption = nullptr;

                if (args.At(it)[1] == '-')
                {
                    // Long-form option
                    auto optIt = m_Options.find(args.At(it).substr(2));
                    if (optIt == m_Options.end() || optIt->second->Type() == OptionType::Parameter)
                    {
                        result.Status = InCommandStatus::UnknownOption;
                        return result;
                    }
                    pOption = optIt->second.get();
                }
                else
                {
                    auto optIt = m_ShortOptions.find(args.At(it)[1]);
                    if (optIt == m_ShortOptions.end())
                    {
                        result.Status = InCommandStatus::UnknownOption;
                        return result;
                    }
                    pOption = optIt->second.get();
                }

                if (pOption)
                {
                    result.Status = pOption->ParseArgs(args, it);
                    if (result.Status != InCommandStatus::Success)
                        return result;
                }
            }
            else
            {
                // Assume parameter option
                if (result.NumParameters == int(m_ParameterOptions.size()))
                {
                    result.Status = InCommandStatus::UnexpectedArgument;
                    return result;
                }

                result.Status = m_ParameterOptions[result.NumParameters]->ParseArgs(args, it);
                if (result.Status != InCommandStatus::Success)
                    return result;
                result.NumParameters++;
            }
        }

        if (result.NumParameters < int(m_ParameterOptions.size()))
        {
            result.Status = InCommandStatus::MissingParameters;
        }

        return result;
    }

    //------------------------------------------------------------------------------------------------
    const COption& CCommandScope::GetOption(const char* name) const
    {
        auto it = m_Options.find(name);
        if (it == m_Options.end())
            throw InCommandException(InCommandStatus::UnknownOption);

        return *it->second;
    }

    //------------------------------------------------------------------------------------------------
    CCommandScope& CCommandScope::DeclareSubcommand(const char* name, const char* description, int scopeId)
    {
        auto it = m_Subcommands.find(name);
        if (it != m_Subcommands.end())
            throw InCommandException(InCommandStatus::DuplicateCommand);

        auto result = m_Subcommands.emplace(name, std::make_shared<CCommandScope>(name, description, scopeId));
        result.first->second->m_pSuperScope = this;
        return *result.first->second;
    }

    //------------------------------------------------------------------------------------------------
    std::string CCommandScope::CommandChainString() const
    {
        std::ostringstream s;
        if (m_pSuperScope)
        {
            s << m_pSuperScope->CommandChainString();
            s << " ";
        }

        s << m_Name;

        return s.str();
    }

    //------------------------------------------------------------------------------------------------
    std::string CCommandScope::UsageString() const
    {
        std::ostringstream s;
        s << "USAGE" << std::endl;
        s << std::endl;
        static const int colwidth = 30;

        std::string commandChainString = CommandChainString();

        if (!m_Subcommands.empty())
        {
            s << "  " + commandChainString;

            s << " [<command> [<command options>]]" << std::endl;
        }

        if (!m_Options.empty())
        {
            s << "  " + commandChainString;

            // Parameter options first
            for (auto& nko : m_ParameterOptions)
            {
                s << " <" << nko->Name() << ">";
            }

            s << " [<options>]" << std::endl;
        }

        s << std::endl;

        if (!m_Subcommands.empty())
        {
            s << "COMMANDS" << std::endl;
            s << std::endl;

            for (auto subIt : m_Subcommands)
            {
                s << std::setw(colwidth) << std::left << "  " + subIt.second->Name();
                s << subIt.second->m_Description << std::endl;
            };
        }

        s << std::endl;

        if (!m_Options.empty())
        {
            // Parameter options first
            s << "OPTIONS" << std::endl;
            s << std::endl;

            // Parameter options details
            for (auto& nko : m_ParameterOptions)
            {
                s << std::setw(colwidth) << std::left << "  " + OptionUsageString(nko.get());
                s << nko->Description() << std::endl;
            }

            // Keyed-options details
            for (auto& ko : m_Options)
            {
                auto type = ko.second->Type();
                if (type == OptionType::Parameter)
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
    const COption& CCommandScope::DeclareSwitchOption(InCommandBool& boundValue, const char* name, const char *description, char shortKey)
    {
        auto it = m_Options.find(name);
        if (it != m_Options.end())
            throw InCommandException(InCommandStatus::DuplicateOption);

        auto insert = m_Options.emplace(name, std::make_shared<CSwitchOption>(boundValue, name, description, shortKey));
        if (shortKey)
            m_ShortOptions.insert(std::make_pair(shortKey, insert.first->second));
        return *insert.first->second;
    }

    //------------------------------------------------------------------------------------------------
    const COption &CCommandScope::DeclareVariableOption(CInCommandValue& boundValue, const char* name, const char *description, char shortKey)
    {
        auto it = m_Options.find(name);
        if (it != m_Options.end())
            throw InCommandException(InCommandStatus::DuplicateOption);

        auto insert = m_Options.emplace(name, std::make_shared<CVariableOption>(boundValue, name, description, shortKey));
        if (shortKey)
            m_ShortOptions.insert(std::make_pair(shortKey, insert.first->second));
        return *insert.first->second;
    }

    //------------------------------------------------------------------------------------------------
    const COption &CCommandScope::DeclareVariableOption(CInCommandValue& boundValue, const char* name, int domainSize, const char* domain[], const char *description, char shortKey)
    {
        auto it = m_Options.find(name);
        if (it != m_Options.end())
            throw InCommandException(InCommandStatus::DuplicateOption);

        auto insert = m_Options.emplace(name, std::make_shared<CVariableOption>(boundValue, name, domainSize, domain, description, shortKey));
        if (shortKey)
            m_ShortOptions.insert(std::make_pair(shortKey, insert.first->second));
        return *insert.first->second;
    }

    //------------------------------------------------------------------------------------------------
    const COption& CCommandScope::DeclareParameterOption(CInCommandValue& boundValue, const char* name, const char* description)
    {
        auto it = m_Options.find(name);
        if (it != m_Options.end())
            throw InCommandException(InCommandStatus::DuplicateOption);


        std::shared_ptr<CParameterOption> pOption = std::make_shared<CParameterOption>(boundValue, name, description);

        // Add the parameter option to the declared options map
        m_Options.insert(std::make_pair(name, pOption));

        // Add to the array of parameter options
        m_ParameterOptions.push_back(pOption);
        return *m_ParameterOptions.back();
    }

    std::string CCommandScope::ErrorString(const OptionScanResult& result, const CArgumentList& argList, const CArgumentIterator& argIt) const
    {
        std::ostringstream s;

        switch (result.Status)
        {
        case InCommandStatus::Success:
            s << "Success";
            break;

        case InCommandStatus::UnexpectedArgument:
            s << "Unexpected Argument: \"" << argList.At(argIt) << "\"";
            break;

        case InCommandStatus::InvalidValue:
            s << "Invalid Value: \"" << argList.At(argIt) << "\"";
            break;

        case InCommandStatus::MissingOptionValue:
            s << "Missing Option Value";
            break;

        case InCommandStatus::MissingParameters: {
            s << "Missing Parameters: ";
            const char* separator = "";
            for (size_t i = result.NumParameters; i < this->m_ParameterOptions.size(); ++i)
            {
                s << m_ParameterOptions[i]->Name() << separator;
                separator = ", ";
            }
            break; }

        case InCommandStatus::UnknownOption:
            s << "Unknown Option: \"" << argList.At(argIt) << "\"";
            break;

        default:
            s << "Unknown Error";
            break;
        }

        return s.str();
    }
}
