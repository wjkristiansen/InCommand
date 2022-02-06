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
	int CCommandScope::ParseArguments( int argc, const char* argv[], int index)
	{
		if (argc <= index)
			return 0;

		// Is the first argument a subcommand?
		auto subIt = m_Subcommands.find(argv[index]);
		if (subIt != m_Subcommands.end())
		{
			int result = subIt->second->ParseArguments(argc, argv, index + 1);
			m_ActiveCommandScopeId = subIt->second->GetActiveCommandScopeId();
			return result;
		}

		// Parse the options
		for (; index < argc;)
		{
			if (argv[index][0] == '-')
			{
				if (argv[index][1] == '-')
				{
					// Long-form option
					auto it = m_Options.find(argv[index] + 2);
					if(it == m_Options.end() || it->second->GetType() == OptionType::NonKeyed)
						throw InCommandException(InCommandError::UnexpectedArgument, argv[index], index);

					index = it->second->ParseArgs(argc, argv, index);
				}
				else
				{
					// TODO: Short-form option
					throw InCommandException(InCommandError::UnexpectedArgument, argv[index], index);
				}
			}
			else
			{
				// Assume non-keyed option
				if (m_NumPresentNonKeyed == m_NonKeyedOptions.size())
					throw InCommandException(InCommandError::UnexpectedArgument, argv[index], index);

				index = m_NonKeyedOptions[m_NumPresentNonKeyed]->ParseArgs(argc, argv, index);
				m_NumPresentNonKeyed++;
			}
		}

		m_ActiveCommandScopeId = m_ScopeId;
		m_IsActive = true;

		return argc;
	}

	//------------------------------------------------------------------------------------------------
	const COption& CCommandScope::GetOption(const char* name) const
	{
		auto it = m_Options.find(name);
		if (it == m_Options.end())
			throw InCommandException(InCommandError::UnknownOption, name, -1);

		return *it->second;
	}

	//------------------------------------------------------------------------------------------------
	CCommandScope& CCommandScope::DeclareSubcommand(const char* name, int scopeId)
	{
		auto it = m_Subcommands.find(name);
		if (it != m_Subcommands.end())
			throw InCommandException(InCommandError::DuplicateCommand, name, -1);

		auto result = m_Subcommands.emplace(name, std::make_unique<CCommandScope>(name, scopeId));
		return *result.first->second;
	}

	//------------------------------------------------------------------------------------------------
	const COption &CCommandScope::DeclareSwitchOption(const char* name)
	{
		auto it = m_Options.find(name);
		if (it != m_Options.end())
			throw InCommandException(InCommandError::DuplicateOption, name, -1);

		auto insert = m_Options.emplace(name, std::make_shared<CSwitchOption>(name, nullptr));
		return *insert.first->second;
	}

	//------------------------------------------------------------------------------------------------
	const COption &CCommandScope::DeclareVariableOption(const char* name, const char* defaultValue)
	{
		auto it = m_Options.find(name);
		if (it != m_Options.end())
			throw InCommandException(InCommandError::DuplicateOption, name, -1);

		auto insert = m_Options.emplace(name, std::make_shared<CVariableOption>(name, defaultValue, nullptr));
		return *insert.first->second;
	}

	//------------------------------------------------------------------------------------------------
	const COption &CCommandScope::DeclareVariableOption(const char* name, int domainSize, const char* domain[], int defaultIndex)
	{
		auto it = m_Options.find(name);
		if (it != m_Options.end())
			throw InCommandException(InCommandError::DuplicateOption, name, -1);

		auto insert = m_Options.emplace(name, std::make_shared<CVariableOption>(name, domainSize, domain, defaultIndex, nullptr));
		return *insert.first->second;
	}

	//------------------------------------------------------------------------------------------------
	const COption& CCommandScope::DeclareNonKeyedOption(const char* name)
	{
		auto it = m_Options.find(name);
		if (it != m_Options.end())
			throw InCommandException(InCommandError::DuplicateOption, name, -1);


        std::shared_ptr<CNonKeyedOption> pOption = std::make_shared<CNonKeyedOption>(name, nullptr);

        // Add the non-keyed option to the declared options map
		m_Options.insert(std::make_pair(name, pOption));

        // Add to the array of non-keyed options
		m_NonKeyedOptions.push_back(pOption);
		return *m_NonKeyedOptions.back();
	}
}
