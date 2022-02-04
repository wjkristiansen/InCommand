#include <iostream>

#include "InCommand.h"

CInCommandParser::CInCommandParser()
{
}

int CInCommandParser::ParseArguments(int argc, const char* argv[])
{
	for (int i = 0; i < argc; ++i)
	{
		auto it = m_Arguments.find(argv[i]);
		if (it == m_Arguments.end())
			throw(std::exception("Invalid argument"));

		switch (it->second.Type)
		{
		case ArgumentType::Switch: {
			it->second.IsPresent = true;
			break; }

		case ArgumentType::Variable: {
			it->second.IsPresent = true;
			++i;
			if (i == argc)
				throw(std::exception("Syntax error"));

			it->second.Value = argv[i];
			break; }

		case ArgumentType::Options: {
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

CInCommandParser::Argument &CInCommandParser::AddArgument(const char* name, ArgumentType type)
{
	auto it = m_Arguments.find(name);
	if (it != m_Arguments.end())
		throw(std::exception("Duplicate argument"));

	auto insert = m_Arguments.emplace(name, type);
	return insert.first->second;
}

void CInCommandParser::AddSwitchArgument(const char* name)
{
	AddArgument(name, ArgumentType::Switch);
}

bool CInCommandParser::GetSwitchValue(const char* name) const
{
	auto it = m_Arguments.find(name);
	if (it == m_Arguments.end())
		throw(std::exception("Unknown argument"));

	if (it->second.Type != ArgumentType::Switch)
		throw(std::exception("Not a switch"));

	return it->second.IsPresent;
}

void CInCommandParser::AddVariableArgument(const char* name, const char* defaultValue)
{
	auto& arg = AddArgument(name, ArgumentType::Variable);
	arg.Value = defaultValue;
}

const char* CInCommandParser::GetVariableValue(const char* name) const
{
	auto it = m_Arguments.find(name);
	if (it == m_Arguments.end())
		throw(std::exception("Unknown argument"));

	if (it->second.Type != ArgumentType::Variable)
		throw(std::exception("Not a variable"));

	return it->second.Value.c_str();
}
  
void CInCommandParser::AddOptionsArgument(const char* name, int numOptionValues, const char *optionValues[], int defaultOptionIndex)
{
	auto& arg = AddArgument(name, ArgumentType::Options);
	for (int i = 0; i < numOptionValues; ++i)
	{
		auto insert = arg.OptionValues.emplace(optionValues[i]);
		if (!insert.second)
			throw(std::exception("Duplicate option values"));
	}
}

const char* CInCommandParser::GetOptionsValue(const char* name) const
{
	auto it = m_Arguments.find(name);
	if (it == m_Arguments.end())
		throw(std::exception("Unknown argument"));

	if (it->second.Type != ArgumentType::Options)
		throw(std::exception("Not an options argument"));

	return it->second.Value.c_str();
}
