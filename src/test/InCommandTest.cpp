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
    int argc = _countof(argv);

    InCommand::InCommandBool IsReal;
    InCommand::InCommandString Color;
    InCommand::InCommandString Name("Fred");

    const char* colors[] = { "red", "green", "blue" };

    InCommand::CCommandScope Parser;
    Parser.DeclareSwitchOption(IsReal, "is-real");
    Parser.DeclareVariableOption(Name, "name");
    Parser.DeclareVariableOption(Color, "color", 3, colors);

    InCommand::CArgumentList args(argc, argv);
    InCommand::CArgumentIterator it = args.Begin();
    ++it; // Skip app name
    Parser.FetchOptions(args, it);

    EXPECT_TRUE(IsReal);
    EXPECT_EQ(std::string("Fred"),Name.Get());
    EXPECT_EQ(std::string("red"), Color.Get());
}

TEST(InCommand, NonKeyedParams)
{
    const char* argv[] =
    {
        "foo.exe",
        "myfile1.txt",
        "--some-switch",
        "myfile2.txt",
        "myfile3.txt",
    };
    int argc = _countof(argv);

    InCommand::InCommandBool SomeSwitch;
    InCommand::InCommandString File1;
    InCommand::InCommandString File2;
    InCommand::InCommandString File3;

    InCommand::CCommandScope Parser;
    Parser.DeclareNonKeyedOption(File1, "file1");
    Parser.DeclareNonKeyedOption(File2, "file2");
    Parser.DeclareNonKeyedOption(File3, "file3");
    Parser.DeclareSwitchOption(SomeSwitch, "some-switch");

    InCommand::CArgumentList args(argc, argv);
    InCommand::CArgumentIterator it = args.Begin();
    ++it; // Skip app name

    Parser.FetchOptions(args, it);

    EXPECT_TRUE(SomeSwitch);
    EXPECT_EQ(File1.Get(), std::string(argv[1]));
    EXPECT_EQ(File2.Get(), std::string(argv[3]));
    EXPECT_EQ(File3.Get(), std::string(argv[4]));
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

    InCommand::CCommandScope RootCommand;
    InCommand::CCommandScope& PlantCommand = RootCommand.DeclareSubcommand("plant", to_underlying(ScopeId::Plant));
    InCommand::CCommandScope& TreeCommand = PlantCommand.DeclareSubcommand("tree", to_underlying(ScopeId::Tree));
    InCommand::CCommandScope& ShrubCommand = PlantCommand.DeclareSubcommand("shrub", to_underlying(ScopeId::Shrub));
    InCommand::CCommandScope& AnimalCommand = RootCommand.DeclareSubcommand("animal", to_underlying(ScopeId::Animal));
    InCommand::CCommandScope& DogCommand = AnimalCommand.DeclareSubcommand("dog", to_underlying(ScopeId::Dog));
    InCommand::CCommandScope& CatCommand = AnimalCommand.DeclareSubcommand("cat", to_underlying(ScopeId::Cat));

    for(int i = 0; i < 3; ++i)
    {
        InCommand::InCommandBool Help;
        InCommand::InCommandBool List;
        InCommand::InCommandBool Climb;
        InCommand::InCommandBool Prune;
        InCommand::InCommandBool Burn;
        InCommand::InCommandBool Walk;
        InCommand::InCommandString Lives("9");

        auto LateDeclareOptions = [&](InCommand::CCommandScope& CommandScope)
        {
            switch (static_cast<ScopeId>(CommandScope.GetScopeId()))
            {
            case ScopeId::Root:
                CommandScope.DeclareSwitchOption(Help, "help");
                break;
            case ScopeId::Plant:
                CommandScope.DeclareSwitchOption(List, "list");
                break;
            case ScopeId::Tree:
                CommandScope.DeclareSwitchOption(Climb, "climb");
                break;
            case ScopeId::Shrub:
                CommandScope.DeclareSwitchOption(Prune, "prune");
                CommandScope.DeclareSwitchOption(Burn, "burn");
                break;
            case ScopeId::Animal:
                CommandScope.DeclareSwitchOption(List, "list");
                break;
            case ScopeId::Dog:
                CommandScope.DeclareSwitchOption(Walk, "walk");
                break;
            case ScopeId::Cat:
                CommandScope.DeclareVariableOption(Lives, "lives");
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
            int argc = _countof(argv);

            InCommand::CArgumentList args(argc, argv);
            InCommand::CArgumentIterator it = args.Begin();
            ++it; // Skip app name

            InCommand::CCommandScope* pScope;
            EXPECT_EQ(InCommand::InCommandResult::Success, RootCommand.FetchCommandScope(args, it, &pScope));
            LateDeclareOptions(*pScope);
            EXPECT_EQ(InCommand::InCommandResult::Success, pScope->FetchOptions(args, it));

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
            int argc = _countof(argv);

            InCommand::CArgumentList args(argc, argv);
            InCommand::CArgumentIterator it = args.Begin();
            ++it; // Skip app name

            InCommand::CCommandScope* pScope;
            EXPECT_EQ(InCommand::InCommandResult::Success, RootCommand.FetchCommandScope(args, it, &pScope));
            LateDeclareOptions(*pScope);
            EXPECT_EQ(InCommand::InCommandResult::Success, pScope->FetchOptions(args, it));

            EXPECT_EQ(Lives.Get(), std::string("8"));

            break; }
        case 2: {
            const char* argv[] =
            {
                "app.exe",
                "--help",
            };
            int argc = _countof(argv);

            InCommand::CArgumentList args(argc, argv);
            InCommand::CArgumentIterator it = args.Begin();
            ++it; // Skip app name

            InCommand::CCommandScope* pScope;
            EXPECT_EQ(InCommand::InCommandResult::Success, RootCommand.FetchCommandScope(args, it, &pScope));
            LateDeclareOptions(*pScope);
            EXPECT_EQ(InCommand::InCommandResult::Success, pScope->FetchOptions(args, it));

            EXPECT_TRUE(Help);

            break; }
        }
    }
}
