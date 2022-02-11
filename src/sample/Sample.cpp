#include <iostream>
#include <string>

#include "InCommand.h"

int main(int argc, const char *argv[])
{
    InCommand::CCommandScope AppCmd("sample", "Sample app for using InCommand command line parser.", 0);
    InCommand::CCommandScope &AddCmd = AppCmd.DeclareSubcommand("add", "Adds two integers", 1);
    InCommand::CCommandScope &MultiplyCmd = AppCmd.DeclareSubcommand("multiply", "Multiplies two integers", 2);

    InCommand::InCommandBool Help;
    AppCmd.DeclareSwitchOption(Help, "help", "Display help for sample commands.", 'h');
    AddCmd.DeclareSwitchOption(Help, "help", "Display help for sample add.", 'h');
    MultiplyCmd.DeclareSwitchOption(Help, "help", "Display help for sample multiply.", 'h');

    InCommand::InCommandInt Val1;
    InCommand::InCommandInt Val2;
    InCommand::InCommandString Message("");
    AddCmd.DeclareParameterOption(Val1, "value1", "First add value");
    AddCmd.DeclareParameterOption(Val2, "value2", "Second add value");
    AddCmd.DeclareVariableOption(Message, "message", "Print <message> N-times where N = value1 + value2", 'm');

    MultiplyCmd.DeclareParameterOption(Val1, "value1", "First multiply value");
    MultiplyCmd.DeclareParameterOption(Val2, "value2", "Second multiply value");
    MultiplyCmd.DeclareVariableOption(Message, "message", "Print <message> N-times where N = value1 * value2", 'm');

    InCommand::CArgumentList ArgList(argc, argv);
    InCommand::CArgumentIterator ArgIt = ArgList.Begin();
    ArgIt++; // Skip past app name in command line

    InCommand::CCommandScope &Cmd = AppCmd.ScanCommandArgs(ArgList, ArgIt);
    Cmd.ScanOptionArgs(ArgList, ArgIt);

    if(Help || 0 == Cmd.GetScopeId())
    {
        std::cout << Cmd.UsageString() << std::endl;
        return 0;
    }

    int result = 0;
    switch(Cmd.GetScopeId())
    {
    case 1: { // Add
        result = Val1.Get() + Val2.Get();
        std::cout << Val1.Get() << " + " << Val2.Get() << " = " << result << std::endl;
        break; }
    case 2: {// Multiple
        result = Val1.Get() * Val2.Get();
        std::cout << Val1.Get() << " * " << Val2.Get() << " = " << result << std::endl;
        break; }
    default:
        return -1;
    }

    if (Message.Get()[0] != 0)
    {
        for (int i = 0; i < result; ++i)
            std::cout << Message.Get() << std::endl;
    }

    return 0;
}