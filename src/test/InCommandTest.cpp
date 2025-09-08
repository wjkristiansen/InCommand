#include <gtest/gtest.h>
#include <algorithm>

#include "InCommand.h"

TEST(InCommand, UsageStringMethods)
{
    // Create parser
    InCommand::CommandParser parser("myapp");
    auto& rootCmdDesc = parser.GetRootCommandBlockDesc();
    rootCmdDesc.SetDescription("My application");

    // Add some options to the root command
    rootCmdDesc.DeclareOption(InCommand::OptionType::Switch, "verbose").SetDescription("Show help information");
    rootCmdDesc.DeclareOption(InCommand::OptionType::Variable, "config").SetDescription("Configuration file path");

    // Add inner command blocks
    auto& buildCmdDesc = rootCmdDesc.DeclareSubCommandBlock("build");
    buildCmdDesc.SetDescription("Build the project");
    buildCmdDesc.DeclareOption(InCommand::OptionType::Switch, "verbose").SetDescription("Enable verbose output");
    buildCmdDesc.DeclareOption(InCommand::OptionType::Variable, "target").SetDescription("Build target");

    auto& testCmdDesc = rootCmdDesc.DeclareSubCommandBlock("test");
    testCmdDesc.SetDescription("Run tests");
    testCmdDesc.DeclareOption(InCommand::OptionType::Switch, "coverage").SetDescription("Generate coverage report");

    // Test SimpleUsageString
    std::string usageString = parser.SimpleUsageString();
    EXPECT_FALSE(usageString.empty());
    EXPECT_TRUE(usageString.find("myapp") != std::string::npos);
    EXPECT_TRUE(usageString.find("--verbose") != std::string::npos);
    EXPECT_TRUE(usageString.find("--config") != std::string::npos);

    // Test OptionDetailsString
    std::string detailsString = parser.OptionDetailsString();
    EXPECT_FALSE(detailsString.empty());
    EXPECT_TRUE(detailsString.find("Show help information") != std::string::npos);
    EXPECT_TRUE(detailsString.find("Configuration file path") != std::string::npos);
    EXPECT_TRUE(detailsString.find("Build the project") != std::string::npos);
    EXPECT_TRUE(detailsString.find("Run tests") != std::string::npos);

    // Test with specific command block
    std::string buildUsage = buildCmdDesc.SimpleUsageString();
    EXPECT_TRUE(buildUsage.find("build") != std::string::npos);
    EXPECT_TRUE(buildUsage.find("--verbose") != std::string::npos);
    EXPECT_TRUE(buildUsage.find("--target") != std::string::npos);

    std::string buildDetails = buildCmdDesc.OptionDetailsString();
    EXPECT_TRUE(buildDetails.find("Enable verbose output") != std::string::npos);
    EXPECT_TRUE(buildDetails.find("Build target") != std::string::npos);
}

TEST(InCommand, BasicOptions)
{
    // Create a command block description hierarchy for testing basic option parsing
    InCommand::CommandParser parser("test");
    InCommand::CommandBlockDesc& rootCmd = parser.GetRootCommandBlockDesc();
    
    // Add global help switch with alias
    rootCmd.DeclareOption(InCommand::OptionType::Switch, "verbose", 'v');
    
    // Create inner command blocks
    auto& fooCmd = rootCmd.DeclareSubCommandBlock("foo");
    fooCmd.DeclareOption(InCommand::OptionType::Variable, "number");
    
    auto& barCmd = rootCmd.DeclareSubCommandBlock("bar");
    barCmd.DeclareOption(InCommand::OptionType::Variable, "word");
    barCmd.DeclareOption(InCommand::OptionType::Variable, "name", 'n');
    
    auto& bazCmd = barCmd.DeclareSubCommandBlock("baz");
    bazCmd.DeclareOption(InCommand::OptionType::Variable, "color")
        .SetDomain({"red", "green", "blue", "yellow", "purple"});
    
    auto& zapCmd = rootCmd.DeclareSubCommandBlock("zap");
    zapCmd.DeclareOption(InCommand::OptionType::Parameter, "file1");
    zapCmd.DeclareOption(InCommand::OptionType::Parameter, "file2");

    // Test 1: --verbose foo --number 42
    {
        const char *argv[] = {"test", "--verbose", "foo", "--number", "42"};
        int argc = std::size(argv);

        InCommand::CommandParser parser("test");
        auto& testRootCmdDesc = parser.GetRootCommandBlockDesc();
        testRootCmdDesc = rootCmd;
        const InCommand::CommandBlock &cmdBlock = parser.ParseArgs(argc, argv);
        
        // Should be at foo level, but help is at root level
        EXPECT_TRUE(parser.GetCommandBlock(0).IsOptionSet("verbose"));
        EXPECT_EQ(cmdBlock.GetOptionValue("number"), "42");
    }

    // Test 2: bar --word hello baz --color red
    {
        const char *argv[] = {"test", "bar", "--word", "hello", "baz", "--color", "red"};
        int argc = std::size(argv);

        InCommand::CommandParser parser("test");
        auto& testRootCmdDesc = parser.GetRootCommandBlockDesc();
        testRootCmdDesc = rootCmd;
        const InCommand::CommandBlock &cmdBlock = parser.ParseArgs(argc, argv);
        
        // Should be at baz level
        EXPECT_EQ(parser.GetNumCommandBlocks(), 3); // root -> bar -> baz
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
        auto& testRootCmdDesc = parser.GetRootCommandBlockDesc();
        testRootCmdDesc = rootCmd;
        const InCommand::CommandBlock &cmdBlock = parser.ParseArgs(argc, argv);
        
        EXPECT_TRUE(cmdBlock.IsOptionSet("verbose"));
    }

    // Test 4: --verbose (long form)
    {
        const char *argv[] = {"app", "--verbose"};
        int argc = std::size(argv);

        InCommand::CommandParser parser("testapp");
        auto& testRootCmdDesc = parser.GetRootCommandBlockDesc();
        testRootCmdDesc = rootCmd;
        const InCommand::CommandBlock &cmdBlock = parser.ParseArgs(argc, argv);
        
        EXPECT_TRUE(cmdBlock.IsOptionSet("verbose"));
    }

    // Test 5: bar --name Anna (with alias)
    {
        const char *argv[] = {"app", "bar", "--name", "Anna"};
        int argc = std::size(argv);

        InCommand::CommandParser parser("testapp"); auto& testRootCmdDesc = parser.GetRootCommandBlockDesc(); testRootCmdDesc = rootCmd;
        const InCommand::CommandBlock &cmdBlock = parser.ParseArgs(argc, argv);
        
        EXPECT_EQ(cmdBlock.GetOptionValue("name"), "Anna");
    }

    // Test 6: bar -n Anna (short alias)
    {
        const char *argv[] = {"app", "bar", "-n", "Anna"};
        int argc = std::size(argv);

        InCommand::CommandParser parser("testapp"); auto& testRootCmdDesc = parser.GetRootCommandBlockDesc(); testRootCmdDesc = rootCmd;
        const InCommand::CommandBlock &cmdBlock = parser.ParseArgs(argc, argv);
        
        EXPECT_EQ(cmdBlock.GetOptionValue("name"), "Anna");
    }
}

