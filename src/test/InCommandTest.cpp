#include <gtest/gtest.h>

#include "InCommand.h"

TEST(InCommand, BasicParams)
{
    const char* argv[] =
    {
        "--is-real",
        "--color",
        "red",
    };
    int argc = _countof(argv);

    CInCommandParser Parser;
    auto &IsRealParam = Parser.DeclareSwitchParameter("is-real");
    auto &NameParam = Parser.DeclareVariableParameter("name", "Fred");
    const char *options[] = { "red", "green", "blue" };
    auto &ColorParam = Parser.DeclareOptionsVariableParameter("color", 3, options, 1);

    Parser.ParseParameterArguments(argc, argv); 

    EXPECT_EQ(IsRealParam.IsPresent(), true);
    EXPECT_EQ(NameParam.GetValueAsString(), std::string("Fred"));
    EXPECT_EQ(ColorParam.GetValueAsString(), std::string("red"));
}
