#include <sstream>

#include "InCommand.h"

namespace InCommand
{
	//------------------------------------------------------------------------------------------------
	CCommandScope::CCommandScope(const char* name, const char* description, int scopeId) :
		m_ScopeId(scopeId),
		m_Description(description ? description : "")
	{
		if (name)
			m_Name = name;
	}

	//------------------------------------------------------------------------------------------------
	CCommandScope &CCommandScope::ScanCommandArgs(const CArgumentList& args, CArgumentIterator& it)
	{
		CCommandScope *pScope = this;

		if (it != args.End())
		{
			auto subIt = m_Subcommands.find(args.At(it));
			if (subIt != m_Subcommands.end())
			{
				++it;
				pScope = &(subIt->second->ScanCommandArgs(args, it));
			}
		}

		return *pScope;
	}

	//------------------------------------------------------------------------------------------------
	InCommandResult CCommandScope::ScanOptionArgs(const CArgumentList& args, CArgumentIterator& it) const
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
				const COption* pOption = nullptr;

				if (args.At(it)[1] == '-')
				{
					// Long-form option
					auto optIt = m_Options.find(args.At(it) + 2);
					if(optIt == m_Options.end() || optIt->second->GetType() == OptionType::NonKeyed)
						return InCommandResult::UnexpectedArgument;
					pOption = optIt->second.get();
				}
				else
				{
					auto optIt = m_ShortOptions.find(args.At(it)[1]);
					pOption = optIt->second.get();
				}

				if (pOption)
				{
					auto result = pOption->ParseArgs(args, it);
					if (result != InCommandResult::Success)
						return result;
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
	CCommandScope& CCommandScope::DeclareSubcommand(const char* name, const char* description, int scopeId)
	{
		auto it = m_Subcommands.find(name);
		if (it != m_Subcommands.end())
			throw InCommandException(InCommandResult::DuplicateCommand);

		auto result = m_Subcommands.emplace(name, std::make_shared<CCommandScope>(name, description, scopeId));
		result.first->second->m_pSuperScope = this;
		return *result.first->second;
	}

	//------------------------------------------------------------------------------------------------
	std::string CCommandScope::CommandChainString() const
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
	std::string CCommandScope::UsageString() const
	{
		std::ostringstream s;
		s << "USAGE" << std::endl;
		s << std::endl;

		std::string commandChainString = CommandChainString();

		if (!m_Subcommands.empty())
		{
			s << commandChainString;

			s << " [<command> [<command options>]]" << std::endl;
		}

		if (!m_Options.empty())
		{
			s << commandChainString;

			// Non-keyed options first
			for (auto& nko : m_NonKeyedOptions)
			{
				s << " <" << nko->GetName() << ">";
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
				s << subIt.second->GetName() << "  <description here...>" << std::endl;
			};
		}

		s << std::endl;

		if (!m_Options.empty())
		{
			// Non-keyed options first
			s << "OPTIONS" << std::endl;
			s << std::endl;

			// Non-keyed options details
			for (auto& nko : m_NonKeyedOptions)
			{
				s << nko->GetName() << "  <description here...>" << std::endl;
			}

			// Keyed-options details
			for (auto& ko : m_Options)
			{
				auto type = ko.second->GetType();
				if (type == OptionType::NonKeyed)
					continue; // Already dumped
				s << "--" << ko.second->GetName();
				if (ko.second->GetType() == OptionType::Variable)
					s << " <value>";
				s << "  <description here>" << std::endl;
			}

			s << std::endl;
		}

		return s.str();
	}

	//------------------------------------------------------------------------------------------------
	const COption& CCommandScope::DeclareSwitchOption(InCommandBool& boundValue, const char* name, const char *description, char shortKey)
	{
		auto it = m_Options.find(name);
		if (it != m_Options.end())
			throw InCommandException(InCommandResult::DuplicateOption);

		auto insert = m_Options.emplace(name, std::make_shared<CSwitchOption>(boundValue, name, description, shortKey));
		if (shortKey)
			m_ShortOptions.insert(std::make_pair(shortKey, insert.first->second));
		return *insert.first->second;
	}

	//------------------------------------------------------------------------------------------------
	const COption &CCommandScope::DeclareVariableOption(InCommandString& boundValue, const char* name, const char *description, char shortKey)
	{
		auto it = m_Options.find(name);
		if (it != m_Options.end())
			throw InCommandException(InCommandResult::DuplicateOption);

		auto insert = m_Options.emplace(name, std::make_shared<CVariableOption>(boundValue, name, description, shortKey));
		if (shortKey)
			m_ShortOptions.insert(std::make_pair(shortKey, insert.first->second));
		return *insert.first->second;
	}

	//------------------------------------------------------------------------------------------------
	const COption &CCommandScope::DeclareVariableOption(InCommandString& boundValue, const char* name, int domainSize, const char* domain[], const char *description, char shortKey)
	{
		auto it = m_Options.find(name);
		if (it != m_Options.end())
			throw InCommandException(InCommandResult::DuplicateOption);

		auto insert = m_Options.emplace(name, std::make_shared<CVariableOption>(boundValue, name, domainSize, domain, description, shortKey));
		if (shortKey)
			m_ShortOptions.insert(std::make_pair(shortKey, insert.first->second));
		return *insert.first->second;
	}

	//------------------------------------------------------------------------------------------------
	const COption& CCommandScope::DeclareNonKeyedOption(InCommandString& boundValue, const char* name, const char* description)
	{
		auto it = m_Options.find(name);
		if (it != m_Options.end())
			throw InCommandException(InCommandResult::DuplicateOption);


        std::shared_ptr<CNonKeyedOption> pOption = std::make_shared<CNonKeyedOption>(boundValue, name, description);

        // Add the non-keyed option to the declared options map
		m_Options.insert(std::make_pair(name, pOption));

        // Add to the array of non-keyed options
		m_NonKeyedOptions.push_back(pOption);
		return *m_NonKeyedOptions.back();
	}
}
