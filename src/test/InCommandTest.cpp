#include <gtest/gtest.h>

#include "InCommand.h"

TEST(InCommand, DumpCommands)
{
    const char* argv[] =
    {
        "one.exe",
        "-two",
        "--three",
        "four",
        "-five",
        "--six:seven"
    };
    int argc = _countof(argv);

    class CTestParser : public CInCommandParser
    {
    public:
        virtual int ProcessArgument(int arg, const char* argv[])
        {
            ++m_NumArgs;
            return arg;
        }

        int m_NumArgs = 0;
    };

    CTestParser Parser;
    
    Parser.ParseArguments(argc, argv);
    EXPECT_EQ(Parser.m_NumArgs, 6);
}
