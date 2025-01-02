#include <sstream>
#include <iomanip>
#include <stack>

#include "InCommand.h"

namespace InCommand
{
    //------------------------------------------------------------------------------------------------
    size_t CCommandReader::AddVariableOrSwitchOption(
        OptionType type,
        size_t categoryId,
        const std::string &name,
        const std::string &description,
        const std::vector<std::string> &domain,
        char shortName)
    {
        if (categoryId >= m_CategoryDescs.size())
            throw Exception(Status::OutOfRange);

        auto &categoryDesc = m_CategoryDescs[categoryId];

        if (categoryDesc.OptionDescIdByNameMap.find(name) != categoryDesc.OptionDescIdByNameMap.end())
            throw Exception(Status::DuplicateOption);

        size_t optionId = m_NextOptionId;
        ++m_NextOptionId;
        if(type == OptionType::Variable && domain.size() > 0)
            categoryDesc.OptionDescsMap.emplace(
                std::piecewise_construct,
                std::forward_as_tuple(optionId),
                std::forward_as_tuple(OptionType::Variable, name, description, domain));
        else
            categoryDesc.OptionDescsMap.emplace(
                std::piecewise_construct,
                std::forward_as_tuple(optionId),
                std::forward_as_tuple(type, name, description));

        categoryDesc.OptionDescIdByNameMap.emplace(name, optionId);
        if (shortName != '-')
            categoryDesc.OptionDescIdByShortNameMap.emplace(shortName, optionId);

        return optionId;
    }

    //------------------------------------------------------------------------------------------------
    Status CCommandReader::ReadCommandExpression(int argc, const char *argv[], CCommandExpression &commandExpression)
    {
        size_t categoryIndex = 0;
        size_t levelIndex = commandExpression.AddCategoryLevel(categoryIndex);
        // Assume the first argument is the app name so select the root category
        for (int i = 1; i < argc; ++i)
        {
            auto &categoryLevel = commandExpression.m_CategoryLevels[levelIndex];

            std::string arg(argv[i]);
            CategoryDesc &categoryDesc = m_CategoryDescs[categoryIndex];

            // Is this a variable or switch?
            if (arg[0] == '-')
            {
                size_t optionId;

                // Is this a short or long name
                if (arg[1] == '-')
                {
                    // Long name
                    std::string name(arg.substr(2));
                    auto it = categoryDesc.OptionDescIdByNameMap.find(name);
                    if (it == categoryDesc.OptionDescIdByNameMap.end())
                        return Status::UnknownOption;
                    
                    optionId = it->second;
                }
                else
                {
                    // Short name
                    if (arg[1] == '\0' || arg[2] != '\0')
                        return Status::UnexpectedArgument;
                    
                    auto it = categoryDesc.OptionDescIdByShortNameMap.find(arg[1]);
                    if (it == categoryDesc.OptionDescIdByShortNameMap.end())
                        return Status::UnknownOption;

                    optionId = it->second;
                }

                const OptionDesc &variableDesc = categoryDesc.OptionDescsMap.at(optionId);

                if (variableDesc.Type == OptionType::Variable)
                {
                    // Read the value
                    ++i;
                    if (i >= argc)
                        return Status::MissingVariableValue;
                    
                    std::string value(argv[i]);

                    if(value[0] == '-')
                        return Status::MissingVariableValue;

                    if (variableDesc.Domain.size() > 0)
                    {
                        // Verify the value is in the declared domain
                        auto dit = variableDesc.Domain.find(value);

                        if (dit == variableDesc.Domain.end())
                            return Status::InvalidValue;
                    }
                    
                    commandExpression.m_VariableAndParameterMap.emplace(optionId, value);
                }
                else
                {
                    commandExpression.m_Switches.emplace(optionId);
                }
            }
            else
            {
                // Is this a sub-category?
                auto it = categoryDesc.SubCategoryMap.find(argv[i]);
                if (it != categoryDesc.SubCategoryMap.end())
                {
                    levelIndex = commandExpression.AddCategoryLevel(it->second);
                    categoryIndex = it->second;
                }
                else
                {
                    if (categoryLevel.ParameterCount == categoryDesc.ParameterIds.size())
                    {
                        return Status::UnexpectedArgument;
                    }
                    else
                    {
                        size_t parameterId = categoryDesc.ParameterIds[categoryLevel.ParameterCount];
                        commandExpression.m_VariableAndParameterMap.emplace(parameterId, argv[i]);
                        categoryLevel.ParameterCount++;
                    }
                }
            }
        }

        return Status::Success;
    }

