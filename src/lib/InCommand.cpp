#include <iostream>

#include "InCommand.h"

namespace InCommand
{
	//------------------------------------------------------------------------------------------------
	CCommandScope::CCommandScope() :
		m_Prefix("--")
	{
	}

	//------------------------------------------------------------------------------------------------
	void CCommandScope::SetPrefix(const char* prefix)
	{
		m_Prefix = prefix;
	}

	//------------------------------------------------------------------------------------------------
	int CCommandScope::ParseArguments( int argc, const char* argv[], int index)
	{
		if (argc <= index)
			return 0;

		// Is the first argument a subcommand?
		auto subIt = m_Subcommands.find(argv[index]);
		if (subIt != m_Subcommands.end())
			return subIt->second->ParseArguments(argc, argv, index + 1);

		// Parse the options
		for (; index < argc;)
		{
			auto it = m_Options.find(argv[index]);
			if (it == m_Options.end())
			{
				if (m_NumPresentNonKeyed == m_NonKeyedOptions.size())
					throw InCommandException(InCommandError::UnexpectedArgument, argv[index], index);

				index = m_NonKeyedOptions[m_NumPresentNonKeyed]->ParseArgs(argc, argv, index);
				m_NumPresentNonKeyed++;
			}
			else
			{
				index = it->second->ParseArgs(argc, argv, index);
			}
		}

		return argc;
	}

	//------------------------------------------------------------------------------------------------
	const COption& CCommandScope::GetOption(const char* name) const
	{
		std::string key = m_Prefix + name;
		auto it = m_Options.find(key);
		if (it == m_Options.end())
			throw InCommandException(InCommandError::UnknownOption, name, -1);

		return *it->second;
	}

	//------------------------------------------------------------------------------------------------
	CCommandScope& CCommandScope::DeclareSubcommand(const char* name)
	{
		auto it = m_Subcommands.find(name);
		if (it != m_Subcommands.end())
			throw InCommandException(InCommandError::DuplicateCommand, name, -1);

		auto result = m_Subcommands.emplace(name, std::make_unique<CCommandScope>());
		return *result.first->second;
	}

	//------------------------------------------------------------------------------------------------
	const COption &CCommandScope::DeclareSwitchOption(const char* name)
	{
		std::string key = m_Prefix + name;
		auto it = m_Options.find(key);
		if (it != m_Options.end())
			throw InCommandException(InCommandError::DuplicateOption, name, -1);

		auto insert = m_Options.emplace(key, std::make_unique<CSwitchOption>(name, nullptr));
		return *insert.first->second;
	}

	//------------------------------------------------------------------------------------------------
	const COption &CCommandScope::DeclareVariableOption(const char* name, const char* defaultValue)
	{
		std::string key = m_Prefix + name;
		auto it = m_Options.find(key);
		if (it != m_Options.end())
			throw InCommandException(InCommandError::DuplicateOption, name, -1);

		auto insert = m_Options.emplace(key, std::make_unique<CVariableOption>(name, defaultValue, nullptr));
		return *insert.first->second;
	}

	//------------------------------------------------------------------------------------------------
	const COption &CCommandScope::DeclareVariableOption(const char* name, int domainSize, const char* domain[], int defaultIndex)
	{
		std::string key = m_Prefix + name;
		auto it = m_Options.find(key);
		if (it != m_Options.end())
			throw InCommandException(InCommandError::DuplicateOption, name, -1);

		auto insert = m_Options.emplace(key, std::make_unique<CVariableOption>(name, domainSize, domain, defaultIndex, nullptr));
		return *insert.first->second;
	}

	//------------------------------------------------------------------------------------------------
	const COption& CCommandScope::DeclareNonKeyedOption(const char* name)
	{
		m_NonKeyedOptions.push_back(std::make_unique<CNonKeyedOption>(name, nullptr));
		return *m_NonKeyedOptions.back();
	}
}
