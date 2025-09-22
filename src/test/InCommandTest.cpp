#include <gtest/gtest.h>
#include <algorithm>
#include <iostream>
#include <sstream>

#include "InCommand.h"

TEST(InCommand, HelpStringGeneration)
{
    // Create parser
    InCommand::CommandParser parser("myapp");
    auto& rootCmdDesc = parser.GetAppCommandDecl();
    rootCmdDesc.SetDescription("My application");

    // Add some options to the root command
    rootCmdDesc.AddOption(InCommand::OptionType::Switch, "verbose").SetDescription("Show help information");
    rootCmdDesc.AddOption(InCommand::OptionType::Variable, "config").SetDescription("Configuration file path");

    // Add inner command blocks
    auto& buildBlockDescDesc = rootCmdDesc.AddSubCommand("build");
    buildBlockDescDesc.SetDescription("Build the project");
    buildBlockDescDesc.AddOption(InCommand::OptionType::Switch, "verbose").SetDescription("Enable verbose output");
    buildBlockDescDesc.AddOption(InCommand::OptionType::Variable, "target").SetDescription("Build target");

    auto& testCmdDesc = rootCmdDesc.AddSubCommand("test");
    testCmdDesc.SetDescription("Run tests");
    testCmdDesc.AddOption(InCommand::OptionType::Switch, "coverage").SetDescription("Generate coverage report");

    // Test GetHelpString(0) for root command help after parsing
    const char* rootArgs[] = {"myapp"};
    auto result = parser.ParseArgs(1, rootArgs);
    EXPECT_EQ(1, result);
    std::string helpString = parser.GetHelpString(0);
    EXPECT_FALSE(helpString.empty());
    EXPECT_TRUE(helpString.find("myapp") != std::string::npos);
    EXPECT_TRUE(helpString.find("--verbose") != std::string::npos);
    EXPECT_TRUE(helpString.find("--config") != std::string::npos);
    EXPECT_TRUE(helpString.find("Show help information") != std::string::npos);
    EXPECT_TRUE(helpString.find("Configuration file path") != std::string::npos);
    EXPECT_TRUE(helpString.find("Build the project") != std::string::npos);
    EXPECT_TRUE(helpString.find("Run tests") != std::string::npos);

    // Test GetHelpString() for rightmost command block (build subcommand)
    const char* buildArgs[] = {"myapp", "build"};
    auto buildResult = parser.ParseArgs(2, buildArgs);
    EXPECT_EQ(2, buildResult);
    std::string buildHelp = parser.GetHelpString(); // Should target rightmost (build command)
    EXPECT_TRUE(buildHelp.find("build") != std::string::npos);
    EXPECT_TRUE(buildHelp.find("--verbose") != std::string::npos); // Global option
    EXPECT_TRUE(buildHelp.find("--target") != std::string::npos);  // Local option
    EXPECT_TRUE(buildHelp.find("Enable verbose output") != std::string::npos);
    EXPECT_TRUE(buildHelp.find("Build target") != std::string::npos);
}

TEST(InCommand, BasicOptions)
{
    // Create a command block description hierarchy for testing basic option parsing
    InCommand::CommandParser parser("test");
    InCommand::CommandDecl& appBlockDesc = parser.GetAppCommandDecl();
    
    // Add global help switch with alias
    appBlockDesc.AddOption(InCommand::OptionType::Switch, "verbose", 'v');
    
    // Create inner command blocks
    auto& fooCmd = appBlockDesc.AddSubCommand("foo");
    fooCmd.AddOption(InCommand::OptionType::Variable, "number");
    
    auto& barCmd = appBlockDesc.AddSubCommand("bar");
    barCmd.AddOption(InCommand::OptionType::Variable, "word");
    barCmd.AddOption(InCommand::OptionType::Variable, "name", 'n');
    
    auto& bazCmd = barCmd.AddSubCommand("baz");
    bazCmd.AddOption(InCommand::OptionType::Variable, "color")
        .SetDomain({"red", "green", "blue", "yellow", "purple"});
    
    auto& zapCmd = appBlockDesc.AddSubCommand("zap");
    zapCmd.AddOption(InCommand::OptionType::Parameter, "file1");
    zapCmd.AddOption(InCommand::OptionType::Parameter, "file2");

    // Test 1: --verbose foo --number 42
    {
        const char *argv[] = {"test", "--verbose", "foo", "--number", "42"};
        int argc = std::size(argv);

        InCommand::CommandParser parser("test");
        auto& testRootCmdDesc = parser.GetAppCommandDecl();
        testRootCmdDesc = appBlockDesc;
        size_t numBlocks = parser.ParseArgs(argc, argv);
        const InCommand::CommandBlock &cmdBlock = parser.GetCommandBlock(numBlocks - 1);
        
        // Should be at foo level, but help is at root level
        EXPECT_TRUE(parser.GetCommandBlock(0).IsOptionSet("verbose"));
        EXPECT_EQ(cmdBlock.GetOptionValue("number"), "42");
    }

    // Test 2: bar --word hello baz --color red
    {
        const char *argv[] = {"test", "bar", "--word", "hello", "baz", "--color", "red"};
        int argc = std::size(argv);

        InCommand::CommandParser parser("test");
        auto& testRootCmdDesc = parser.GetAppCommandDecl();
        testRootCmdDesc = appBlockDesc;
        size_t numBlocks = parser.ParseArgs(argc, argv);
        const InCommand::CommandBlock &cmdBlock = parser.GetCommandBlock(numBlocks - 1);
        
        // Should be at baz level
        EXPECT_EQ(numBlocks, 3); // root -> bar -> baz
        EXPECT_FALSE(parser.GetCommandBlock(0).IsOptionSet("verbose"));
        EXPECT_EQ(parser.GetCommandBlock(1).GetOptionValue("word", "goodbye"), "hello");
        EXPECT_FALSE(parser.GetCommandBlock(1).IsOptionSet("name"));
        EXPECT_EQ(parser.GetCommandBlock(1).GetOptionValue("name", "Bill"), "Bill");
        EXPECT_TRUE(cmdBlock.IsOptionSet("color"));
        EXPECT_EQ(cmdBlock.GetOptionValue("color", "blue"), "red");
    }

    // Test 3: -h (short alias)
    {
        const char *argv[] = {"test", "-v"};
        int argc = std::size(argv);

        InCommand::CommandParser parser("test");
        auto& testRootCmdDesc = parser.GetAppCommandDecl();
        testRootCmdDesc = appBlockDesc;
        size_t numBlocks = parser.ParseArgs(argc, argv);
        const InCommand::CommandBlock &cmdBlock = parser.GetCommandBlock(numBlocks - 1);
        
        EXPECT_TRUE(cmdBlock.IsOptionSet("verbose"));
    }

    // Test 4: --verbose (long form)
    {
        const char *argv[] = {"app", "--verbose"};
        int argc = std::size(argv);

        InCommand::CommandParser parser("testapp");
        auto& testRootCmdDesc = parser.GetAppCommandDecl();
        testRootCmdDesc = appBlockDesc;
        size_t numBlocks = parser.ParseArgs(argc, argv);
        const InCommand::CommandBlock &cmdBlock = parser.GetCommandBlock(numBlocks - 1);
        
        EXPECT_TRUE(cmdBlock.IsOptionSet("verbose"));
    }

    // Test 5: bar --name Anna (with alias)
    {
        const char *argv[] = {"app", "bar", "--name", "Anna"};
        int argc = std::size(argv);

        InCommand::CommandParser parser("testapp"); auto& testRootCmdDesc = parser.GetAppCommandDecl(); testRootCmdDesc = appBlockDesc;
        size_t numBlocks = parser.ParseArgs(argc, argv);
        const InCommand::CommandBlock &cmdBlock = parser.GetCommandBlock(numBlocks - 1);
        
        EXPECT_EQ(cmdBlock.GetOptionValue("name"), "Anna");
    }

    // Test 6: bar -n Anna (short alias)
    {
        const char *argv[] = {"app", "bar", "-n", "Anna"};
        int argc = std::size(argv);

        InCommand::CommandParser parser("testapp"); auto& testRootCmdDesc = parser.GetAppCommandDecl(); testRootCmdDesc = appBlockDesc;
        size_t numBlocks = parser.ParseArgs(argc, argv);
        const InCommand::CommandBlock &cmdBlock = parser.GetCommandBlock(numBlocks - 1);
        
        EXPECT_EQ(cmdBlock.GetOptionValue("name"), "Anna");
    }
}

