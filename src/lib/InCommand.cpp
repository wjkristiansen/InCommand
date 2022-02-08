#include <iostream>

#include "InCommand.h"

namespace InCommand
{
	//------------------------------------------------------------------------------------------------
	CCommandScope::CCommandScope(const char* name, int scopeId) :
		m_ScopeId(scopeId)
	{
		if (name)
			m_Name = name;
	}

	//------------------------------------------------------------------------------------------------
	InCommandResult CCommandScope::FetchCommandScope(const CArgumentList& args, CArgumentIterator& it, CCommandScope ** pScope)
	{
		*pScope = this;

		if (it == args.End())
		{
			return InCommandResult::Success;
		}

		auto subIt = m_Subcommands.find(args.At(it));
		if (subIt != m_Subcommands.end())
		{
			++it;
			auto result = subIt->second->FetchCommandScope(args, it, pScope);
			if (result != InCommandResult::Success)
				return result;
		}

		return InCommandResult::Success;
	}

	//------------------------------------------------------------------------------------------------
	InCommandResult CCommandScope::FetchOptions(const CArgumentList& args, CArgumentIterator& it) const
	{
		if (it == args.End())
		{
			return InCommandResult::Success;
		}

		int NonKeyedOptionIndex = 0;

		// Parse the options
		while(it != args.End())
		{
			if (args.At(it)[0] == '-')
			{
				if (args.At(it)[1] == '-')
				{
					// Long-form option
					auto optIt = m_Options.find(args.At(it) + 2);
					if(optIt == m_Options.end() || optIt->second->GetType() == OptionType::NonKeyed)
						return InCommandResult::UnexpectedArgument;
					auto result = optIt->second->ParseArgs(args, it);
					if (result != InCommandResult::Success)
						return result;
				}
				else
				{
					// TODO: Short-form option
					return InCommandResult::UnexpectedArgument;
				}
			}
			else
			{
				// Assume non-keyed option
				if (NonKeyedOptionIndex == m_NonKeyedOptions.size())
					return InCommandResult::UnexpectedArgument;

				auto result = m_NonKeyedOptions[NonKeyedOptionIndex]->ParseArgs(args, it);
				if (result != InCommandResult::Success)
					return result;
				NonKeyedOptionIndex++;
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
	CCommandScope& CCommandScope::DeclareSubcommand(const char* name, int scopeId)
	{
		auto it = m_Subcommands.find(name);
		if (it != m_Subcommands.end())
			throw InCommandException(InCommandResult::DuplicateCommand);

		auto result = m_Subcommands.emplace(name, std::make_shared<CCommandScope>(name, scopeId));
		return *result.first->second;
	}

	//------------------------------------------------------------------------------------------------
	const COption &CCommandScope::DeclareSwitchOption(InCommandBool &value, const char* name)
	{
		auto it = m_Options.find(name);
		if (it != m_Options.end())
			throw InCommandException(InCommandResult::DuplicateOption);

		auto insert = m_Options.emplace(name, std::make_shared<CSwitchOption>(value, name, nullptr));
		return *insert.first->second;
	}

	//------------------------------------------------------------------------------------------------
	const COption &CCommandScope::DeclareVariableOption(InCommandString& value, const char* name)
	{
		auto it = m_Options.find(name);
		if (it != m_Options.end())
			throw InCommandException(InCommandResult::DuplicateOption);

		auto insert = m_Options.emplace(name, std::make_shared<CVariableOption>(value, name, nullptr));
		return *insert.first->second;
	}

	//------------------------------------------------------------------------------------------------
	const COption &CCommandScope::DeclareVariableOption(InCommandString& value, const char* name, int domainSize, const char* domain[])
	{
		auto it = m_Options.find(name);
		if (it != m_Options.end())
			throw InCommandException(InCommandResult::DuplicateOption);

		auto insert = m_Options.emplace(name, std::make_shared<CVariableOption>(value, name, domainSize, domain, nullptr));
		return *insert.first->second;
	}

	//------------------------------------------------------------------------------------------------
	const COption& CCommandScope::DeclareNonKeyedOption(InCommandString& value, const char* name)
	{
		auto it = m_Options.find(name);
		if (it != m_Options.end())
			throw InCommandException(InCommandResult::DuplicateOption);


        std::shared_ptr<CNonKeyedOption> pOption = std::make_shared<CNonKeyedOption>(value, name, nullptr);

        // Add the non-keyed option to the declared options map
		m_Options.insert(std::make_pair(name, pOption));

        // Add to the array of non-keyed options
		m_NonKeyedOptions.push_back(pOption);
		return *m_NonKeyedOptions.back();
	}
}
