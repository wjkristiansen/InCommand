#include <iostream>
#include <string>

#include "InCommand.h"

int main(int argc, const char *argv[])
{
    InCommand::CCommand AppCmd("sample", "Sample app for using InCommand command line parser.", 0);
    InCommand::CCommand &AddCmd = AppCmd.DeclareSubcommand("add", "Adds two integers", 1);
    InCommand::CCommand &MultiplyCmd = AppCmd.DeclareSubcommand("multiply", "Multiplies two integers", 2);

    InCommand::InCommandBool ShowHelp(false);
    AppCmd.DeclareSwitchOption(ShowHelp, "help", "Display help for sample commands.", 'h');
    AddCmd.DeclareSwitchOption(ShowHelp, "help", "Display help for sample add.", 'h');
    MultiplyCmd.DeclareSwitchOption(ShowHelp, "help", "Display help for sample multiply.", 'h');

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

    InCommand::CCommand &Cmd = AppCmd.FetchCommand(ArgList, ArgIt);
    InCommand::OptionScanResult scanResult = Cmd.ReadOptions(ArgList, ArgIt);
    if (InCommand::InCommandStatus::Success != scanResult.Status)
    {
        std::string errorString = Cmd.ErrorString(scanResult, ArgList, ArgIt);
        std::cout << std::endl;
        std::cout << "Command Line Error: " << errorString << std::endl;
        std::cout << std::endl;
        ShowHelp = true;
    }

    if(ShowHelp || 0 == Cmd.Id())
    {
        std::cout << Cmd.UsageString() << std::endl;
        return 0;
    }

    int result = 0;
    switch(Cmd.Id())
    {
    case 1: { // Add
        result = Val1 + Val2;
        std::cout << Val1 << " + " << Val2 << " = " << result << std::endl;
        break; }
    case 2: {// Multiple
        result = Val1 * Val2;
        std::cout << Val1 << " * " << Val2 << " = " << result << std::endl;
        break; }
    default:
        return -1;
    }

    if (Message.Get()[0] != 0)
    {
        for (int i = 0; i < result; ++i)
            std::cout << Message << std::endl;
    }

    return 0;
}