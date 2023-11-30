#include <iostream>
#include <string>

#include "InCommand.h"

int main(int argc, const char *argv[])
{
    InCommand::Bool ShowHelp;
    InCommand::Int Val1;
    InCommand::Int Val2;
    InCommand::String Message("");

    InCommand::CCommandReader CmdReader("sample", "Sample app for demonstrating use of InCommand command line reader utility", argc, argv);
    CmdReader.DeclareBoolSwitchOption(ShowHelp, "help", "Display help for sample commands.", 'h');

    InCommand::CCommand *pAddCmd = CmdReader.DeclareCommand("add", "Adds two integers", 1);
    pAddCmd->DeclareBoolSwitchOption(ShowHelp, "help", "Display help for sample add.", 'h');
    pAddCmd->DeclareInputOption(Val1, "value1", "First add value");
    pAddCmd->DeclareInputOption(Val2, "value2", "Second add value");
    pAddCmd->DeclareSwitchOption(Message, "message", "Print <message> N-times where N = value1 + value2", 'm');

    InCommand::CCommand *pMulCmd = CmdReader.DeclareCommand("mul", "Multiplies two integers", 2);
    pMulCmd->DeclareBoolSwitchOption(ShowHelp, "help", "Display help for sample multiply.", 'h');
    pMulCmd->DeclareInputOption(Val1, "value1", "First multiply value");
    pMulCmd->DeclareInputOption(Val2, "value2", "Second multiply value");
    pMulCmd->DeclareSwitchOption(Message, "message", "Print <message> N-times where N = value1 * value2", 'm');

    const InCommand::CCommand *pCmd = CmdReader.PreReadCommandArguments();
    InCommand::Status fetchResult = CmdReader.ReadOptionArguments(pCmd);
    if (InCommand::Status::Success != fetchResult)
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