TEST(InCommand, Parameters)
{
    // Create command with parameters
    InCommand::CommandParser parser("app");
    InCommand::CommandDecl& appBlockDesc = parser.GetAppCommandDecl();
    appBlockDesc.AddOption(InCommand::OptionType::Parameter, "file1").SetDescription("file 1");
    appBlockDesc.AddOption(InCommand::OptionType::Parameter, "file2").SetDescription("file 2");
    appBlockDesc.AddOption(InCommand::OptionType::Parameter, "file3").SetDescription("file 3");
    appBlockDesc.AddOption(InCommand::OptionType::Switch, "some-switch").SetDescription("Some switch");

    // Test 1: All parameters provided with switch in middle
    {
        const char *argv[] = {"foo", "myfile1.txt", "--some-switch", "myfile2.txt", "myfile3.txt"};
        const int argc = sizeof(argv) / sizeof(argv[0]);

        InCommand::CommandParser parser("testapp"); auto& testRootCmdDesc = parser.GetAppCommandDecl(); testRootCmdDesc = appBlockDesc;
        size_t numBlocks = parser.ParseArgs(argc, argv);
        const InCommand::CommandBlock &cmdBlock = parser.GetCommandBlock(numBlocks - 1);

        EXPECT_TRUE(cmdBlock.IsOptionSet("some-switch"));
        EXPECT_EQ(cmdBlock.GetOptionValue("file1", ""), std::string("myfile1.txt"));
        EXPECT_EQ(cmdBlock.GetOptionValue("file2", ""), std::string("myfile2.txt"));
        EXPECT_EQ(cmdBlock.GetOptionValue("file3", ""), std::string("myfile3.txt"));
        EXPECT_TRUE(cmdBlock.IsOptionSet("file1"));
        EXPECT_TRUE(cmdBlock.IsOptionSet("file2"));
        EXPECT_TRUE(cmdBlock.IsOptionSet("file3"));
    }

    // Test 2: Only first two parameters provided
    {
        const char *argv[] = {"foo", "myfile1.txt", "--some-switch", "myfile2.txt"};
        const int argc = sizeof(argv) / sizeof(argv[0]);

        InCommand::CommandParser parser("testapp"); auto& testRootCmdDesc = parser.GetAppCommandDecl(); testRootCmdDesc = appBlockDesc;
        size_t numBlocks = parser.ParseArgs(argc, argv);
        const InCommand::CommandBlock &cmdBlock = parser.GetCommandBlock(numBlocks - 1);

        EXPECT_TRUE(cmdBlock.IsOptionSet("some-switch"));
        EXPECT_EQ(cmdBlock.GetOptionValue("file1", ""), std::string("myfile1.txt"));
        EXPECT_EQ(cmdBlock.GetOptionValue("file2", ""), std::string("myfile2.txt"));
        EXPECT_EQ(cmdBlock.GetOptionValue("file3", "nope"), "nope");
        EXPECT_TRUE(cmdBlock.IsOptionSet("file1"));
        EXPECT_TRUE(cmdBlock.IsOptionSet("file2"));
        EXPECT_FALSE(cmdBlock.IsOptionSet("file3"));
    }
}

TEST(InCommand, SubCategories)
{
    // Create nested command structure
    InCommand::CommandParser parser("app");
    InCommand::CommandDecl& appBlockDesc = parser.GetAppCommandDecl();
    appBlockDesc.AddOption(InCommand::OptionType::Switch, "verbose");

    // plant -> tree, shrub
    auto& plantCmd = appBlockDesc.AddSubCommand("plant");
    plantCmd.AddOption(InCommand::OptionType::Switch, "list");
    
    auto& treeCmd = plantCmd.AddSubCommand("tree");
    
    auto& shrubCmd = plantCmd.AddSubCommand("shrub");
    shrubCmd.AddOption(InCommand::OptionType::Switch, "prune");
    shrubCmd.AddOption(InCommand::OptionType::Switch, "burn");

    // animal -> dog, cat
    auto& animalCmd = appBlockDesc.AddSubCommand("animal");
    
    auto& dogCmd = animalCmd.AddSubCommand("dog");
    
    auto& catCmd = animalCmd.AddSubCommand("cat");
    catCmd.AddOption(InCommand::OptionType::Variable, "lives");

    // Test 1: plant shrub --burn
    {
        const char* argv[] = {"app", "plant", "shrub", "--burn"};
        const int argc = sizeof(argv) / sizeof(argv[0]);

        InCommand::CommandParser parser("testapp"); auto& testRootCmdDesc = parser.GetAppCommandDecl(); testRootCmdDesc = appBlockDesc;
        size_t numBlocks = parser.ParseArgs(argc, argv);
        const InCommand::CommandBlock &cmdBlock = parser.GetCommandBlock(numBlocks - 1);
        
        // Should be at the shrub command level
        EXPECT_EQ(&cmdBlock.GetDecl(), &shrubCmd);
        EXPECT_TRUE(cmdBlock.IsOptionSet("burn"));
        EXPECT_FALSE(cmdBlock.IsOptionSet("prune"));
    }

    // Test 2: animal cat --lives 8
    {
        const char* argv[] = {"app", "animal", "cat", "--lives", "8"};
        const int argc = sizeof(argv) / sizeof(argv[0]);

        InCommand::CommandParser parser("testapp"); auto& testRootCmdDesc = parser.GetAppCommandDecl(); testRootCmdDesc = appBlockDesc;
        size_t numBlocks = parser.ParseArgs(argc, argv);
        const InCommand::CommandBlock &cmdBlock = parser.GetCommandBlock(numBlocks - 1);
        
        // Should be at the cat command level
        EXPECT_EQ(&cmdBlock.GetDecl(), &catCmd);
        EXPECT_TRUE(cmdBlock.IsOptionSet("lives"));
        EXPECT_EQ(cmdBlock.GetOptionValue("lives", "9"), "8");
    }

    // Test 3: --verbose (root level)
    {
        const char* argv[] = {"app", "--verbose"};
        const int argc = sizeof(argv) / sizeof(argv[0]);

        InCommand::CommandParser parser("testapp"); auto& testRootCmdDesc = parser.GetAppCommandDecl(); testRootCmdDesc = appBlockDesc;
        size_t numBlocks = parser.ParseArgs(argc, argv);
        const InCommand::CommandBlock &cmdBlock = parser.GetCommandBlock(numBlocks - 1);
        
        // Should be at the root command level
        EXPECT_EQ(&cmdBlock.GetDecl(), &parser.GetAppCommandDecl());
        EXPECT_TRUE(cmdBlock.IsOptionSet("verbose"));
    }
}

