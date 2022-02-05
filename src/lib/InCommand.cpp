#include <iostream>

#include "InCommand.h"

CInCommandParser::CInCommandParser() :
	m_Prefix("--")
{
}

void CInCommandParser::SetPrefix(const char* prefix)
{
	m_Prefix = prefix;
}

int CInCommandParser::ParseParameterArguments(int argc, const char* argv[])
{
	if (argc == 0)
		return 0;

	// Is the first argument a subcommand?
	auto subIt = m_Subcommands.find(argv[0]);
	if (subIt != m_Subcommands.end())
		return subIt->second->ParseParameterArguments(argc - 1, argv + 1);

	// Parse the parameters
	for (int i = 0; i < argc; ++i)
	{
		auto it = m_Parameters.find(argv[i]);
		if (it == m_Parameters.end())
		{
			// Add to the anonymous parameters
			if (m_AnonymousParameters.size() >= m_MaxAnonymousParameters)
				throw(std::exception("Invalid argument"));

			m_AnonymousParameters.push_back(argv[i]);
		}

		switch (it->second.Type)
		{
		case ParameterType::Switch: {
			it->second.IsPresent = true;
			break; }

		case ParameterType::Variable: {
			it->second.IsPresent = true;
			++i;
			if (i == argc)
				throw(std::exception("Syntax error"));

			it->second.Value = argv[i];
			break; }

		case ParameterType::OptionsVariable: {
			it->second.IsPresent = true;
			++i;
			if (i == argc)
				throw(std::exception("Syntax error"));

			// See if the given value is a valid 
			// option value.
			auto vit = it->second.OptionValues.find(argv[i]);
			if (vit == it->second.OptionValues.end())
				throw(std::exception("Invalid option value"));

			it->second.Value = argv[i];
			break; }
		}
	}

	return 0;
}

CInCommandParser::Parameter& CInCommandParser::DeclareParameterImpl(const char* name, ParameterType type)
{
	std::string key = m_Prefix + name;
	auto it = m_Parameters.find(key);
	if (it != m_Parameters.end())
		throw(std::exception("Duplicate parameter"));

	auto insert = m_Parameters.emplace(key, type);
	return insert.first->second;
}

const CInCommandParser::Parameter& CInCommandParser::GetParameterImpl(const char* name, ParameterType type) const
{
	std::string key = m_Prefix + name;
	auto it = m_Parameters.find(key);
	if (it == m_Parameters.end())
		throw(std::exception("No matching parameter"));

	if (it->second.Type != type)
		throw(std::exception("Parameter type mismatch"));

	return it->second;
}

CInCommandParser& CInCommandParser::DeclareSubcommand(const char* name)
{
	auto it = m_Subcommands.find(name);
	if (it != m_Subcommands.end())
		throw(std::exception("Duplicate subcommand"));

	auto result = m_Subcommands.emplace(name, std::make_unique<CInCommandParser>());
	return *result.first->second;
}

void CInCommandParser::DeclareSwitchParameter(const char* name)
{
	DeclareParameterImpl(name, ParameterType::Switch);
}

void CInCommandParser::DeclareVariableParameter(const char* name, const char* defaultValue)
{
	auto& param = DeclareParameterImpl(name, ParameterType::Variable);
	param.Value = defaultValue;
}

void CInCommandParser::DeclareOptionsVariableParameter(const char* name, int numOptionValues, const char* optionValues[], int defaultOptionIndex)
{
	auto& param = DeclareParameterImpl(name, ParameterType::OptionsVariable);
	for (int i = 0; i < numOptionValues; ++i)
	{
		auto insert = param.OptionValues.emplace(optionValues[i]);
		if (!insert.second)
			throw(std::exception("Duplicate option values"));
	}
	param.Value = optionValues[defaultOptionIndex];
}

bool CInCommandParser::GetSwitchValue(const char* name) const
{
	const Parameter& param = GetParameterImpl(name, ParameterType::Switch);
	return param.IsPresent;
}

const char* CInCommandParser::GetVariableValue(const char* name) const
{
	const Parameter& param = GetParameterImpl(name, ParameterType::Variable);
	return param.Value.c_str();
}
  
const char* CInCommandParser::GetOptionsVariableValue(const char* name) const
{
	const Parameter& param = GetParameterImpl(name, ParameterType::OptionsVariable);
	return param.Value.c_str();
}
