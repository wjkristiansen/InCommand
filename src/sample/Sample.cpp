#include <iostream>

#include "InCommand.h"

int main(int argc, const char *argv[])
{
    InCommand::CCommandScope AppCmdScope("sample", "Sample app for using InCommand command line parser.");
    AppCmdScope.DeclareSubcommand("add", "Adds two integers", 1);
    AppCmdScope.DeclareSubcommand("multiply", "Multiplies two integers", 2);

    InCommand::CArgumentList ArgList(argc, argv);
    InCommand::CArgumentIterator ArgIt = ArgList.Begin();
    ArgIt++; // Skip past app name in command line

    return 0;
}