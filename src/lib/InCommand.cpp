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
            if (pOption->GetShortKey())
            {
                s << '-' << pOption->GetShortKey() << ", ";
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
    InCommandResult CCommandScope::ScanOptionArgs(const CArgumentList& args, CArgumentIterator& it) const
    {
        if (it == args.End())
        {
            return InCommandResult::Success;
        }

        int ParameterOptionIndex = 0;

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
                    if(optIt == m_Options.end() || optIt->second->Type() == OptionType::Parameter)
                        return InCommandResult::UnexpectedArgument;
                    pOption = optIt->second.get();
                }
                else
                {
                    auto optIt = m_ShortOptions.find(args.At(it)[1]);
                    if(optIt == m_ShortOptions.end())
                        return InCommandResult::UnexpectedArgument;
                    pOption = optIt->second.get();
                }

                if (pOption)
                {
                    auto result = pOption->ParseArgs(args, it);
                    if (result != InCommandResult::Success)
                        return result;
                }
            }
            else
            {
                // Assume non-keyed option
                if (ParameterOptionIndex == int(m_ParameterOptions.size()))
                    return InCommandResult::UnexpectedArgument;

                auto result = m_ParameterOptions[ParameterOptionIndex]->ParseArgs(args, it);
                if (result != InCommandResult::Success)
                    return result;
                ParameterOptionIndex++;
            }
        }

        return InCommandResult::Success;
    }

    //------------------------------------------------------------------------------------------------
    const COption& CCommandScope::GetOption(const char* name) const
    {
        auto it = m_Options.find(name);
        if (it == m_Options.end())
            throw InCommandException(InCommandResult::UnknownOption);

        return *it->second;
    }

    //------------------------------------------------------------------------------------------------
    CCommandScope& CCommandScope::DeclareSubcommand(const char* name, const char* description, int scopeId)
    {
        auto it = m_Subcommands.find(name);
        if (it != m_Subcommands.end())
            throw InCommandException(InCommandResult::DuplicateCommand);

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

            // Non-keyed options first
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
            // Non-keyed options first
            s << "OPTIONS" << std::endl;
            s << std::endl;

            // Non-keyed options details
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
            throw InCommandException(InCommandResult::DuplicateOption);

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
            throw InCommandException(InCommandResult::DuplicateOption);

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
            throw InCommandException(InCommandResult::DuplicateOption);

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
            throw InCommandException(InCommandResult::DuplicateOption);


        std::shared_ptr<CParameterOption> pOption = std::make_shared<CParameterOption>(boundValue, name, description);

        // Add the non-keyed option to the declared options map
        m_Options.insert(std::make_pair(name, pOption));

        // Add to the array of non-keyed options
        m_ParameterOptions.push_back(pOption);
        return *m_ParameterOptions.back();
    }
}
