#include <gtest/gtest.h>

#include "InCommand.h"

TEST(InCommand, BasicOptions)
{
    InCommand::CCommandReader reader("test");
    auto catId_foo = reader.DeclareCategory("foo");
    auto catId_bar = reader.DeclareCategory("bar");
    auto catId_bar_baz = reader.DeclareCategory(catId_bar, "baz");
    auto catId_zap = reader.DeclareCategory("zap");
    auto switchId_help = reader.DeclareSwitch("help", 'h');
    auto varId_foo_number = reader.DeclareVariable(catId_foo, "number");
    auto varId_bar_word = reader.DeclareVariable(catId_bar, "word");
    auto varId_bar_name = reader.DeclareVariable(catId_bar, "name", 'n');
    auto varId_bar_baz_color = reader.DeclareVariable(catId_bar_baz, "color", { "red", "green", "blue", "yellow", "purple" });
    auto paramId_zap_file1 = reader.DeclareParameter(catId_zap, "file1");
    auto paramId_zap_file2 = reader.DeclareParameter(catId_zap, "file2");

    {
        const char *argv[] =
        {
            "app",
            "--help",
            "foo",
            "--number",
            "42",
        };

        int argc(std::size(argv));

        InCommand::CCommandExpression commandExpression;
        EXPECT_EQ(InCommand::Status::Success, reader.ReadCommandExpression(argc, argv, commandExpression));
        EXPECT_EQ(commandExpression.GetCategoryDepth(), 2);
        EXPECT_TRUE(commandExpression.GetSwitchIsSet(switchId_help));
    }

    {
        const char *argv[] =
        {
            "app",
            "bar",
            "--word",
            "hello",
            "baz",
            "--color",
            "red"
        };

        int argc(std::size(argv));

        InCommand::CCommandExpression commandExpression;
        EXPECT_EQ(InCommand::Status::Success, reader.ReadCommandExpression(argc, argv, commandExpression));
        EXPECT_EQ(commandExpression.GetCategoryDepth(), 3);
        EXPECT_FALSE(commandExpression.GetSwitchIsSet(switchId_help));
        EXPECT_EQ(commandExpression.GetVariableValue(varId_bar_word, "goodbye"), std::string("hello"));
        EXPECT_FALSE(commandExpression.GetVariableIsSet(varId_bar_name));
        EXPECT_EQ(commandExpression.GetVariableValue(varId_bar_name, "Bill"), std::string("Bill"));
        EXPECT_TRUE(commandExpression.GetVariableIsSet(varId_bar_baz_color));
        EXPECT_EQ(commandExpression.GetVariableValue(varId_bar_baz_color, "blue"), std::string("red"));
    }

    {
        const char *argv[] =
        {
            "app",
            "-h",
        };

        int argc(std::size(argv));

        InCommand::CCommandExpression commandExpression;
        EXPECT_EQ(InCommand::Status::Success, reader.ReadCommandExpression(argc, argv, commandExpression));
        EXPECT_EQ(commandExpression.GetCategoryDepth(), 1);
        EXPECT_TRUE(commandExpression.GetSwitchIsSet(switchId_help));
    }

    {
        const char *argv[] =
        {
            "app",
            "--help",
        };

        int argc(std::size(argv));

        InCommand::CCommandExpression commandExpression;
        EXPECT_EQ(InCommand::Status::Success, reader.ReadCommandExpression(argc, argv, commandExpression));
        EXPECT_EQ(commandExpression.GetCategoryDepth(), 1);
        EXPECT_TRUE(commandExpression.GetSwitchIsSet(switchId_help));
    }

    {
        const char *argv[] =
        {
            "app",
            "bar",
            "--name",
            "Anna"
        };

        int argc(std::size(argv));

        InCommand::CCommandExpression commandExpression;
        EXPECT_EQ(InCommand::Status::Success, reader.ReadCommandExpression(argc, argv, commandExpression));
        EXPECT_EQ(commandExpression.GetCategoryDepth(), 2);
        EXPECT_FALSE(commandExpression.GetSwitchIsSet(switchId_help));
        EXPECT_EQ(commandExpression.GetVariableValue(varId_bar_name, "Bill"), std::string("Anna"));
    }

    {
        const char *argv[] =
        {
            "app",
            "bar",
            "-n",
            "Elaine",
            "baz",
            "--color",
            "red"
        };

        int argc(std::size(argv));

        InCommand::CCommandExpression commandExpression;
        EXPECT_EQ(InCommand::Status::Success, reader.ReadCommandExpression(argc, argv, commandExpression));
        EXPECT_EQ(commandExpression.GetCategoryDepth(), 3);
        EXPECT_FALSE(commandExpression.GetSwitchIsSet(switchId_help));
        EXPECT_EQ(commandExpression.GetVariableValue(varId_bar_word, "goodbye"), std::string("goodbye"));
        EXPECT_EQ(commandExpression.GetVariableValue(varId_bar_name, "Bill"), std::string("Elaine"));
        EXPECT_EQ(commandExpression.GetVariableValue(varId_bar_baz_color, "blue"), std::string("red"));
    }

    {
        const char *argv[] =
        {
            "app",
            "zap",
            "file1",
            "file2"
        };

        int argc(std::size(argv));

        InCommand::CCommandExpression commandExpression;
        EXPECT_EQ(InCommand::Status::Success, reader.ReadCommandExpression(argc, argv, commandExpression));
        EXPECT_EQ(commandExpression.GetCategoryDepth(), 2);
        EXPECT_EQ(commandExpression.GetParameterValue(paramId_zap_file1, ""), std::string("file1"));
        EXPECT_EQ(commandExpression.GetParameterValue(paramId_zap_file2, ""), std::string("file2"));
    }

    {
        const char *argv[] =
        {
            "app",
            "bar",
            "baz",
            "--color",
            "orange"
        };

        int argc(std::size(argv));

        InCommand::CCommandExpression commandExpression;
        EXPECT_EQ(InCommand::Status::InvalidValue, reader.ReadCommandExpression(argc, argv, commandExpression));
    }
}