TEST(InCommand, Errors)
{
    // Test 1: Duplicate command block
    {
        InCommand::CommandParser parser("app");
        InCommand::CommandDecl& appBlockDesc = parser.GetAppCommandDecl();
        appBlockDesc.AddSubCommand("goto");
        
        EXPECT_THROW(
            {
                appBlockDesc.AddSubCommand("goto"); // Duplicate
            }, InCommand::ApiException);
    }
    
    // Test 2: Duplicate option
    {
        InCommand::CommandParser parser2("app");
        InCommand::CommandDecl& appBlockDesc = parser2.GetAppCommandDecl();
        appBlockDesc.AddOption(InCommand::OptionType::Switch, "foo");
        
        EXPECT_THROW(
            {
                appBlockDesc.AddOption(InCommand::OptionType::Switch, "foo"); // Duplicate
            }, InCommand::ApiException);
    }

    // Test 3: Unexpected argument (non-existent command)
    {
        const char* argv[] = {"app", "gogo", "--foo", "--bar", "7"};
        const int argc = sizeof(argv) / sizeof(argv[0]);

        InCommand::CommandParser parser3("app");
        InCommand::CommandDecl& appBlockDesc = parser3.GetAppCommandDecl();
        auto& gotoCmd = appBlockDesc.AddSubCommand("goto");
        gotoCmd.AddOption(InCommand::OptionType::Switch, "foo");
        gotoCmd.AddOption(InCommand::OptionType::Variable, "bar");

        EXPECT_THROW(
            {
                parser3.ParseArgs(argc, argv);
            }, InCommand::SyntaxException);
    }

    // Test 4: Unknown option
    {
        const char* argv[] = {"app", "goto", "--foo", "--bar", "7"};
        const int argc = sizeof(argv) / sizeof(argv[0]);

        InCommand::CommandParser parser4("app");
        InCommand::CommandDecl& appBlockDesc = parser4.GetAppCommandDecl();
        auto& gotoCmd = appBlockDesc.AddSubCommand("goto");
        gotoCmd.AddOption(InCommand::OptionType::Switch, "fop"); // Note: "fop" not "foo"
        gotoCmd.AddOption(InCommand::OptionType::Variable, "bar");

        EXPECT_THROW(
            {
                parser4.ParseArgs(argc, argv);
            }, InCommand::SyntaxException);
    }

    // Test 5: Missing variable value
    {
        const char* argv[] = {"app", "--foo"};
        const int argc = sizeof(argv) / sizeof(argv[0]);

        InCommand::CommandParser parser5("app");
        InCommand::CommandDecl& appBlockDesc = parser5.GetAppCommandDecl();
        appBlockDesc.AddOption(InCommand::OptionType::Variable, "foo");

        EXPECT_THROW(
            {
                parser5.ParseArgs(argc, argv);
            }, InCommand::SyntaxException);
    }

    // Test 6: Missing variable value (followed by another option)
    {
        const char* argv[] = {"app", "--foo", "--bar"};
        const int argc = sizeof(argv) / sizeof(argv[0]);

        InCommand::CommandParser parser6("app");
        InCommand::CommandDecl& appBlockDesc = parser6.GetAppCommandDecl();
        appBlockDesc.AddOption(InCommand::OptionType::Variable, "foo");
        appBlockDesc.AddOption(InCommand::OptionType::Switch, "bar");

        EXPECT_THROW(
            {
                parser6.ParseArgs(argc, argv);
            }, InCommand::SyntaxException);
    }

    // Test 7: Too many parameters
    {
        const char* argv[] = {"app", "param1", "param2", "param3"};
        const int argc = sizeof(argv) / sizeof(argv[0]);

        InCommand::CommandParser parser7("app");
        InCommand::CommandDecl& appBlockDesc = parser7.GetAppCommandDecl();
        appBlockDesc.AddOption(InCommand::OptionType::Parameter, "file1");
        // Only one parameter defined, but three provided

        EXPECT_THROW(
            {
                parser7.ParseArgs(argc, argv);
            }, InCommand::SyntaxException);
    }

    // Test 8: Test alias functionality
    {
        const char* argv[] = {"app", "-vq"};
        const int argc = sizeof(argv) / sizeof(argv[0]);

        InCommand::CommandParser parser8("app");
        InCommand::CommandDecl& appBlockDesc = parser8.GetAppCommandDecl();
        appBlockDesc.AddOption(InCommand::OptionType::Switch, "verbose", 'v');
        appBlockDesc.AddOption(InCommand::OptionType::Switch, "quiet", 'q');

        auto numBlocks = parser8.ParseArgs(argc, argv);
        const InCommand::CommandBlock &cmdBlock = parser8.GetCommandBlock(numBlocks - 1);
        
        EXPECT_TRUE(cmdBlock.IsOptionSet("verbose"));
        EXPECT_TRUE(cmdBlock.IsOptionSet("quiet"));
    }
}

TEST(InCommand, ParameterAliasValidation)
{
    // Test that parameters cannot have aliases
    InCommand::CommandParser parser("testapp");
    auto& rootCmdDesc = parser.GetAppCommandDecl();
    
    // This should throw an ApiException with InvalidOptionType
    EXPECT_THROW(
        {
            rootCmdDesc.AddOption(InCommand::OptionType::Parameter, "filename", 'f');
        }, InCommand::ApiException);
    
    // But switches and variables should work fine
    EXPECT_NO_THROW(
        {
            rootCmdDesc.AddOption(InCommand::OptionType::Switch, "verbose", 'v');
            rootCmdDesc.AddOption(InCommand::OptionType::Variable, "output", 'o');
        });
}