TEST(InCommand, Parameters)
{
    // Create command with parameters
    InCommand::CommandParser parser("app");
    InCommand::CommandBlockDesc& rootCmd = parser.GetRootCommandBlockDesc();
    rootCmd.DeclareOption(InCommand::OptionType::Parameter, "file1").SetDescription("file 1");
    rootCmd.DeclareOption(InCommand::OptionType::Parameter, "file2").SetDescription("file 2");
    rootCmd.DeclareOption(InCommand::OptionType::Parameter, "file3").SetDescription("file 3");
    rootCmd.DeclareOption(InCommand::OptionType::Switch, "some-switch").SetDescription("Some switch");

    // Test 1: All parameters provided with switch in middle
    {
        const char *argv[] = {"foo", "myfile1.txt", "--some-switch", "myfile2.txt", "myfile3.txt"};
        const int argc = sizeof(argv) / sizeof(argv[0]);

        InCommand::CommandParser parser("testapp"); auto& testRootCmdDesc = parser.GetRootCommandBlockDesc(); testRootCmdDesc = rootCmd;
        const InCommand::CommandBlock &cmdBlock = parser.ParseArgs(argc, argv);

        EXPECT_TRUE(cmdBlock.IsOptionSet("some-switch"));
        EXPECT_EQ(cmdBlock.GetParameterValue("file1", ""), std::string("myfile1.txt"));
        EXPECT_EQ(cmdBlock.GetParameterValue("file2", ""), std::string("myfile2.txt"));
        EXPECT_EQ(cmdBlock.GetParameterValue("file3", ""), std::string("myfile3.txt"));
        EXPECT_TRUE(cmdBlock.IsParameterSet("file1"));
        EXPECT_TRUE(cmdBlock.IsParameterSet("file2"));
        EXPECT_TRUE(cmdBlock.IsParameterSet("file3"));
    }

    // Test 2: Only first two parameters provided
    {
        const char *argv[] = {"foo", "myfile1.txt", "--some-switch", "myfile2.txt"};
        const int argc = sizeof(argv) / sizeof(argv[0]);

        InCommand::CommandParser parser("testapp"); auto& testRootCmdDesc = parser.GetRootCommandBlockDesc(); testRootCmdDesc = rootCmd;
        const InCommand::CommandBlock &cmdBlock = parser.ParseArgs(argc, argv);

        EXPECT_TRUE(cmdBlock.IsOptionSet("some-switch"));
        EXPECT_EQ(cmdBlock.GetParameterValue("file1", ""), std::string("myfile1.txt"));
        EXPECT_EQ(cmdBlock.GetParameterValue("file2", ""), std::string("myfile2.txt"));
        EXPECT_EQ(cmdBlock.GetParameterValue("file3", "nope"), "nope");
        EXPECT_TRUE(cmdBlock.IsParameterSet("file1"));
        EXPECT_TRUE(cmdBlock.IsParameterSet("file2"));
        EXPECT_FALSE(cmdBlock.IsParameterSet("file3"));
    }
}

