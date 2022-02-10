#include <iostream>
#include <string>

#include "InCommand.h"

int main(int argc, const char *argv[])
{
    InCommand::CCommandScope AppCmd("sample", "Sample app for using InCommand command line parser.", 0);
    InCommand::CCommandScope &AddCmd = AppCmd.DeclareSubcommand("add", "Adds two integers", 1);
    InCommand::CCommandScope &MultiplyCmd = AppCmd.DeclareSubcommand("multiply", "Multiplies two integers", 2);

    InCommand::InCommandBool Help;
    AppCmd.DeclareSwitchOption(Help, "help", "Display help for sample commands.");
    AddCmd.DeclareSwitchOption(Help, "help", "Display help for sample add.");
    MultiplyCmd.DeclareSwitchOption(Help, "help", "Display help for sample multiply.");

    InCommand::InCommandString Val1;
    InCommand::InCommandString Val2;
    InCommand::InCommandString Message("");
    AddCmd.DeclareNonKeyedOption(Val1, "value1", "First add value");
    AddCmd.DeclareNonKeyedOption(Val2, "value2", "Second add value");
    AddCmd.DeclareVariableOption(Message, "message", "Print <message> N-times where N = value1 + value2");

    MultiplyCmd.DeclareNonKeyedOption(Val1, "value1", "First multiply value");
    MultiplyCmd.DeclareNonKeyedOption(Val2, "value2", "Second multiply value");
    MultiplyCmd.DeclareVariableOption(Message, "message", "Print <message> N-times where N = value1 * value2");

    InCommand::CArgumentList ArgList(argc, argv);
    InCommand::CArgumentIterator ArgIt = ArgList.Begin();
    ArgIt++; // Skip past app name in command line

    InCommand::CCommandScope &Cmd = AppCmd.ScanCommands(ArgList, ArgIt);
    Cmd.ScanOptions(ArgList, ArgIt);

    if(Help || 0 == Cmd.GetScopeId())
    {
        std::cout << Cmd.UsageString() << std::endl;
        return 0;
    }

    int val1 = std::stoi(Val1);
    int val2 = std::stoi(Val2);
    int result = 0;
    switch(Cmd.GetScopeId())
    {
    case 1: { // Add
        result = val1 + val2;
        std::cout << val1 << " + " << val2 << " = " << result << std::endl;
        break; }
    case 2: {// Multiple
        result = val1 * val2;
        std::cout << val1 << " * " << val2 << " = " << result << std::endl;
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