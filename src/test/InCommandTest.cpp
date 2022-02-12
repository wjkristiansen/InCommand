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

    InCommand::CCommand RootCmdScope("app", nullptr);
    RootCmdScope.DeclareSwitchOption(IsReal, "is-real", nullptr);
    RootCmdScope.DeclareVariableOption(Name, "name", nullptr);
    RootCmdScope.DeclareVariableOption(Color, "color", 3, colors, nullptr);

    InCommand::CArgumentList args(argc, argv);
    InCommand::CArgumentIterator it = args.Begin();
    ++it; // Skip app name
    RootCmdScope.FetchOptions(args, it);

    EXPECT_TRUE(IsReal);
    EXPECT_EQ(std::string("Fred"),Name.Get());
    EXPECT_EQ(std::string("red"), Color.Get());
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

    InCommand::CCommand RootCmdScope("foo", nullptr);
    RootCmdScope.DeclareParameterOption(File1, "file1", nullptr);
    RootCmdScope.DeclareParameterOption(File2, "file2", nullptr);
    RootCmdScope.DeclareParameterOption(File3, "file3", nullptr);
    RootCmdScope.DeclareSwitchOption(SomeSwitch, "some-switch", nullptr);

    InCommand::CArgumentList args(argc, argv);
    InCommand::CArgumentIterator it = args.Begin();
    ++it; // Skip app name

    RootCmdScope.FetchOptions(args, it);

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

    for(int i = 0; i < 3; ++i)
    {
        InCommand::InCommandBool Help;
        InCommand::InCommandBool List;
        InCommand::InCommandBool Climb;
        InCommand::InCommandBool Prune;
        InCommand::InCommandBool Burn;
        InCommand::InCommandBool Walk;
        InCommand::InCommandString Lives("9");

        InCommand::CCommand RootCmdScope("app", nullptr);
        RootCmdScope.DeclareSwitchOption(Help, "help", nullptr);
        InCommand::CCommand* pPlantCommand = RootCmdScope.DeclareSubcommand("plant", nullptr, to_underlying(ScopeId::Plant));
        pPlantCommand->DeclareSwitchOption(List, "list", nullptr);
        InCommand::CCommand* pTreeCommand = pPlantCommand->DeclareSubcommand("tree", nullptr, to_underlying(ScopeId::Tree));
        InCommand::CCommand* pShrubCommand = pPlantCommand->DeclareSubcommand("shrub", nullptr, to_underlying(ScopeId::Shrub));
        pShrubCommand->DeclareSwitchOption(Prune, "prune", nullptr);
        pShrubCommand->DeclareSwitchOption(Burn, "burn", nullptr);
        InCommand::CCommand* pAnimalCommand = RootCmdScope.DeclareSubcommand("animal", nullptr, to_underlying(ScopeId::Animal));
        InCommand::CCommand* pDogCommand = pAnimalCommand->DeclareSubcommand("dog", nullptr, to_underlying(ScopeId::Dog));
        InCommand::CCommand* pCatCommand = pAnimalCommand->DeclareSubcommand("cat", nullptr, to_underlying(ScopeId::Cat));

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

            InCommand::CArgumentList args(argc, argv);
            InCommand::CArgumentIterator it = args.Begin();
            ++it; // Skip app name

            InCommand::CCommand *pScope = RootCmdScope.FetchCommand(args, it);
            LateDeclareOptions(pScope);
            EXPECT_EQ(InCommand::InCommandStatus::Success, pScope->FetchOptions(args, it));

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

            InCommand::CArgumentList args(argc, argv);
            InCommand::CArgumentIterator it = args.Begin();
            ++it; // Skip app name

            InCommand::CCommand *pScope = RootCmdScope.FetchCommand(args, it);
            LateDeclareOptions(pScope);
            EXPECT_EQ(InCommand::InCommandStatus::Success, pScope->FetchOptions(args, it));

            EXPECT_EQ(Lives.Get(), std::string("8"));

            break; }
        case 2: {
            const char* argv[] =
            {
                "app.exe",
                "--help",
            };
            const int argc = sizeof(argv) / sizeof(argv[0]);

            InCommand::CArgumentList args(argc, argv);
            InCommand::CArgumentIterator it = args.Begin();
            ++it; // Skip app name

            InCommand::CCommand *pScope = RootCmdScope.FetchCommand(args, it);
            LateDeclareOptions(pScope);
            EXPECT_EQ(InCommand::InCommandStatus::Success, pScope->FetchOptions(args, it));

            EXPECT_TRUE(Help);

            break; }
        }
    }
}
