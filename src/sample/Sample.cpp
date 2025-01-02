#include <iostream>
#include <string>
#include <assert.h>

#include "InCommand.h"

int main(int argc, const char *argv[])
{
    InCommand::CCommandReader cmdReader("sample");

    auto cat_Add = cmdReader.DeclareCategory(0, "add", "Adds two integers");
    auto cat_Mul = cmdReader.DeclareCategory(0, "mul", "Multiplies two integers");

    assert(cat_Add == 1);
    assert(cat_Mul == 2);

    auto switch_Help = cmdReader.DeclareSwitch(0, "help", "", 'h');

    auto switch_Add_Help = cmdReader.DeclareSwitch(cat_Add, "help", "", 'h');
    auto param_Add_Val1 = cmdReader.DeclareParameter(cat_Add, "value1", "First add value");
    auto param_Add_Val2 = cmdReader.DeclareParameter(cat_Add, "value2", "Second add value");
    auto var_Add_Message = cmdReader.DeclareVariable(cat_Add, "message", "Print <message> N-times where N = value1 + value2", 'm');

    auto switch_Mul_Help = cmdReader.DeclareSwitch(cat_Mul, "help", "", 'h');
    auto param_Mul_Val1 = cmdReader.DeclareParameter(cat_Mul, "value1", "First multiply value");
    auto param_Mul_Val2 = cmdReader.DeclareParameter(cat_Mul, "value2", "Second multiply value");
    auto var_Mul_Message = cmdReader.DeclareVariable(cat_Mul, "message", "Print <message> N-times where N = value1 * value2", 'm');

    InCommand::CCommandExpression cmdExp;
    if (InCommand::Status::Success != cmdReader.ReadCommandExpression(argc, argv, cmdExp))
    {
        //     std::cout << "Command Line Error: " << cmdReader.LastErrorString() << std::endl;
        std::cout << std::endl;
        std::string helpString = cmdReader.SimpleUsageString(0);
        std::cout << helpString << std::endl;
        return -1;
    }

    if (cmdExp.GetSwitchIsSet(switch_Help) || cmdExp.GetCategoryDepth() == 1)
    {
        std::cout << std::endl;
        std::string helpString = cmdReader.SimpleUsageString(0);
        std::cout << helpString << std::endl;
        return 0;
    }

    if (cmdExp.GetSwitchIsSet(switch_Add_Help))
    {
        std::cout << std::endl;
        std::cout << cmdReader.SimpleUsageString(cat_Add) << std::endl;
        std::cout << cmdReader.OptionDetailsString(cat_Add);
        return 0;
    }

    if (cmdExp.GetSwitchIsSet(switch_Mul_Help))
    {
        std::cout << std::endl;
        std::cout << cmdReader.SimpleUsageString(cat_Mul) << std::endl;
        std::cout << cmdReader.OptionDetailsString(cat_Mul);
        return 0;
    }

    int result = 0;
    int val1;
    int val2;
    std::string message;
    switch (cmdExp.GetCategoryId(1))
    {
    case 1: { // Add
        auto val1string = cmdExp.GetParameterValue(param_Add_Val1, std::string());
        auto val2string = cmdExp.GetParameterValue(param_Add_Val2, std::string());

        if (val1string.empty() || val2string.empty())
        {
            std::cout << std::endl;
            std::cout << cmdReader.SimpleUsageString(cat_Add) << std::endl;
            return -1;
        }
        
        val1 = std::stoi(val1string);
        val2 = std::stoi(val2string);

        message = cmdExp.GetVariableValue(var_Add_Message, std::string());
        result = val1 + val2;
        std::cout << val1 << " + " << val2 << " = " << result << std::endl;
        break; }

    case 2: {// Multiply
        auto val1string = cmdExp.GetParameterValue(param_Mul_Val1, std::string());
        auto val2string = cmdExp.GetParameterValue(param_Mul_Val2, std::string());

        if (val1string.empty() || val2string.empty())
        {
            std::cout << std::endl;
            std::cout << cmdReader.SimpleUsageString(cat_Mul) << std::endl;
            return -1;
        }
        
        val1 = std::stoi(val1string);
        val2 = std::stoi(val2string);
        
        message = cmdExp.GetVariableValue(var_Mul_Message, std::string());
        result = val1 * val2;
        std::cout << val1 << " * " << val2 << " = " << result << std::endl;
        break; }
    default:
        return -1;
    }

    if (!message.empty())
    {
        for (int i = 0; i < result; ++i)
            std::cout << message << std::endl;
    }

    return 0;
}