TEST(InCommand, VariableDelimiters)
{
    // Test parser with = as delimiter
    InCommand::CommandParser parser("myapp", InCommand::VariableDelimiter::Equals);
    auto& rootCmdDesc = parser.GetAppCommandDecl();
    
    // Add options
    rootCmdDesc.AddOption(InCommand::OptionType::Variable, "name", 'n').SetDescription("User name");
    rootCmdDesc.AddOption(InCommand::OptionType::Variable, "output").SetDescription("Output file");
    rootCmdDesc.AddOption(InCommand::OptionType::Switch, "verbose", 'v').SetDescription("Verbose mode");

    // Test --name=value format
    {
        const char* args[] = {"myapp", "--name=John", "--output=file.txt", "-v"};
        auto numBlocks = parser.ParseArgs(4, args);
        const InCommand::CommandBlock &result = parser.GetCommandBlock(numBlocks - 1);

        EXPECT_TRUE(result.IsOptionSet("name"));
        EXPECT_EQ(result.GetOptionValue("name"), "John");
        
        EXPECT_TRUE(result.IsOptionSet("output"));
        EXPECT_EQ(result.GetOptionValue("output"), "file.txt");
        
        EXPECT_TRUE(result.IsOptionSet("verbose"));
    }

    // Test -n=value format (short option with delimiter)
    {
        const char* args[] = {"myapp", "-n=Jane", "-v"};
        size_t numBlocks = parser.ParseArgs(3, args);
        const InCommand::CommandBlock &result = parser.GetCommandBlock(numBlocks - 1);
        
        EXPECT_TRUE(result.IsOptionSet("name"));
        EXPECT_EQ(result.GetOptionValue("name"), "Jane");
        
        EXPECT_TRUE(result.IsOptionSet("verbose"));
    }

    // Test mixed formats (traditional space-separated and delimiter-based)
    {
        const char* args[] = {"myapp", "--name=Bob", "--output", "result.txt", "-v"};
        size_t numBlocks = parser.ParseArgs(5, args);
        const InCommand::CommandBlock &result = parser.GetCommandBlock(numBlocks - 1);
        
        EXPECT_TRUE(result.IsOptionSet("name"));
        EXPECT_EQ(result.GetOptionValue("name"), "Bob");
        
        EXPECT_TRUE(result.IsOptionSet("output"));
        EXPECT_EQ(result.GetOptionValue("output"), "result.txt");
        
        EXPECT_TRUE(result.IsOptionSet("verbose"));
    }

    // Test error: switch with delimiter should fail
    {
        const char* args[] = {"myapp", "--verbose=true"};
        EXPECT_THROW(parser.ParseArgs(2, args), InCommand::SyntaxException);
    }

    // Test error: short switch with delimiter should fail
    {
        const char* args[] = {"myapp", "-v=true"};
        EXPECT_THROW(parser.ParseArgs(2, args), InCommand::SyntaxException);
    }

    // Test parser with colon delimiter
    InCommand::CommandParser colonParser("myapp", InCommand::VariableDelimiter::Colon);
    auto& colonRootCmdDesc = colonParser.GetAppCommandDecl();
    colonRootCmdDesc.AddOption(InCommand::OptionType::Variable, "name").SetDescription("User name");

    {
        const char* args[] = {"myapp", "--name:Alice"};
        size_t numBlocks = colonParser.ParseArgs(2, args);
        const InCommand::CommandBlock &result = colonParser.GetCommandBlock(numBlocks - 1);
        
        EXPECT_TRUE(result.IsOptionSet("name"));
        EXPECT_EQ(result.GetOptionValue("name"), "Alice");
    }

    // Test parser with explicit whitespace delimiter (traditional format only)
    InCommand::CommandParser whitespaceParser("myapp", InCommand::VariableDelimiter::Whitespace);
    auto& whitespaceRootCmdDesc = whitespaceParser.GetAppCommandDecl();
    whitespaceRootCmdDesc.AddOption(InCommand::OptionType::Variable, "name").SetDescription("User name");
    whitespaceRootCmdDesc.AddOption(InCommand::OptionType::Switch, "verbose").SetDescription("Verbose mode");

    // Traditional whitespace-separated format should work
    {
        const char* args[] = {"myapp", "--name", "Alice", "--verbose"};
        size_t numBlocks = whitespaceParser.ParseArgs(4, args);
        const InCommand::CommandBlock &result = whitespaceParser.GetCommandBlock(numBlocks - 1);
        
        EXPECT_TRUE(result.IsOptionSet("name"));
        EXPECT_EQ(result.GetOptionValue("name"), "Alice");
        EXPECT_TRUE(result.IsOptionSet("verbose"));
    }

    // Delimiter format should not work (treated as unknown option)
    {
        const char* args[] = {"myapp", "--name=Alice"};
        EXPECT_THROW(whitespaceParser.ParseArgs(2, args), InCommand::SyntaxException);
    }

    // Test parser without explicit delimiter (defaults to whitespace - traditional format only)
    InCommand::CommandParser traditionalParser("myapp");
    auto& traditionalRootCmdDesc = traditionalParser.GetAppCommandDecl();
    traditionalRootCmdDesc.AddOption(InCommand::OptionType::Variable, "name").SetDescription("User name");

    // Traditional format should work with default constructor
    {
        const char* args[] = {"myapp", "--name", "Bob"};
        size_t numBlocks = traditionalParser.ParseArgs(3, args);
        const InCommand::CommandBlock &result = traditionalParser.GetCommandBlock(numBlocks - 1);
        
        EXPECT_TRUE(result.IsOptionSet("name"));
        EXPECT_EQ(result.GetOptionValue("name"), "Bob");
    }
}

TEST(InCommand, MidChainCommandBlocksWithParameters)
{
    // Test command structure: app container run image_name
    // where 'container' is mid-chain and has parameters, 'run' is final command
    InCommand::CommandParser parser("app");
    InCommand::CommandDecl& appBlockDesc = parser.GetAppCommandDecl();
    
    auto& containerCmd = appBlockDesc.AddSubCommand("container");
    containerCmd.AddOption(InCommand::OptionType::Parameter, "container_id").SetDescription("Container ID");
    containerCmd.AddOption(InCommand::OptionType::Switch, "all", 'a').SetDescription("Show all containers");
    
    auto& runCmd = containerCmd.AddSubCommand("run");
    runCmd.AddOption(InCommand::OptionType::Parameter, "image").SetDescription("Image name");
    runCmd.AddOption(InCommand::OptionType::Variable, "port", 'p').SetDescription("Port mapping");
    
    // Test: app container 12345 run ubuntu --port 8080
    {
        const char* argv[] = {"app", "container", "12345", "run", "ubuntu", "--port", "8080"};
        int argc = std::size(argv);

        InCommand::CommandParser parser("testapp");
        auto& testRootCmdDesc = parser.GetAppCommandDecl();
        testRootCmdDesc = appBlockDesc;
        auto numBlocks = parser.ParseArgs(argc, argv);
        const InCommand::CommandBlock &cmdBlock = parser.GetCommandBlock(numBlocks - 1);

        // Should be at 'run' command level
        EXPECT_EQ(cmdBlock.GetOptionValue("image"), "ubuntu");
        EXPECT_EQ(cmdBlock.GetOptionValue("port"), "8080");
        
        // Check that mid-chain 'container' command received its parameter
        const InCommand::CommandBlock &containerBlock = parser.GetCommandBlock(1);
        EXPECT_TRUE(containerBlock.IsOptionSet("container_id"));
        EXPECT_EQ(containerBlock.GetOptionValue("container_id"), "12345");
    }
    
    // Test: app container --all run ubuntu (switch on mid-chain)
    {
        const char* argv[] = {"app", "container", "--all", "run", "ubuntu"};
        int argc = std::size(argv);

        InCommand::CommandParser parser("testapp");
        auto& testRootCmdDesc = parser.GetAppCommandDecl();
        testRootCmdDesc = appBlockDesc;
        auto numBlocks = parser.ParseArgs(argc, argv);
        const InCommand::CommandBlock &cmdBlock = parser.GetCommandBlock(numBlocks - 1);

        // Should be at 'run' command level
        EXPECT_EQ(cmdBlock.GetOptionValue("image"), "ubuntu");
        
        // Check that mid-chain 'container' command has switch set
        EXPECT_EQ(parser.GetNumCommandBlocks(), 3);
        EXPECT_TRUE(parser.GetCommandBlock(1).IsOptionSet("all"));
        EXPECT_FALSE(parser.GetCommandBlock(1).IsOptionSet("container_id"));
    }
}

