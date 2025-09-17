#include <iostream>
#include <string>
#include <random>
#include <assert.h>

#include "InCommand.h"

// Enum class for command block unique IDs
enum class CommandId
{
    Root,
    Add,
    Mul,
    Roshambo
};

int main(int argc, const char *argv[])
{
    // Create InCommand parser
    InCommand::CommandParser parser("sample");
    
    // Enable auto-help with default option names, outputting to cout by default
    parser.EnableAutoHelp("help", 'h');
    
    // Customize the auto-help description
    parser.SetAutoHelpDescription("Display comprehensive usage information and examples");
    
    // Variables for template binding
    int value1 = 0;
    int value2 = 0;
    std::string message;
    
    // Configure the root descriptor through the parser
    InCommand::CommandBlockDesc& rootCmdDesc = parser.GetRootCommandBlockDesc();
    rootCmdDesc.SetDescription("Sample application demonstrating InCommand")
               .SetUniqueId(CommandId::Root);
    
    // Declare a global verbose option that will be available for all commands
    parser.DeclareGlobalOption(InCommand::OptionType::Switch, "verbose", 'v')
        .SetDescription("Enable verbose output globally");

    // Note: Auto-help is explicitly enabled with --help/-h option
    // This will automatically handle help requests for all command contexts!

    // Add inner command blocks for each operation
    InCommand::CommandBlockDesc& addCmdDesc = rootCmdDesc.DeclareSubCommandBlock("add");
    addCmdDesc
        .SetDescription("Adds two integers")
        .SetUniqueId(CommandId::Add);
    addCmdDesc
        .DeclareOption(InCommand::OptionType::Parameter, "value1")
        .BindTo(value1)
        .SetDescription("First add value");
    addCmdDesc
        .DeclareOption(InCommand::OptionType::Parameter, "value2")
        .BindTo(value2)
        .SetDescription("Second add value");
    addCmdDesc
        .DeclareOption(InCommand::OptionType::Switch, "quiet", 'q')
        .SetDescription("Suppress normal output, show only result");
    addCmdDesc
        .DeclareOption(InCommand::OptionType::Variable, "message", 'm')
        .BindTo(message)
        .SetDescription("Print <message> N-times where N = value1 + value2");

    // Add inner command blocks for each operation
    InCommand::CommandBlockDesc& mulCmdDesc = rootCmdDesc.DeclareSubCommandBlock("mul");
    mulCmdDesc
        .SetDescription("Adds two integers")
        .SetUniqueId(CommandId::Add);
    mulCmdDesc
        .DeclareOption(InCommand::OptionType::Parameter, "value1")
        .BindTo(value1)
        .SetDescription("First add value");
    mulCmdDesc
        .DeclareOption(InCommand::OptionType::Parameter, "value2")
        .BindTo(value2)
        .SetDescription("Second add value");
    mulCmdDesc
        .DeclareOption(InCommand::OptionType::Switch, "quiet", 'q')
        .SetDescription("Suppress normal output, show only result");
    mulCmdDesc
        .DeclareOption(InCommand::OptionType::Variable, "message", 'm')
        .BindTo(message)
        .SetDescription("Print <message> N-times where N = value1 + value2");

    InCommand::CommandBlockDesc& roshamboCmdDesc = rootCmdDesc.DeclareSubCommandBlock("roshambo");
    roshamboCmdDesc
        .SetDescription("Play Roshambo")
        .SetUniqueId(CommandId::Roshambo);
    roshamboCmdDesc
        .DeclareOption(InCommand::OptionType::Variable, "choice")
            .SetDomain({ "rock", "paper", "scissors" })
            .SetDescription("1-2-3 Shoot!");
    
    try 
    {
        // ParseArgs returns the number of parsed command blocks
        // Auto-help (--help/-h) is automatically handled and will output to the configured stream
        size_t numBlocks = parser.ParseArgs(argc, argv);
        
        // Check if help was requested - if so, just exit since help was already output
        if (parser.WasAutoHelpRequested())
        {
            return 0;
        }
        
        // Get the rightmost (most specific) command block
        const InCommand::CommandBlock &cmdBlock = parser.GetCommandBlock(numBlocks - 1);
        
        // Check if the global verbose option is set
        if (parser.IsGlobalOptionSet("verbose"))
        {
            const size_t verboseBlockIndex = parser.GetGlobalOptionContext("verbose");

            if (verboseBlockIndex == 0)
            {
                std::cout << "InCommand Sample Application" << std::endl;
                std::cout << "This sample demonstrates how to build structured command-line interfaces" << std::endl;
                std::cout << "with hierarchical commands, global options, and type-safe argument parsing." << std::endl;
            }
        }
        
        // Now we can determine which command was actually parsed by checking the unique ID
        CommandId commandId = cmdBlock.GetDesc().GetUniqueId<CommandId>();
        
        switch (commandId)
        {
        case CommandId::Root:
            std::cout << "Error: No command specified" << std::endl;
            std::cout << "Use --help for usage information" << std::endl;
            return -1;
            break;
            
        case CommandId::Add:
            {
                // Check if required parameters were provided
                if (!cmdBlock.IsParameterSet("value1") || !cmdBlock.IsParameterSet("value2"))
                {
                    std::cout << std::endl;
                    std::cout << "Error: Both value1 and value2 are required for add command" << std::endl;
                    return -1;
                }
                
                // Values are already converted and stored in bound variables!
                int result = value1 + value2;
                
                bool verbose = parser.IsGlobalOptionSet("verbose");
                bool quiet = cmdBlock.IsOptionSet("quiet");
                
                if (verbose)
                {
                    std::cout << "Verbose: Adding " << value1 << " and " << value2 << std::endl;
                    if (quiet)
                    {
                        std::cout << "Verbose: Quiet mode enabled - suppressing normal output" << std::endl;
                    }
                }
                
                if (!quiet)
                {
                    std::cout << value1 << " + " << value2 << " = " << result << std::endl;
                }
                else
                {
                    std::cout << result << std::endl;  // Just the result in quiet mode
                }
                
                if (!message.empty())
                {
                    if (verbose)
                    {
                        std::cout << "Verbose: Printing message '" << message << "' " << result << " times" << std::endl;
                    }
                    for (int i = 0; i < result; ++i)
                        std::cout << message << std::endl;
                }
            }
            break;
            
        case CommandId::Mul:
            {
                // Check if required parameters were provided
                if (!cmdBlock.IsParameterSet("value1") || !cmdBlock.IsParameterSet("value2"))
                {
                    std::cout << std::endl;
                    std::cout << "Error: Both value1 and value2 are required for mul command" << std::endl;
                    return -1;
                }
                
                // Values are already converted and stored in bound variables!
                int result = value1 * value2;
                
                bool verbose = parser.IsGlobalOptionSet("verbose");
                bool quiet = cmdBlock.IsOptionSet("quiet");
                
                if (verbose)
                {
                    std::cout << "Verbose: Multiplying " << value1 << " and " << value2 << std::endl;
                    if (quiet)
                    {
                        std::cout << "Verbose: Quiet mode enabled - suppressing normal output" << std::endl;
                    }
                }
                
                if (!quiet)
                {
                    std::cout << value1 << " * " << value2 << " = " << result << std::endl;
                }
                else
                {
                    std::cout << result << std::endl;  // Just the result in quiet mode
                }
                
                if (!message.empty())
                {
                    if (verbose)
                    {
                        std::cout << "Verbose: Printing message '" << message << "' " << result << " times" << std::endl;
                    }
                    for (int i = 0; i < result; ++i)
                        std::cout << message << std::endl;
                }
            }
            break;
            
        case CommandId::Roshambo:
            {
                auto playerMove = cmdBlock.GetOptionValue("choice", "");
                
                if (playerMove.empty())
                {
                    std::cout << std::endl;
                    std::cout << "Error: move is required for roshambo command" << std::endl;
                    std::cout << "Usage: sample roshambo --move <rock|paper|scissors>" << std::endl;
                    return -1;
                }

                // Seed the random number generator
                std::random_device rd;
                std::mt19937 gen(rd());
                std::uniform_int_distribution<> distrib(1, 3);
                int randomInt = distrib(gen);

                std::string computerMove;
                switch (randomInt)
                {
                case 1:
                    computerMove = "rock";
                    break;
                case 2:
                    computerMove = "paper";
                    break;
                case 3:
                    computerMove = "scissors";
                    break;
                }

                std::cout << "Your move: " << playerMove << std::endl;
                std::cout << "My Move: " << computerMove << std::endl;

                if (playerMove == computerMove)
                {
                    std::cout << "Tie :|" << std::endl;
                }
                else if ((playerMove == "rock" && computerMove == "scissors") ||
                         (playerMove == "paper" && computerMove == "rock") ||
                         (playerMove == "scissors" && computerMove == "paper"))
                {
                    std::cout << "You Win :(" << std::endl;
                }
                else
                {
                    std::cout << "I Win! :)" << std::endl;
                }
            }
            break;
            
        default:
            std::cout << "Error: Unknown command" << std::endl;
            std::cout << "Use --help for usage information" << std::endl;
            return -1;
        }
    }
    catch (const InCommand::ApiException& e)
    {
        std::cout << "API Error: " << e.GetMessage() << std::endl;
        return -1;
    }
    catch (const InCommand::SyntaxException& e)
    {
        std::cout << "Command Line Error: " << e.GetMessage() << std::endl;
        if (!e.GetToken().empty())
        {
            std::cout << "Problem with: " << e.GetToken() << std::endl;
        }
        std::cout << "Use --help for usage information" << std::endl;
        return -1;
    }
    catch (const std::exception& e)
    {
        std::cout << "Standard exception: " << e.what() << std::endl;
        return -1;
    }

    return 0;
}
