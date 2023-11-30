#include <gtest/gtest.h>

#include "InCommand.h"

TEST(InCommand, BasicOptions)
{
    const char* argv[] =
    {
        "app",
        "--is-real",
        "--color",
        "red",
    };
    const int argc = sizeof(argv) / sizeof(argv[0]);

    InCommand::Bool IsReal;
    InCommand::String Color;
    InCommand::String Name("Fred");

    const char* colors[] = { "red", "green", "blue" };

    InCommand::CCommandReader CmdReader("app", "test argument list", argc, argv);
    CmdReader.DeclareBoolSwitchOption(IsReal, "is-real", nullptr);
    CmdReader.DeclareSwitchOption(Name, "name", nullptr);
    CmdReader.DeclareSwitchOption(Color, "color", 3, colors, nullptr);

    InCommand::CArgumentList args(argc, argv);
    InCommand::CArgumentIterator it = args.Begin();
    CmdReader.ReadOptionArguments(nullptr);

    EXPECT_TRUE(IsReal);
    EXPECT_EQ(std::string("Fred"),Name.Value());
    EXPECT_EQ(std::string("red"), Color.Value());
}

TEST(InCommand, OptionOptions)
{
    const char* argv[] =
    {
        "foo",
        "myfile1.txt",
        "--some-switch",
        "myfile2.txt",
        "myfile3.txt",
    };
    const int argc = sizeof(argv) / sizeof(argv[0]);

    InCommand::Bool SomeSwitch;
    InCommand::String File1;
    InCommand::String File2;
    InCommand::String File3;

    InCommand::CCommandReader CmdReader("app", "test argument list", argc, argv);
    CmdReader.DeclareInputOption(File1, "file1", nullptr);
    CmdReader.DeclareInputOption(File2, "file2", nullptr);
    CmdReader.DeclareInputOption(File3, "file3", nullptr);
    CmdReader.DeclareBoolSwitchOption(SomeSwitch, "some-switch", nullptr);

    CmdReader.ReadOptionArguments(nullptr);

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
        InCommand::Bool Help;
        InCommand::Bool List;
        InCommand::Bool Climb;
        InCommand::Bool Prune;
        InCommand::Bool Burn;
        InCommand::Bool Walk;
        InCommand::String Lives("9");

        InCommand::CCommandReader CmdReader("app", "test argument list", 0, nullptr);
        CmdReader.DeclareBoolSwitchOption(Help, "help", nullptr);
        InCommand::CCommand* pPlantCommand = CmdReader.DeclareCommand("plant", nullptr, to_underlying(ScopeId::Plant));
        pPlantCommand->DeclareBoolSwitchOption(List, "list", nullptr);
        InCommand::CCommand* pShrubCommand = pPlantCommand->DeclareCommand("shrub", nullptr, to_underlying(ScopeId::Shrub));
        pShrubCommand->DeclareBoolSwitchOption(Prune, "prune", nullptr);
        pShrubCommand->DeclareBoolSwitchOption(Burn, "burn", nullptr);
        InCommand::CCommand* pAnimalCommand = CmdReader.DeclareCommand("animal", nullptr, to_underlying(ScopeId::Animal));
        pAnimalCommand->DeclareCommand("dog", nullptr, to_underlying(ScopeId::Dog));
        pAnimalCommand->DeclareCommand("cat", nullptr, to_underlying(ScopeId::Cat));

        auto LateDeclareOptions = [&](InCommand::CCommand* pCommandScope)
        {
            switch (static_cast<ScopeId>(pCommandScope->Id()))
            {
            case ScopeId::Tree:
                pCommandScope->DeclareBoolSwitchOption(Climb, "climb", nullptr);
                break;
            case ScopeId::Animal:
                pCommandScope->DeclareBoolSwitchOption(List, "list", nullptr);
                break;
            case ScopeId::Dog:
                pCommandScope->DeclareBoolSwitchOption(Walk, "walk", nullptr);
                break;
            case ScopeId::Cat:
                pCommandScope->DeclareSwitchOption(Lives, "lives", nullptr);
                break;
            }

        };

        switch (i)
        {
        case 0: {
            const char* argv[] =
            {
                "app",
                "plant",
                "shrub",
                "--burn",
            };
            const int argc = sizeof(argv) / sizeof(argv[0]);

            CmdReader.Reset(argc, argv);
            InCommand::CCommand *pCommand = CmdReader.PreReadCommandArguments();
            LateDeclareOptions(pCommand);
            EXPECT_EQ(InCommand::Status::Success, CmdReader.ReadOptionArguments(pCommand));

            EXPECT_TRUE(Burn);
            EXPECT_FALSE(Prune);

            break; }
        case 1: {
            const char* argv[] =
            {
                "app",
                "animal",
                "cat",
                "--lives",
                "8"
            };
            const int argc = sizeof(argv) / sizeof(argv[0]);

            CmdReader.Reset(argc, argv);

            InCommand::CCommand *pCommand = CmdReader.PreReadCommandArguments();
            LateDeclareOptions(pCommand);
            EXPECT_EQ(InCommand::Status::Success, CmdReader.ReadOptionArguments(pCommand));

            EXPECT_EQ(Lives.Value(), std::string("8"));

            break; }
        case 2: {
            const char* argv[] =
            {
                "app",
                "--help",
            };
            const int argc = sizeof(argv) / sizeof(argv[0]);

            CmdReader.Reset(argc, argv);

            InCommand::CCommand* pCommand = CmdReader.PreReadCommandArguments();
            LateDeclareOptions(pCommand);
            EXPECT_EQ(InCommand::Status::Success, CmdReader.ReadOptionArguments(pCommand));

            EXPECT_TRUE(Help);

            break; }
        }
    }
}