TEST(InCommand, ParametersWithSubCommandNameCollisions)
{
    // Test when parameter names collide with sub-command names
    InCommand::CommandParser parser("app");
    InCommand::CommandDecl& appBlockDesc = parser.GetAppCommandDecl();
    appBlockDesc.AddOption(InCommand::OptionType::Parameter, "build").SetDescription("Build file parameter");
    appBlockDesc.AddOption(InCommand::OptionType::Parameter, "test").SetDescription("Test file parameter");
    
    // Add sub-commands with same names as parameters
    auto& buildBlockDesc = appBlockDesc.AddSubCommand("build");
    buildBlockDesc.AddOption(InCommand::OptionType::Switch, "verbose").SetDescription("Verbose build");
    
    auto& testCmd = appBlockDesc.AddSubCommand("test");
    testCmd.AddOption(InCommand::OptionType::Switch, "coverage").SetDescription("Coverage report");
    
    // Test 1: Use as parameter (positional arguments)
    {
        const char* argv[] = {"app", "mybuild.json", "mytest.json"};
        int argc = std::size(argv);

        InCommand::CommandParser parser("testapp");
        auto& testRootCmdDesc = parser.GetAppCommandDecl();
        testRootCmdDesc = appBlockDesc;
        auto numBlocks = parser.ParseArgs(argc, argv);
        const InCommand::CommandBlock &cmdBlock = parser.GetCommandBlock(numBlocks - 1);

        // Should stay at root level and treat as parameters
        EXPECT_EQ(&cmdBlock.GetDecl(), &parser.GetAppCommandDecl());
        EXPECT_TRUE(cmdBlock.IsOptionSet("build"));
        EXPECT_TRUE(cmdBlock.IsOptionSet("test"));
        EXPECT_EQ(cmdBlock.GetOptionValue("build"), "mybuild.json");
        EXPECT_EQ(cmdBlock.GetOptionValue("test"), "mytest.json");
    }
    
    // Test 2: Use as sub-command (when followed by options)
    {
        const char* argv[] = {"app", "build", "--verbose"};
        int argc = std::size(argv);

        InCommand::CommandParser parser("testapp");
        auto& testRootCmdDesc = parser.GetAppCommandDecl();
        testRootCmdDesc = appBlockDesc;
        auto numBlocks = parser.ParseArgs(argc, argv);
        const InCommand::CommandBlock &cmdBlock = parser.GetCommandBlock(numBlocks - 1);

        // Should be at build command level
        EXPECT_EQ(&cmdBlock.GetDecl(), &buildBlockDesc);
        EXPECT_TRUE(cmdBlock.IsOptionSet("verbose"));
    }
    
    // Test 3: Use as sub-command (standalone)
    {
        const char* argv[] = {"app", "test"};
        int argc = std::size(argv);

        InCommand::CommandParser parser("testapp");
        auto& testRootCmdDesc = parser.GetAppCommandDecl();
        testRootCmdDesc = appBlockDesc;
        auto numBlocks = parser.ParseArgs(argc, argv);
        const InCommand::CommandBlock &cmdBlock = parser.GetCommandBlock(numBlocks - 1);

        // Should be at test command level
        EXPECT_EQ(&cmdBlock.GetDecl(), &testCmd);
        EXPECT_FALSE(cmdBlock.IsOptionSet("coverage"));
    }
}

TEST(InCommand, DuplicateAliasCharacters)
{
    // Test that duplicate alias characters across different options throw ApiException
    InCommand::CommandParser parser("app");
    InCommand::CommandDecl& appBlockDesc = parser.GetAppCommandDecl();
    
    // First option with alias 'v'
    appBlockDesc.AddOption(InCommand::OptionType::Switch, "verbose", 'v');
    
    // Test 1: Duplicate alias in same command block should throw
    EXPECT_THROW(
        {
            appBlockDesc.AddOption(InCommand::OptionType::Switch, "version", 'v'); // Duplicate 'v'
        }, InCommand::ApiException);
    
    // Test 2: Duplicate alias with different option type should also throw
    EXPECT_THROW(
        {
            appBlockDesc.AddOption(InCommand::OptionType::Variable, "value", 'v'); // Duplicate 'v'
        }, InCommand::ApiException);
    
    // Test 3: Same alias in different command blocks should be allowed
    auto& subCmdBlockDesc = appBlockDesc.AddSubCommand("sub");
    EXPECT_NO_THROW(
        {
            subCmdBlockDesc.AddOption(InCommand::OptionType::Switch, "verify", 'v'); // Same 'v' in different block is OK
        });
    
    // Test 4: Verify both aliases work independently
    {
        const char* argv[] = {"app", "-v", "sub", "-v"};
        int argc = std::size(argv);

        InCommand::CommandParser parser("testapp");
        auto& testRootCmdDesc = parser.GetAppCommandDecl();
        testRootCmdDesc = appBlockDesc;
        auto numBlocks = parser.ParseArgs(argc, argv);
        const InCommand::CommandBlock &cmdBlock = parser.GetCommandBlock(numBlocks - 1);

        // Should be at sub command level
        EXPECT_EQ(&cmdBlock.GetDecl(), &subCmdBlockDesc);
        EXPECT_TRUE(cmdBlock.IsOptionSet("verify"));
        
        // Check that root command also had its verbose set
        const InCommand::CommandBlock &firstBlock = parser.GetCommandBlock(0);
        EXPECT_TRUE(firstBlock.IsOptionSet("verbose"));
    }
    
    // Test 5: Multiple different aliases should work fine
    EXPECT_NO_THROW(
        {
            appBlockDesc.AddOption(InCommand::OptionType::Switch, "quiet", 'q');
            appBlockDesc.AddOption(InCommand::OptionType::Variable, "output", 'o');
            appBlockDesc.AddOption(InCommand::OptionType::Switch, "reset", 'r');
        });
}

// Custom converters for testing
struct UppercaseConverter
{
    std::string operator()(const std::string& str) const
    {
        std::string result = str;
        std::transform(result.begin(), result.end(), result.begin(), ::toupper);
        return result;
    }
};

// Custom enum type for testing
enum class LogLevel
{
    Debug,
    Info,
    Warning,
    Error
};

// Custom converter for enum class
struct LogLevelConverter
{
    LogLevel operator()(const std::string& str) const
    {
        if (str == "debug") return LogLevel::Debug;
        if (str == "info") return LogLevel::Info;
        if (str == "warning") return LogLevel::Warning;
        if (str == "error") return LogLevel::Error;
        throw InCommand::SyntaxException(InCommand::SyntaxError::InvalidValue, 
                                        "Invalid log level (expected: debug, info, warning, error)", str);
    }
};

TEST(InCommand, TemplateBinding_FundamentalTypes)
{
    InCommand::CommandParser parser("testapp");
    auto& appBlockDesc = parser.GetAppCommandDecl();
    
    // Test variables for binding
    int intValue = 0;
    float floatValue = 0.0f;
    double doubleValue = 0.0;
    char charValue = '\0';
    std::string stringValue;
    
    // Add options with bindings
    appBlockDesc.AddOption(InCommand::OptionType::Variable, "int-opt").BindTo(intValue);
    appBlockDesc.AddOption(InCommand::OptionType::Variable, "float-opt").BindTo(floatValue);
    appBlockDesc.AddOption(InCommand::OptionType::Variable, "double-opt").BindTo(doubleValue);
    appBlockDesc.AddOption(InCommand::OptionType::Variable, "char-opt").BindTo(charValue);
    appBlockDesc.AddOption(InCommand::OptionType::Variable, "string-opt").BindTo(stringValue);
    
    // Test arguments
    const char *argv[] =
    {
        "testapp",
        "--int-opt", "42",
        "--float-opt", "3.14",
        "--double-opt", "2.718",
        "--char-opt", "A",
        "--string-opt", "hello"
    };
    int argc = sizeof(argv) / sizeof(argv[0]);
    
    // Parse arguments
    parser.ParseArgs(argc, argv);
    
    // Verify that values were bound correctly
    EXPECT_EQ(intValue, 42);
    EXPECT_FLOAT_EQ(floatValue, 3.14f);
    EXPECT_DOUBLE_EQ(doubleValue, 2.718);
    EXPECT_EQ(charValue, 'A');
    EXPECT_EQ(stringValue, "hello");
}

TEST(InCommand, TemplateBinding_Parameters)
{
    InCommand::CommandParser parser("testapp");
    auto& appBlockDesc = parser.GetAppCommandDecl();
    
    // Test variables for parameter binding
    std::string filename;
    int count = 0;
    
    // Add parameters with bindings
    appBlockDesc.AddOption(InCommand::OptionType::Parameter, "filename").BindTo(filename);
    appBlockDesc.AddOption(InCommand::OptionType::Parameter, "count").BindTo(count);
    
    // Test arguments
    const char *argv[] =
    {
        "testapp",
        "test.txt",
        "100"
    };
    int argc = sizeof(argv) / sizeof(argv[0]);
    
    // Parse arguments
    parser.ParseArgs(argc, argv);
    
    // Verify that parameter values were bound correctly
    EXPECT_EQ(filename, "test.txt");
    EXPECT_EQ(count, 100);
}

