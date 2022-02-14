#include <iostream>
#include <string>

#include "InCommand.h"

int main(int argc, const char *argv[])
{
    InCommand::InCommandBool ShowHelp;
    InCommand::InCommandInt Val1;
    InCommand::InCommandInt Val2;
    InCommand::InCommandString Message("");

    InCommand::CCommandReader CmdReader("sample", "Sample app for demonstrating use of InCommand command line reader utility", argc, argv);

    CmdReader.DefaultCommandCtx()->DeclareSwitchOption(ShowHelp, "help", "Display help for sample commands.", 'h');

    InCommand::CCommandCtx *pAddCmd = CmdReader.DefaultCommandCtx()->DeclareCommandCtx("add", "Adds two integers", 1);
    pAddCmd->DeclareSwitchOption(ShowHelp, "help", "Display help for sample add.", 'h');
    pAddCmd->DeclareParameterOption(Val1, "value1", "First add value");
    pAddCmd->DeclareParameterOption(Val2, "value2", "Second add value");
    pAddCmd->DeclareVariableOption(Message, "message", "Print <message> N-times where N = value1 + value2", 'm');

    InCommand::CCommandCtx *pMulCmd = CmdReader.DefaultCommandCtx()->DeclareCommandCtx("mul", "Multiplies two integers", 2);
    pMulCmd->DeclareSwitchOption(ShowHelp, "help", "Display help for sample multiply.", 'h');
    pMulCmd->DeclareParameterOption(Val1, "value1", "First multiply value");
    pMulCmd->DeclareParameterOption(Val2, "value2", "Second multiply value");
    pMulCmd->DeclareVariableOption(Message, "message", "Print <message> N-times where N = value1 * value2", 'm');

    InCommand::CCommandCtx *pCmd = CmdReader.ReadCommandCtx();
    InCommand::InCommandStatus fetchResult = CmdReader.ReadOptions();
    if (InCommand::InCommandStatus::Success != fetchResult)
    {
        std::cout << std::endl;
        std::cout << "Command Line Error: " << CmdReader.LastErrorString() << std::endl;
        ShowHelp = true;
    }

    if(ShowHelp || 0 == pCmd->Id())
    {
        std::cout << std::endl;
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

    if (Message.Value()[0] != 0)
    {
        for (int i = 0; i < result; ++i)
            std::cout << Message << std::endl;
    }

    return 0;
}