#include <iostream>
#include <string>
#include <random>
#include <assert.h>

#include "InCommand.h"

int main(int argc, const char *argv[])
{
    InCommand::CCommandReader cmdReader("sample");

    auto cat_Add = cmdReader.DeclareCategory("add", "Adds two integers");
    auto cat_Mul = cmdReader.DeclareCategory("mul", "Multiplies two integers");
    auto cat_Roshambo = cmdReader.DeclareCategory("roshambo", "Play Roshambo");

    auto switch_Help = cmdReader.DeclareSwitch("help", 'h');

    auto switch_Add_Help = cmdReader.DeclareSwitch(cat_Add, "help", 'h');
    auto param_Add_Val1 = cmdReader.DeclareParameter(cat_Add, "value1", "First add value");
    auto param_Add_Val2 = cmdReader.DeclareParameter(cat_Add, "value2", "Second add value");
    auto var_Add_Message = cmdReader.DeclareVariable(cat_Add, "message", 'm', "Print <message> N-times where N = value1 + value2");

    auto switch_Mul_Help = cmdReader.DeclareSwitch(cat_Mul, "help", 'h');
    auto param_Mul_Val1 = cmdReader.DeclareParameter(cat_Mul, "value1", "First multiply value");
    auto param_Mul_Val2 = cmdReader.DeclareParameter(cat_Mul, "value2", "Second multiply value");
    auto var_Mul_Message = cmdReader.DeclareVariable(cat_Mul, "message", 'm', "Print <message> N-times where N = value1 * value2");

    auto var_Roshambo_Move = cmdReader.DeclareVariable(cat_Roshambo, "move", { "rock", "paper", "scissors" }, "1-2-3 Shoot!");

    InCommand::CCommandExpression cmdExp;
    if (InCommand::Status::Success != cmdReader.ReadCommandExpression(argc, argv, cmdExp))
    {
        std::string errorString;
        cmdReader.GetLastReadError(errorString);
        std::cout << std::endl;
        std::cout << errorString << std::endl;
        std::cout << std::endl;
        std::string helpString = cmdReader.SimpleUsageString(cmdExp.GetCategory());
        std::cout << helpString << std::endl;
        return -1;
    }

    if (cmdExp.GetSwitchIsSet(switch_Help) ||
        cmdExp.GetSwitchIsSet(switch_Add_Help) ||
        cmdExp.GetSwitchIsSet(switch_Mul_Help))
    {
        std::cout << std::endl;
        std::string helpString = cmdReader.SimpleUsageString(cmdExp.GetCategory());
        std::cout << helpString << std::endl;
        return 0;
    }

    int result = 0;
    int val1;
    int val2;
    std::string message;

    if (cat_Add == cmdExp.GetCategory())
    { // Add
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
    }
    else if (cat_Mul == cmdExp.GetCategory())
    { // Multiply
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
    }
    else if (cat_Roshambo == cmdExp.GetCategory())
    {
        // Seed the random number generator
        std::random_device rd;
        std::mt19937 gen(rd()); // Mersenne Twister engine seeded with rd()

        // Define the distribution range for integers
        std::uniform_int_distribution<> distrib(1, 3);
        int randomInt = distrib(gen); // Generate random integer

        std::string playerMove = cmdExp.GetVariableValue(var_Roshambo_Move, "");

        std::string computerMove;
        switch (randomInt)
        {
        case 1:
            computerMove = "rock";
            break;
        case 2:
            computerMove = "paper";
            break;
        case 3:
            computerMove = "scissors";
            break;
        default:
            return -1;
        }

        std::cout << "Your move: " << playerMove << std::endl;
        std::cout << "My Move: " << computerMove << std::endl;

        if (playerMove == computerMove)
        {
            std::cout << "Tie :|" << std::endl;
        }
        else if ((playerMove == "rock" && computerMove == "scissors") ||
                 (playerMove == "paper" && computerMove == "rock") ||
                 (playerMove == "scissors" && computerMove == "paper"))
        {
            std::cout << "You Win :(" << std::endl;
        }
        else
        {
            std::cout << "I Win! :)" << std::endl;
        }
    }
    else
    {
        return -1;
    }

    if (!message.empty())
    {
        for (int i = 0; i < result; ++i)
            std::cout << message << std::endl;
    }

    return 0;
}