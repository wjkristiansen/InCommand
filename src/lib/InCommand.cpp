#include <sstream>
#include <iomanip>
#include <stack>

#include "InCommand.h"

namespace InCommand
{
    //------------------------------------------------------------------------------------------------
    size_t CCommandReader::AddVariableOrSwitchOption(
        ArgumentType type,
        CategoryHandle category,
        const std::string &name,
        char shortName,
        const std::vector<std::string> &domain,
        const std::string &description)
    {
        if (category.m_Value >= m_CategoryDescs.size())
            throw Exception(Status::InvalidHandle);

        auto &categoryDesc = m_CategoryDescs[category.m_Value];

        if (categoryDesc.OptionDescIndexByNameMap.find(name) != categoryDesc.OptionDescIndexByNameMap.end())
            throw Exception(Status::DuplicateOption);

        size_t optionIndex = m_OptionsDescs.size();
        if(type == ArgumentType::Variable && domain.size() > 0)
            m_OptionsDescs.emplace_back(ArgumentType::Variable, name, description, domain);
        else
            m_OptionsDescs.emplace_back(type, name, description);

        categoryDesc.OptionDescIndexByNameMap.emplace(name, optionIndex);
        if (shortName != '-')
            categoryDesc.OptionDescIndexByShortNameMap.emplace(shortName, optionIndex);

        return optionIndex;
    }

    //------------------------------------------------------------------------------------------------
    Status CCommandReader::ReadCommandExpression(int argc, const char *argv[], CCommandExpression &commandExpression)
    {
        bool ignoreSwitchesAndVariables = false;
        size_t categoryIndex = 0;
        size_t levelIndex = commandExpression.AddCategoryLevel(RootCategory);
        // Assume the first argument is the app name so select the root category
        for (int i = 1; i < argc; ++i)
        {
            auto &categoryLevel = commandExpression.m_CategoryLevels[levelIndex];

            std::string arg(argv[i]);
            CategoryDesc &categoryDesc = m_CategoryDescs[categoryIndex];

            // Is this a variable or switch?
            if (!ignoreSwitchesAndVariables && arg[0] == '-')
            {
                size_t optionIndex;

                // Is this a short or long name
                if (arg[1] == '-')
                {
                    // Long name
                    std::string name(arg.substr(2));
                    if (name.empty())
                    {
                        ignoreSwitchesAndVariables = true;
                        continue;
                    }
                    
                    auto it = categoryDesc.OptionDescIndexByNameMap.find(name);
                    if (it == categoryDesc.OptionDescIndexByNameMap.end())
                        return Status::UnknownOption;
                    
                    optionIndex = it->second;
                }
                else
                {
                    // Short name
                    if (arg[1] == '\0' || arg[2] != '\0')
                        return Status::UnexpectedArgument;
                    
                    auto it = categoryDesc.OptionDescIndexByShortNameMap.find(arg[1]);
                    if (it == categoryDesc.OptionDescIndexByShortNameMap.end())
                        return Status::UnknownOption;

                    optionIndex = it->second;
                }

                const OptionDesc &optionDesc = m_OptionsDescs.at(optionIndex);

                if (optionDesc.Type == ArgumentType::Variable)
                {
                    // Read the value
                    ++i;
                    if (i >= argc)
                        return Status::MissingVariableValue;
                    
                    std::string value(argv[i]);

                    if(value[0] == '-')
                        return Status::MissingVariableValue;

                    if (optionDesc.Domain.size() > 0)
                    {
                        // Verify the value is in the declared domain
                        auto dit = optionDesc.Domain.find(value);

                        if (dit == optionDesc.Domain.end())
                            return Status::InvalidValue;
                    }
                    
                    commandExpression.m_VariableMap.emplace(VariableHandle(optionIndex), value);
                }
                else
                {
                    commandExpression.m_Switches.emplace(SwitchHandle(optionIndex));
                }
            }
            else
            {
                // Is this a sub-category?
                auto it = categoryDesc.SubCategoryMap.find(argv[i]);
                if (it != categoryDesc.SubCategoryMap.end())
                {
                    levelIndex = commandExpression.AddCategoryLevel(it->second);
                    categoryIndex = it->second.m_Value;
                }
                else
                {
                    if (categoryLevel.ParameterCount == categoryDesc.ParameterIds.size())
                    {
                        return Status::UnexpectedArgument;
                    }
                    else
                    {
                        ParameterHandle parameter = ParameterHandle(categoryDesc.ParameterIds[categoryLevel.ParameterCount]);
                        commandExpression.m_ParameterMap.emplace(parameter, argv[i]);
                        categoryLevel.ParameterCount++;
                    }
                }
            }
        }

        return Status::Success;
    }

    //------------------------------------------------------------------------------------------------
    std::string CCommandReader::SimpleUsageString(CategoryHandle category) const
    {
        std::ostringstream s;
        const CategoryDesc &catDesc = m_CategoryDescs[category.m_Value];

        for (auto it = catDesc.SubCategoryMap.begin(); it != catDesc.SubCategoryMap.end(); ++it)
        {
            s << SimpleUsageString(it->second);
        }

        std::stack<CategoryHandle> categoryStack;
        for (CategoryHandle ch = category; ch != NullCategory; ch = m_CategoryDescs[ch.m_Value].Parent)
        {
            categoryStack.push(ch);
        }

        while (!categoryStack.empty())
        {
            s << m_CategoryDescs[categoryStack.top().m_Value].Name;
            categoryStack.pop();
            s << " ";
        }

        // Parameters
        for (auto it = catDesc.ParameterIds.begin(); it != catDesc.ParameterIds.end(); ++it)
        {
            const OptionDesc &parameterDesc = m_OptionsDescs[*it];
            s << "[<" << parameterDesc.Name << ">]";
            s << " ";
        }

        // Switches and Variables
        for (auto it = catDesc.OptionDescIndexByNameMap.begin(); it != catDesc.OptionDescIndexByNameMap.end(); ++it)
        {
            const OptionDesc &desc = m_OptionsDescs[it->second];
            s << "[--" << desc.Name;
            if (desc.Type == ArgumentType::Variable)
                s << " <value>";
            s << "] ";
        }

        s << std::endl;

        return s.str();
    }

    std::string CCommandReader::OptionDetailsString(CategoryHandle category) const
    {
        std::ostringstream s;
        const CategoryDesc &catDesc = m_CategoryDescs[category.m_Value];
        static const int colwidth = 30;

        // For the given category level, list out all options with
        // a non-empty description.

        // Parameters
        for (auto it = catDesc.ParameterIds.begin(); it != catDesc.ParameterIds.end(); ++it)
        {
            const OptionDesc &parameterDesc = m_OptionsDescs[*it];
            if (!parameterDesc.Description.empty())
            {
                s << std::setw(colwidth) << std::left << "  " + parameterDesc.Name << parameterDesc.Description << std::endl;
            }
        }

        // Switches and Variables
        for (auto it = catDesc.OptionDescIndexByNameMap.begin(); it != catDesc.OptionDescIndexByNameMap.end(); ++it)
        {
            const OptionDesc &desc = m_OptionsDescs[it->second];
            if (!desc.Description.empty())
            {
                s << std::setw(colwidth) << std::left << "  --" + desc.Name;
                if (desc.Name.length() + 4 > colwidth)
                {
                    s << std::endl;
                    s << std::setw(colwidth) << ' ';
                }
                s << desc.Description << std::endl;
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
        //         if (type == ArgumentType::Input)
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