TEST(InCommand, SubCategories)
{
    // Create nested command structure
    InCommand::CommandParser parser("app");
    InCommand::CommandBlockDesc& rootCmd = parser.GetRootCommandBlockDesc();
    rootCmd.DeclareOption(InCommand::OptionType::Switch, "verbose");

    // plant -> tree, shrub
    auto& plantCmd = rootCmd.DeclareSubCommandBlock("plant");
    plantCmd.DeclareOption(InCommand::OptionType::Switch, "list");
    
    auto& treeCmd = plantCmd.DeclareSubCommandBlock("tree");
    
    auto& shrubCmd = plantCmd.DeclareSubCommandBlock("shrub");
    shrubCmd.DeclareOption(InCommand::OptionType::Switch, "prune");
    shrubCmd.DeclareOption(InCommand::OptionType::Switch, "burn");

    // animal -> dog, cat
    auto& animalCmd = rootCmd.DeclareSubCommandBlock("animal");
    
    auto& dogCmd = animalCmd.DeclareSubCommandBlock("dog");
    
    auto& catCmd = animalCmd.DeclareSubCommandBlock("cat");
    catCmd.DeclareOption(InCommand::OptionType::Variable, "lives");

    // Test 1: plant shrub --burn
    {
        const char* argv[] = {"app", "plant", "shrub", "--burn"};
        const int argc = sizeof(argv) / sizeof(argv[0]);

        InCommand::CommandParser parser("testapp"); auto& testRootCmdDesc = parser.GetRootCommandBlockDesc(); testRootCmdDesc = rootCmd;
        const InCommand::CommandBlock &cmdBlock = parser.ParseArgs(argc, argv);
        
        // Should be at the shrub command level
        EXPECT_EQ(&cmdBlock.GetDesc(), &shrubCmd);
        EXPECT_TRUE(cmdBlock.IsOptionSet("burn"));
        EXPECT_FALSE(cmdBlock.IsOptionSet("prune"));
    }

    // Test 2: animal cat --lives 8
    {
        const char* argv[] = {"app", "animal", "cat", "--lives", "8"};
        const int argc = sizeof(argv) / sizeof(argv[0]);

        InCommand::CommandParser parser("testapp"); auto& testRootCmdDesc = parser.GetRootCommandBlockDesc(); testRootCmdDesc = rootCmd;
        const InCommand::CommandBlock &cmdBlock = parser.ParseArgs(argc, argv);
        
        // Should be at the cat command level
        EXPECT_EQ(&cmdBlock.GetDesc(), &catCmd);
        EXPECT_TRUE(cmdBlock.IsOptionSet("lives"));
        EXPECT_EQ(cmdBlock.GetOptionValue("lives", "9"), "8");
    }

    // Test 3: --verbose (root level)
    {
        const char* argv[] = {"app", "--verbose"};
        const int argc = sizeof(argv) / sizeof(argv[0]);

        InCommand::CommandParser parser("testapp"); auto& testRootCmdDesc = parser.GetRootCommandBlockDesc(); testRootCmdDesc = rootCmd;
        const InCommand::CommandBlock &cmdBlock = parser.ParseArgs(argc, argv);
        
        // Should be at the root command level
        EXPECT_EQ(&cmdBlock.GetDesc(), &parser.GetRootCommandBlockDesc());
        EXPECT_TRUE(cmdBlock.IsOptionSet("verbose"));
    }
}

TEST(InCommand, Errors)
{
    // Test 1: Duplicate command block
    {
        InCommand::CommandParser parser("app");
        InCommand::CommandBlockDesc& rootCmd = parser.GetRootCommandBlockDesc();
        rootCmd.DeclareSubCommandBlock("goto");
        
        EXPECT_THROW(
            {
                rootCmd.DeclareSubCommandBlock("goto"); // Duplicate
            }, InCommand::ApiException);
    }
    
    // Test 2: Duplicate option
    {
        InCommand::CommandParser parser2("app");
        InCommand::CommandBlockDesc& rootCmd = parser2.GetRootCommandBlockDesc();
        rootCmd.DeclareOption(InCommand::OptionType::Switch, "foo");
        
        EXPECT_THROW(
            {
                rootCmd.DeclareOption(InCommand::OptionType::Switch, "foo"); // Duplicate
            }, InCommand::ApiException);
    }

    // Test 3: Unexpected argument (non-existent command)
    {
        const char* argv[] = {"app", "gogo", "--foo", "--bar", "7"};
        const int argc = sizeof(argv) / sizeof(argv[0]);

        InCommand::CommandParser parser3("app");
        InCommand::CommandBlockDesc& rootCmd = parser3.GetRootCommandBlockDesc();
        auto& gotoCmd = rootCmd.DeclareSubCommandBlock("goto");
        gotoCmd.DeclareOption(InCommand::OptionType::Switch, "foo");
        gotoCmd.DeclareOption(InCommand::OptionType::Variable, "bar");

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
        InCommand::CommandBlockDesc& rootCmd = parser4.GetRootCommandBlockDesc();
        auto& gotoCmd = rootCmd.DeclareSubCommandBlock("goto");
        gotoCmd.DeclareOption(InCommand::OptionType::Switch, "fop"); // Note: "fop" not "foo"
        gotoCmd.DeclareOption(InCommand::OptionType::Variable, "bar");

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
        InCommand::CommandBlockDesc& rootCmd = parser5.GetRootCommandBlockDesc();
        rootCmd.DeclareOption(InCommand::OptionType::Variable, "foo");

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
        InCommand::CommandBlockDesc& rootCmd = parser6.GetRootCommandBlockDesc();
        rootCmd.DeclareOption(InCommand::OptionType::Variable, "foo");
        rootCmd.DeclareOption(InCommand::OptionType::Switch, "bar");

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
        InCommand::CommandBlockDesc& rootCmd = parser7.GetRootCommandBlockDesc();
        rootCmd.DeclareOption(InCommand::OptionType::Parameter, "file1");
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
        InCommand::CommandBlockDesc& rootCmd = parser8.GetRootCommandBlockDesc();
        rootCmd.DeclareOption(InCommand::OptionType::Switch, "verbose", 'v');
        rootCmd.DeclareOption(InCommand::OptionType::Switch, "quiet", 'q');

        const InCommand::CommandBlock &cmdBlock = parser8.ParseArgs(argc, argv);
        
        EXPECT_TRUE(cmdBlock.IsOptionSet("verbose"));
        EXPECT_TRUE(cmdBlock.IsOptionSet("quiet"));
    }
}