TEST(InCommand, TemplateBinding_CustomConverter)
{
    InCommand::CommandParser parser("testapp");
    auto& appBlockDesc = parser.GetAppCommandDecl();
    
    std::string upperValue;
    
    // Add option with custom converter
    appBlockDesc.AddOption(InCommand::OptionType::Variable, "upper").BindTo(upperValue, UppercaseConverter{});
    
    const char *argv[] =
    {
        "testapp",
        "--upper", "hello world"
    };
    int argc = sizeof(argv) / sizeof(argv[0]);
    
    parser.ParseArgs(argc, argv);
    
    EXPECT_EQ(upperValue, "HELLO WORLD");
}

TEST(InCommand, TemplateBinding_ErrorHandling)
{
    InCommand::CommandParser parser("testapp");
    auto& appBlockDesc = parser.GetAppCommandDecl();
    
    int intValue = 0;
    
    // Add option with integer binding
    appBlockDesc.AddOption(InCommand::OptionType::Variable, "number").BindTo(intValue);
    
    // Test with invalid integer value
    const char *argv[] =
    {
        "testapp",
        "--number", "not_a_number"
    };
    int argc = sizeof(argv) / sizeof(argv[0]);
    
    // Should throw SyntaxException for invalid value
    EXPECT_THROW(parser.ParseArgs(argc, argv), InCommand::SyntaxException);
}

TEST(InCommand, TemplateBinding_SwitchBoolBinding)
{
    // Verify that Switch options can now bind directly to bool variables.
    // Presence of switch sets bool to true; absence leaves default unchanged.
    {
        InCommand::CommandParser parser("testapp");
        auto& appBlockDesc = parser.GetAppCommandDecl();
        bool verbose = false; // default false
        appBlockDesc.AddOption(InCommand::OptionType::Switch, "verbose", 'v').BindTo(verbose);
        const char* argv[] = {"testapp", "--verbose"};
        parser.ParseArgs(2, argv);
        EXPECT_TRUE(verbose);
    }

    // Absence should not flip an already true default to false
    {
        InCommand::CommandParser parser("testapp");
        auto& appBlockDesc = parser.GetAppCommandDecl();
        bool featureEnabled = true; // caller chooses default true
        appBlockDesc.AddOption(InCommand::OptionType::Switch, "feature").BindTo(featureEnabled);
        const char* argv[] = {"testapp"};
        parser.ParseArgs(1, argv);
        EXPECT_TRUE(featureEnabled); // unchanged
    }

    // Grouped aliases with binding
    {
        InCommand::CommandParser parser("testapp");
        auto& appBlockDesc = parser.GetAppCommandDecl();
        bool a=false, b=false, c=false;
        appBlockDesc.AddOption(InCommand::OptionType::Switch, "alpha", 'a').BindTo(a);
        appBlockDesc.AddOption(InCommand::OptionType::Switch, "bravo", 'b').BindTo(b);
        appBlockDesc.AddOption(InCommand::OptionType::Switch, "charlie", 'c').BindTo(c);
        const char* argv[] = {"testapp", "-abc"};
        parser.ParseArgs(2, argv);
        EXPECT_TRUE(a);
        EXPECT_TRUE(b);
        EXPECT_TRUE(c);
    }
}

TEST(InCommand, TemplateBinding_BooleanVariables)
{
    InCommand::CommandParser parser("testapp");
    auto& appBlockDesc = parser.GetAppCommandDecl();
    
    bool flag = false;
    
    // Add boolean variable (not switch)
    appBlockDesc.AddOption(InCommand::OptionType::Variable, "flag").BindTo(flag);
    
    // Test "true"
    const char* argv1[] = {"testapp", "--flag", "true"};
    flag = false;
    parser.ParseArgs(3, argv1);
    EXPECT_TRUE(flag);
    
    // Reset parser for next test
    InCommand::CommandParser parser2("testapp");
    auto& appBlock2 = parser2.GetAppCommandDecl();
    appBlock2.AddOption(InCommand::OptionType::Variable, "flag").BindTo(flag);
    
    // Test "false" 
    const char* argv2[] = {"testapp", "--flag", "false"};
    flag = true;
    parser2.ParseArgs(3, argv2);
    EXPECT_FALSE(flag);
}

TEST(InCommand, TemplateBinding_CustomEnumType)
{
    InCommand::CommandParser parser("testapp");
    auto& appBlockDesc = parser.GetAppCommandDecl();
    
    LogLevel logLevel = LogLevel::Info;  // Default value
    
    // Add option with custom enum converter
    appBlockDesc.AddOption(InCommand::OptionType::Variable, "log-level", 'l')
           .BindTo(logLevel, LogLevelConverter{})
           .SetDescription("Set logging level");
    
    // Test each enum value
    {
        const char* argv[] = {"testapp", "--log-level", "debug"};
        logLevel = LogLevel::Info;  // Reset to different value
        parser.ParseArgs(3, argv);
        EXPECT_EQ(logLevel, LogLevel::Debug);
    }
    
    // Test with short alias
    {
        InCommand::CommandParser parser2("testapp");
        auto& appBlock2 = parser2.GetAppCommandDecl();
        LogLevel logLevel2 = LogLevel::Debug;
        appBlock2.AddOption(InCommand::OptionType::Variable, "log-level", 'l')
               .BindTo(logLevel2, LogLevelConverter{});
        
        const char* argv[] = {"testapp", "-l", "error"};
        parser2.ParseArgs(3, argv);
        EXPECT_EQ(logLevel2, LogLevel::Error);
    }
    
    // Test parameter binding with enum
    {
        InCommand::CommandParser parser3("testapp");
        auto& rootCmd3 = parser3.GetAppCommandDecl();
        LogLevel paramLevel = LogLevel::Debug;
        rootCmd3.AddOption(InCommand::OptionType::Parameter, "level")
               .BindTo(paramLevel, LogLevelConverter{});
        
        const char* argv[] = {"testapp", "warning"};
        parser3.ParseArgs(2, argv);
        EXPECT_EQ(paramLevel, LogLevel::Warning);
    }
    
    // Test invalid enum value throws exception
    {
        InCommand::CommandParser parser4("testapp");
        auto& rootCmd4 = parser4.GetAppCommandDecl();
        LogLevel invalidLevel = LogLevel::Info;
        rootCmd4.AddOption(InCommand::OptionType::Variable, "log-level")
               .BindTo(invalidLevel, LogLevelConverter{});
        
        const char* argv[] = {"testapp", "--log-level", "invalid"};
        EXPECT_THROW(parser4.ParseArgs(3, argv), InCommand::SyntaxException);
        
        // Verify the exception contains helpful information
        try
        {
            parser4.ParseArgs(3, argv);
            FAIL() << "Expected SyntaxException";
        }
        catch (const InCommand::SyntaxException &e)
        {
            EXPECT_EQ(e.GetError(), InCommand::SyntaxError::InvalidValue);
            EXPECT_EQ(e.GetToken(), "invalid");
            EXPECT_TRUE(std::string(e.GetMessage()).find("Invalid log level") != std::string::npos);
        }
    }
}

//------------------------------------------------------------------------------------------------
// Global Options Test
//------------------------------------------------------------------------------------------------

