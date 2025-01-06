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
    enum class ArgumentType
    {
        Category,
        Variable,
        Switch,
        Parameter,
    };
    
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
        NotFound,
        InvalidHandle,
    };

    //------------------------------------------------------------------------------------------------
    struct ReadErrorDesc
    {
        Status ErrorStatus;
        int ArgIndex;
        std::string ArgString;
        const void *ContextPtr;
    };

    //------------------------------------------------------------------------------------------------
    template<ArgumentType Type>
    class HandleHasher;

    //------------------------------------------------------------------------------------------------
    template<ArgumentType Type>
    class Handle
    {
        friend class CCommandReader;
        friend class HandleHasher<Type>;
        size_t m_Value;

    public:
        explicit Handle(size_t value) : m_Value(value) {}
        bool operator<(const Handle &o) const { return m_Value < o.m_Value; }
        bool operator==(const Handle &o) const { return m_Value == o.m_Value; }
        bool operator!=(const Handle &o) const { return m_Value != o.m_Value; }
    };

    //------------------------------------------------------------------------------------------------
    template<ArgumentType Type>
    class HandleHasher
    {
    public:
        size_t operator()(const Handle<Type> &h) const { return h.m_Value; }
    };

    //------------------------------------------------------------------------------------------------
    using CategoryHandle = Handle<ArgumentType::Category>;
    using ParameterHandle = Handle<ArgumentType::Parameter>;
    using VariableHandle = Handle<ArgumentType::Variable>;
    using SwitchHandle = Handle<ArgumentType::Switch>;

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

        Status GetStatus() const { return m_Status; }
        const std::string &GetMessage() const { return m_Message; }
    };

    //------------------------------------------------------------------------------------------------
    class CCommandExpression
    {
        friend class CCommandReader;

        struct CategoryLevel
        {
            CategoryLevel(CategoryHandle category) :
                Category(category)
            {
            }
            
            CategoryHandle Category;
            size_t ParameterCount = 0;
        };

        std::vector<CategoryLevel> m_CategoryLevels;
        std::unordered_map<VariableHandle, std::string, HandleHasher<ArgumentType::Variable>> m_VariableMap;
        std::unordered_map<ParameterHandle, std::string, HandleHasher<ArgumentType::Parameter>> m_ParameterMap;
        std::unordered_set<SwitchHandle, HandleHasher<ArgumentType::Switch>> m_Switches;

        size_t AddCategoryLevel(CategoryHandle category)
        {
            size_t levelIndex = m_CategoryLevels.size();
            m_CategoryLevels.push_back(category);
            return levelIndex;
        }

    public:
        CCommandExpression() = default;

        CategoryHandle GetCategory() const
        {
            return m_CategoryLevels.back().Category;
        }

        const std::string &GetParameterValue(ParameterHandle parameter, const std::string &defaultValue) const
        {
            auto it = m_ParameterMap.find(parameter);
            if (it == m_ParameterMap.end())
                return defaultValue;
            
            return it->second;
        }

        const std::string &GetVariableValue(VariableHandle variable, const std::string &defaultValue) const
        {
            auto it = m_VariableMap.find(variable);
            if (it == m_VariableMap.end())
                return defaultValue;

            return it->second;
        }

        bool GetParameterIsSet(ParameterHandle parameter) const
        {
            auto it = m_ParameterMap.find(parameter);
            return it != m_ParameterMap.end();
        }

        bool GetVariableIsSet(VariableHandle variable) const
        {
            auto it = m_VariableMap.find(variable);
            return it != m_VariableMap.end();
        }

        bool GetSwitchIsSet(SwitchHandle sh) const
        {
            auto it = m_Switches.find(sh);
            return it != m_Switches.end();
        }
    };

    inline const CategoryHandle RootCategory = CategoryHandle(0);
    inline const CategoryHandle NullCategory = CategoryHandle(size_t(0) - 1);

    //------------------------------------------------------------------------------------------------
    class CCommandReader
    {
        struct OptionDesc
        {
            ArgumentType Type;
            std::string Name;
            std::string Description;
            std::set<std::string> Domain;

            OptionDesc(ArgumentType type, const std::string &name, const std::string &description) :
                Type(type),
                Name(name),
                Description(description)
            {
            }

            OptionDesc(ArgumentType type, const std::string &name, const std::string &description, const std::vector<std::string> domain) :
                Type(type),
                Name(name),
                Description(description),
                Domain(std::set<std::string>(domain.begin(), domain.end()))
            {
            }
        };

        struct CategoryDesc
        {
            CategoryDesc(CategoryHandle parent, const std::string &name, const std::string &description) :
                Parent(parent),
                Name(name),
                Description(description)
            {
            }
            
            CategoryHandle Parent;
            std::string Name;
            std::string Description;
            std::map<std::string, CategoryHandle> SubCategoryMap;
            std::map<std::string, size_t> OptionDescIndexByNameMap;
            std::unordered_map<char, size_t> OptionDescIndexByShortNameMap;
            std::vector<size_t> ParameterIds;
        };

        CategoryDesc &CategoryDescThrow(size_t categoryIndex) // throw Exception
        {
            if (categoryIndex >= m_CategoryDescs.size())
                throw Exception(Status::OutOfRange);

            return m_CategoryDescs[categoryIndex];
        }

        size_t AddVariableOrSwitchOption(
            ArgumentType type,
            CategoryHandle category,
            const std::string &name,
            char shortName,
            const std::vector<std::string> &domain,
            const std::string &description);

    private:
        std::vector<CategoryDesc> m_CategoryDescs;
        std::vector<OptionDesc> m_OptionsDescs;
        ReadErrorDesc m_LastReadError;

    public:
        CCommandReader(const std::string appName) :
            m_CategoryDescs(1, CategoryDesc(NullCategory, appName, ""))
        {
        }

        CategoryHandle DeclareCategory(CategoryHandle parent, const std::string &name, const std::string &description = std::string())
        {
            if (parent.m_Value >= m_CategoryDescs.size())
                throw Exception(Status::InvalidHandle);

            CategoryHandle category = CategoryHandle(m_CategoryDescs.size());
            m_CategoryDescs.emplace_back(parent, name, description);
            m_CategoryDescs[parent.m_Value].SubCategoryMap.emplace(name, category);
            return category;
        }

        CategoryHandle DeclareCategory(const std::string &name, const std::string &description = std::string())
        {
            return DeclareCategory(RootCategory, name, description);
        }

        ParameterHandle DeclareParameter(CategoryHandle category, const std::string &name, const std::string &description = std::string())
        {
            if (category.m_Value >= m_CategoryDescs.size())
                throw Exception(Status::OutOfRange);
            size_t index = m_OptionsDescs.size();
            m_OptionsDescs.emplace_back(ArgumentType::Parameter, name, description);
            m_CategoryDescs[category.m_Value].ParameterIds.push_back(index);
            return ParameterHandle(index);
        }

        ParameterHandle DeclareParameter(const std::string &name, const std::string &description = std::string())
        {
            return DeclareParameter(RootCategory, name, description);
        }

        VariableHandle DeclareVariable(CategoryHandle category, const std::string &name, const std::string &description = std::string())
        {
            return VariableHandle(AddVariableOrSwitchOption(ArgumentType::Variable, category, name, '-', {}, description));
        }

        VariableHandle DeclareVariable(CategoryHandle category, const std::string &name, const std::vector<std::string> &domain, const std::string &description = std::string())
        {
            return VariableHandle(AddVariableOrSwitchOption(ArgumentType::Variable, category, name, '-', domain, description));
        }

        VariableHandle DeclareVariable(const std::string &name, const std::vector<std::string> &domain, const std::string &description = std::string())
        {
            return DeclareVariable(RootCategory, name, domain, description);
        }

        VariableHandle DeclareVariable(CategoryHandle category, const std::string &name, char shortName, const std::string &description = std::string())
        {
            return VariableHandle(AddVariableOrSwitchOption(ArgumentType::Variable, category, name, shortName, {}, description));
        }

        VariableHandle DeclareVariable(const std::string &name, char shortName, const std::string &description = std::string())
        {
            return DeclareVariable(RootCategory, name, shortName, description);
        }

        VariableHandle DeclareVariable(CategoryHandle category, const std::string &name, char shortName, const std::vector<std::string> &domain, const std::string &description = std::string())
        {
            return VariableHandle(AddVariableOrSwitchOption(ArgumentType::Variable, category, name, shortName, domain, description));
        }
        
        VariableHandle DeclareVariable(const std::string &name, char shortName, const std::vector<std::string> &domain, const std::string &description = std::string())
        {
            return DeclareVariable(RootCategory, name, shortName, domain, description);
        }

        SwitchHandle DeclareSwitch(CategoryHandle category, const std::string &name, const std::string &description = std::string())
        {
            return SwitchHandle(AddVariableOrSwitchOption(ArgumentType::Switch, category, name, '-', {}, description));
        }

        SwitchHandle DeclareSwitch(const std::string &name, const std::string &description = std::string())
        {
            return DeclareSwitch(RootCategory, name, description);
        }

        SwitchHandle DeclareSwitch(CategoryHandle category, const std::string &name, char shortName, const std::string &description = std::string())
        {
            return SwitchHandle(AddVariableOrSwitchOption(ArgumentType::Switch, category, name, shortName, {}, description));
        }
        
        SwitchHandle DeclareSwitch(const std::string &name, char shortName, const std::string &description = std::string())
        {
            return DeclareSwitch(RootCategory, name, shortName, description);
        }

        Status ReadCommandExpression(int argc, const char *argv[], CCommandExpression &commandExpression);
        std::string SimpleUsageString(CategoryHandle category) const;
        std::string OptionDetailsString(CategoryHandle category) const;
        Status SetLastReadError(Status status, int argIndex, const char *argv[], const void *contextPtr)
        {
            m_LastReadError.ErrorStatus = status;
            m_LastReadError.ArgIndex = argIndex;
            m_LastReadError.ArgString = argv[argIndex];
            m_LastReadError.ContextPtr = contextPtr;
            return status;
        }

        Status GetLastReadError(std::string &errorString) const;
    };
}