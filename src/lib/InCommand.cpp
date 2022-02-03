#include <iostream>

#include "InCommand.h"

CInCommandParser::CInCommandParser()
{
}

void CInCommandParser::ParseArguments(int argc, const char* argv[])
{
	for (int i = 0; i < argc; ++i)
	{
		i = ProcessArgument(i, argv);
	}
}

int CInCommandParser::ProcessArgument(int arg, const char* argv[])
{
	return arg;
}