TEST(InCommand, Parameters)
{
    InCommand::CCommandReader CmdReader("app");
    auto fileId1 = CmdReader.DeclareParameter("file1", "file 1");
    auto fileId2 = CmdReader.DeclareParameter("file2", "file 2");
    auto fileId3 = CmdReader.DeclareParameter("file3", "file 3");
    auto someSwitchId = CmdReader.DeclareSwitch("some-switch", "Some switch");

    {
        const char *argv[] =
        {
            "foo",
            "myfile1.txt",
            "--some-switch",
            "myfile2.txt",
            "myfile3.txt",
        };
        const int argc = sizeof(argv) / sizeof(argv[0]);

        InCommand::CCommandExpression cmdExpression;
        EXPECT_EQ(InCommand::Status::Success, CmdReader.ReadCommandExpression(argc, argv, cmdExpression));

        EXPECT_TRUE(cmdExpression.GetSwitchIsSet(someSwitchId));
        EXPECT_EQ(cmdExpression.GetParameterValue(fileId1, ""), std::string(argv[1]));
        EXPECT_EQ(cmdExpression.GetParameterValue(fileId2, ""), std::string(argv[3]));
        EXPECT_EQ(cmdExpression.GetParameterValue(fileId3, ""), std::string(argv[4]));
        EXPECT_TRUE(cmdExpression.GetParameterIsSet(fileId1));
        EXPECT_TRUE(cmdExpression.GetParameterIsSet(fileId2));
        EXPECT_TRUE(cmdExpression.GetParameterIsSet(fileId3));
    }

    {
        const char *argv[] =
        {
            "foo",
            "myfile1.txt",
            "--some-switch",
            "myfile2.txt",
        };
        const int argc = sizeof(argv) / sizeof(argv[0]);

        InCommand::CCommandExpression cmdExpression;
        EXPECT_EQ(InCommand::Status::Success, CmdReader.ReadCommandExpression(argc, argv, cmdExpression));

        EXPECT_TRUE(cmdExpression.GetSwitchIsSet(someSwitchId));
        EXPECT_EQ(cmdExpression.GetParameterValue(fileId1, ""), std::string(argv[1]));
        EXPECT_EQ(cmdExpression.GetParameterValue(fileId2, ""), std::string(argv[3]));
        EXPECT_EQ(cmdExpression.GetParameterValue(fileId3, "nope"), "nope");
        EXPECT_TRUE(cmdExpression.GetParameterIsSet(fileId1));
        EXPECT_TRUE(cmdExpression.GetParameterIsSet(fileId2));
        EXPECT_FALSE(cmdExpression.GetParameterIsSet(fileId3));
    }
}

