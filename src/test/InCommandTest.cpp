#include <gtest/gtest.h>

#include "InCommand.h"

TEST(InCommand, BasicOptions)
{
    InCommand::CCommandReader reader("test");
    size_t catId_foo = reader.DeclareCategory(0, "foo");
    size_t catId_bar = reader.DeclareCategory(0, "bar");
    size_t catId_bar_baz = reader.DeclareCategory(catId_bar, "baz");
    size_t catId_zap = reader.DeclareCategory(0, "zap");
    auto switchId_help = reader.DeclareSwitch(0, "help", 'h');
    auto varId_foo_number = reader.DeclareVariable(catId_foo, "number");
    auto varId_bar_word = reader.DeclareVariable(catId_bar, "word");
    auto varId_bar_name = reader.DeclareVariable(catId_bar, "name", 'n');
    auto varId_bar_baz_color = reader.DeclareVariable(catId_bar_baz, "color", { "red", "green", "blue", "yellow", "purple" });
    size_t paramId_zap_file1 = reader.DeclareParameter(catId_zap, "file1");
    size_t paramId_zap_file2 = reader.DeclareParameter(catId_zap, "file2");

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
    size_t fileId1 = CmdReader.DeclareParameter(0, "file1", "file 1");
    size_t fileId2 = CmdReader.DeclareParameter(0, "file2", "file 2");
    size_t fileId3 = CmdReader.DeclareParameter(0, "file3", "file 3");
    size_t someSwitchId = CmdReader.DeclareSwitch(0, "some-switch", "Some switch");

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
    enum class CategoryId : size_t
    {
        Root = 0,
        Plant,
        Tree,
        Shrub,
        Animal,
        Dog,
        Cat
    };

    InCommand::CCommandReader CmdReader("app");
    auto switchId_help = CmdReader.DeclareSwitch(0, "help", "");

    EXPECT_EQ(to_underlying(CategoryId::Plant), CmdReader.DeclareCategory(0, "plant", ""));
    EXPECT_EQ(to_underlying(CategoryId::Tree), CmdReader.DeclareCategory(to_underlying(CategoryId::Plant), "tree", ""));
    EXPECT_EQ(to_underlying(CategoryId::Shrub), CmdReader.DeclareCategory(to_underlying(CategoryId::Plant), "shrub", ""));
    EXPECT_EQ(to_underlying(CategoryId::Animal), CmdReader.DeclareCategory(0, "animal", ""));
    EXPECT_EQ(to_underlying(CategoryId::Dog), CmdReader.DeclareCategory(to_underlying(CategoryId::Animal), "dog", ""));
    EXPECT_EQ(to_underlying(CategoryId::Cat), CmdReader.DeclareCategory(to_underlying(CategoryId::Animal), "cat", ""));

    auto listId = CmdReader.DeclareSwitch(to_underlying(CategoryId::Plant), "list", "");
    auto pruneId = CmdReader.DeclareSwitch(to_underlying(CategoryId::Shrub), "prune", "");
    auto burnId = CmdReader.DeclareSwitch(to_underlying(CategoryId::Shrub), "burn", "");
    auto livesId = CmdReader.DeclareVariable(to_underlying(CategoryId::Cat), "lives", "");

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
            EXPECT_EQ(CmdExpression.GetCategoryId(0), 0);
            EXPECT_EQ(CmdExpression.GetCategoryId(1), to_underlying(CategoryId::Plant));
            EXPECT_EQ(CmdExpression.GetCategoryId(2), to_underlying(CategoryId::Shrub));
            EXPECT_TRUE(CmdExpression.GetSwitchIsSet(burnId));
            EXPECT_FALSE(CmdExpression.GetSwitchIsSet(pruneId));

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
            EXPECT_EQ(CmdExpression.GetCategoryId(0), 0);
            EXPECT_EQ(CmdExpression.GetCategoryId(1), to_underlying(CategoryId::Animal));
            EXPECT_EQ(CmdExpression.GetCategoryId(2), to_underlying(CategoryId::Cat));
            EXPECT_TRUE(CmdExpression.GetVariableIsSet(livesId));
            EXPECT_EQ(CmdExpression.GetVariableValue(livesId, "9"), "8");

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
            EXPECT_EQ(CmdExpression.GetCategoryId(0), 0);
            EXPECT_TRUE(CmdExpression.GetSwitchIsSet(switchId_help));

            break; }
        }
    }
}

