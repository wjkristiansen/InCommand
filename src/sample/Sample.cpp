#include <iostream>
#include <string>

#include "InCommand.h"

int main(int argc, const char *argv[])
{
    InCommand::InCommandBool ShowHelp(false);
    InCommand::InCommandInt Val1;
    InCommand::InCommandInt Val2;
    InCommand::InCommandString Message("");

    InCommand::CCommand AppCmd("sample", "Sample app for using InCommand command line parser.", 0);
    AppCmd.DeclareSwitchOption(ShowHelp, "help", "Display help for sample commands.", 'h');

    InCommand::CCommand *pAddCmd = AppCmd.DeclareSubcommand("add", "Adds two integers", 1);
    pAddCmd->DeclareSwitchOption(ShowHelp, "help", "Display help for sample add.", 'h');
    pAddCmd->DeclareParameterOption(Val1, "value1", "First add value");
    pAddCmd->DeclareParameterOption(Val2, "value2", "Second add value");
    pAddCmd->DeclareVariableOption(Message, "message", "Print <message> N-times where N = value1 + value2", 'm');

    InCommand::CCommand *pMulCmd = AppCmd.DeclareSubcommand("mul", "Multiplies two integers", 2);
    pMulCmd->DeclareSwitchOption(ShowHelp, "help", "Display help for sample multiply.", 'h');
    pMulCmd->DeclareParameterOption(Val1, "value1", "First multiply value");
    pMulCmd->DeclareParameterOption(Val2, "value2", "Second multiply value");
    pMulCmd->DeclareVariableOption(Message, "message", "Print <message> N-times where N = value1 * value2", 'm');

    InCommand::CArgumentList ArgList(argc, argv);
    InCommand::CArgumentIterator ArgIt = ArgList.Begin();

    InCommand::CCommand *pCmd = AppCmd.FetchCommand(ArgList, ArgIt);
    InCommand::InCommandStatus fetchResult = pCmd->FetchOptions(ArgList, ArgIt);
    if (InCommand::InCommandStatus::Success != fetchResult)
    {
        std::string errorString = pCmd->ErrorString(fetchResult, ArgList, ArgIt);
        std::cout << std::endl;
        std::cout << "Command Line Error: " << errorString << std::endl;
        std::cout << std::endl;
        ShowHelp = true;
    }

    if(ShowHelp || 0 == pCmd->Id())
    {
        std::cout << pCmd->UsageString() << std::endl;
        return 0;
    }

    int result = 0;
    switch(pCmd->Id())
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