#include <gtest/gtest.h>

#include "InCommand.h"

TEST(InCommand, DumpCommands)
{
    const char* argv[] =
    {
        "--is-real",
        "--color",
        "red",
    };
    int argc = _countof(argv);

    CInCommandParser Parser;
    Parser.DeclareSwitchParameter("is-real");
    Parser.DeclareVariableParameter("name", "Fred");
    const char *options[] = { "red", "green", "blue" };
    Parser.DeclareOptionsVariableParameter("color", 3, options, 1);

    Parser.ParseParameterArguments(argc, argv); 

    EXPECT_EQ(Parser.GetSwitchValue("is-real"), true);
    EXPECT_EQ(std::string(Parser.GetVariableValue("name")), std::string("Fred"));
    EXPECT_EQ(std::string(Parser.GetOptionsVariableValue("color")), std::string("red"));
}