TEST(InCommand, Errors)
{
    {
        InCommand::CCommandReader CmdReader("app");
        CmdReader.DeclareCategory(0, "goto", "");
        try
        {
            CmdReader.DeclareCategory(0, "goto", ""); // Duplicate category "goto"
        }
        catch(const InCommand::Exception& e)
        {
            EXPECT_EQ(e.Status(), InCommand::Status::DuplicateCategory);
        }
    }
    
    {
        InCommand::CCommandReader CmdReader("app");
        CmdReader.DeclareSwitch(0, "foo", "");
        try
        {
            CmdReader.DeclareSwitch(0, "foo", ""); // Duplicate switch option "foo"
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

        const size_t gotoId = CmdReader.DeclareCategory(0, "goto", "");
        CmdReader.DeclareSwitch(gotoId, "foo", "");
        CmdReader.DeclareVariable(gotoId, "bar", "");

        InCommand::CCommandExpression cmdExp;
        EXPECT_EQ(InCommand::Status::UnexpectedArgument, CmdReader.ReadCommandExpression(argc, argv, cmdExp));
    }

    {
        const char* argv[] = { "app", "goto", "--foo", "--bar", "7" };
        const int argc = sizeof(argv) / sizeof(argv[0]);

        InCommand::CCommandReader CmdReader("app");

        const size_t gotoId = CmdReader.DeclareCategory(0, "goto");
        CmdReader.DeclareSwitch(gotoId, "fop");
        CmdReader.DeclareVariable(gotoId, "bar");

        InCommand::CCommandExpression cmdExp;
        EXPECT_EQ(InCommand::Status::UnknownOption, CmdReader.ReadCommandExpression(argc, argv, cmdExp));
    }

    {
        const char* argv[] = { "app", "--foo", "hello" };
        const int argc = sizeof(argv) / sizeof(argv[0]);

        InCommand::CCommandReader CmdReader("app");

        size_t gotoId = CmdReader.DeclareCategory(0, "goto");
        CmdReader.DeclareVariable(0, "foo", {"cat", "dog", "fish"});

        InCommand::CCommandExpression cmdExp;
        EXPECT_EQ(InCommand::Status::InvalidValue, CmdReader.ReadCommandExpression(argc, argv, cmdExp));
    }

    {
        const char* argv[] = { "app", "--foo"};
        const int argc = sizeof(argv) / sizeof(argv[0]);

        InCommand::CCommandReader CmdReader("app");
        CmdReader.DeclareVariable(0, "foo", {"cat", "dog", "fish"});

        InCommand::CCommandExpression cmdExp;
        EXPECT_EQ(InCommand::Status::MissingVariableValue, CmdReader.ReadCommandExpression(argc, argv, cmdExp));
    }

    {
        const char* argv[] = { "app", "--foo", "--bar"};
        const int argc = sizeof(argv) / sizeof(argv[0]);

        InCommand::CCommandReader CmdReader("app");
        CmdReader.DeclareVariable(0, "foo", { "cat", "dog", "fish" });
        CmdReader.DeclareSwitch(0, "bar");

        InCommand::CCommandExpression cmdExp;
        EXPECT_EQ(InCommand::Status::MissingVariableValue, CmdReader.ReadCommandExpression(argc, argv, cmdExp));
    }

    {
        const char* argv[] = { "app", "--foo", "7" };
        const int argc = sizeof(argv) / sizeof(argv[0]);

        InCommand::CCommandReader CmdReader("app");

        CmdReader.DeclareVariable(0, "foo", { "1", "3", "5" });

        InCommand::CCommandExpression cmdExp;
        EXPECT_EQ(InCommand::Status::InvalidValue, CmdReader.ReadCommandExpression(argc, argv, cmdExp));
    }
}