    //------------------------------------------------------------------------------------------------
    std::string CCommandReader::SimpleUsageString(size_t categoryId) const
    {
        std::ostringstream s;
        const CategoryDesc &catDesc = m_CategoryDescs[categoryId];

        for (auto it = catDesc.SubCategoryMap.begin(); it != catDesc.SubCategoryMap.end(); ++it)
        {
            s << SimpleUsageString(it->second);
        }

        std::stack<size_t> categoryStack;
        for (size_t id = categoryId; id != size_t(0 - 1); id = m_CategoryDescs[id].ParentId)
        {
            categoryStack.push(id);
        }

        while (!categoryStack.empty())
        {
            s << m_CategoryDescs[categoryStack.top()].Name;
            categoryStack.pop();
            s << " ";
        }

        // Parameters
        for (auto it = catDesc.ParameterDescsMap.begin(); it != catDesc.ParameterDescsMap.end(); ++it)
        {
            s << "[<" << it->second.Name << ">]";
            s << " ";
        }

        // Switches and Variables
        for (auto it = catDesc.OptionDescsMap.begin(); it != catDesc.OptionDescsMap.end(); ++it)
        {
            s << "[--" << it->second.Name;
            if (it->second.Type == OptionType::Variable)
                s << " <value>";
            s << "] ";
        }

        s << std::endl;

        return s.str();
    }

    std::string CCommandReader::OptionDetailsString(size_t categoryId) const
    {
        std::ostringstream s;
        const CategoryDesc &catDesc = m_CategoryDescs[categoryId];
        static const int colwidth = 30;

        // For the given category level, list out all options with
        // a non-empty description.

        // Parameters
        for (auto it = catDesc.ParameterDescsMap.begin(); it != catDesc.ParameterDescsMap.end(); ++it)
        {
            if (!it->second.Description.empty())
            {
                s << std::setw(colwidth) << std::left << "  " + it->second.Name << it->second.Description << std::endl;
            }
        }

        // Switches and Variables
        for (auto it = catDesc.OptionDescsMap.begin(); it != catDesc.OptionDescsMap.end(); ++it)
        {
            if (!it->second.Description.empty())
            {
                s << std::setw(colwidth) << std::left << "  --" + it->second.Name;
                if (it->second.Name.length() + 4 > colwidth)
                {
                    s << std::endl;
                    s << std::setw(colwidth) << ' ';
                }
                s << it->second.Description << std::endl;
            }
        }

        s << std::endl;

        return s.str();
    }

    //------------------------------------------------------------------------------------------------
    // std::string CCommandReader::UsageString(size_t) const
    // {
    //     return std::string();

        // std::ostringstream s;
        // s << "USAGE:" << std::endl;
        // s << std::endl;
        // static const int colwidth = 30;

        // if (!m_Options.empty())
        // {
        //     s << "  " + commandScopeString;

        //     // Option arguments first
        //     for (auto& nko : m_InputOptions)
        //     {
        //         s << " <" << nko->Name() << ">";
        //     }

        //     s << " [<options>]" << std::endl;
        // }

        // if (!m_InnerCommands.empty())
        // {
        //     s << "  " + commandScopeString;

        //     s << " [<command> [<options>]]" << std::endl;
        // }

        // s << std::endl;

        // if (!m_InnerCommands.empty())
        // {
        //     s << "COMMANDS" << std::endl;
        //     s << std::endl;

        //     for (auto subIt : m_InnerCommands)
        //     {
        //         s << std::setw(colwidth) << std::left << "  " + subIt.second->Name();
        //         s << subIt.second->m_Description << std::endl;
        //     };
        // }

        // if (!m_Options.empty())
        // {
        //     // Input options first
        //     for (auto& inputOption : m_InputOptions)
        //     {
        //         s << " <" << inputOption->Name() << ">";
        //     }
        // }

        // s << std::endl;

        // s << std::endl;

        // if (!m_Options.empty())
        // {
        //     // Option first
        //     s << "OPTIONS" << std::endl;
        //     s << std::endl;

        //     // Option details
        //     for (auto& inputOption : m_InputOptions)
        //     {
        //         s << std::setw(colwidth) << std::left << "  " + OptionUsageString(inputOption.get());
        //         s << inputOption->Description() << std::endl;
        //     }

        //     // Keyed-options details
        //     for (auto& ko : m_Options)
        //     {
        //         auto type = ko.second->Type();
        //         if (type == OptionType::Input)
        //             continue; // Already dumped
        //         s << std::setw(colwidth) << std::left << "  " + OptionUsageString(ko.second.get());
        //         if (OptionUsageString(ko.second.get()).length() + 4 > colwidth)
        //         {
        //             s << std::endl;
        //             s << std::setw(colwidth) << ' ';
        //         }
        //         s << ko.second->Description() << std::endl;
        //     }

        //     s << std::endl;
        // }

        // return s.str();
    // }
}
