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
            if (pParameter->Type() == ParameterType::Switch)
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

        if (!m_Parameters.empty())
        {
            s << "  " + commandScopeString;

            // Parameter options first
            for (auto& nko : m_InputParameters)
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

        if (!m_Parameters.empty())
        {
            // Input parameters first
            for (auto& inputParam : m_InputParameters)
            {
                s << " <" << inputParam->Name() << ">";
            }
        }

        s << std::endl;

        s << std::endl;

        if (!m_Parameters.empty())
        {
            // Parameter options first
            s << "OPTIONS" << std::endl;
            s << std::endl;

            // Parameter options details
            for (auto& inputParam : m_InputParameters)
            {
                s << std::setw(colwidth) << std::left << "  " + ParameterUsageString(inputParam.get());
                s << inputParam->Description() << std::endl;
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
    const CParameter* CCommand::DeclareBoolSwitchParameter(Bool& boundValue, const char* name, const char *description, char shortKey)
    {
        auto it = m_Parameters.find(name);
        if (it != m_Parameters.end())
            throw Exception(Status::DuplicateParameter);

        auto insert = m_Parameters.emplace(name, std::make_shared<CBoolParameter>(boundValue, name, description, shortKey));
        if (shortKey)
            m_ShortParameters.insert(std::make_pair(shortKey, insert.first->second));
        return insert.first->second.get();
    }

    //------------------------------------------------------------------------------------------------
    const CParameter* CCommand::DeclareSwitchParameter(Value& boundValue, const char* name, const char *description, char shortKey)
    {
        auto it = m_Parameters.find(name);
        if (it != m_Parameters.end())
            throw Exception(Status::DuplicateParameter);

        auto insert = m_Parameters.emplace(name, std::make_shared<CSwitchParameter>(boundValue, name, description, shortKey));
        if (shortKey)
            m_ShortParameters.insert(std::make_pair(shortKey, insert.first->second));
        return insert.first->second.get();
    }

    //------------------------------------------------------------------------------------------------
    const CParameter* CCommand::DeclareSwitchParameter(Value& boundValue, const char* name, int domainSize, const char* domainValueStrings[], const char *description, char shortKey)
    {
        auto it = m_Parameters.find(name);
        if (it != m_Parameters.end())
            throw Exception(Status::DuplicateParameter);

        Domain domain(domainSize, domainValueStrings);
        auto insert = m_Parameters.emplace(name, std::make_shared<CSwitchParameter>(boundValue, name, domain, description, shortKey));
        if (shortKey)
            m_ShortParameters.insert(std::make_pair(shortKey, insert.first->second));
        return insert.first->second.get();
    }

    //------------------------------------------------------------------------------------------------
    const CParameter* CCommand::DeclareSwitchParameter(Value& boundValue, const char* name, const Domain &domain, const char* description, char shortKey)
    {
        auto it = m_Parameters.find(name);
        if (it != m_Parameters.end())
            throw Exception(Status::DuplicateParameter);

        auto insert = m_Parameters.emplace(name, std::make_shared<CSwitchParameter>(boundValue, name, domain, description, shortKey));
        if (shortKey)
            m_ShortParameters.insert(std::make_pair(shortKey, insert.first->second));
        return insert.first->second.get();
    }

    //------------------------------------------------------------------------------------------------
    const CParameter* CCommand::DeclareInputParameter(Value& boundValue, const char* name, const char* description)
    {
        auto it = m_Parameters.find(name);
        if (it != m_Parameters.end())
            throw Exception(Status::DuplicateParameter);


        std::shared_ptr<CInputParameter> pParameter = std::make_shared<CInputParameter>(boundValue, name, description);

        // Add the parameter option to the declared options map
        m_Parameters.insert(std::make_pair(name, pParameter));

        // Add to the array of parameter options
        m_InputParameters.push_back(pParameter);
        return m_InputParameters.back().get();
    }

    //------------------------------------------------------------------------------------------------
    CCommandReader::CCommandReader(const char* appName, const char *defaultDescription, int argc, const char* argv[]) :
        m_ArgList(argc, argv),
        CCommand(appName, defaultDescription)
    {
    }

    //------------------------------------------------------------------------------------------------
    CCommand *CCommandReader::PreReadCommandArguments()
    {
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
    Status CCommandReader::ReadParameterArguments(const CCommand *pCommand)
    {
        if (!pCommand)
            pCommand = PreReadCommandArguments();

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
                    auto optIt = pCommand->m_Parameters.find(m_ArgList.At(m_ArgIt).substr(2));
                    if (optIt == pCommand->m_Parameters.end() || optIt->second->Type() == ParameterType::Input)
                    {
                        result = Status::UnknownParameter;
                        return result;
                    }
                    pParameter = optIt->second.get();
                }
                else
                {
                    auto optIt = pCommand->m_ShortParameters.find(m_ArgList.At(m_ArgIt)[1]);
                    if (optIt == pCommand->m_ShortParameters.end())
                    {
                        result = Status::UnknownParameter;
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
                if (m_InputParameterArgsRead == pCommand->m_InputParameters.size())
                {
                    result = Status::UnexpectedArgument;
                    return result;
                }

                result = pCommand->m_InputParameters[m_InputParameterArgsRead]->ParseArgs(m_ArgList, m_ArgIt);
                if (result != Status::Success)
                    return result;
                m_InputParameterArgsRead++;
            }
        }

        return result;
    }

    //------------------------------------------------------------------------------------------------
    Status CCommandReader::ReadArguments()
    {
        CCommand *pCommand = PreReadCommandArguments();

        return ReadParameterArguments(pCommand);
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

        case Status::MissingParameterValue:
            s << "Missing Parameter Value";
            break;

        case Status::UnknownParameter:
            s << "Unknown Parameter: \"" << m_ArgList.At(m_ArgIt) << "\"";
            break;

        default:
            s << "Unknown Error";
            break;
        }

        return s.str();

    }

}