TEST(InCommand, GlobalOptionsBasic)
{
    InCommand::CommandParser parser("testapp");
    auto& appBlockDesc = parser.GetAppCommandDecl();
    
    // Add a global verbose option
    parser.AddGlobalOption(InCommand::OptionType::Switch, "verbose", 'v')
        .SetDescription("Enable verbose output");
    
    // Add a subcommand
    auto& buildBlockDesc = appBlockDesc.AddSubCommand("build");
    buildBlockDesc.AddOption(InCommand::OptionType::Parameter, "target")
        .SetDescription("Build target");
    
    // Test 1: Global option at root level
    {
        const char* argv[] = {"testapp", "--verbose"};
        const int argc = sizeof(argv) / sizeof(argv[0]);
        
        parser.ParseArgs(argc, argv);

        // Global option should be accessible via parser methods
        EXPECT_TRUE(parser.IsGlobalOptionSet("verbose"));
        
        // Global option should also be accessible from first command block
        EXPECT_EQ(parser.GetGlobalOptionBlockIndex("verbose"), 0);
    }
    
    // Test 2: Global option with subcommand
    {
        InCommand::CommandParser parser2("testapp");
        auto& appBlock2 = parser2.GetAppCommandDecl();
        
        // Re-Add the same structure for a fresh test
        parser2.AddGlobalOption(InCommand::OptionType::Switch, "verbose", 'v');
        auto& buildBlockDesc2 = appBlock2.AddSubCommand("build");
        buildBlockDesc2.AddOption(InCommand::OptionType::Parameter, "target");
        
        const char* argv[] = {"testapp", "build", "--verbose", "debug"};
        const int argc = sizeof(argv) / sizeof(argv[0]);
        
        auto numBlocks = parser2.ParseArgs(argc, argv);
        const InCommand::CommandBlock &cmdBlock = parser2.GetCommandBlock(numBlocks - 1);

        // Global option should be accessible via parser methods
        EXPECT_TRUE(parser2.IsGlobalOptionSet("verbose"));
        
        // Should be at build command level
        EXPECT_EQ(cmdBlock.GetDecl().GetName(), "build");
        EXPECT_TRUE(cmdBlock.IsOptionSet("target"));
        EXPECT_EQ(cmdBlock.GetOptionValue("target"), "debug");
    }
}

//------------------------------------------------------------------------------------------------
// Global Option Locality Test
//------------------------------------------------------------------------------------------------

TEST(InCommand, GlobalOptionLocality)
{
    InCommand::CommandParser parser("testapp");
    
    // Setup global options
    parser.AddGlobalOption(InCommand::OptionType::Switch, "verbose", 'v')
        .SetDescription("Enable verbose output");
    parser.AddGlobalOption(InCommand::OptionType::Variable, "config", 'c')
        .SetDescription("Configuration file");
    
    // Setup command hierarchy with unique IDs for identification
    auto& appBlockDesc = parser.GetAppCommandDecl();
    appBlockDesc.SetDescription("Test application").SetUniqueId(1);
    
    auto& subCmdBlockDesc1 = appBlockDesc.AddSubCommand("subcmd1");
    subCmdBlockDesc1.SetDescription("Sub command 1").SetUniqueId(2);
    
    auto& subCmdBlockDesc2 = subCmdBlockDesc1.AddSubCommand("subcmd2");
    subCmdBlockDesc2.SetDescription("Sub command 2").SetUniqueId(3);
    
    // Test 1: Global option specified at left-most command block
    {
        const char* args[] = { "testapp", "--verbose", "subcmd1", "subcmd2" };
        size_t numBlocks = parser.ParseArgs(4, args);
        const InCommand::CommandBlock &result = parser.GetCommandBlock(numBlocks - 1);
        
        EXPECT_TRUE(parser.IsGlobalOptionSet("verbose"));
        EXPECT_FALSE(parser.IsGlobalOptionSet("config"));
        
        // GetGlobalOptionBlockIndex should return the left-most command block
        EXPECT_EQ(parser.GetGlobalOptionBlockIndex("verbose"), 0);
        
        // Option that wasn't set should throw
        EXPECT_THROW(parser.GetGlobalOptionBlockIndex("config"), InCommand::ApiException);
        
        // Final command should be subcmd2
        EXPECT_EQ(result.GetDecl().GetUniqueId<int>(), 3);
    }
    
    // Test 2: Global option specified at intermediate level
    {
        InCommand::CommandParser parser2("testapp");
        parser2.AddGlobalOption(InCommand::OptionType::Switch, "debug", 'd');
        parser2.AddGlobalOption(InCommand::OptionType::Variable, "output", 'o');
        
        auto& appBlockDesc2 = parser2.GetAppCommandDecl();
        appBlockDesc2.SetUniqueId(10);
        
        auto& subCmdBlockDesc1_2 = appBlockDesc2.AddSubCommand("subcmd1");
        subCmdBlockDesc1_2.SetUniqueId(20);
        
        auto& subCmdBlockDesc2_2 = subCmdBlockDesc1_2.AddSubCommand("subcmd2");
        subCmdBlockDesc2_2.SetUniqueId(30);
        
        const char* args[] = { "testapp", "subcmd1", "--debug", "--output", "file.txt", "subcmd2" };
        size_t numBlocks = parser2.ParseArgs(6, args);
        const InCommand::CommandBlock &result = parser2.GetCommandBlock(numBlocks - 1);
        
        EXPECT_TRUE(parser2.IsGlobalOptionSet("debug"));
        EXPECT_TRUE(parser2.IsGlobalOptionSet("output"));
        EXPECT_EQ(parser2.GetGlobalOptionValue("output"), "file.txt");
        
        // GetGlobalOptionBlockIndex should return the subcmd1 command block  
        EXPECT_EQ(parser2.GetGlobalOptionBlockIndex("debug"), 1); // subcmd1 block
        EXPECT_EQ(parser2.GetGlobalOptionBlockIndex("output"), 1); // subcmd1 block
        
        // Final command should be subcmd2
        EXPECT_EQ(result.GetDecl().GetUniqueId<int>(), 30);
    }
    
    // Test 3: Global option specified at deepest level
    {
        InCommand::CommandParser parser3("testapp");
        parser3.AddGlobalOption(InCommand::OptionType::Switch, "trace", 't');
        
        auto& appBlockDesc3 = parser3.GetAppCommandDecl();
        appBlockDesc3.SetUniqueId(100);
        
        auto& subCmdBlockDesc1_3 = appBlockDesc3.AddSubCommand("subcmd1");
        subCmdBlockDesc1_3.SetUniqueId(200);
        
        auto& subCmdBlockDesc2_3 = subCmdBlockDesc1_3.AddSubCommand("subcmd2");
        subCmdBlockDesc2_3.SetUniqueId(300);
        
        const char* args[] = { "testapp", "subcmd1", "subcmd2", "--trace" };
        size_t numBlocks = parser3.ParseArgs(4, args);
        const InCommand::CommandBlock &result = parser3.GetCommandBlock(numBlocks - 1);
        
        EXPECT_TRUE(parser3.IsGlobalOptionSet("trace"));
        
        // GetGlobalOptionBlockIndex should return the subcmd2 command block
        EXPECT_EQ(parser3.GetGlobalOptionBlockIndex("trace"), 2); // subcmd2 block
        
        // Final command should be subcmd2
        EXPECT_EQ(result.GetDecl().GetUniqueId<int>(), 300);
    }
    
    // Test 4: Multiple global options at different levels
    {
        InCommand::CommandParser parser4("testapp");
        parser4.AddGlobalOption(InCommand::OptionType::Switch, "verbose", 'v');
        parser4.AddGlobalOption(InCommand::OptionType::Switch, "debug", 'd');
        parser4.AddGlobalOption(InCommand::OptionType::Variable, "config", 'c');
        
        auto& appBlockDesc4 = parser4.GetAppCommandDecl();
        appBlockDesc4.SetUniqueId(1000);
        
        auto& subCmdBlockDesc1_4 = appBlockDesc4.AddSubCommand("subcmd1");
        subCmdBlockDesc1_4.SetUniqueId(2000);
        
        const char* args[] = { "testapp", "--verbose", "subcmd1", "--debug", "--config", "test.conf" };
        size_t numBlocks = parser4.ParseArgs(6, args);
        const InCommand::CommandBlock &result = parser4.GetCommandBlock(numBlocks - 1);
        
        EXPECT_TRUE(parser4.IsGlobalOptionSet("verbose"));
        EXPECT_TRUE(parser4.IsGlobalOptionSet("debug"));
        EXPECT_TRUE(parser4.IsGlobalOptionSet("config"));
        
        // verbose should in block 0
        EXPECT_EQ(parser4.GetGlobalOptionBlockIndex("verbose"), 0);
        
        // debug and config should be in block 1
        EXPECT_EQ(parser4.GetGlobalOptionBlockIndex("debug"), 1);
        EXPECT_EQ(parser4.GetGlobalOptionBlockIndex("config"), 1);
        
        EXPECT_EQ(result.GetDecl().GetUniqueId<int>(), 2000);
    }
}

