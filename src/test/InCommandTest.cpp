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

    InCommand::Bool IsReal;
    InCommand::String Color;
    InCommand::String Name("Fred");

    const char* colors[] = { "red", "green", "blue" };

    InCommand::CCommandReader CmdReader("app", "test argument list", argc, argv);
    CmdReader.RootCommand()->DeclareBoolParameter(IsReal, "is-real", nullptr);
    CmdReader.RootCommand()->DeclareOptionParameter(Name, "name", nullptr);
    CmdReader.RootCommand()->DeclareOptionParameter(Color, "color", 3, colors, nullptr);

    InCommand::CArgumentList args(argc, argv);
    InCommand::CArgumentIterator it = args.Begin();
    CmdReader.ReadParameterArguments();

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

    InCommand::Bool SomeSwitch;
    InCommand::String File1;
    InCommand::String File2;
    InCommand::String File3;

    InCommand::CCommandReader CmdReader("app", "test argument list", argc, argv);
    CmdReader.RootCommand()->DeclareInputParameter(File1, "file1", nullptr);
    CmdReader.RootCommand()->DeclareInputParameter(File2, "file2", nullptr);
    CmdReader.RootCommand()->DeclareInputParameter(File3, "file3", nullptr);
    CmdReader.RootCommand()->DeclareBoolParameter(SomeSwitch, "some-switch", nullptr);

    CmdReader.ReadParameterArguments();

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
        CmdReader.RootCommand()->DeclareBoolParameter(Help, "help", nullptr);
        InCommand::CCommand* pPlantCommand = CmdReader.RootCommand()->DeclareCommand("plant", nullptr, to_underlying(ScopeId::Plant));
        pPlantCommand->DeclareBoolParameter(List, "list", nullptr);
        InCommand::CCommand* pShrubCommand = pPlantCommand->DeclareCommand("shrub", nullptr, to_underlying(ScopeId::Shrub));
        pShrubCommand->DeclareBoolParameter(Prune, "prune", nullptr);
        pShrubCommand->DeclareBoolParameter(Burn, "burn", nullptr);
        InCommand::CCommand* pAnimalCommand = CmdReader.RootCommand()->DeclareCommand("animal", nullptr, to_underlying(ScopeId::Animal));
        pAnimalCommand->DeclareCommand("dog", nullptr, to_underlying(ScopeId::Dog));
        pAnimalCommand->DeclareCommand("cat", nullptr, to_underlying(ScopeId::Cat));

        auto LateDeclareOptions = [&](InCommand::CCommand* pCommandScope)
        {
            switch (static_cast<ScopeId>(pCommandScope->Id()))
            {
            case ScopeId::Tree:
                pCommandScope->DeclareBoolParameter(Climb, "climb", nullptr);
                break;
            case ScopeId::Animal:
                pCommandScope->DeclareBoolParameter(List, "list", nullptr);
                break;
            case ScopeId::Dog:
                pCommandScope->DeclareBoolParameter(Walk, "walk", nullptr);
                break;
            case ScopeId::Cat:
                pCommandScope->DeclareOptionParameter(Lives, "lives", nullptr);
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
            InCommand::CCommand *pCtx = CmdReader.ReadCommandArguments();
            LateDeclareOptions(pCtx);
            EXPECT_EQ(InCommand::Status::Success, CmdReader.ReadParameterArguments());

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

            InCommand::CCommand *pCtx = CmdReader.ReadCommandArguments();
            LateDeclareOptions(pCtx);
            EXPECT_EQ(InCommand::Status::Success, CmdReader.ReadParameterArguments());

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

            InCommand::CCommand* pCtx = CmdReader.ReadCommandArguments();
            LateDeclareOptions(pCtx);
            EXPECT_EQ(InCommand::Status::Success, CmdReader.ReadParameterArguments());

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
        auto *pGotoCmd = CmdReader.RootCommand()->DeclareCommand("goto", nullptr, 1);
        try
        {
            CmdReader.RootCommand()->DeclareCommand("goto", nullptr, 1);; // Duplicate command "goto"
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
        CmdReader.RootCommand()->DeclareBoolParameter(Foo, "foo", nullptr, 0);
        try
        {
            CmdReader.RootCommand()->DeclareOptionParameter(Bar, "foo", nullptr, 0); // Duplicate option "foo"
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

        auto* pGotoCmd = CmdReader.RootCommand()->DeclareCommand("goto", nullptr, 1);
        pGotoCmd->DeclareBoolParameter(Foo, "foo", nullptr, 0);
        pGotoCmd->DeclareOptionParameter(Bar, "bar", nullptr, 0);

        EXPECT_EQ(InCommand::Status::UnexpectedArgument, CmdReader.ReadParameterArguments());
        EXPECT_EQ(1, CmdReader.ReadIndex());
    }

    {
        const char* argv[] = { "app", "goto", "--fop", "--bar", "7" };
        const int argc = sizeof(argv) / sizeof(argv[0]);
        InCommand::CCommandReader CmdReader("app", "test argument list", argc, argv);

        auto* pGotoCmd = CmdReader.RootCommand()->DeclareCommand("goto", nullptr, 1);
        pGotoCmd->DeclareBoolParameter(Foo, "foo", nullptr, 0);
        pGotoCmd->DeclareOptionParameter(Bar, "bar", nullptr, 0);

        EXPECT_EQ(InCommand::Status::UnknownOption, CmdReader.ReadParameterArguments());
        EXPECT_EQ(2, CmdReader.ReadIndex());
    }

    {
        const char* argv[] = { "app", "goto", "--foo", "--bar", "hello" };
        const int argc = sizeof(argv) / sizeof(argv[0]);
        InCommand::CCommandReader CmdReader("app", "test argument list", argc, argv);

        auto* pGotoCmd = CmdReader.RootCommand()->DeclareCommand("goto", nullptr, 1);
        pGotoCmd->DeclareBoolParameter(Foo, "foo", nullptr, 0);
        pGotoCmd->DeclareOptionParameter(Bar, "bar", nullptr, 0);

        EXPECT_EQ(InCommand::Status::InvalidValue, CmdReader.ReadParameterArguments());
        EXPECT_EQ(4, CmdReader.ReadIndex());
    }

    {
        const char* argv[] = { "app", "goto", "--foo", "--bar"};
        const int argc = sizeof(argv) / sizeof(argv[0]);
        InCommand::CCommandReader CmdReader("app", "test argument list", argc, argv);

        auto* pGotoCmd = CmdReader.RootCommand()->DeclareCommand("goto", nullptr, 1);
        pGotoCmd->DeclareBoolParameter(Foo, "foo", nullptr, 0);
        pGotoCmd->DeclareOptionParameter(Bar, "bar", nullptr, 0);

        EXPECT_EQ(InCommand::Status::MissingOptionValue, CmdReader.ReadParameterArguments());
        EXPECT_EQ(4, CmdReader.ReadIndex());
    }

    {
        const char* argv[] = { "app", "goto", "--foo", "--bar", "7" };
        const int argc = sizeof(argv) / sizeof(argv[0]);
        InCommand::CCommandReader CmdReader("app", "test argument list", argc, argv);

        auto* pGotoCmd = CmdReader.RootCommand()->DeclareCommand("goto", nullptr, 1);
        pGotoCmd->DeclareBoolParameter(Foo, "foo", nullptr, 0);
        const char* barValues[] = { "1", "3", "5" };
        pGotoCmd->DeclareOptionParameter(Bar, "bar", 3, barValues, nullptr, 0);

        EXPECT_EQ(InCommand::Status::InvalidValue, CmdReader.ReadParameterArguments());
        EXPECT_EQ(4, CmdReader.ReadIndex());
    }
}