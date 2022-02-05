#include <iostream>

#include "InCommand.h"

//------------------------------------------------------------------------------------------------
CInCommandParser::CInCommandParser() :
	m_Prefix("--")
{
}

//------------------------------------------------------------------------------------------------
void CInCommandParser::SetPrefix(const char* prefix)
{
	m_Prefix = prefix;
}

//------------------------------------------------------------------------------------------------
int CInCommandParser::ParseParameterArguments(int argc, const char* argv[])
{
	if (argc == 0)
		return 0;

	// Is the first argument a subcommand?
	auto subIt = m_Subcommands.find(argv[0]);
	if (subIt != m_Subcommands.end())
		return subIt->second->ParseParameterArguments(argc - 1, argv + 1);

	// Parse the parameters
	for (int i = 0; i < argc;)
	{
		auto it = m_Parameters.find(argv[i]);
		if (it == m_Parameters.end())
		{
			if (m_NumPresentNonKeyed == m_NonKeyedParameters.size())
				throw std::exception("Unexpected argument");

			i += m_NonKeyedParameters[m_NumPresentNonKeyed]->ParseArgs(argc - i, argv + i);
			m_NumPresentNonKeyed++;
		}
		else
		{
			i += it->second->ParseArgs(argc - i, argv + i);
		}
	}

	return argc;
}

//------------------------------------------------------------------------------------------------
const CParameter& CInCommandParser::GetParameter(const char* name) const
{
	std::string key = m_Prefix + name;
	auto it = m_Parameters.find(key);
	if (it == m_Parameters.end())
		throw(std::exception("No matching parameter"));

	return *it->second;
}

//------------------------------------------------------------------------------------------------
CInCommandParser& CInCommandParser::DeclareSubcommand(const char* name)
{
	auto it = m_Subcommands.find(name);
	if (it != m_Subcommands.end())
		throw(std::exception("Duplicate subcommand"));

	auto result = m_Subcommands.emplace(name, std::make_unique<CInCommandParser>());
	return *result.first->second;
}

//------------------------------------------------------------------------------------------------
const CParameter &CInCommandParser::DeclareSwitchParameter(const char* name)
{
	std::string key = m_Prefix + name;
	auto it = m_Parameters.find(key);
	if (it != m_Parameters.end())
		throw(std::exception("Duplicate parameter"));

	auto insert = m_Parameters.emplace(key, std::make_unique<CSwitchParameter>(name, nullptr));
	return *insert.first->second;
}

//------------------------------------------------------------------------------------------------
const CParameter &CInCommandParser::DeclareVariableParameter(const char* name, const char* defaultValue)
{
	std::string key = m_Prefix + name;
	auto it = m_Parameters.find(key);
	if (it != m_Parameters.end())
		throw(std::exception("Duplicate parameter"));

	auto insert = m_Parameters.emplace(key, std::make_unique<CVariableParameter>(name, defaultValue, nullptr));
	return *insert.first->second;
}

//------------------------------------------------------------------------------------------------
const CParameter &CInCommandParser::DeclareOptionsVariableParameter(const char* name, int numOptionValues, const char* optionValues[], int defaultOptionIndex)
{
	std::string key = m_Prefix + name;
	auto it = m_Parameters.find(key);
	if (it != m_Parameters.end())
		throw(std::exception("Duplicate parameter"));

	auto insert = m_Parameters.emplace(key, std::make_unique<CVariableParameter>(name, numOptionValues, optionValues, defaultOptionIndex, nullptr));
	return *insert.first->second;
}

//------------------------------------------------------------------------------------------------
const CParameter& CInCommandParser::DeclareNonKeyedParameter(const char* name)
{
	m_NonKeyedParameters.push_back(std::make_unique<CNonKeyedParameter>(name, nullptr));
	return *m_NonKeyedParameters.back();
}
