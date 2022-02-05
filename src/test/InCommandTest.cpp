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

    InCommand::CCommandScope Parser;
    auto &IsRealParam = Parser.DeclareSwitchParameter("is-real");
    auto &NameParam = Parser.DeclareVariableParameter("name", "Fred");
    const char *options[] = { "red", "green", "blue" };
    auto &ColorParam = Parser.DeclareOptionsVariableParameter("color", 3, options, 1);

    Parser.ParseParameterArguments(0, argc, argv); 

    EXPECT_EQ(IsRealParam.IsPresent(), true);
    EXPECT_EQ(NameParam.GetValueAsString(), std::string("Fred"));
    EXPECT_EQ(ColorParam.GetValueAsString(), std::string("red"));
}

TEST(InCommand, NonKeyedParams)
{
    const char* argv[] =
    {
        "myfile1.txt",
        "--some-switch",
        "myfile2.txt",
        "myfile3.txt",
    };
    int argc = _countof(argv);

    InCommand::CCommandScope Parser;
    auto& File1 = Parser.DeclareNonKeyedParameter("file1");
    auto& File2 = Parser.DeclareNonKeyedParameter("file2");
    auto& File3 = Parser.DeclareNonKeyedParameter("file3");
    auto& Switch = Parser.DeclareSwitchParameter("some-switch");

    Parser.ParseParameterArguments(0, argc, argv);

    EXPECT_EQ(Switch.IsPresent(), true);
    EXPECT_EQ(File1.GetValueAsString(), std::string(argv[0]));
    EXPECT_EQ(File2.GetValueAsString(), std::string(argv[2]));
    EXPECT_EQ(File3.GetValueAsString(), std::string(argv[3]));
}
