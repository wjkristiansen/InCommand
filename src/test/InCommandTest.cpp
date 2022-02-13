#include <gtest/gtest.h>

#include "InCommand.h"

TEST(InCommand, BasicParams)
{
    const char* argv[] =
    {
        "app",
        "--is-real",
        "--color",
        "red",
    };
    const int argc = sizeof(argv) / sizeof(argv[0]);

    InCommand::InCommandBool IsReal;
    InCommand::InCommandString Color;
    InCommand::InCommandString Name("Fred");

    const char* colors[] = { "red", "green", "blue" };

    InCommand::CCommandReader CmdReader("app", "test argument list", argc, argv);
    CmdReader.DefaultCommand()->DeclareSwitchOption(IsReal, "is-real", nullptr);
    CmdReader.DefaultCommand()->DeclareVariableOption(Name, "name", nullptr);
    CmdReader.DefaultCommand()->DeclareVariableOption(Color, "color", 3, colors, nullptr);

    InCommand::CArgumentList args(argc, argv);
    InCommand::CArgumentIterator it = args.Begin();
    CmdReader.ReadOptions();

    EXPECT_TRUE(IsReal);
    EXPECT_EQ(std::string("Fred"),Name.Value());
    EXPECT_EQ(std::string("red"), Color.Value());
}

TEST(InCommand, ParameterParams)
{
    const char* argv[] =
    {
        "foo.exe",
        "myfile1.txt",
        "--some-switch",
        "myfile2.txt",
        "myfile3.txt",
    };
    const int argc = sizeof(argv) / sizeof(argv[0]);

    InCommand::InCommandBool SomeSwitch;
    InCommand::InCommandString File1;
    InCommand::InCommandString File2;
    InCommand::InCommandString File3;

    InCommand::CCommandReader CmdReader("app", "test argument list", argc, argv);
    CmdReader.DefaultCommand()->DeclareParameterOption(File1, "file1", nullptr);
    CmdReader.DefaultCommand()->DeclareParameterOption(File2, "file2", nullptr);
    CmdReader.DefaultCommand()->DeclareParameterOption(File3, "file3", nullptr);
    CmdReader.DefaultCommand()->DeclareSwitchOption(SomeSwitch, "some-switch", nullptr);

    CmdReader.ReadOptions();

    EXPECT_TRUE(SomeSwitch);
    EXPECT_EQ(File1.Value(), std::string(argv[1]));
    EXPECT_EQ(File2.Value(), std::string(argv[3]));
    EXPECT_EQ(File3.Value(), std::string(argv[4]));
}

template<typename E>
constexpr auto to_underlying(E e) noexcept
{
    return static_cast<std::underlying_type_t<E>>(e);
}

