#include <gtest/gtest.h>

#include "InCommand.h"

TEST(InCommand, DumpCommands)
{
    const char* argv[] =
    {
        "app.exe",
        "-is-real",
        "-color",
        "red",
    };
    int argc = _countof(argv);

    CInCommandParser Parser;
    Parser.AddSwitchArgument("-is-real");
    Parser.AddVariableArgument("-name", "Fred");
    const char *options[] = { "red", "green", "blue" };
    Parser.AddOptionsArgument("-color", 3, options, 1);
    Parser.ParseArguments(argc - 1, argv + 1); 
    EXPECT_EQ(Parser.GetSwitchValue("-is-real"), true);
    EXPECT_EQ(std::string(Parser.GetVariableValue("-name")), std::string("Fred"));
    EXPECT_EQ(std::string(Parser.GetOptionsValue("-color")), std::string("red"));
}
