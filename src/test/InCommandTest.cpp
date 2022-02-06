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

    Parser.ParseArguments(argc, argv, 0); 

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

    Parser.ParseArguments(argc, argv);

    EXPECT_EQ(Switch.IsPresent(), true);
    EXPECT_EQ(File1.GetValueAsString(), std::string(argv[1]));
    EXPECT_EQ(File2.GetValueAsString(), std::string(argv[3]));
    EXPECT_EQ(Parser.GetOption("file2").GetValueAsString(), std::string(argv[3]));
    EXPECT_EQ(File3.GetValueAsString(), std::string(argv[4]));
}

TEST(InCommand, SubCommands)
{
    for(int i = 0; i < 3; ++i)
    {
        InCommand::CCommandScope RootCommand;
        auto& HelpOption = RootCommand.DeclareSwitchOption("help");

        InCommand::CCommandScope& PlantCommand = RootCommand.DeclareSubcommand("plant", 1);
        auto &PlantListOption = PlantCommand.DeclareSwitchOption("list");

        InCommand::CCommandScope& TreeCommand = PlantCommand.DeclareSubcommand("tree", 2);
        auto &TreeClimbOption = TreeCommand.DeclareSwitchOption("climb");

        InCommand::CCommandScope& ShrubCommand = PlantCommand.DeclareSubcommand("shrub", 3);
        auto &ShrubPruneOption = ShrubCommand.DeclareSwitchOption("prune");
        auto &ShrubBurnOption = ShrubCommand.DeclareSwitchOption("burn");

        InCommand::CCommandScope& AnimalCommand = RootCommand.DeclareSubcommand("animal", 4);
        auto &AnimalListOption = AnimalCommand.DeclareSwitchOption("list");

        InCommand::CCommandScope& DogCommand = AnimalCommand.DeclareSubcommand("dog", 5);
        auto &DalkWalkOption = DogCommand.DeclareSwitchOption("walk");

        InCommand::CCommandScope& CatCommand = AnimalCommand.DeclareSubcommand("cat", 6);
        auto &CatLivesOption = CatCommand.DeclareVariableOption("lives", "9");

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

            RootCommand.ParseArguments(argc, argv, 0);

            EXPECT_EQ(3, RootCommand.GetActiveCommandScopeId());
            EXPECT_EQ(false, RootCommand.IsActive());
            EXPECT_EQ(false, PlantCommand.IsActive());
            EXPECT_EQ(false, TreeCommand.IsActive());
            EXPECT_EQ(true, ShrubCommand.IsActive());
            EXPECT_EQ(false, AnimalCommand.IsActive());
            EXPECT_EQ(false, DogCommand.IsActive());
            EXPECT_EQ(false, CatCommand.IsActive());

            EXPECT_TRUE(ShrubBurnOption.IsPresent());
            EXPECT_FALSE(ShrubPruneOption.IsPresent());

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

            RootCommand.ParseArguments(argc, argv, 0);

            EXPECT_EQ(6, RootCommand.GetActiveCommandScopeId());
            EXPECT_EQ(false, RootCommand.IsActive());
            EXPECT_EQ(false, PlantCommand.IsActive());
            EXPECT_EQ(false, TreeCommand.IsActive());
            EXPECT_EQ(false, ShrubCommand.IsActive());
            EXPECT_EQ(false, AnimalCommand.IsActive());
            EXPECT_EQ(false, DogCommand.IsActive());
            EXPECT_EQ(true, CatCommand.IsActive());

            EXPECT_EQ(CatLivesOption.GetValueAsString(), std::string("8"));

            break; }
        case 2: {
            const char* argv[] =
            {
                "--help",
            };
            int argc = _countof(argv);

            RootCommand.ParseArguments(argc, argv, 0);

            EXPECT_EQ(0, RootCommand.GetActiveCommandScopeId());
            EXPECT_EQ(true, RootCommand.IsActive());
            EXPECT_EQ(false, PlantCommand.IsActive());
            EXPECT_EQ(false, TreeCommand.IsActive());
            EXPECT_EQ(false, ShrubCommand.IsActive());
            EXPECT_EQ(false, AnimalCommand.IsActive());
            EXPECT_EQ(false, DogCommand.IsActive());
            EXPECT_EQ(false, CatCommand.IsActive());

            EXPECT_TRUE(HelpOption.IsPresent());

            break; }
        }
    }
}