TEST(InCommand, SubCommands)
{
    enum class ScopeId : int
    {
        Root,
        Plant,
        Tree,
        Shrub,
        Animal,
        Dog,
        Cat
    };

    for(int i = 0; i < 3; ++i)
    {
        InCommand::InCommandBool Help;
        InCommand::InCommandBool List;
        InCommand::InCommandBool Climb;
        InCommand::InCommandBool Prune;
        InCommand::InCommandBool Burn;
        InCommand::InCommandBool Walk;
        InCommand::InCommandString Lives("9");

        InCommand::CCommandReader CmdReader("app", "test argument list", 0, nullptr);
        CmdReader.DefaultCommand()->DeclareSwitchOption(Help, "help", nullptr);
        InCommand::CCommand* pPlantCommand = CmdReader.DefaultCommand()->DeclareSubcommand("plant", nullptr, to_underlying(ScopeId::Plant));
        pPlantCommand->DeclareSwitchOption(List, "list", nullptr);
        InCommand::CCommand* pShrubCommand = pPlantCommand->DeclareSubcommand("shrub", nullptr, to_underlying(ScopeId::Shrub));
        pShrubCommand->DeclareSwitchOption(Prune, "prune", nullptr);
        pShrubCommand->DeclareSwitchOption(Burn, "burn", nullptr);
        InCommand::CCommand* pAnimalCommand = CmdReader.DefaultCommand()->DeclareSubcommand("animal", nullptr, to_underlying(ScopeId::Animal));
        pAnimalCommand->DeclareSubcommand("dog", nullptr, to_underlying(ScopeId::Dog));
        pAnimalCommand->DeclareSubcommand("cat", nullptr, to_underlying(ScopeId::Cat));

        auto LateDeclareOptions = [&](InCommand::CCommand* pCommandScope)
        {
            switch (static_cast<ScopeId>(pCommandScope->Id()))
            {
            case ScopeId::Tree:
                pCommandScope->DeclareSwitchOption(Climb, "climb", nullptr);
                break;
            case ScopeId::Animal:
                pCommandScope->DeclareSwitchOption(List, "list", nullptr);
                break;
            case ScopeId::Dog:
                pCommandScope->DeclareSwitchOption(Walk, "walk", nullptr);
                break;
            case ScopeId::Cat:
                pCommandScope->DeclareVariableOption(Lives, "lives", nullptr);
                break;
            }

        };

        switch (i)
        {
        case 0: {
            const char* argv[] =
            {
                "app.exe",
                "plant",
                "shrub",
                "--burn",
            };
            const int argc = sizeof(argv) / sizeof(argv[0]);

            CmdReader.Reset(argc, argv);
            InCommand::CCommand *pScope = CmdReader.ReadCommand();
            LateDeclareOptions(pScope);
            EXPECT_EQ(InCommand::InCommandStatus::Success, CmdReader.ReadOptions());

            EXPECT_TRUE(Burn);
            EXPECT_FALSE(Prune);

            break; }
        case 1: {
            const char* argv[] =
            {
                "app.exe",
                "animal",
                "cat",
                "--lives",
                "8"
            };
            const int argc = sizeof(argv) / sizeof(argv[0]);

            CmdReader.Reset(argc, argv);

            InCommand::CCommand *pScope = CmdReader.ReadCommand();
            LateDeclareOptions(pScope);
            EXPECT_EQ(InCommand::InCommandStatus::Success, CmdReader.ReadOptions());

            EXPECT_EQ(Lives.Value(), std::string("8"));

            break; }
        case 2: {
            const char* argv[] =
            {
                "app.exe",
                "--help",
            };
            const int argc = sizeof(argv) / sizeof(argv[0]);

            CmdReader.Reset(argc, argv);

            InCommand::CCommand* pScope = CmdReader.ReadCommand();
            LateDeclareOptions(pScope);
            EXPECT_EQ(InCommand::InCommandStatus::Success, CmdReader.ReadOptions());

            EXPECT_TRUE(Help);

            break; }
        }
    }
}