template<typename E>
constexpr auto to_underlying(E e) noexcept
{
    return static_cast<std::underlying_type_t<E>>(e);
}

TEST(InCommand, SubCategories)
{
    InCommand::CCommandReader CmdReader("app");
    auto switchId_help = CmdReader.DeclareSwitch("help", "");

    auto plantHandle = CmdReader.DeclareCategory("plant", "");
    auto treeHandle = CmdReader.DeclareCategory(plantHandle, "tree", "");
    auto shrubHandle = CmdReader.DeclareCategory(plantHandle, "shrub", "");
    auto animalHandle = CmdReader.DeclareCategory("animal", "");
    auto dogHandle = CmdReader.DeclareCategory(animalHandle, "dog", "");
    auto catHandle = CmdReader.DeclareCategory(animalHandle, "cat", "");

    auto listHandle = CmdReader.DeclareSwitch(plantHandle, "list", "");
    auto pruneHandle = CmdReader.DeclareSwitch(shrubHandle, "prune", "");
    auto burnHandle = CmdReader.DeclareSwitch(shrubHandle, "burn", "");
    auto livesHandle = CmdReader.DeclareVariable(catHandle, "lives", "");

    for(int i = 0; i < 3; ++i)
    {
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

            InCommand::CCommandExpression CmdExpression;
            EXPECT_EQ(InCommand::Status::Success, CmdReader.ReadCommandExpression(argc, argv, CmdExpression));
            EXPECT_EQ(CmdExpression.GetCategoryDepth(), 3);
            EXPECT_EQ(CmdExpression.GetCategory(0), InCommand::RootCategory);
            EXPECT_EQ(CmdExpression.GetCategory(1), plantHandle);
            EXPECT_EQ(CmdExpression.GetCategory(2), shrubHandle);
            EXPECT_TRUE(CmdExpression.GetSwitchIsSet(burnHandle));
            EXPECT_FALSE(CmdExpression.GetSwitchIsSet(pruneHandle));

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

            InCommand::CCommandExpression CmdExpression;
            EXPECT_EQ(InCommand::Status::Success, CmdReader.ReadCommandExpression(argc, argv, CmdExpression));
            EXPECT_EQ(CmdExpression.GetCategoryDepth(), 3);
            EXPECT_EQ(CmdExpression.GetCategory(0), InCommand::RootCategory);
            EXPECT_EQ(CmdExpression.GetCategory(1), animalHandle);
            EXPECT_EQ(CmdExpression.GetCategory(2), catHandle);
            EXPECT_TRUE(CmdExpression.GetVariableIsSet(livesHandle));
            EXPECT_EQ(CmdExpression.GetVariableValue(livesHandle, "9"), "8");

            break; }
        case 2: {
            const char* argv[] =
            {
                "app",
                "--help",
            };
            const int argc = sizeof(argv) / sizeof(argv[0]);

            InCommand::CCommandExpression CmdExpression;
            EXPECT_EQ(InCommand::Status::Success, CmdReader.ReadCommandExpression(argc, argv, CmdExpression));
            EXPECT_EQ(CmdExpression.GetCategoryDepth(), 1);
            EXPECT_EQ(CmdExpression.GetCategory(0), InCommand::RootCategory);
            EXPECT_TRUE(CmdExpression.GetSwitchIsSet(switchId_help));

            break; }
        }
    }
}

