#pragma once

#include <memory>
#include <vector>
#include <string>
#include <map>
#include <set>
#include <deque>
#include <unordered_set>
#include <unordered_map>
#include <ostream>
#include <sstream>
#include <optional>

namespace InCommand
{
    //------------------------------------------------------------------------------------------------
    enum class Status : int
    {
        Success,
        DuplicateCategory,
        DuplicateOption,
        UnexpectedArgument,
        UnknownOption,
        MissingVariableValue,
        TooManyParameters,
        InvalidValue,
        OutOfRange,
        NotFound
    };

    //------------------------------------------------------------------------------------------------
    class Exception
    {
        Status m_Status;
        std::string m_Message;

    public:
        Exception(Status status, const std::string &message = std::string()) :
            m_Status(status),
            m_Message(message)
        {
        }

        Status Status() const { return m_Status; }
        const std::string &Message() const { return m_Message; }
    };

    //------------------------------------------------------------------------------------------------
    class CCommandExpression
    {
        friend class CCommandReader;

        struct CategoryLevel
        {
            CategoryLevel(size_t categoryId) :
                CategoryId(categoryId)
            {
            }
            
            size_t CategoryId;
            size_t ParameterCount = 0;
        };

        std::vector<CategoryLevel> m_CategoryLevels;
        std::unordered_map<size_t, std::string> m_VariableAndParameterMap;
        std::unordered_set<size_t> m_Switches;

        size_t AddCategoryLevel(size_t categoryId)
        {
            size_t levelIndex = m_CategoryLevels.size();
            m_CategoryLevels.push_back(categoryId);
            return levelIndex;
        }

    public:
        CCommandExpression() = default;

        size_t GetCategoryDepth() const { return m_CategoryLevels.size(); }

        size_t GetCategoryId(size_t levelIndex) const
        {
            if (levelIndex > m_CategoryLevels.size())
                throw Exception(Status::OutOfRange);
            
            return m_CategoryLevels[levelIndex].CategoryId;
        }

        const std::string &GetParameterValue(size_t parameterId, const std::string &defaultValue) const
        {
            auto it = m_VariableAndParameterMap.find(parameterId);
            if (it == m_VariableAndParameterMap.end())
                return defaultValue;
            
            return it->second;
        }

        const std::string &GetVariableValue(size_t variableId, const std::string &defaultValue) const
        {
            auto it = m_VariableAndParameterMap.find(variableId);
            if (it == m_VariableAndParameterMap.end())
                return defaultValue;

            return it->second;
        }

        bool GetVariableIsSet(size_t variableId) const
        {
            auto it = m_VariableAndParameterMap.find(variableId);
            return it != m_VariableAndParameterMap.end();
        }

        bool GetSwitchIsSet(size_t switchId) const
        {
            auto it = m_Switches.find(switchId);
            return it != m_Switches.end();
        }
    };

    //------------------------------------------------------------------------------------------------
    class CCommandReader
    {
        enum class OptionType
        {
            Variable,
            Switch,
            Parameter,
        };
        
        struct OptionDesc
        {
            OptionType Type;
            std::string Name;
            std::string Description;
            std::set<std::string> Domain;

            OptionDesc(OptionType type, const std::string &name, const std::string &description) :
                Type(type),
                Name(name),
                Description(description)
            {
            }

            OptionDesc(OptionType type, const std::string &name, const std::string &description, const std::vector<std::string> domain) :
                Type(type),
                Name(name),
                Description(description),
                Domain(std::set<std::string>(domain.begin(), domain.end()))
            {
            }
        };

        struct CategoryDesc
        {
            CategoryDesc(size_t parentId, const std::string &name, const std::string &description) :
                ParentId(parentId),
                Name(name),
                Description(description)
            {
            }
            
            size_t ParentId;
            std::string Name;
            std::string Description;
            std::map<std::string, size_t> SubCategoryMap;
            std::map<std::string, size_t> OptionDescIdByNameMap;
            std::unordered_map<char, size_t> OptionDescIdByShortNameMap;
            std::unordered_map<size_t, OptionDesc> OptionDescsMap;
            std::unordered_map<size_t, OptionDesc> ParameterDescsMap;
            std::vector<size_t> ParameterIds;
        };

        CategoryDesc &CategoryDescThrow(size_t categoryId) // throw Exception
        {
            if (categoryId >= m_CategoryDescs.size())
                throw Exception(Status::OutOfRange);

            return m_CategoryDescs[categoryId];
        }

        size_t AddVariableOrSwitchOption(
            OptionType type,
            size_t categoryId,
            const std::string &name,
            const std::string &description,
            const std::vector<std::string> &domain,
            char shortName);

    private:
        size_t m_NextOptionId = 1;
        std::vector<CategoryDesc> m_CategoryDescs;

    public:
        CCommandReader(const std::string appName) :
            m_CategoryDescs(1, CategoryDesc(size_t(0 - 1), appName, ""))
        {
        }

        size_t DeclareCategory(size_t parentId, const std::string &name, const std::string &description)
        {
            if (parentId >= m_CategoryDescs.size())
                throw Exception(Status::OutOfRange);

            size_t id = m_CategoryDescs.size();
            m_CategoryDescs.emplace_back(parentId, name, description);
            m_CategoryDescs[parentId].SubCategoryMap.emplace(name, id);
            return id;
        }

        size_t DeclareParameter(size_t categoryId, const std::string &name, const std::string &description)
        {
            if (categoryId >= m_CategoryDescs.size())
                throw Exception(Status::OutOfRange);
            size_t index = m_NextOptionId;
            ++m_NextOptionId;
            m_CategoryDescs[categoryId].ParameterDescsMap.emplace(
                std::piecewise_construct,
                std::forward_as_tuple(index),
                std::forward_as_tuple(OptionType::Parameter, name, description));
            m_CategoryDescs[categoryId].ParameterIds.push_back(index);
            return index;
        }

        size_t DeclareVariable(size_t categoryId, const std::string &name, const std::string &description)
        {
            return AddVariableOrSwitchOption(OptionType::Variable, categoryId, name, description, {}, '-');
        }

        size_t DeclareVariable(size_t categoryId, const std::string &name, const std::string &description, const std::vector<std::string> &domain)
        {
            return AddVariableOrSwitchOption(OptionType::Variable, categoryId, name, description, domain, '-');
        }

        size_t DeclareVariable(size_t categoryId, const std::string &name, const std::string &description, char shortName)
        {
            return AddVariableOrSwitchOption(OptionType::Variable, categoryId, name, description, {}, shortName);
        }

        size_t DeclareVariable(size_t categoryId, const std::string &name, const std::string &description, const std::vector<std::string> &domain, char shortName)
        {
            return AddVariableOrSwitchOption(OptionType::Variable, categoryId, name, description, domain, shortName);
        }

        size_t DeclareSwitch(size_t categoryId, const std::string &name, const std::string &description)
        {
            return AddVariableOrSwitchOption(OptionType::Switch, categoryId, name, description, {}, '-');
        }

        size_t DeclareSwitch(size_t categoryId, const std::string &name, const std::string &description, char shortName)
        {
            return AddVariableOrSwitchOption(OptionType::Switch, categoryId, name, description, {}, shortName);
        }

        Status ReadCommandExpression(int argc, const char *argv[], CCommandExpression &commandExpression);
        std::string SimpleUsageString(size_t categoryId) const;
        std::string OptionDetailsString(size_t categoryId) const;
    };
}