TEST(InCommand, ParameterAliasValidation)
{
    // Test that parameters cannot have aliases
    InCommand::CommandParser parser("testapp");
    auto& rootCmdDesc = parser.GetRootCommandBlockDesc();
    
    // This should throw an ApiException with InvalidOptionType
    EXPECT_THROW(
        {
            rootCmdDesc.DeclareOption(InCommand::OptionType::Parameter, "filename", 'f');
        }, InCommand::ApiException);
    
    // But switches and variables should work fine
    EXPECT_NO_THROW(
        {
            rootCmdDesc.DeclareOption(InCommand::OptionType::Switch, "verbose", 'v');
            rootCmdDesc.DeclareOption(InCommand::OptionType::Variable, "output", 'o');
        });
}

TEST(InCommand, VariableDelimiters)
{
    // Test parser with = as delimiter
    InCommand::CommandParser parser("myapp", InCommand::VariableDelimiter::Equals);
    auto& rootCmdDesc = parser.GetRootCommandBlockDesc();
    
    // Add options
    rootCmdDesc.DeclareOption(InCommand::OptionType::Variable, "name", 'n').SetDescription("User name");
    rootCmdDesc.DeclareOption(InCommand::OptionType::Variable, "output").SetDescription("Output file");
    rootCmdDesc.DeclareOption(InCommand::OptionType::Switch, "verbose", 'v').SetDescription("Verbose mode");

    // Test --name=value format
    {
        const char* args[] = {"myapp", "--name=John", "--output=file.txt", "-v"};
        const InCommand::CommandBlock &result = parser.ParseArgs(4, args);
        
        EXPECT_TRUE(result.IsOptionSet("name"));
        EXPECT_EQ(result.GetOptionValue("name"), "John");
        
        EXPECT_TRUE(result.IsOptionSet("output"));
        EXPECT_EQ(result.GetParameterValue("output"), "file.txt");
        
        EXPECT_TRUE(result.IsOptionSet("verbose"));
    }

    // Test -n=value format (short option with delimiter)
    {
        const char* args[] = {"myapp", "-n=Jane", "-v"};
        const InCommand::CommandBlock &result = parser.ParseArgs(3, args);
        
        EXPECT_TRUE(result.IsOptionSet("name"));
        EXPECT_EQ(result.GetOptionValue("name"), "Jane");
        
        EXPECT_TRUE(result.IsOptionSet("verbose"));
    }

    // Test mixed formats (traditional space-separated and delimiter-based)
    {
        const char* args[] = {"myapp", "--name=Bob", "--output", "result.txt", "-v"};
        const InCommand::CommandBlock &result = parser.ParseArgs(5, args);
        
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
    auto& colonRootCmdDesc = colonParser.GetRootCommandBlockDesc();
    colonRootCmdDesc.DeclareOption(InCommand::OptionType::Variable, "name").SetDescription("User name");

    {
        const char* args[] = {"myapp", "--name:Alice"};
        const InCommand::CommandBlock &result = colonParser.ParseArgs(2, args);
        
        EXPECT_TRUE(result.IsOptionSet("name"));
        EXPECT_EQ(result.GetOptionValue("name"), "Alice");
    }

    // Test parser with explicit whitespace delimiter (traditional format only)
    InCommand::CommandParser whitespaceParser("myapp", InCommand::VariableDelimiter::Whitespace);
    auto& whitespaceRootCmdDesc = whitespaceParser.GetRootCommandBlockDesc();
    whitespaceRootCmdDesc.DeclareOption(InCommand::OptionType::Variable, "name").SetDescription("User name");
    whitespaceRootCmdDesc.DeclareOption(InCommand::OptionType::Switch, "verbose").SetDescription("Verbose mode");

    // Traditional whitespace-separated format should work
    {
        const char* args[] = {"myapp", "--name", "Alice", "--verbose"};
        const InCommand::CommandBlock &result = whitespaceParser.ParseArgs(4, args);
        
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
    auto& traditionalRootCmdDesc = traditionalParser.GetRootCommandBlockDesc();
    traditionalRootCmdDesc.DeclareOption(InCommand::OptionType::Variable, "name").SetDescription("User name");

    // Traditional format should work with default constructor
    {
        const char* args[] = {"myapp", "--name", "Bob"};
        const InCommand::CommandBlock &result = traditionalParser.ParseArgs(3, args);
        
        EXPECT_TRUE(result.IsOptionSet("name"));
        EXPECT_EQ(result.GetOptionValue("name"), "Bob");
    }
}

TEST(InCommand, MidChainCommandBlocksWithParameters)
{
    // Test command structure: app container run image_name
    // where 'container' is mid-chain and has parameters, 'run' is final command
    InCommand::CommandParser parser("app");
    InCommand::CommandBlockDesc& rootCmd = parser.GetRootCommandBlockDesc();
    
    auto& containerCmd = rootCmd.DeclareSubCommandBlock("container");
    containerCmd.DeclareOption(InCommand::OptionType::Parameter, "container_id").SetDescription("Container ID");
    containerCmd.DeclareOption(InCommand::OptionType::Switch, "all", 'a').SetDescription("Show all containers");
    
    auto& runCmd = containerCmd.DeclareSubCommandBlock("run");
    runCmd.DeclareOption(InCommand::OptionType::Parameter, "image").SetDescription("Image name");
    runCmd.DeclareOption(InCommand::OptionType::Variable, "port", 'p').SetDescription("Port mapping");
    
    // Test: app container 12345 run ubuntu --port 8080
    {
        const char* argv[] = {"app", "container", "12345", "run", "ubuntu", "--port", "8080"};
        int argc = std::size(argv);

        InCommand::CommandParser parser("testapp");
        auto& testRootCmdDesc = parser.GetRootCommandBlockDesc();
        testRootCmdDesc = rootCmd;
        const InCommand::CommandBlock &cmdBlock = parser.ParseArgs(argc, argv);
        
        // Should be at 'run' command level
        EXPECT_EQ(cmdBlock.GetParameterValue("image"), "ubuntu");
        EXPECT_EQ(cmdBlock.GetOptionValue("port"), "8080");
        
        // Check that mid-chain 'container' command received its parameter
        const InCommand::CommandBlock &containerBlock = parser.GetCommandBlock(1);
        EXPECT_TRUE(containerBlock.IsParameterSet("container_id"));
        EXPECT_EQ(containerBlock.GetParameterValue("container_id"), "12345");
    }
    
    // Test: app container --all run ubuntu (switch on mid-chain)
    {
        const char* argv[] = {"app", "container", "--all", "run", "ubuntu"};
        int argc = std::size(argv);

        InCommand::CommandParser parser("testapp");
        auto& testRootCmdDesc = parser.GetRootCommandBlockDesc();
        testRootCmdDesc = rootCmd;
        const InCommand::CommandBlock &cmdBlock = parser.ParseArgs(argc, argv);
        
        // Should be at 'run' command level
        EXPECT_EQ(cmdBlock.GetParameterValue("image"), "ubuntu");
        
        // Check that mid-chain 'container' command has switch set
        EXPECT_EQ(parser.GetNumCommandBlocks(), 3);
        EXPECT_TRUE(parser.GetCommandBlock(1).IsOptionSet("all"));
        EXPECT_FALSE(parser.GetCommandBlock(1).IsParameterSet("container_id"));
    }
}

TEST(InCommand, ParametersWithSubCommandNameCollisions)
{
    // Test when parameter names collide with sub-command names
    InCommand::CommandParser parser("app");
    InCommand::CommandBlockDesc& rootCmd = parser.GetRootCommandBlockDesc();
    rootCmd.DeclareOption(InCommand::OptionType::Parameter, "build").SetDescription("Build file parameter");
    rootCmd.DeclareOption(InCommand::OptionType::Parameter, "test").SetDescription("Test file parameter");
    
    // Add sub-commands with same names as parameters
    auto& buildCmd = rootCmd.DeclareSubCommandBlock("build");
    buildCmd.DeclareOption(InCommand::OptionType::Switch, "verbose").SetDescription("Verbose build");
    
    auto& testCmd = rootCmd.DeclareSubCommandBlock("test");
    testCmd.DeclareOption(InCommand::OptionType::Switch, "coverage").SetDescription("Coverage report");
    
    // Test 1: Use as parameter (positional arguments)
    {
        const char* argv[] = {"app", "mybuild.json", "mytest.json"};
        int argc = std::size(argv);

        InCommand::CommandParser parser("testapp");
        auto& testRootCmdDesc = parser.GetRootCommandBlockDesc();
        testRootCmdDesc = rootCmd;
        const InCommand::CommandBlock &cmdBlock = parser.ParseArgs(argc, argv);
        
        // Should stay at root level and treat as parameters
        EXPECT_EQ(&cmdBlock.GetDesc(), &parser.GetRootCommandBlockDesc());
        EXPECT_TRUE(cmdBlock.IsParameterSet("build"));
        EXPECT_TRUE(cmdBlock.IsParameterSet("test"));
        EXPECT_EQ(cmdBlock.GetParameterValue("build"), "mybuild.json");
        EXPECT_EQ(cmdBlock.GetParameterValue("test"), "mytest.json");
    }
    
    // Test 2: Use as sub-command (when followed by options)
    {
        const char* argv[] = {"app", "build", "--verbose"};
        int argc = std::size(argv);

        InCommand::CommandParser parser("testapp");
        auto& testRootCmdDesc = parser.GetRootCommandBlockDesc();
        testRootCmdDesc = rootCmd;
        const InCommand::CommandBlock &cmdBlock = parser.ParseArgs(argc, argv);
        
        // Should be at build command level
        EXPECT_EQ(&cmdBlock.GetDesc(), &buildCmd);
        EXPECT_TRUE(cmdBlock.IsOptionSet("verbose"));
    }
    
    // Test 3: Use as sub-command (standalone)
    {
        const char* argv[] = {"app", "test"};
        int argc = std::size(argv);

        InCommand::CommandParser parser("testapp");
        auto& testRootCmdDesc = parser.GetRootCommandBlockDesc();
        testRootCmdDesc = rootCmd;
        const InCommand::CommandBlock &cmdBlock = parser.ParseArgs(argc, argv);
        
        // Should be at test command level
        EXPECT_EQ(&cmdBlock.GetDesc(), &testCmd);
        EXPECT_FALSE(cmdBlock.IsOptionSet("coverage"));
    }
}

TEST(InCommand, DuplicateAliasCharacters)
{
    // Test that duplicate alias characters across different options throw ApiException
    InCommand::CommandParser parser("app");
    InCommand::CommandBlockDesc& rootCmd = parser.GetRootCommandBlockDesc();
    
    // First option with alias 'v'
    rootCmd.DeclareOption(InCommand::OptionType::Switch, "verbose", 'v');
    
    // Test 1: Duplicate alias in same command block should throw
    EXPECT_THROW(
        {
            rootCmd.DeclareOption(InCommand::OptionType::Switch, "version", 'v'); // Duplicate 'v'
        }, InCommand::ApiException);
    
    // Test 2: Duplicate alias with different option type should also throw
    EXPECT_THROW(
        {
            rootCmd.DeclareOption(InCommand::OptionType::Variable, "value", 'v'); // Duplicate 'v'
        }, InCommand::ApiException);
    
    // Test 3: Same alias in different command blocks should be allowed
    auto& subCmd = rootCmd.DeclareSubCommandBlock("sub");
    EXPECT_NO_THROW(
        {
            subCmd.DeclareOption(InCommand::OptionType::Switch, "verify", 'v'); // Same 'v' in different block is OK
        });
    
    // Test 4: Verify both aliases work independently
    {
        const char* argv[] = {"app", "-v", "sub", "-v"};
        int argc = std::size(argv);

        InCommand::CommandParser parser("testapp");
        auto& testRootCmdDesc = parser.GetRootCommandBlockDesc();
        testRootCmdDesc = rootCmd;
        const InCommand::CommandBlock &cmdBlock = parser.ParseArgs(argc, argv);
        
        // Should be at sub command level
        EXPECT_EQ(&cmdBlock.GetDesc(), &subCmd);
        EXPECT_TRUE(cmdBlock.IsOptionSet("verify"));
        
        // Check that root command also had its verbose set
        const InCommand::CommandBlock &firstBlock = parser.GetCommandBlock(0);
        EXPECT_TRUE(firstBlock.IsOptionSet("verbose"));
    }
    
    // Test 5: Multiple different aliases should work fine
    EXPECT_NO_THROW(
        {
            rootCmd.DeclareOption(InCommand::OptionType::Switch, "quiet", 'q');
            rootCmd.DeclareOption(InCommand::OptionType::Variable, "output", 'o');
            rootCmd.DeclareOption(InCommand::OptionType::Switch, "reset", 'r');
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
    auto& rootCmd = parser.GetRootCommandBlockDesc();
    
    // Test variables for binding
    int intValue = 0;
    float floatValue = 0.0f;
    double doubleValue = 0.0;
    char charValue = '\0';
    std::string stringValue;
    
    // Declare options with bindings
    rootCmd.DeclareOption(InCommand::OptionType::Variable, "int-opt").BindTo(intValue);
    rootCmd.DeclareOption(InCommand::OptionType::Variable, "float-opt").BindTo(floatValue);
    rootCmd.DeclareOption(InCommand::OptionType::Variable, "double-opt").BindTo(doubleValue);
    rootCmd.DeclareOption(InCommand::OptionType::Variable, "char-opt").BindTo(charValue);
    rootCmd.DeclareOption(InCommand::OptionType::Variable, "string-opt").BindTo(stringValue);
    
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
    auto& rootCmd = parser.GetRootCommandBlockDesc();
    
    // Test variables for parameter binding
    std::string filename;
    int count = 0;
    
    // Declare parameters with bindings
    rootCmd.DeclareOption(InCommand::OptionType::Parameter, "filename").BindTo(filename);
    rootCmd.DeclareOption(InCommand::OptionType::Parameter, "count").BindTo(count);
    
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
    auto& rootCmd = parser.GetRootCommandBlockDesc();
    
    std::string upperValue;
    
    // Declare option with custom converter
    rootCmd.DeclareOption(InCommand::OptionType::Variable, "upper").BindTo(upperValue, UppercaseConverter{});
    
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
    auto& rootCmd = parser.GetRootCommandBlockDesc();
    
    int intValue = 0;
    
    // Declare option with integer binding
    rootCmd.DeclareOption(InCommand::OptionType::Variable, "number").BindTo(intValue);
    
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

TEST(InCommand, TemplateBinding_SwitchError)
{
    InCommand::CommandParser parser("testapp");
    auto& rootCmd = parser.GetRootCommandBlockDesc();
    
    bool switchValue = false;
    
    // Should throw ApiException when trying to bind to a switch
    EXPECT_THROW(
        rootCmd.DeclareOption(InCommand::OptionType::Switch, "flag").BindTo(switchValue),
        InCommand::ApiException
    );
}

TEST(InCommand, TemplateBinding_BooleanVariables)
{
    InCommand::CommandParser parser("testapp");
    auto& rootCmd = parser.GetRootCommandBlockDesc();
    
    bool flag = false;
    
    // Declare boolean variable (not switch)
    rootCmd.DeclareOption(InCommand::OptionType::Variable, "flag").BindTo(flag);
    
    // Test "true"
    const char* argv1[] = {"testapp", "--flag", "true"};
    flag = false;
    parser.ParseArgs(3, argv1);
    EXPECT_TRUE(flag);
    
    // Reset parser for next test
    InCommand::CommandParser parser2("testapp");
    auto& rootCmd2 = parser2.GetRootCommandBlockDesc();
    rootCmd2.DeclareOption(InCommand::OptionType::Variable, "flag").BindTo(flag);
    
    // Test "false" 
    const char* argv2[] = {"testapp", "--flag", "false"};
    flag = true;
    parser2.ParseArgs(3, argv2);
    EXPECT_FALSE(flag);
}

TEST(InCommand, TemplateBinding_CustomEnumType)
{
    InCommand::CommandParser parser("testapp");
    auto& rootCmd = parser.GetRootCommandBlockDesc();
    
    LogLevel logLevel = LogLevel::Info;  // Default value
    
    // Declare option with custom enum converter
    rootCmd.DeclareOption(InCommand::OptionType::Variable, "log-level", 'l')
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
        auto& rootCmd2 = parser2.GetRootCommandBlockDesc();
        LogLevel logLevel2 = LogLevel::Debug;
        rootCmd2.DeclareOption(InCommand::OptionType::Variable, "log-level", 'l')
               .BindTo(logLevel2, LogLevelConverter{});
        
        const char* argv[] = {"testapp", "-l", "error"};
        parser2.ParseArgs(3, argv);
        EXPECT_EQ(logLevel2, LogLevel::Error);
    }
    
    // Test parameter binding with enum
    {
        InCommand::CommandParser parser3("testapp");
        auto& rootCmd3 = parser3.GetRootCommandBlockDesc();
        LogLevel paramLevel = LogLevel::Debug;
        rootCmd3.DeclareOption(InCommand::OptionType::Parameter, "level")
               .BindTo(paramLevel, LogLevelConverter{});
        
        const char* argv[] = {"testapp", "warning"};
        parser3.ParseArgs(2, argv);
        EXPECT_EQ(paramLevel, LogLevel::Warning);
    }
    
    // Test invalid enum value throws exception
    {
        InCommand::CommandParser parser4("testapp");
        auto& rootCmd4 = parser4.GetRootCommandBlockDesc();
        LogLevel invalidLevel = LogLevel::Info;
        rootCmd4.DeclareOption(InCommand::OptionType::Variable, "log-level")
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
    auto& rootCmd = parser.GetRootCommandBlockDesc();
    
    // Declare a global verbose option
    parser.DeclareGlobalOption(InCommand::OptionType::Switch, "verbose", 'v')
        .SetDescription("Enable verbose output");
    
    // Add a subcommand
    auto& buildCmd = rootCmd.DeclareSubCommandBlock("build");
    buildCmd.DeclareOption(InCommand::OptionType::Parameter, "target")
        .SetDescription("Build target");
    
    // Test 1: Global option at root level
    {
        const char* argv[] = {"testapp", "--verbose"};
        const int argc = sizeof(argv) / sizeof(argv[0]);
        
        const InCommand::CommandBlock &cmdBlock = parser.ParseArgs(argc, argv);
        
        // Global option should be accessible via parser methods
        EXPECT_TRUE(parser.IsGlobalOptionSet("verbose"));
        
        // Global option should also be accessible from first command block
        EXPECT_EQ(parser.GetGlobalOptionContext("verbose"), 0);
    }
    
    // Test 2: Global option with subcommand
    {
        InCommand::CommandParser parser2("testapp");
        auto& rootCmd2 = parser2.GetRootCommandBlockDesc();
        
        // Re-declare the same structure for a fresh test
        parser2.DeclareGlobalOption(InCommand::OptionType::Switch, "verbose", 'v');
        auto& buildCmd2 = rootCmd2.DeclareSubCommandBlock("build");
        buildCmd2.DeclareOption(InCommand::OptionType::Parameter, "target");
        
        const char* argv[] = {"testapp", "build", "--verbose", "debug"};
        const int argc = sizeof(argv) / sizeof(argv[0]);
        
        const InCommand::CommandBlock &cmdBlock = parser2.ParseArgs(argc, argv);
        
        // Global option should be accessible via parser methods
        EXPECT_TRUE(parser2.IsGlobalOptionSet("verbose"));
        
        // Should be at build command level
        EXPECT_EQ(cmdBlock.GetDesc().GetName(), "build");
        EXPECT_TRUE(cmdBlock.IsParameterSet("target"));
        EXPECT_EQ(cmdBlock.GetParameterValue("target"), "debug");
    }
}

//------------------------------------------------------------------------------------------------
// Global Option Locality Test
//------------------------------------------------------------------------------------------------

TEST(InCommand, GlobalOptionLocality)
{
    InCommand::CommandParser parser("testapp");
    
    // Setup global options
    parser.DeclareGlobalOption(InCommand::OptionType::Switch, "verbose", 'v')
        .SetDescription("Enable verbose output");
    parser.DeclareGlobalOption(InCommand::OptionType::Variable, "config", 'c')
        .SetDescription("Configuration file");
    
    // Setup command hierarchy with unique IDs for identification
    auto& rootDesc = parser.GetRootCommandBlockDesc();
    rootDesc.SetDescription("Test application").SetUniqueId(1);
    
    auto& subCmd1 = rootDesc.DeclareSubCommandBlock("subcmd1");
    subCmd1.SetDescription("Sub command 1").SetUniqueId(2);
    
    auto& subCmd2 = subCmd1.DeclareSubCommandBlock("subcmd2");
    subCmd2.SetDescription("Sub command 2").SetUniqueId(3);
    
    // Test 1: Global option specified at left-most command block
    {
        const char* args[] = { "testapp", "--verbose", "subcmd1", "subcmd2" };
        const InCommand::CommandBlock &result = parser.ParseArgs(4, args);
        
        EXPECT_TRUE(parser.IsGlobalOptionSet("verbose"));
        EXPECT_FALSE(parser.IsGlobalOptionSet("config"));
        
        // GetGlobalOptionContext should return the left-most command block
        EXPECT_EQ(parser.GetGlobalOptionContext("verbose"), 0);
        
        // Option that wasn't set should throw
        EXPECT_THROW(parser.GetGlobalOptionContext("config"), InCommand::ApiException);
        
        // Final command should be subcmd2
        EXPECT_EQ(result.GetDesc().GetUniqueId<int>(), 3);
    }
    
    // Test 2: Global option specified at intermediate level
    {
        InCommand::CommandParser parser2("testapp");
        parser2.DeclareGlobalOption(InCommand::OptionType::Switch, "debug", 'd');
        parser2.DeclareGlobalOption(InCommand::OptionType::Variable, "output", 'o');
        
        auto& rootDesc2 = parser2.GetRootCommandBlockDesc();
        rootDesc2.SetUniqueId(10);
        
        auto& subCmd1_2 = rootDesc2.DeclareSubCommandBlock("subcmd1");
        subCmd1_2.SetUniqueId(20);
        
        auto& subCmd2_2 = subCmd1_2.DeclareSubCommandBlock("subcmd2");
        subCmd2_2.SetUniqueId(30);
        
        const char* args[] = { "testapp", "subcmd1", "--debug", "--output", "file.txt", "subcmd2" };
        const InCommand::CommandBlock &result = parser2.ParseArgs(6, args);
        
        EXPECT_TRUE(parser2.IsGlobalOptionSet("debug"));
        EXPECT_TRUE(parser2.IsGlobalOptionSet("output"));
        EXPECT_EQ(parser2.GetGlobalOptionValue("output"), "file.txt");
        
        // GetGlobalOptionContext should return the subcmd1 command block  
        EXPECT_EQ(parser2.GetGlobalOptionContext("debug"), 1); // subcmd1 block
        EXPECT_EQ(parser2.GetGlobalOptionContext("output"), 1); // subcmd1 block
        
        // Final command should be subcmd2
        EXPECT_EQ(result.GetDesc().GetUniqueId<int>(), 30);
    }
    
    // Test 3: Global option specified at deepest level
    {
        InCommand::CommandParser parser3("testapp");
        parser3.DeclareGlobalOption(InCommand::OptionType::Switch, "trace", 't');
        
        auto& rootDesc3 = parser3.GetRootCommandBlockDesc();
        rootDesc3.SetUniqueId(100);
        
        auto& subCmd1_3 = rootDesc3.DeclareSubCommandBlock("subcmd1");
        subCmd1_3.SetUniqueId(200);
        
        auto& subCmd2_3 = subCmd1_3.DeclareSubCommandBlock("subcmd2");
        subCmd2_3.SetUniqueId(300);
        
        const char* args[] = { "testapp", "subcmd1", "subcmd2", "--trace" };
        const InCommand::CommandBlock &result = parser3.ParseArgs(4, args);
        
        EXPECT_TRUE(parser3.IsGlobalOptionSet("trace"));
        
        // GetGlobalOptionContext should return the subcmd2 command block
        EXPECT_EQ(parser3.GetGlobalOptionContext("trace"), 2); // subcmd2 block
        
        // Final command should be subcmd2
        EXPECT_EQ(result.GetDesc().GetUniqueId<int>(), 300);
    }
    
    // Test 4: Multiple global options at different levels
    {
        InCommand::CommandParser parser4("testapp");
        parser4.DeclareGlobalOption(InCommand::OptionType::Switch, "verbose", 'v');
        parser4.DeclareGlobalOption(InCommand::OptionType::Switch, "debug", 'd');
        parser4.DeclareGlobalOption(InCommand::OptionType::Variable, "config", 'c');
        
        auto& rootDesc4 = parser4.GetRootCommandBlockDesc();
        rootDesc4.SetUniqueId(1000);
        
        auto& subCmd1_4 = rootDesc4.DeclareSubCommandBlock("subcmd1");
        subCmd1_4.SetUniqueId(2000);
        
        const char* args[] = { "testapp", "--verbose", "subcmd1", "--debug", "--config", "test.conf" };
        const InCommand::CommandBlock &result = parser4.ParseArgs(6, args);
        
        EXPECT_TRUE(parser4.IsGlobalOptionSet("verbose"));
        EXPECT_TRUE(parser4.IsGlobalOptionSet("debug"));
        EXPECT_TRUE(parser4.IsGlobalOptionSet("config"));
        
        // verbose should in block 0
        EXPECT_EQ(parser4.GetGlobalOptionContext("verbose"), 0);
        
        // debug and config should be in block 1
        EXPECT_EQ(parser4.GetGlobalOptionContext("debug"), 1);
        EXPECT_EQ(parser4.GetGlobalOptionContext("config"), 1);
        
        EXPECT_EQ(result.GetDesc().GetUniqueId<int>(), 2000);
    }
}