TEST(InCommand, Errors)
{
    {
        InCommand::CCommandReader CmdReader("app");
        CmdReader.DeclareCategory("goto");
        try
        {
            CmdReader.DeclareCategory("goto"); // Duplicate category "goto"
        }
        catch(const InCommand::Exception& e)
        {
            EXPECT_EQ(e.Status(), InCommand::Status::DuplicateCategory);
        }
    }
    
    {
        InCommand::CCommandReader CmdReader("app");
        CmdReader.DeclareSwitch("foo");
        try
        {
            CmdReader.DeclareSwitch("foo", "Blah"); // Duplicate switch option "foo"
        }
        catch (const InCommand::Exception& e)
        {
            EXPECT_EQ(e.Status(), InCommand::Status::DuplicateOption);
        }
    }

    {
        const char* argv[] = { "app", "gogo", "--foo", "--bar", "7" };
        const int argc = sizeof(argv) / sizeof(argv[0]);

        InCommand::CCommandReader CmdReader("app");

        const InCommand::CategoryHandle gotoId = CmdReader.DeclareCategory("goto");
        CmdReader.DeclareSwitch(gotoId, "foo", "");
        CmdReader.DeclareVariable(gotoId, "bar", "");

        InCommand::CCommandExpression cmdExp;
        EXPECT_EQ(InCommand::Status::UnexpectedArgument, CmdReader.ReadCommandExpression(argc, argv, cmdExp));
    }

    {
        const char* argv[] = { "app", "goto", "--foo", "--bar", "7" };
        const int argc = sizeof(argv) / sizeof(argv[0]);

        InCommand::CCommandReader CmdReader("app");

        const auto gotoId = CmdReader.DeclareCategory("goto");
        CmdReader.DeclareSwitch(gotoId, "fop");
        CmdReader.DeclareVariable(gotoId, "bar");

        InCommand::CCommandExpression cmdExp;
        EXPECT_EQ(InCommand::Status::UnknownOption, CmdReader.ReadCommandExpression(argc, argv, cmdExp));
    }

    {
        const char* argv[] = { "app", "--foo", "hello" };
        const int argc = sizeof(argv) / sizeof(argv[0]);

        InCommand::CCommandReader CmdReader("app");

        const auto gotoId = CmdReader.DeclareCategory("goto");
        CmdReader.DeclareVariable("foo", {"cat", "dog", "fish"});

        InCommand::CCommandExpression cmdExp;
        EXPECT_EQ(InCommand::Status::InvalidValue, CmdReader.ReadCommandExpression(argc, argv, cmdExp));
    }

    {
        const char* argv[] = { "app", "--foo"};
        const int argc = sizeof(argv) / sizeof(argv[0]);

        InCommand::CCommandReader CmdReader("app");
        CmdReader.DeclareVariable("foo", {"cat", "dog", "fish"});

        InCommand::CCommandExpression cmdExp;
        EXPECT_EQ(InCommand::Status::MissingVariableValue, CmdReader.ReadCommandExpression(argc, argv, cmdExp));
    }

    {
        const char* argv[] = { "app", "--foo", "--bar"};
        const int argc = sizeof(argv) / sizeof(argv[0]);

        InCommand::CCommandReader CmdReader("app");
        CmdReader.DeclareVariable("foo", { "cat", "dog", "fish" });
        CmdReader.DeclareSwitch("bar");

        InCommand::CCommandExpression cmdExp;
        EXPECT_EQ(InCommand::Status::MissingVariableValue, CmdReader.ReadCommandExpression(argc, argv, cmdExp));
    }

    {
        const char* argv[] = { "app", "--foo", "7" };
        const int argc = sizeof(argv) / sizeof(argv[0]);

        InCommand::CCommandReader CmdReader("app");

        CmdReader.DeclareVariable("foo", { "1", "3", "5" });

        InCommand::CCommandExpression cmdExp;
        EXPECT_EQ(InCommand::Status::InvalidValue, CmdReader.ReadCommandExpression(argc, argv, cmdExp));
    }
}