TEST(InCommand, Errors)
{
    InCommand::Bool Foo;
    InCommand::Int Bar;

    {
        const char* argv[] = { "app", "goto", "--foo", "--bar" };
        const int argc = sizeof(argv) / sizeof(argv[0]);
        InCommand::CCommandReader CmdReader("app", "test argument list", argc, argv);
        auto *pGotoCmd = CmdReader.DeclareCommand("goto", nullptr, 1);
        try
        {
            CmdReader.DeclareCommand("goto", nullptr, 1);; // Duplicate command "goto"
        }
        catch(const InCommand::Exception& e)
        {
            EXPECT_EQ(e.status, InCommand::Status::DuplicateCommand);
        }
    }
    
    {
        const char* argv[] = { "app", "goto", "--foo", "--bar", "7" };
        const int argc = sizeof(argv) / sizeof(argv[0]);
        InCommand::CCommandReader CmdReader("app", "test argument list", argc, argv);
        CmdReader.DeclareBoolSwitchOption(Foo, "foo", nullptr, 0);
        try
        {
            CmdReader.DeclareSwitchOption(Bar, "foo", nullptr, 0); // Duplicate option "foo"
        }
        catch (const InCommand::Exception& e)
        {
            EXPECT_EQ(e.status, InCommand::Status::DuplicateOption);
        }
    }

    {
        const char* argv[] = { "app", "gogo", "--foo", "--bar", "7" };
        const int argc = sizeof(argv) / sizeof(argv[0]);

        InCommand::CCommandReader CmdReader("app", "test argument list", argc, argv);

        auto* pGotoCmd = CmdReader.DeclareCommand("goto", nullptr, 1);
        pGotoCmd->DeclareBoolSwitchOption(Foo, "foo", nullptr, 0);
        pGotoCmd->DeclareSwitchOption(Bar, "bar", nullptr, 0);

        EXPECT_EQ(InCommand::Status::UnexpectedArgument, CmdReader.ReadArguments());
        EXPECT_EQ(1, CmdReader.ReadIndex());
    }

    {
        const char* argv[] = { "app", "goto", "--fop", "--bar", "7" };
        const int argc = sizeof(argv) / sizeof(argv[0]);

        InCommand::CCommandReader CmdReader("app", "test argument list", argc, argv);

        auto* pGotoCmd = CmdReader.DeclareCommand("goto", nullptr, 1);
        pGotoCmd->DeclareBoolSwitchOption(Foo, "foo", nullptr, 0);
        pGotoCmd->DeclareSwitchOption(Bar, "bar", nullptr, 0);

        EXPECT_EQ(InCommand::Status::UnknownOption, CmdReader.ReadArguments());
        EXPECT_EQ(2, CmdReader.ReadIndex());
    }

    {
        const char* argv[] = { "app", "goto", "--foo", "--bar", "hello" };
        const int argc = sizeof(argv) / sizeof(argv[0]);

        InCommand::CCommandReader CmdReader("app", "test argument list", argc, argv);

        auto* pGotoCmd = CmdReader.DeclareCommand("goto", nullptr, 1);
        pGotoCmd->DeclareBoolSwitchOption(Foo, "foo", nullptr, 0);
        pGotoCmd->DeclareSwitchOption(Bar, "bar", nullptr, 0);

        EXPECT_EQ(InCommand::Status::InvalidValue, CmdReader.ReadArguments());
        EXPECT_EQ(4, CmdReader.ReadIndex());
    }

    {
        const char* argv[] = { "app", "goto", "--foo", "--bar"};
        const int argc = sizeof(argv) / sizeof(argv[0]);

        InCommand::CCommandReader CmdReader("app", "test argument list", argc, argv);

        auto* pGotoCmd = CmdReader.DeclareCommand("goto", nullptr, 1);
        pGotoCmd->DeclareBoolSwitchOption(Foo, "foo", nullptr, 0);
        pGotoCmd->DeclareSwitchOption(Bar, "bar", nullptr, 0);

        EXPECT_EQ(InCommand::Status::MissingOptionValue, CmdReader.ReadArguments());
        EXPECT_EQ(4, CmdReader.ReadIndex());
    }

    {
        const char* argv[] = { "app", "goto", "--foo", "--bar", "7" };
        const int argc = sizeof(argv) / sizeof(argv[0]);

        InCommand::CCommandReader CmdReader("app", "test argument list", argc, argv);

        auto* pGotoCmd = CmdReader.DeclareCommand("goto", nullptr, 1);
        pGotoCmd->DeclareBoolSwitchOption(Foo, "foo", nullptr, 0);
        const char* barValues[] = { "1", "3", "5" };
        pGotoCmd->DeclareSwitchOption(Bar, "bar", 3, barValues, nullptr, 0);

        EXPECT_EQ(InCommand::Status::InvalidValue, CmdReader.ReadArguments());
        EXPECT_EQ(4, CmdReader.ReadIndex());
    }
}