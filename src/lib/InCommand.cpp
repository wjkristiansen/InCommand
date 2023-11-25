#include <sstream>
#include <iomanip>

#include "InCommand.h"

namespace InCommand
{
    static std::string ParameterUsageString(const CParameter *pParameter)
    {
        //------------------------------------------------------------------------------------------------
        std::ostringstream s;
        if (pParameter->Type() == ParameterType::Input)
        {
            s << "<" << pParameter->Name() << ">";
        }
        else
        {
            if (pParameter->ShortKey())
            {
                s << '-' << pParameter->ShortKey() << ", ";
            }
            s << "--" << pParameter->Name();
            if (pParameter->Type() == ParameterType::Option)
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
        auto it = m_Subcommands.find(name);
        if (it != m_Subcommands.end())
            throw Exception(Status::DuplicateCommand);

        auto result = m_Subcommands.emplace(name, std::make_shared<CCommand>(name, description, scopeId));
        result.first->second->m_pSuperScope = this;
        return result.first->second.get();
    }

    //------------------------------------------------------------------------------------------------
    std::string CCommand::CommandChainString() const
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
    std::string CCommand::UsageString() const
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

        if (!m_Parameters.empty())
        {
            s << "  " + commandChainString;

            // Parameter options first
            for (auto& nko : m_InputParameters)
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

        if (!m_Parameters.empty())
        {
            // Parameter options first
            s << "OPTIONS" << std::endl;
            s << std::endl;

            // Parameter options details
            for (auto& nko : m_InputParameters)
            {
                s << std::setw(colwidth) << std::left << "  " + ParameterUsageString(nko.get());
                s << nko->Description() << std::endl;
            }

            // Keyed-options details
            for (auto& ko : m_Parameters)
            {
                auto type = ko.second->Type();
                if (type == ParameterType::Input)
                    continue; // Already dumped
                s << std::setw(colwidth) << std::left << "  " + ParameterUsageString(ko.second.get());
                if (ParameterUsageString(ko.second.get()).length() + 4 > colwidth)
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
    const CParameter* CCommand::DeclareBoolParameter(Bool& boundValue, const char* name, const char *description, char shortKey)
    {
        auto it = m_Parameters.find(name);
        if (it != m_Parameters.end())
            throw Exception(Status::DuplicateOption);

        auto insert = m_Parameters.emplace(name, std::make_shared<CBoolParameter>(boundValue, name, description, shortKey));
        if (shortKey)
            m_ShortOptions.insert(std::make_pair(shortKey, insert.first->second));
        return insert.first->second.get();
    }

    //------------------------------------------------------------------------------------------------
    const CParameter* CCommand::DeclareOptionParameter(Value& boundValue, const char* name, const char *description, char shortKey)
    {
        auto it = m_Parameters.find(name);
        if (it != m_Parameters.end())
            throw Exception(Status::DuplicateOption);

        auto insert = m_Parameters.emplace(name, std::make_shared<COptionParameter>(boundValue, name, description, shortKey));
        if (shortKey)
            m_ShortOptions.insert(std::make_pair(shortKey, insert.first->second));
        return insert.first->second.get();
    }

    //------------------------------------------------------------------------------------------------
    const CParameter* CCommand::DeclareOptionParameter(Value& boundValue, const char* name, int domainSize, const char* domainValueStrings[], const char *description, char shortKey)
    {
        auto it = m_Parameters.find(name);
        if (it != m_Parameters.end())
            throw Exception(Status::DuplicateOption);

        Domain domain(domainSize, domainValueStrings);
        auto insert = m_Parameters.emplace(name, std::make_shared<COptionParameter>(boundValue, name, domain, description, shortKey));
        if (shortKey)
            m_ShortOptions.insert(std::make_pair(shortKey, insert.first->second));
        return insert.first->second.get();
    }

    //------------------------------------------------------------------------------------------------
    const CParameter* CCommand::DeclareOptionParameter(Value& boundValue, const char* name, const Domain &domain, const char* description, char shortKey)
    {
        auto it = m_Parameters.find(name);
        if (it != m_Parameters.end())
            throw Exception(Status::DuplicateOption);

        auto insert = m_Parameters.emplace(name, std::make_shared<COptionParameter>(boundValue, name, domain, description, shortKey));
        if (shortKey)
            m_ShortOptions.insert(std::make_pair(shortKey, insert.first->second));
        return insert.first->second.get();
    }

    //------------------------------------------------------------------------------------------------
    const CParameter* CCommand::DeclareInputParameter(Value& boundValue, const char* name, const char* description)
    {
        auto it = m_Parameters.find(name);
        if (it != m_Parameters.end())
            throw Exception(Status::DuplicateOption);


        std::shared_ptr<CInputParameter> pParameter = std::make_shared<CInputParameter>(boundValue, name, description);

        // Add the parameter option to the declared options map
        m_Parameters.insert(std::make_pair(name, pParameter));

        // Add to the array of parameter options
        m_InputParameters.push_back(pParameter);
        return m_InputParameters.back().get();
    }

    //------------------------------------------------------------------------------------------------
    CCommandReader::CCommandReader(const char* appName, const char *defaultDescription, int argc, const char* argv[]) :
        m_RootCommand(appName, defaultDescription, 0),
        m_ArgList(argc, argv)
    {}

    //------------------------------------------------------------------------------------------------
    CCommand *CCommandReader::ReadCommandArguments()
    {
        m_ArgIt = m_ArgList.Begin();

        CCommand *pCommand = &m_RootCommand;

        ++m_ArgIt;
        while (m_ArgIt != m_ArgList.End())
        {
            auto subIt = pCommand->m_Subcommands.find(m_ArgList.At(m_ArgIt));
            if (subIt == pCommand->m_Subcommands.end())
                break;
            pCommand = subIt->second.get();
            ++m_ArgIt;
        }

        m_pActiveCommand = pCommand;

        return m_pActiveCommand;
    }

    //------------------------------------------------------------------------------------------------
    Status CCommandReader::ReadParameterArguments()
    {
        if (!m_pActiveCommand)
            ReadCommandArguments();

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
                const CParameter* pParameter = nullptr;

                if (m_ArgList.At(m_ArgIt)[1] == '-')
                {
                    // Long-form option
                    auto optIt = m_pActiveCommand->m_Parameters.find(m_ArgList.At(m_ArgIt).substr(2));
                    if (optIt == m_pActiveCommand->m_Parameters.end() || optIt->second->Type() == ParameterType::Input)
                    {
                        result = Status::UnknownOption;
                        return result;
                    }
                    pParameter = optIt->second.get();
                }
                else
                {
                    auto optIt = m_pActiveCommand->m_ShortOptions.find(m_ArgList.At(m_ArgIt)[1]);
                    if (optIt == m_pActiveCommand->m_ShortOptions.end())
                    {
                        result = Status::UnknownOption;
                        return result;
                    }
                    pParameter = optIt->second.get();
                }

                if (pParameter)
                {
                    result = pParameter->ParseArgs(m_ArgList, m_ArgIt);
                    if (result != Status::Success)
                        return result;
                }
            }
            else
            {
                // Assume parameter option
                if (m_ParametersRead == m_pActiveCommand->m_InputParameters.size())
                {
                    result = Status::UnexpectedArgument;
                    return result;
                }

                result = m_pActiveCommand->m_InputParameters[m_ParametersRead]->ParseArgs(m_ArgList, m_ArgIt);
                if (result != Status::Success)
                    return result;
                m_ParametersRead++;
            }
        }

        return result;
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
