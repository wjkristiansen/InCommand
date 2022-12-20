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
    CCommandCtx::CCommandCtx(const char* name, const char* description, int scopeId) :
        m_ScopeId(scopeId),
        m_Description(description ? description : "")
    {
        if (name)
            m_Name = name;
    }

    //------------------------------------------------------------------------------------------------
    CCommandCtx* CCommandCtx::FetchCommandCtx(CCommandReader* pReader)
    {
        CCommandCtx *pScope = this;

        ++pReader->m_ArgIt;
        if (pReader->m_ArgIt != pReader->m_ArgList.End())
        {
            auto subIt = m_Subcommands.find(pReader->m_ArgList.At(pReader->m_ArgIt));
            if (subIt != m_Subcommands.end())
            {
                pScope = subIt->second->FetchCommandCtx(pReader);
            }
        }

        return pScope;
    }

    //------------------------------------------------------------------------------------------------
    InCommandStatus CCommandCtx::FetchOptions(CCommandReader *pReader) const
    {
        InCommandStatus result = InCommandStatus::Success;

        if (pReader->m_ArgIt == pReader->m_ArgList.End())
        {
            return result;
        }

        // Parse the options
        while(pReader->m_ArgIt != pReader->m_ArgList.End())
        {
            if (pReader->m_ArgList.At(pReader->m_ArgIt)[0] == '-')
            {
                const COption* pOption = nullptr;

                if (pReader->m_ArgList.At(pReader->m_ArgIt)[1] == '-')
                {
                    // Long-form option
                    auto optIt = m_Options.find(pReader->m_ArgList.At(pReader->m_ArgIt).substr(2));
                    if (optIt == m_Options.end() || optIt->second->Type() == OptionType::Parameter)
                    {
                        result = InCommandStatus::UnknownOption;
                        return result;
                    }
                    pOption = optIt->second.get();
                }
                else
                {
                    auto optIt = m_ShortOptions.find(pReader->m_ArgList.At(pReader->m_ArgIt)[1]);
                    if (optIt == m_ShortOptions.end())
                    {
                        result = InCommandStatus::UnknownOption;
                        return result;
                    }
                    pOption = optIt->second.get();
                }

                if (pOption)
                {
                    result = pOption->ParseArgs(pReader->m_ArgList, pReader->m_ArgIt);
                    if (result != InCommandStatus::Success)
                        return result;
                }
            }
            else
            {
                // Assume parameter option
                if (pReader->m_ParametersRead == m_ParameterOptions.size())
                {
                    result = InCommandStatus::UnexpectedArgument;
                    return result;
                }

                result = m_ParameterOptions[pReader->m_ParametersRead]->ParseArgs(pReader->m_ArgList, pReader->m_ArgIt);
                if (result != InCommandStatus::Success)
                    return result;
                pReader->m_ParametersRead++;
            }
        }

        return result;
    }

    //------------------------------------------------------------------------------------------------
    CCommandCtx* CCommandCtx::DeclareCommandCtx(const char* name, const char* description, int scopeId)
    {
        auto it = m_Subcommands.find(name);
        if (it != m_Subcommands.end())
            throw InCommandException(InCommandStatus::DuplicateCommand);

        auto result = m_Subcommands.emplace(name, std::make_shared<CCommandCtx>(name, description, scopeId));
        result.first->second->m_pSuperScope = this;
        return result.first->second.get();
    }

    //------------------------------------------------------------------------------------------------
    std::string CCommandCtx::CommandChainString() const
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
    std::string CCommandCtx::UsageString() const
    {
        std::ostringstream s;
        s << m_Description << std::endl;
        s << std::endl;
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
    const COption* CCommandCtx::DeclareSwitchOption(InCommandBool& boundValue, const char* name, const char *description, char shortKey)
    {
        auto it = m_Options.find(name);
        if (it != m_Options.end())
            throw InCommandException(InCommandStatus::DuplicateOption);

        auto insert = m_Options.emplace(name, std::make_shared<CSwitchOption>(boundValue, name, description, shortKey));
        if (shortKey)
            m_ShortOptions.insert(std::make_pair(shortKey, insert.first->second));
        return insert.first->second.get();
    }

    //------------------------------------------------------------------------------------------------
    const COption* CCommandCtx::DeclareVariableOption(CInCommandValue& boundValue, const char* name, const char *description, char shortKey)
    {
        auto it = m_Options.find(name);
        if (it != m_Options.end())
            throw InCommandException(InCommandStatus::DuplicateOption);

        auto insert = m_Options.emplace(name, std::make_shared<CVariableOption>(boundValue, name, description, shortKey));
        if (shortKey)
            m_ShortOptions.insert(std::make_pair(shortKey, insert.first->second));
        return insert.first->second.get();
    }

    //------------------------------------------------------------------------------------------------
    const COption* CCommandCtx::DeclareVariableOption(CInCommandValue& boundValue, const char* name, int domainSize, const char* domain[], const char *description, char shortKey)
    {
        auto it = m_Options.find(name);
        if (it != m_Options.end())
            throw InCommandException(InCommandStatus::DuplicateOption);

        auto insert = m_Options.emplace(name, std::make_shared<CVariableOption>(boundValue, name, domainSize, domain, description, shortKey));
        if (shortKey)
            m_ShortOptions.insert(std::make_pair(shortKey, insert.first->second));
        return insert.first->second.get();
    }

    //------------------------------------------------------------------------------------------------
    const COption* CCommandCtx::DeclareVariableOption(CInCommandValue& boundValue, const char* name, int domainSize, const CInCommandValue *pDomainValues, const char* description, char shortKey)
    {
        auto it = m_Options.find(name);
        if (it != m_Options.end())
            throw InCommandException(InCommandStatus::DuplicateOption);

        auto insert = m_Options.emplace(name, std::make_shared<CVariableOption>(boundValue, name, domainSize, pDomainValues, description, shortKey));
        if (shortKey)
            m_ShortOptions.insert(std::make_pair(shortKey, insert.first->second));
        return insert.first->second.get();
    }

    //------------------------------------------------------------------------------------------------
    const COption* CCommandCtx::DeclareParameterOption(CInCommandValue& boundValue, const char* name, const char* description)
    {
        auto it = m_Options.find(name);
        if (it != m_Options.end())
            throw InCommandException(InCommandStatus::DuplicateOption);


        std::shared_ptr<CParameterOption> pOption = std::make_shared<CParameterOption>(boundValue, name, description);

        // Add the parameter option to the declared options map
        m_Options.insert(std::make_pair(name, pOption));

        // Add to the array of parameter options
        m_ParameterOptions.push_back(pOption);
        return m_ParameterOptions.back().get();
    }

    //------------------------------------------------------------------------------------------------
    CCommandReader::CCommandReader(const char* appName, const char *defaultDescription, int argc, const char* argv[]) :
        m_DefaultCtx(appName, defaultDescription, 0),
        m_ArgList(argc, argv)
    {}

    //------------------------------------------------------------------------------------------------
    CCommandCtx *CCommandReader::ReadCommandCtx()
    {
        m_ArgIt = m_ArgList.Begin();
        m_pActiveCtx = m_DefaultCtx.FetchCommandCtx(this);
        m_LastStatus = InCommandStatus::Success;
        return m_pActiveCtx;
    }

    //------------------------------------------------------------------------------------------------
    InCommandStatus CCommandReader::ReadOptions()
    {
        if (!m_pActiveCtx)
            ReadCommandCtx();

        m_LastStatus =  m_pActiveCtx->FetchOptions(this);
        return m_LastStatus;
    }

    //------------------------------------------------------------------------------------------------
    std::string CCommandReader::LastErrorString() const
    {
        std::ostringstream s;

        switch (m_LastStatus)
        {
        case InCommandStatus::Success:
            s << "Success";
            break;

        case InCommandStatus::UnexpectedArgument:
            s << "Unexpected Argument: \"" << m_ArgList.At(m_ArgIt) << "\"";
            break;

        case InCommandStatus::InvalidValue:
            s << "Invalid Value: \"" << m_ArgList.At(m_ArgIt) << "\"";
            break;

        case InCommandStatus::MissingOptionValue:
            s << "Missing Option Value";
            break;

        case InCommandStatus::UnknownOption:
            s << "Unknown Option: \"" << m_ArgList.At(m_ArgIt) << "\"";
            break;

        default:
            s << "Unknown Error";
            break;
        }

        return s.str();

    }

}