TEST(InCommand, Errors)
{
    InCommand::InCommandBool Foo;
    InCommand::InCommandInt Bar;

    {
        const char* argv[] = { "app", "goto", "--foo", "--bar" };
        const int argc = sizeof(argv) / sizeof(argv[0]);
        InCommand::CCommandReader CmdReader("app", "test argument list", argc, argv);
        auto *pGotoCmd = CmdReader.DefaultCommand()->DeclareSubcommand("goto", nullptr, 1);
        try
        {
            CmdReader.DefaultCommand()->DeclareSubcommand("goto", nullptr, 1);; // Duplicate command "goto"
        }
        catch(const InCommand::InCommandException& e)
        {
            EXPECT_EQ(e.status, InCommand::InCommandStatus::DuplicateCommand);
        }
    }
    
    {
        const char* argv[] = { "app", "goto", "--foo", "--bar", "7" };
        const int argc = sizeof(argv) / sizeof(argv[0]);
        InCommand::CCommandReader CmdReader("app", "test argument list", argc, argv);
        CmdReader.DefaultCommand()->DeclareSwitchOption(Foo, "foo", nullptr, 0);
        try
        {
            CmdReader.DefaultCommand()->DeclareVariableOption(Bar, "foo", nullptr, 0); // Duplicate option "foo"
        }
        catch (const InCommand::InCommandException& e)
        {
            EXPECT_EQ(e.status, InCommand::InCommandStatus::DuplicateOption);
        }
    }

    {
        const char* argv[] = { "app", "gogo", "--foo", "--bar", "7" };
        const int argc = sizeof(argv) / sizeof(argv[0]);
        InCommand::CCommandReader CmdReader("app", "test argument list", argc, argv);

        auto* pGotoCmd = CmdReader.DefaultCommand()->DeclareSubcommand("goto", nullptr, 1);
        pGotoCmd->DeclareSwitchOption(Foo, "foo", nullptr, 0);
        pGotoCmd->DeclareVariableOption(Bar, "bar", nullptr, 0);

        EXPECT_EQ(InCommand::InCommandStatus::UnexpectedArgument, CmdReader.ReadOptions());
        EXPECT_EQ(1, CmdReader.ReadIndex());
    }

    {
        const char* argv[] = { "app", "goto", "--fop", "--bar", "7" };
        const int argc = sizeof(argv) / sizeof(argv[0]);
        InCommand::CCommandReader CmdReader("app", "test argument list", argc, argv);

        auto* pGotoCmd = CmdReader.DefaultCommand()->DeclareSubcommand("goto", nullptr, 1);
        pGotoCmd->DeclareSwitchOption(Foo, "foo", nullptr, 0);
        pGotoCmd->DeclareVariableOption(Bar, "bar", nullptr, 0);

        EXPECT_EQ(InCommand::InCommandStatus::UnknownOption, CmdReader.ReadOptions());
        EXPECT_EQ(2, CmdReader.ReadIndex());
    }

    {
        const char* argv[] = { "app", "goto", "--foo", "--bar", "hello" };
        const int argc = sizeof(argv) / sizeof(argv[0]);
        InCommand::CCommandReader CmdReader("app", "test argument list", argc, argv);

        auto* pGotoCmd = CmdReader.DefaultCommand()->DeclareSubcommand("goto", nullptr, 1);
        pGotoCmd->DeclareSwitchOption(Foo, "foo", nullptr, 0);
        pGotoCmd->DeclareVariableOption(Bar, "bar", nullptr, 0);

        EXPECT_EQ(InCommand::InCommandStatus::InvalidValue, CmdReader.ReadOptions());
        EXPECT_EQ(4, CmdReader.ReadIndex());
    }

    {
        const char* argv[] = { "app", "goto", "--foo", "--bar"};
        const int argc = sizeof(argv) / sizeof(argv[0]);
        InCommand::CCommandReader CmdReader("app", "test argument list", argc, argv);

        auto* pGotoCmd = CmdReader.DefaultCommand()->DeclareSubcommand("goto", nullptr, 1);
        pGotoCmd->DeclareSwitchOption(Foo, "foo", nullptr, 0);
        pGotoCmd->DeclareVariableOption(Bar, "bar", nullptr, 0);

        EXPECT_EQ(InCommand::InCommandStatus::MissingOptionValue, CmdReader.ReadOptions());
        EXPECT_EQ(4, CmdReader.ReadIndex());
    }

    {
        const char* argv[] = { "app", "goto", "--foo", "--bar", "7" };
        const int argc = sizeof(argv) / sizeof(argv[0]);
        InCommand::CCommandReader CmdReader("app", "test argument list", argc, argv);

        auto* pGotoCmd = CmdReader.DefaultCommand()->DeclareSubcommand("goto", nullptr, 1);
        pGotoCmd->DeclareSwitchOption(Foo, "foo", nullptr, 0);
        const char* barValues[] = { "1", "3", "5" };
        pGotoCmd->DeclareVariableOption(Bar, "bar", 3, barValues, nullptr, 0);

        EXPECT_EQ(InCommand::InCommandStatus::InvalidValue, CmdReader.ReadOptions());
        EXPECT_EQ(4, CmdReader.ReadIndex());
    }
}