TEST(InCommand, AutoHelpDescriptionCustomization)
{
    // Test customizing auto-help description
    std::ostringstream output;
    InCommand::CommandParser parser("testapp");
    parser.EnableAutoHelp("help", 'h', output);
    parser.SetAutoHelpDescription("Display usage and command information");
    
    auto& appBlockDesc = parser.GetAppCommandDecl();
    appBlockDesc.SetDescription("Test application");
    
    const char* args[] = { "testapp", "--help" };
    parser.ParseArgs(2, args);
    
    // Verify help was requested and output contains our custom description
    EXPECT_TRUE(parser.WasAutoHelpRequested());
    std::string helpOutput = output.str();
    EXPECT_TRUE(helpOutput.find("testapp") != std::string::npos);
}

TEST(InCommand, AutoHelpDisabled)
{
    // Test that when auto-help is disabled, help options don't automatically exist
    InCommand::CommandParser parser("testapp");
    parser.DisableAutoHelp();
    
    auto& appBlockDesc = parser.GetAppCommandDecl();
    appBlockDesc.SetDescription("Test application");
    
    // Should parse without issues since no help option is automatically declared
    const char* args[] = { "testapp" };
    parser.ParseArgs(1, args);
    
    // Help option should not be available unless manually declared
    EXPECT_FALSE(parser.IsGlobalOptionSet("help"));
}

TEST(InCommand, AutoHelpCustomization)
{
    // Test customizing auto-help option name and alias
    std::ostringstream output;
    InCommand::CommandParser parser("testapp");
    parser.EnableAutoHelp("usage", 'u', output);
    
    auto& appBlockDesc = parser.GetAppCommandDecl();
    appBlockDesc.SetDescription("Test application");
    
    // The custom help option should be auto-declared during parsing
    const char* args[] = { "testapp" };
    parser.ParseArgs(1, args);
}

TEST(InCommand, AutoHelpConflictHandling)
{
    // Test that auto-help throws an exception when there's a conflict
    std::ostringstream output;
    InCommand::CommandParser parser("testapp");
    
    // Manually Add a conflicting help option first
    parser.AddGlobalOption(InCommand::OptionType::Variable, "help", 'h')
        .SetDescription("Manual help option");
    
    // Now try to enable auto-help with the same option name - should throw
    EXPECT_THROW(parser.EnableAutoHelp("help", 'h', output), InCommand::ApiException);
}

// Note: Testing actual help output with exit() is complex in unit tests
// since exit() terminates the process. In a real application, you might
// want to refactor the help handling to use a callback or return a value
// instead of calling exit() directly.

TEST(InCommand, AutoHelpOutput)
{
    // Test that auto-help generates appropriate help output
    std::ostringstream output;
    InCommand::CommandParser parser("testapp");
    parser.EnableAutoHelp("help", 'h', output);
    
    auto& appBlockDesc = parser.GetAppCommandDecl();
    appBlockDesc.SetDescription("Test application for auto-help");
    
    // Test 1: Help at root level
    {
        const char* args[] = { "testapp", "--help" };
        
        parser.ParseArgs(2, args);
        
        EXPECT_TRUE(parser.WasAutoHelpRequested());
        std::string helpText = output.str();
        EXPECT_TRUE(helpText.find("testapp") != std::string::npos);
        EXPECT_TRUE(helpText.find("Test application for auto-help") != std::string::npos);
        
        // When help is requested, no command blocks should be available (except empty root)
        EXPECT_EQ(parser.GetNumCommandBlocks(), 1);
    }
}

TEST(InCommand, AutoHelpCustomOption)
{
    // Test custom auto-help option name and alias
    std::ostringstream output;
    InCommand::CommandParser parser("testapp");
    parser.EnableAutoHelp("usage", 'u', output);
    
    auto& appBlockDesc = parser.GetAppCommandDecl();
    appBlockDesc.SetDescription("Test application");
    
    const char* args[] = { "testapp", "--usage" };
    
    parser.ParseArgs(2, args);
    
    EXPECT_TRUE(parser.WasAutoHelpRequested());
    std::string helpText = output.str();
    EXPECT_TRUE(helpText.find("testapp") != std::string::npos);
    EXPECT_TRUE(helpText.find("Test application") != std::string::npos);
}

TEST(InCommand, AutoHelpDisabledNoException)
{
    // Test that when auto-help is disabled, help options don't cause exceptions
    InCommand::CommandParser parser("testapp");
    parser.DisableAutoHelp();
    
    auto& appBlockDesc = parser.GetAppCommandDecl();
    appBlockDesc.SetDescription("Test application");
    
    // Should parse without requesting help
    const char* args[] = { "testapp" };
    size_t numBlocks = parser.ParseArgs(1, args);
    const InCommand::CommandBlock& result = parser.GetCommandBlock(numBlocks - 1);
    
    // Should complete normally without help being requested
    EXPECT_FALSE(parser.WasAutoHelpRequested());
    EXPECT_EQ(result.GetDecl().GetName(), "testapp");
}

TEST(InCommand, NormalParsingStillWorks)
{
    // Test that normal parsing without help requests works correctly
    InCommand::CommandParser parser("testapp");
    
    auto& appBlockDesc = parser.GetAppCommandDecl();
    appBlockDesc.SetDescription("Test application");
    
    // Add global verbose option
    parser.AddGlobalOption(InCommand::OptionType::Switch, "verbose", 'v');
    
    auto& subCmdBlockDesc = appBlockDesc.AddSubCommand("build");
    subCmdBlockDesc.AddOption(InCommand::OptionType::Variable, "target", 't');
    
    const char* args[] = { "testapp", "--verbose", "build", "--target", "release" };
    size_t numBlocks = parser.ParseArgs(5, args);
    const InCommand::CommandBlock& result = parser.GetCommandBlock(numBlocks - 1);
    
    // Should not request help
    EXPECT_FALSE(parser.WasAutoHelpRequested());
    
    // Should have normal parsing results
    EXPECT_EQ(result.GetDecl().GetName(), "build");
    EXPECT_TRUE(parser.IsGlobalOptionSet("verbose"));
    EXPECT_EQ(result.GetOptionValue("target"), "release");
}
