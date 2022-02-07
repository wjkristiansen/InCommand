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
    auto &IsRealParam = Parser.DeclareSwitchOption("is-real");
    auto &NameParam = Parser.DeclareVariableOption("name", "Fred");
    const char *domain[] = { "red", "green", "blue" };
    auto &ColorParam = Parser.DeclareVariableOption("color", 3, domain, 1);

    Parser.ParseOptions(argc, argv, 0); 

    EXPECT_EQ(IsRealParam.IsPresent(), true);
    EXPECT_EQ(NameParam.GetValueAsString(), std::string("Fred"));
    EXPECT_EQ(ColorParam.GetValueAsString(), std::string("red"));
    EXPECT_EQ(Parser.GetOption("color").GetValueAsString(), std::string("red"));
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

    InCommand::CCommandScope Parser;
    auto& File1 = Parser.DeclareNonKeyedOption("file1");
    auto& File2 = Parser.DeclareNonKeyedOption("file2");
    auto& File3 = Parser.DeclareNonKeyedOption("file3");
    auto& Switch = Parser.DeclareSwitchOption("some-switch");

    Parser.ParseOptions(argc, argv, 1);

    EXPECT_EQ(Switch.IsPresent(), true);
    EXPECT_EQ(File1.GetValueAsString(), std::string(argv[1]));
    EXPECT_EQ(File2.GetValueAsString(), std::string(argv[3]));
    EXPECT_EQ(Parser.GetOption("file2").GetValueAsString(), std::string(argv[3]));
    EXPECT_EQ(File3.GetValueAsString(), std::string(argv[4]));
}

template<typename E>
constexpr auto to_underlying(E e) noexcept
{
    return static_cast<std::underlying_type_t<E>>(e);
}

TEST(InCommand, SubCommands)
{
    for(int i = 0; i < 3; ++i)
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

        auto LateDeclareOptions = [](InCommand::CCommandScope& CommandScope)
        {
            switch (static_cast<ScopeId>(CommandScope.GetScopeId()))
            {
            case ScopeId::Root:
                CommandScope.DeclareSwitchOption("help");
                break;
            case ScopeId::Plant:
                CommandScope.DeclareSwitchOption("list");
                break;
            case ScopeId::Tree:
                CommandScope.DeclareSwitchOption("climb");
                break;
            case ScopeId::Shrub:
                CommandScope.DeclareSwitchOption("prune");
                CommandScope.DeclareSwitchOption("burn");
                break;
            case ScopeId::Animal:
                CommandScope.DeclareSwitchOption("list");
                break;
            case ScopeId::Dog:
                CommandScope.DeclareSwitchOption("walk");
                break;
            case ScopeId::Cat:
                CommandScope.DeclareVariableOption("lives", "9");
                break;
            }
        };

        switch (i)
        {
        case 0: {
            const char* argv[] =
            {
                "plant",
                "shrub",
                "--burn",
            };
            int argc = _countof(argv);

            int index = RootCommand.ParseSubcommands(argc, argv, 0);
            InCommand::CCommandScope& CommandScope = RootCommand.GetActiveCommandScope();
            LateDeclareOptions(CommandScope);
            CommandScope.ParseOptions(argc, argv, index);

            EXPECT_EQ(false, RootCommand.IsActive());
            EXPECT_EQ(false, PlantCommand.IsActive());
            EXPECT_EQ(false, TreeCommand.IsActive());
            EXPECT_EQ(true, ShrubCommand.IsActive());
            EXPECT_EQ(false, AnimalCommand.IsActive());
            EXPECT_EQ(false, DogCommand.IsActive());
            EXPECT_EQ(false, CatCommand.IsActive());

            EXPECT_TRUE(CommandScope.GetOption("burn").IsPresent());
            EXPECT_FALSE(CommandScope.GetOption("prune").IsPresent());

            break; }
        case 1: {
            const char* argv[] =
            {
                "animal",
                "cat",
                "--lives",
                "8"
            };
            int argc = _countof(argv);

            int index = RootCommand.ParseSubcommands(argc, argv, 0);
            InCommand::CCommandScope& CommandScope = RootCommand.GetActiveCommandScope();
            LateDeclareOptions(CommandScope);
            CommandScope.ParseOptions(argc, argv, index);

            EXPECT_EQ(false, RootCommand.IsActive());
            EXPECT_EQ(false, PlantCommand.IsActive());
            EXPECT_EQ(false, TreeCommand.IsActive());
            EXPECT_EQ(false, ShrubCommand.IsActive());
            EXPECT_EQ(false, AnimalCommand.IsActive());
            EXPECT_EQ(false, DogCommand.IsActive());
            EXPECT_EQ(true, CatCommand.IsActive());

            EXPECT_EQ(CommandScope.GetOption("lives").GetValueAsString(), std::string("8"));

            break; }
        case 2: {
            const char* argv[] =
            {
                "--help",
            };
            int argc = _countof(argv);

            int index = RootCommand.ParseSubcommands(argc, argv, 0);
            InCommand::CCommandScope& CommandScope = RootCommand.GetActiveCommandScope();
            LateDeclareOptions(CommandScope);
            CommandScope.ParseOptions(argc, argv, index);

            EXPECT_EQ(true, RootCommand.IsActive());
            EXPECT_EQ(false, PlantCommand.IsActive());
            EXPECT_EQ(false, TreeCommand.IsActive());
            EXPECT_EQ(false, ShrubCommand.IsActive());
            EXPECT_EQ(false, AnimalCommand.IsActive());
            EXPECT_EQ(false, DogCommand.IsActive());
            EXPECT_EQ(false, CatCommand.IsActive());

            EXPECT_TRUE(CommandScope.GetOption("help").IsPresent());

            break; }
        }
    }
}
