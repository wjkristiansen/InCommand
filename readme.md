# InCommand

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)

**InCommand** is a modern, hierarchical command-line interface (CLI) argument parser for C++ applications. It provides a clean, type-safe API for defining complex command structures with subcommands, options, parameters, and aliases.

---

## Features

**Hierarchical Command Structure**: Support for nested command blocks (e.g., `git remote add origin url`)  
**Multiple Option Types**: Switches, variables, and positional parameters  
**Multi-Occurrence Options**: Opt-in support for options that can appear multiple times, with first-value default semantics and indexed access  
**Type-Safe Value Binding**: Template method for automatic type conversion and variable binding  
**Global Options**: Options available across all command contexts  
**Auto-Help System**: Configurable automatic help generation with context-sensitive output  
**Variable Delimiters**: Support for packed syntax (`--name=value`) alongside traditional whitespace format  
**Alias Support**: Single-character aliases with switch grouping (`-vqh`)  
**API Validation**: Prevents logical errors (e.g., parameters with aliases) at setup time  
**Syntax Error Reporting**: Detailed error messages with problematic token identification  

---

## Quick Start

```cpp
#include "InCommand.h"
using namespace InCommand;

int main(int argc, const char *argv[])
{
    // Create InCommand parser - it owns the root command descriptor
    CommandParser parser("mybuildtool");
    auto& rootCmdDesc = parser.GetAppCommandDecl();
    rootCmdDesc.SetDescription("My awesome application");

    // Enable auto-help
    parser.EnableAutoHelp("help", 'h');

    // Add a subcommand with cascading style
    CommandDecl& buildCmdDesc = rootCmdDesc.AddSubCommand("build");
    buildCmdDesc.SetDescription("Build the project");
    buildCmdDesc.AddOption(OptionType::Switch, "verbose", 'v')
        .SetDescription("Enable verbose output");
    buildCmdDesc.AddOption(OptionType::Variable, "target")
        .SetDescription("Build target");
    buildCmdDesc.AddOption(OptionType::Parameter, "project")
        .SetDescription("Project file");

    try
    {
        // ParseArgs returns the number of command blocks that were parsed
        auto numBlocks = parser.ParseArgs(argc, argv);
        
        // Check if help was requested
        if (parser.WasAutoHelpRequested()) {
            return 0; // Help was displayed automatically, exit gracefully
        }
        
        // Get the rightmost (most specific) command block
        const CommandBlock &result = parser.GetCommandBlock(numBlocks - 1);
        
        // Check which command was parsed
        if (result.GetDecl().GetName() == "build")
        {
            bool verbose = result.IsOptionSet("verbose");
            std::string target = result.GetOptionValue("target", "debug");
            std::string project = result.GetOptionValue("project");
        }
    }
    catch (const SyntaxException& e)
    {
        std::cout << "Error: " << e.GetMessage() << std::endl;
        return -1;
    }

    return 0;
}
```

---

## Command Syntax and Structure

### Command Blocks

InCommand organizes command-line arguments into a **ordered sequence of command blocks**, where each block represents a command context with its own set of options. This architecture enables complex, git-like command structures while maintaining clean separation of concerns.

```text
Command Line:

myapp --verbose build --target Debug package --output-dir dist installer

Parsed as Ordered Command Blocks:  

[0] "myapp"
    └── Option: --verbose

[1] "build"
    └── Option: --target Debug

[2] "package"
    ├── Option: --output-dir dist  
    └── Parameter: installer
```

### Options

Options are specified within a command block context. Options come in three distinct types:

- Switches - Boolean flags
- Variables - Name/value pairs
- Parameters - Ordered values

#### Switches

Boolean flags that are either present (true) or absent (false).

**Syntax**: `--<name>` or `-<single-character-alias>`

```shell
myapp --verbose --quiet    # Two switches
myapp -v -q                # Two switches using aliases
myapp -vq                  # Grouped switch aliases
```

#### Variables  

Options that require a value argument.

**Syntax**: `--name value` or `-n value`

```shell
myapp --output file.txt    # Space-separated
myapp -o file.txt          # Single character alias form
```

#### Parameters

Positional arguments without option prefixes. Order matters and they are consumed in declaration order.

**Syntax**: Direct values

```shell
# Two parameters
myapp input.txt output.txt

# Active command block `origin` with URL parameter
git remote add origin https://github.com/user/repo.git
```

### Option Aliases

InCommand supports single-character aliases for switches and variables, enabling concise command-line usage. Aliases provide familiar, efficient shortcuts while maintaining full option name compatibility.

#### Switch Alias Grouping

Switches support **alias grouping**, allowing multiple single-character aliases to be combined into a single argument:

```shell
# Individual switch aliases
myapp -v -q -h

# Grouped switch aliases (equivalent to above)
myapp -vqh

# Mixed: some grouped, some individual
myapp -vq --help

# Grouping with full names
myapp --verbose -qh
```

#### Variable Alias Usage

Variables use aliases individually and cannot be grouped:

```shell
# Variable aliases work individually
myapp -o output.txt -c config.json

# Equivalent using full names
myapp --output output.txt --config config.json

# Mixed usage
myapp -o output.txt --config config.json
```

#### Alias Declaration

```cpp
// Switches with aliases
rootCmd.AddOption(OptionType::Switch, "verbose", 'v');
rootCmd.AddOption(OptionType::Switch, "quiet", 'q');
rootCmd.AddOption(OptionType::Switch, "help", 'h');

// Variables with aliases
rootCmd.AddOption(OptionType::Variable, "output", 'o');
rootCmd.AddOption(OptionType::Variable, "config", 'c');

// Parameters cannot have aliases
rootCmd.AddOption(OptionType::Parameter, "input-file");  // No alias allowed
```

#### Alias Rules and Restrictions

- **Switches Only**: Only switches can be grouped (`-abc` = `-a -b -c`)
- **Variables Individual**: Variable aliases must be used individually (`-o file.txt`)

---

## Multiple Occurrences (AllowMultiple)

Some options may appear multiple times on the command line (like `git commit -m "Title" -m "Details"`). InCommand supports this via an opt-in API on `OptionDecl`.

Key points:
- Enable per-option with `AllowMultiple()`. By default, additional occurrences are ignored for options that don’t allow multiples.
- Default getters return the first value when multiple values are present.
- Use indexed getters and count methods to access additional occurrences.
- Primarily used with variable options; may also be applied to parameters if your command grammar allows repeated positional values. Switches are presence-only; repeating a switch is idempotent.

Declare and parse:
```cpp
auto& commit = rootCmd.AddSubCommand("commit");
commit.AddOption(OptionType::Variable, "message", 'm')
      .AllowMultiple()
      .SetDescription("Commit message lines");

auto count = parser.ParseArgs(argc, argv);
const auto& block = parser.GetCommandBlock(count - 1); // commit block
```

Access values within the command block:
```cpp
// Default getter returns the first occurrence
std::string title = block.GetOptionValue("message");

// Iterate all occurrences
size_t n = block.GetOptionValueCount("message");
for (size_t i = 0; i < n; ++i) {
    const std::string& part = block.GetOptionValue("message", i);
    // use part
}
// Errors:
// - block.GetOptionValue("message", n) throws ApiException(ApiError::OutOfRange)
// - block.GetOptionValue("missing", 0) throws ApiException(ApiError::OptionNotFound)
```

Global options support multiple occurrences as well and track the command-block index for each occurrence:
```cpp
parser.AddGlobalOption(OptionType::Variable, "config", 'c').AllowMultiple();

parser.ParseArgs(argc, argv);

// First occurrence value
const std::string& firstConfig = parser.GetGlobalOptionValue("config");

// Iterate all occurrences with their origin blocks
size_t gcount = parser.GetGlobalOptionValueCount("config");
for (size_t i = 0; i < gcount; ++i) {
    const std::string& value = parser.GetGlobalOptionValue("config", i);
    size_t originBlockIndex = parser.GetGlobalOptionBlockIndex("config", i);
    const auto& originBlock = parser.GetCommandBlock(originBlockIndex);
    // use value with its originating context
}

// Convenience: last occurrence origin
size_t lastCtx = parser.GetGlobalOptionBlockIndex("config");
// Errors:
// - parser.GetGlobalOptionValue("config", gcount) throws ApiException(ApiError::OutOfRange)
// - parser.GetGlobalOptionValue("missing", 0) throws ApiException(ApiError::OptionNotFound)
// - parser.GetGlobalOptionBlockIndex("config", gcount) throws ApiException(ApiError::OutOfRange)
```

## Type-Safe Value Binding

InCommand provides a template method `BindTo()` for automatic type conversion and variable binding. When options are parsed, values are automatically converted from strings to the target variable type.

### Basic Example

```cpp
#include "InCommand.h"

int main(int argc, const char* argv[])
{
    CommandParser parser("myapp");
    auto& rootCmd = parser.GetAppCommandDecl();
    
    // Variables to bind to
    int count = 10;
    float rate = 1.5f;
    bool verbose = false;
    std::string filename;
    
    // Add options with type-safe bindings
    rootCmd.AddOption(OptionType::Variable, "count", 'c')
           .BindTo(count)
           .SetDescription("Number of items");
           
    rootCmd.AddOption(OptionType::Variable, "rate", 'r')
           .BindTo(rate)
           .SetDescription("Processing rate");
           
    rootCmd.AddOption(OptionType::Variable, "verbose", 'v')
           .BindTo(verbose)
           .SetDescription("Enable verbose output");
           
    rootCmd.AddOption(OptionType::Parameter, "filename")
           .BindTo(filename)
           .SetDescription("Input filename");
    
    // Parse arguments - values are automatically converted and assigned
    parser.ParseArgs(argc, argv);
    
    // Variables are ready to use with correct types!
    std::cout << "Count: " << count << "\n";        // int
    std::cout << "Rate: " << rate << "\n";          // float  
    std::cout << "Verbose: " << verbose << "\n";    // bool
    std::cout << "File: " << filename << "\n";      // string
    
    return 0;
}

// Command examples:
// myapp --count 42 --rate 3.14 --verbose true input.txt
// myapp -c 100 -r 0.5 -v yes data.csv
```

### Supported Types

The default converter supports fundamental types:

- **Numeric**: `int`, `unsigned int`, `long`, `unsigned long`, `long long`, `unsigned long long`, `float`, `double`
- **Character**: `char` (uses first character of string)
- **Boolean**: `bool` (accepts "true"/"false", "1"/"0", "yes"/"no", "on"/"off")
- **String**: `std::string` (no conversion needed)

### Custom Converters

For complex types, provide custom conversion functors:

```cpp
// Custom converter for uppercase strings
struct UppercaseConverter
{
    std::string operator()(const std::string& str) const
    {
        std::string result = str;
        std::transform(result.begin(), result.end(), result.begin(), ::toupper);
        return result;
    }
};

// Custom converter for enums
enum class LogLevel { Debug, Info, Warning, Error };

struct LogLevelConverter
{
    LogLevel operator()(const std::string& str) const
    {
        if (str == "debug") return LogLevel::Debug;
        if (str == "info") return LogLevel::Info;
        if (str == "warning") return LogLevel::Warning;
        if (str == "error") return LogLevel::Error;
        throw SyntaxException(SyntaxError::InvalidValue, "Invalid log level", str);
    }
};

// Usage with custom converters
std::string name;
LogLevel logLevel;

rootCmd.AddOption(OptionType::Variable, "name")
       .BindTo(name, UppercaseConverter{});
       
rootCmd.AddOption(OptionType::Variable, "log-level")
       .BindTo(logLevel, LogLevelConverter{});
```

### Error Handling

Type conversion errors automatically throw `SyntaxException`:

```cpp
int number;
rootCmd.AddOption(OptionType::Variable, "number").BindTo(number);

// Command: myapp --number abc
// Result: SyntaxException with message "Invalid value for numeric type (token: abc)"
```

### Method Signature

```cpp
template<typename T, typename Converter = DefaultConverter<T>>
OptionDecl& BindTo(T& variable, Converter converter = Converter{});
```

**Parameters:**
- `T` - Target type (automatically deduced from variable reference)
- `Converter` - Conversion functor (defaults to `DefaultConverter<T>`)

**Returns:** Reference to `OptionDecl` for method chaining

**Constraints:**
- `Variable` and `Parameter` options support binding to any supported type
- `Switch` options may be bound only to `bool` (presence sets the variable to `true`, absence leaves its prior value)

### Binding Switches to Booleans

Switch options can be bound directly to a `bool` using `BindTo()`. Binding semantics are intentionally simple and predictable:

Behavior:
* The variable's value before parsing is its default (e.g., `false` or `true`, as you choose).
* Each time the bound switch appears on the command line, the variable is set to `true`.
* If the switch does not appear, the variable remains unchanged.
* Grouped aliases (e.g., `-abc`) trigger each bound switch's callback, setting all associated booleans to `true`.
* Attempting to bind a switch to a non-`bool` type throws `ApiException`.

Example:
```cpp
bool verbose = false;      // default chosen by the application
bool featureEnabled = true; // pre-enabled feature, user can confirm by passing --feature (idempotent)

rootCmd.AddOption(OptionType::Switch, "verbose", 'v').BindTo(verbose);
rootCmd.AddOption(OptionType::Switch, "feature").BindTo(featureEnabled);

parser.ParseArgs(argc, argv);

// If user passed --verbose or -v -> verbose == true
// If user omitted --feature       -> featureEnabled stays true (default preserved)
```

Grouped aliases:
```cpp
bool a=false, b=false, c=false;
rootCmd.AddOption(OptionType::Switch, "alpha", 'a').BindTo(a);
rootCmd.AddOption(OptionType::Switch, "bravo", 'b').BindTo(b);
rootCmd.AddOption(OptionType::Switch, "charlie", 'c').BindTo(c);

// Command: myapp -abc  => a == b == c == true
```

Tip: If you need tri-state semantics (unset / enabled / disabled) consider using:
* A bound `bool` defaulting to `false` plus a separate `--no-<name>` switch, or
* A `Variable` option bound to an enum `{ Auto, On, Off }`.

---

## Global Options

Global options are available across all command contexts and can be specified at any command block stage. They are perfect for options like `--verbose`, `--config`, or `--help` that should work everywhere in your application.

Only Switch or Variable option types can be global. Positional Parameter option types only exist within a command block stage.

### Declaring Global Options

```cpp
InCommand::CommandParser parser("myapp");

// Add global options
parser.AddGlobalOption(OptionType::Switch, "verbose", 'v')
      .SetDescription("Enable verbose output globally");

parser.AddGlobalOption(OptionType::Variable, "config", 'c')
      .SetDescription("Configuration file path");

// These work from any command context:
// myapp --verbose build --target release
// myapp build --verbose --target release  
// myapp --config myconfig.json test --coverage
```

### Context-Aware Global Option Interpretation

While global options are accessible from anywhere, InCommand preserves the **local context** where each global option was specified. This enables sophisticated custom interpretation based on which command the user was working with when they set the option.

### Global vs Local Options

```cpp
// Global option - available everywhere
parser.AddGlobalOption(OptionType::Switch, "verbose", 'v');

// Local option - only available on specific command
auto& buildCmd = rootCmd.AddSubCommand("build");
buildCmd.AddOption(OptionType::Variable, "target")
        .SetDescription("Build target (Debug/Release)");
```

### Accessing Global Options

```cpp
auto numBlocks = parser.ParseArgs(argc, argv);

// Check global options from anywhere
if (parser.IsGlobalOptionSet("verbose")) {
    std::cout << "Verbose mode enabled\n";
}

// Get global option value (first occurrence if multiple)
if (parser.IsGlobalOptionSet("config")) {
    std::string configFile = parser.GetGlobalOptionValue("config");
    loadConfiguration(configFile);
}

// Multiple occurrences: count and indexed access
size_t cfgCount = parser.GetGlobalOptionValueCount("config");
for (size_t i = 0; i < cfgCount; ++i) {
    const std::string& path = parser.GetGlobalOptionValue("config", i);
    size_t ctx = parser.GetGlobalOptionBlockIndex("config", i); // per-occurrence origin
    // ... use path with its originating context block
}

// Find where the global option was specified
size_t context = parser.GetGlobalOptionBlockIndex("verbose");
std::cout << "Verbose was set in command block " << context << "\n";
```

#### Command Context Identification

```cpp
auto numBlocks = parser.ParseArgs(argc, argv);

if (parser.IsGlobalOptionSet("verbose")) {
    size_t contextIndex = parser.GetGlobalOptionBlockIndex("verbose");
    const auto& contextBlock = parser.GetCommandBlock(contextIndex);
    
    if (contextBlock.GetDecl().GetName() == "build") {
        // Verbose was used with build command - enable build-specific verbose
        std::cout << "Enabling detailed build output\n";
        setBuildVerbosity(VerboseLevel::Detailed);
    } else if (contextBlock.GetDecl().GetName() == "test") {
        // Verbose was used with test command - enable test-specific verbose  
        std::cout << "Enabling detailed test output\n";
        setTestVerbosity(VerboseLevel::Detailed);
    } else {
        // Verbose was used at root level - general verbose
        std::cout << "Enabling general verbose output\n";
        setGlobalVerbosity(VerboseLevel::Standard);
    }
}
```

#### Advanced Context-Based Behavior

```cpp
enum class CommandContext { Root, Build, Test, Deploy };

// Set unique IDs for easy identification
rootCmd.SetUniqueId(CommandContext::Root);
buildCmd.SetUniqueId(CommandContext::Build);
testCmd.SetUniqueId(CommandContext::Test);
deployCmd.SetUniqueId(CommandContext::Deploy);

// Parse and handle context-sensitive global options
auto numBlocks = parser.ParseArgs(argc, argv);

if (parser.IsGlobalOptionSet("dry-run")) {
    size_t contextIndex = parser.GetGlobalOptionBlockIndex("dry-run");
    const auto& contextBlock = parser.GetCommandBlock(contextIndex);
    
    switch (contextBlock.GetDecl().GetUniqueId<CommandContext>()) {
        case CommandContext::Build:
            std::cout << "Dry-run mode: Will show build steps without executing\n";
            configureBuildDryRun();
            break;
            
        case CommandContext::Deploy:
            std::cout << "Dry-run mode: Will show deployment plan without changes\n";
            configureDeploymentDryRun();
            break;
            
        case CommandContext::Root:
            std::cout << "Dry-run mode: General simulation mode enabled\n";
            configureGeneralDryRun();
            break;
            
        default:
            std::cout << "Dry-run mode: Default behavior\n";
            break;
    }
}
```

#### Real-World Example: Configuration Context

Global options like `--config` can have different meanings depending on where they're used:

```cpp
if (parser.IsGlobalOptionSet("config")) {
    std::string configPath = parser.GetGlobalOptionValue("config");
    size_t contextIndex = parser.GetGlobalOptionBlockIndex("config");
    const auto& contextBlock = parser.GetCommandBlock(contextIndex);
    
    if (contextBlock.GetDecl().GetName() == "build") {
        // Load build-specific configuration
        loadBuildConfig(configPath);
        std::cout << "Using build configuration: " << configPath << "\n";
    } else if (contextBlock.GetDecl().GetName() == "test") {
        // Load test-specific configuration
        loadTestConfig(configPath);
        std::cout << "Using test configuration: " << configPath << "\n";
    } else {
        // Load general application configuration
        loadGeneralConfig(configPath);
        std::cout << "Using general configuration: " << configPath << "\n";
    }
}
```

#### Benefits of Context-Aware Global Options

- **Flexible Semantics**: Same option name can have nuanced behavior per command
- **User-Friendly**: Users don't need to remember different option names for different contexts
- **Powerful Customization**: Developers can implement sophisticated, context-sensitive behaviors
- **Backward Compatibility**: Options work as expected in all contexts while enabling advanced features

This approach allows applications to provide both the convenience of global options and the precision of context-specific behavior, giving developers maximum flexibility in interpreting user intent.

---

## Unique Variable Delimiter Support

InCommand supports three delimiter styles for variable assignment:

- Traditional: `--name value` 
- Equals: `--name=value`
- Colon: `--name:value`

This feature makes InCommand particularly suitable for applications that need to match existing CLI tool conventions or provide users with their preferred syntax style.

---

## Auto-Help System

InCommand provides a sophisticated automatic help system that generates context-sensitive help output. The help system integrates global and local options seamlessly and provides detailed usage information.

### Output Stream Configuration

The auto-help system uses C++ streams for flexible output handling. By default, help text is written to `std::cout`, but you can specify any `std::ostream` when enabling auto-help for custom handling such as logging, testing, or specialized formatting.

**Default: Help output goes to std::cout**
```cpp
InCommand::CommandParser parser("myapp");
parser.EnableAutoHelp("help", 'h');
```

**Capture help text for processing**
```cpp
InCommand::CommandParser parser("myapp");
std::ostringstream helpBuffer;
parser.EnableAutoHelp("help", 'h', helpBuffer);
```

**Write help to a file**
```cpp
InCommand::CommandParser parser("myapp");
std::ofstream helpFile("help.txt");
parser.EnableAutoHelp("help", 'h', helpFile);
```

**Send help to stderr instead of stdout**
```cpp
InCommand::CommandParser parser("myapp");
parser.EnableAutoHelp("help", 'h', std::cerr);
```

This flexibility enables scenarios like:

- **Testing**: Capture help output for validation in unit tests
- **Logging**: Direct help requests to application logs
- **File Output**: Save help documentation to files
- **Error Streams**: Send help to stderr for debugging workflows
- **UI integration**: Send help text to a UI element

### Enabling Auto-Help

```cpp
InCommand::CommandParser parser("myapp");

// Enable auto-help with default settings (--help, -h)
parser.EnableAutoHelp("help", 'h');

// Customize the help description
parser.SetAutoHelpDescription("Display comprehensive usage information and examples");
```

### Context-Sensitive Help

The auto-help system automatically shows the appropriate help based on where the help option appears:

**Main application help**
```shell
$ myapp --help

My application

Usage: myapp [--help]
  --help, -h                  Display comprehensive usage information and examples
  build                       Build the project
  test                        Run tests
```

**Subcommand help**
```shell
$ myapp build --help

Build the project

Usage: myapp build [--help] [--verbose] [--target <value>] [<project>]
  --help, -h                  Display comprehensive usage information and examples
  --verbose, -v               Enable verbose output
  --target                    Build target
  <project>                   Project file
```

**Nested subcommand help**
```shell
$ git remote add --help

Add a remote repository

Usage: git remote add [--help] <name> <url>
  --help, -h                  Show help information
  <name>                      Remote name
  <url>                       Remote URL
```

---

## Help Generation and Error Reporting

#### Complete Help with Description, Usage, and Options

The recommended way to generate help output is to use the `GetHelpString()` method on `CommandParser`:

```cpp
std::cout << parser.GetHelpString();
```

This prints the command description (if set), usage string, and option details in a single, formatted output. For subcommands, use the corresponding descriptor:

```cpp
std::cout << parser.GetHelpString(commandBlockIndex);
```

**Sample Output:**

```shell
Sample application demonstrating InCommand
Usage: sample [--help]
  --help, -h                  Show help information
  add                         Adds two numbers together
  mul                         Multiplies two numbers together
```

For subcommands:

```shell
Multiplies two numbers together
Usage: sample mul [--help] <x> <y>
  --help, -h                  Show help information
  <x>                         First number
  <y>                         Second number
```

#### Context-Sensitive Help Generation

```shell
Build the project

Usage:
mybuildtool [options] build [--verbose] [--help] [--target <value>] <project>
  --verbose, -v               Enable verbose output globally
  --help, -h                  Show detailed help information and usage examples
  --target                    Build target
  <project>                   Project file
```

### Error Reporting for End Users

InCommand provides detailed, actionable error messages when users make command-line mistakes:

#### Unknown Option Errors

```shell
$ mybuildtool --invalid-option
Error: Unknown option: --invalid-option
```

#### Missing Required Values

```shell
$ mybuildtool build --target
Error: Option '--target' requires a value
```

#### Invalid Command Structure 

```shell
$ mybuildtool build invalid-subcommand
Error: Unknown parameter or subcommand: invalid-subcommand
```

#### Helpful Token Identification

```cpp
try
{
    parser.ParseArgs(argc, argv);
} catch (const SyntaxException& e)
{
    std::cout << "Error: " << e.GetMessage() << std::endl;
    if (!e.GetToken().empty())
    {
        std::cout << "Problem with token: '" << e.GetToken() << "'" << std::endl;
    }
}
```

**User sees:**

```shell
$ mybuildtool --target=value  # Not supported yet
Error: Unexpected character in option
Problem with token: '--target=value'
```

### Context-Aware Help

Help output adapts to the current command context:

```shell
# Root level help
$ mybuildtool --help
Usage: mybuildtool [--help]

# Subcommand help  
$ mybuildtool build --help
Usage: mybuildtool build [--verbose] [--target <value>] [<project>]

Options:
  --verbose, -v               Enable verbose output
  --target                    Build target
  <project>                   Project file (required)
```

---

## Error Handling
Two distinct exception types:
* `ApiException` (developer misuse): duplicate names, invalid alias on parameter, missing unique ID access, etc.
* `SyntaxException` (user input): unknown option, missing value, invalid value, too many parameters, unexpected argument.

Example user error handling:
```cpp
try { parser.ParseArgs(argc, argv); }
catch (const SyntaxException& e) {
    std::cout << "Error: " << e.GetMessage() << "\n";
    if (!e.GetToken().empty()) std::cout << "Problem with token: '" << e.GetToken() << "'\n";
}
```
Sample messages:
```
Error: Unknown option
Error: Option '--target' requires a value
Error: Unexpected argument
```

### Detailed User Error Examples
Unknown option:
```shell
$ mybuildtool --invalid-option
Error: Unknown option
```
Missing variable value:
```shell
$ mybuildtool build --target
Error: Option '--target' requires a value
```
Invalid command structure / unexpected argument:
```shell
$ mybuildtool build invalid-subcommand
Error: Unexpected argument
```
Token identification:
```cpp
try { parser.ParseArgs(argc, argv); }
catch (const SyntaxException& e) {
    std::cout << "Error: " << e.GetMessage() << "\n";
    if (!e.GetToken().empty()) std::cout << "Problem with token: '" << e.GetToken() << "'\n";
}
```
Example output:
```
$ mybuildtool --target=value  # if delimiter mode not enabled
Error: Unexpected argument
Problem with token: '--target=value'
```

---

## Core API

This section provides comprehensive documentation for all public classes and methods in InCommand.

### Class Overview

- **CommandParser** - Main entry point, owns the command structure
- **CommandDecl** - Tree node describing command blocks in hierarchical structure
- **CommandBlock** - Parsed command result within a series of ordered blocks
- **OptionDecl** - Describes individual options, supports method chaining
### CommandParser

The main entry point for InCommand. Owns and manages the command structure hierarchy.

#### Constructors

```cpp
CommandParser(const std::string& appName, VariableDelimiter variableDelimiter = VariableDelimiter::Whitespace)
```

Creates a new command parser with support for a specific variable assignment delimiter format. Each parser instance supports one delimiter type. The default delimiter type is `VariableDelimiter::Whitespace`.

**Examples:**
- `CommandParser("myapp")` - Traditional whitespace-separated format only (`--name value`)
- `CommandParser("myapp", VariableDelimiter::Whitespace)` - Explicit whitespace-separated format (`--name value`)
- `CommandParser("myapp", VariableDelimiter::Equals)` - Enables `--name=value` and `-n=value` formats
- `CommandParser("myapp", VariableDelimiter::Colon)` - Enables `--name:value` and `-n:value` formats

#### `CommandParser::GetAppCommandDecl()`

```cpp
CommandDecl& GetAppCommandDecl()
const CommandDecl& GetAppCommandDecl() const
```

Returns a reference to the root command block descriptor. Use this to configure your application's commands and options.

#### `CommandParser::AddGlobalOption()`

```cpp
OptionDecl& AddGlobalOption(OptionType type, const std::string& name, char alias = 0)
```

Adds a global option that is available to all command blocks in the hierarchy. Global options can be specified at any level of the command chain and are accessible from any command block after parsing.

**Parameters:**
- `type`: The option type (Switch, Variable, or Parameter)
- `name`: The option name
- `alias`: Optional single-character alias (0 for no alias)

**Returns:** Reference to the created OptionDecl for method chaining

**Example:**
```cpp
parser.AddGlobalOption(OptionType::Switch, "verbose", 'v')
      .SetDescription("Enable verbose output");
// Usage: myapp --verbose build  OR  myapp build --verbose
```

#### `CommandParser::ParseArgs()`

```cpp
size_t ParseArgs(int argc, const char* argv[])
```

Parses command-line arguments and returns the number of command blocks that were parsed. Throws `SyntaxException` if parsing fails. Use `GetCommandBlock(index)` to access specific command blocks, where index ranges from 0 to the returned count minus 1.

#### `CommandParser::GetNumCommandBlocks()`

```cpp
size_t GetNumCommandBlocks() const
```

Returns the number of command blocks parsed from `ParseArgs()`.

#### `CommandParser::GetCommandBlock()`

```cpp
const CommandBlock &GetCommandBlock(size_t index) const
```

Returns the command block at the requested `index` using zero-based indexing. Throws `ApiException` if no command blocks have been parsed or if index exceeds the last block parsed.

#### Global Option Access Methods

```cpp
bool IsGlobalOptionSet(const std::string& name) const
```

Returns `true` if the named global option was set anywhere in the command chain during parsing.

```cpp
const std::string& GetGlobalOptionValue(const std::string& name) const
```

Returns the value of a global variable or parameter. If the option supports multiple occurances, then returns the first value of a global variable option when set multiple times. Throws `ApiException` if the option doesn't exist or has no value.

```cpp
const std::string& GetGlobalOptionValue(const std::string& name, size_t index) const
```

Returns the value of the `index`-th occurrence (0-based) for the named global variable option.

```cpp
size_t GetGlobalOptionValueCount(const std::string& name) const
```

Returns how many times the named global option occurred.

```cpp
size_t GetGlobalOptionBlockIndex(const std::string& name) const
```

Returns the command block index where the global option was set most recently (last occurrence). Throws `ApiException` if the global option was not set.

```cpp
size_t GetGlobalOptionBlockIndex(const std::string& name, size_t index) const
```

Returns the command block index associated with occurrence at `index` of the named global option.

#### Auto-Help Configuration

#### `CommandParser::EnableAutoHelp()`

```cpp
void EnableAutoHelp(const std::string& optionName, char alias, std::ostream& outputStream = std::cout)
```

Enables automatic help functionality with the specified option name and alias. When users specify this option, context-sensitive help is automatically generated and output to the specified stream. Throws `ApiException` if the option name or alias conflicts with existing options.

#### `CommandParser::SetAutoHelpDescription()`

```cpp
void SetAutoHelpDescription(const std::string& description)
```

Sets a custom description for the auto-help option. Default is "Show context-sensitive help information".

#### `CommandParser::DisableAutoHelp()`

Disables automatic help functionality.

#### `CommandParser::WasAutoHelpRequested()`

```cpp
bool WasAutoHelpRequested() const
```

Returns `true` if the auto-help option was used during the last parse operation. Use this to determine if your application should exit after parsing.

#### Help String Generation

#### `CommandParser::GetHelpString()`

```cpp
std::string GetHelpString() const
```

Generates complete help text for the rightmost (most specific) parsed command block. Automatically integrates global options with local options and shows context-sensitive help. Requires that `ParseArgs()` has been called first.

#### `CommandParser::GetHelpString(size_t commandBlockIndex)`

```cpp
std::string GetHelpString(size_t commandBlockIndex) const
```

Generates complete help text for a specific command block by index (0 = root command). Automatically integrates global options with local options. Requires that `ParseArgs()` has been called first.

### CommandDecl

Describes a command or subcommand with its available options. Represents the tree structure of your command hierarchy.

#### `CommandDecl::AddOption()` - Basic Form

```cpp
OptionDecl& AddOption(OptionType type, const std::string& name)
```

Adds an option without an alias. Returns a reference to the newly created OptionDecl.
- **type**: `OptionType::Switch`, `OptionType::Variable`, or `OptionType::Parameter`
- **name**: The option name (e.g., "verbose", "output", "input-file")

#### `CommandDecl::AddOption()` - With Alias

```cpp
OptionDecl& AddOption(OptionType type, const std::string& name, char alias)
```

Adds an option with a single-character alias. **Parameters cannot have aliases** and will throw `ApiException`.
- **type**: `OptionType::Switch` or `OptionType::Variable` (NOT `Parameter`)
- **name**: The option name
- **alias**: Single character alias (e.g., 'v', 'o', 'h')

#### `CommandDecl::AddSubCommand()`

```cpp
CommandDecl& AddSubCommand(const std::string& name)
```

Adds a new subcommand. Returns a reference to the new command block descriptor for configuration.

#### `CommandDecl::SetDescription()`

```cpp
CommandDecl& SetDescription(const std::string& description)
```

Sets the description for this command. Used in help generation. Returns `*this` for method chaining.

#### `CommandDecl::SetUniqueId()`

```cpp
template<typename T>
void SetUniqueId(T id)
```

Assigns a user-defined ID to this command block descriptor. The ID can be any type (typically an enum) and is useful for switch-based command handling in your application logic.

#### `CommandDecl::GetUniqueId()`

```cpp
template<typename T>  
T GetUniqueId() const
```

Retrieves the previously assigned unique ID. Throws `ApiException` if no ID has been set or the type doesn't match the stored ID type.

#### `CommandDecl::HasUniqueId()`

```cpp
bool HasUniqueId() const
```

Returns `true` if a unique ID has been assigned to this command block descriptor, `false` otherwise.

#### `CommandDecl::GetName()`

```cpp
const std::string& GetName() const
```

Returns the name of this command block (the command word that users type).

#### `CommandDecl::GetDescription()`

```cpp
const std::string& GetDescription() const
```

Returns the description text for this command block. Empty string if no description has been set.

---

### CommandBlock

Represents the parsed command-line arguments for a specific command in the chain. Forms a linked list from the root command to the most specific subcommand.

#### `CommandBlock::IsOptionSet()`

```cpp
bool IsOptionSet(const std::string& name) const
```

Returns `true` if the named switch was present on the command line, or if the named variable/parameter has a value.

#### `CommandBlock::GetOptionValue()` - Basic Form

```cpp
const std::string& GetOptionValue(const std::string& name) const
```

Returns the value of a variable or parameter. Returns the value of the first occurance when multiple values are present. Throws `ApiException` if the option doesn't exist or has no value.

#### `CommandBlock::GetOptionValue()` - With Default

```cpp
const std::string& GetOptionValue(const std::string& name, const std::string& defaultValue) const
```

Returns the value of a variable or parameter, or the provided default value if the option wasn't set.

Returns the value of a parameter, or the provided default value if the parameter wasn't set.

#### `CommandBlock::GetOptionValue()` - Indexed

```cpp
const std::string& GetOptionValue(const std::string& name, size_t index) const
```

Returns the value at the specified `index` when the option allows multiple occurrences.

#### `CommandBlock::GetOptionValueCount()`

```cpp
size_t GetOptionValueCount(const std::string& name) const
```

Returns the number of occurrences for the named option.

#### `CommandBlock::GetDecl()`

```cpp
const CommandDecl& GetDecl() const
```

Returns a reference to the command block descriptor that was used to create this parsed command block.

---

### OptionDecl

Describes an individual option (switch, variable, or parameter). Returned by `AddOption()` methods.

#### `OptionDecl::SetDescription()`

```cpp
OptionDecl& SetDescription(const std::string& description)
```

Sets the description for this option. Used in help generation. Returns `*this` for method chaining.

#### `OptionDecl::SetDomain()`

```cpp
OptionDecl& SetDomain(const std::vector<std::string>& domain)
```

Restricts the option to a specific set of valid values. Useful for variables with limited choices.

#### `OptionDecl::GetType()`

```cpp
OptionType GetType() const
```

Returns the type of this option (`OptionType::Switch`, `OptionType::Variable`, or `OptionType::Parameter`).

#### `OptionDecl::GetName()`

```cpp
const std::string& GetName() const
```

Returns the name of this option as specified when it was created.

#### `OptionDecl::GetDescription()`

```cpp
const std::string& GetDescription() const
```

Returns the description text for this option. Empty string if no description has been set.

#### `OptionDecl::GetAlias()`

```cpp
char GetAlias() const
```

Returns the single-character alias for this option. Returns 0 if no alias has been set.

#### `OptionDecl::GetDomain()`

```cpp
const std::vector<std::string>& GetDomain() const
```

Returns the list of valid values for this option. Empty vector if no domain restrictions have been set.

#### `OptionDecl::BindTo()`

```cpp
template<typename T, typename Converter = DefaultConverter<T>>
OptionDecl& BindTo(T& variable, Converter converter = Converter{})
```

Binds the option's parsed value to a typed variable with automatic type conversion. When the command line is parsed, the string value is converted to the target type and assigned to the variable. Returns `*this` for method chaining.

**Parameters:**
- `T` - Target type (automatically deduced from variable reference)
- `Converter` - Conversion functor (defaults to `DefaultConverter<T>`)

**Conversion Support:**
- **Fundamental types**: `int`, `float`, `double`, `char`, `bool`, etc.
- **String**: `std::string` (no conversion)
- **Custom types**: Provide custom converter functor

**Error Handling:**
- Conversion failures throw `SyntaxException` with `SyntaxError::InvalidValue`
- Type mismatch or unsupported types detected at compile time

---

### Option Types

#### Switch Options

Boolean flags that are either present (true) or absent (false).
```cpp
rootCmdDesc.AddOption(OptionType::Switch, "verbose", 'v');
// Usage: myapp --verbose  or  myapp -v  or  myapp -vq (grouped)
```

#### Variable Options

Options that require a value argument.
```cpp
rootCmdDesc.AddOption(OptionType::Variable, "output", 'o');
// Usage: myapp --output file.txt  or  myapp -o file.txt
```

#### Parameter Options

Positional arguments without option prefixes. Order matters and they are consumed in declaration order.
```cpp
rootCmdDesc.AddOption(OptionType::Parameter, "input-file");
// Usage: myapp input.txt
// NOTE: Parameters cannot have aliases
```

#### Alias Rules

- Only **switches** and **variables** can have aliases
- Only **switches** can be grouped (`-vq` = `-v -q`)
- **Parameters** cannot have aliases (will throw `ApiException`)
- **Variables** cannot be part of grouped aliases

---

### Enums and Types

#### OptionType

Specifies the type of option when calling `AddOption()`.

```cpp
enum class OptionType
{
    Switch,     // Boolean flag (--verbose, -v)
    Variable,   // Option with value (--output file.txt, -o file.txt)  
    Parameter   // Positional argument (input.txt)
}
```

#### VariableDelimiter

Specifies the delimiter format for variable assignment when creating a `CommandParser`.

```cpp
enum class VariableDelimiter
{
    Whitespace, // Traditional space-separated format: --name value, -n value
    Equals,     // Packed equals format: --name=value, -n=value
    Colon       // Packed colon format: --name:value, -n:value
}
```

**Usage Examples:**
- `CommandParser("myapp")` - Defaults to `Whitespace`
- `CommandParser("myapp", VariableDelimiter::Whitespace)` - Explicit whitespace format
- `CommandParser("myapp", VariableDelimiter::Equals)` - Enable `--name=value` syntax  
- `CommandParser("myapp", VariableDelimiter::Colon)` - Enable `--name:value` syntax

All delimiter formats support both traditional space-separated and the specified packed syntax.

#### ApiError

Error codes for `ApiException`. Indicates developer API misuse.

```cpp
enum class ApiError
{
    None,                   // No error
    OutOfMemory,            // Memory allocation failed
    DuplicateCommandBlock,  // Command name already exists
    DuplicateOption,        // Option name or alias already exists
    UniqueIdNotAssigned,    // Trying to get unassigned unique ID
    OptionNotFound,         // Referenced option doesn't exist
    ParameterNotFound,      // Referenced parameter doesn't exist
    InvalidOptionType,      // Invalid operation for option type (e.g., alias on parameter)
    InvalidUniqueIdType,    // Unique ID type is invalid
    OutOfRange,             // An out-of-range index was used
}
```

#### SyntaxError

Error codes for `SyntaxException`. Indicates user command-line syntax errors.

```cpp
enum class SyntaxError
{
    None,                 // No error
    UnknownOption,        // Unrecognized option flag
    MissingVariableValue, // Variable option missing required value
    UnexpectedArgument,   // Extra or invalid argument
    TooManyParameters,    // More parameters than expected
    InvalidValue,         // Value outside defined domain
    OptionNotSet          // Accessing unset option value
}
```

---

### Exception Types

InCommand provides two distinct exception types for different error scenarios:

#### ApiException

Thrown when developers misuse the API during command setup. These are programming errors that should be caught during development.

##### `ApiException::GetMessage()`

```cpp
const std::string& GetMessage() const
```

Returns a human-readable description of the API misuse error.

##### `ApiException::GetError()`

```cpp
ApiError GetError() const
```

Returns the specific `ApiError` enum value indicating the type of API error that occurred.

#### Common Scenarios

- **Duplicate Options**: Declaring the same option name twice
- **Duplicate Aliases**: Using the same alias character twice  
- **Invalid Option Types**: Trying to add aliases to parameters
- **Option Not Found**: Referencing non-existent options
- **Out of Memory**: Memory allocation failures
- **Unique ID Issues**: Accessing unassigned unique IDs

```cpp
try
{
    auto& rootCmdDesc = parser.GetAppCommandDecl();
    rootCmdDesc.AddOption(OptionType::Switch, "help");
    rootCmdDesc.AddOption(OptionType::Switch, "help"); // Duplicate!
    
    // This also throws ApiException:
    rootCmdDesc.AddOption(OptionType::Parameter, "file", 'f'); // Parameters can't have aliases!
}
catch (const ApiException& e)
{
    std::cout << "API Error: " << e.GetMessage() << std::endl;
    // Handle programming error - fix your code!
}
```

#### SyntaxException

Thrown when end-users provide invalid command-line syntax. These represent user input errors, not programming mistakes.

##### `SyntaxException::GetMessage()`

```cpp
const std::string& GetMessage() const
```

Returns a human-readable description of the syntax error.

##### `SyntaxException::GetToken()`

```cpp
const std::string& GetToken() const
```

Returns the specific command-line token that caused the error, if available. May be empty for some error types.

##### `SyntaxException::GetError()`

```cpp
SyntaxError GetError() const
```

Returns the specific `SyntaxError` enum value indicating the type of syntax error that occurred.

#### Common Scenarios

- **Unknown Options**: User provides unrecognized flags
- **Missing Values**: Variables require values but none provided
- **Invalid Values**: Values outside defined domains
- **Unexpected Arguments**: Extra parameters or unknown subcommands
- **Parsing Errors**: Malformed option syntax

```cpp
try
{
    parser.ParseArgs(argc, argv);
} catch (const SyntaxException& e)
{
    std::cout << "Command Line Error: " << e.GetMessage() << std::endl;
    if (!e.GetToken().empty())
    {
        std::cout << "Problem with: " << e.GetToken() << std::endl;
    }
}
```

---

## Unique Variable Delimiter Support

InCommand supports three delimiter styles for variable assignment:

- Traditional: `--name value` 
- Equals: `--name=value`
- Colon: `--name:value`

This feature makes InCommand particularly suitable for applications that need to match existing CLI tool conventions or provide users with their preferred syntax style.

---

## Examples

### Git-like Command Structure

```cpp
CommandParser parser("git");
auto& gitCmdDesc = parser.GetAppCommandDecl();

// git remote
auto& remoteCmdDesc = gitCmdDesc.AddSubCommand("remote");
remoteCmdDesc.SetDescription("Manage remote repositories");

// git remote add
auto& addCmdDesc = remoteCmdDesc.AddSubCommand("add");
addCmdDesc.SetDescription("Add a remote repository")
           .AddOption(OptionType::Parameter, "name")
               .SetDescription("Remote name");
addCmdDesc.AddOption(OptionType::Parameter, "url")
    .SetDescription("Remote URL");

// Usage: git remote add origin https://github.com/user/repo.git
```

### Command Identification with Unique IDs

Unique, user-defined type IDs can be assigned to Command Block, allowing developers to use `switch` blocks to trivially walk the command chain.

```cpp
enum class MyCommandId { Root, Build, Test, Deploy };

CommandParser parser("myapp");
auto& rootCmdDesc = parser.GetAppCommandDecl();
rootCmdDesc.SetUniqueId(MyCommandId::Root);

auto& buildCmdDesc = rootCmdDesc.AddSubCommand("build");
buildCmdDesc.SetUniqueId(MyCommandId::Build);

auto& testCmdDesc = rootCmdDesc.AddSubCommand("test");
testCmdDesc.SetUniqueId(MyCommandId::Test);

auto& deployCmdDesc = rootCmdDesc.AddSubCommand("deploy");
deployCmdDesc.SetUniqueId(MyCommandId::Deploy);

auto numBlocks = parser.ParseArgs(argc, argv);
switch (parser.GetCommandBlock(numBlocks - 1).GetDecl().GetUniqueId<MyCommandId>())
{
    case MyCommandId::Root: /* handle root command */ break;
    case MyCommandId::Test:  /* handle test */  break;
    case MyCommandId::Build: /* handle build */ break;
    case MyCommandId::Deploy:  /* handle deploy */  break;
}
```

---

## Building

InCommand uses CMake for building:

```shell
mkdir build
cd build
cmake ..
cmake --build . --config Debug
ctest -C Debug  # Run tests
```

---

## Requirements

- **C++17** or later
- **CMake 3.14** or later  

---

## License

InCommand is distributed under the MIT License. See the full text in the `LICENSE` file.

Copyright (c) 2025 wjkristiansen@hotmail.com

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice (including this paragraph)
shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

### Third-Party Components
This project uses GoogleTest, which is licensed under the BSD-3-Clause license.
That license applies to the GoogleTest sources and is included with its distribution.

---

## Notes / Future Enhancements (Optional)
Potential additions:
* FAQ (design rationale for single delimiter strategy)
* Extended example repository links
* Generated man-page output helper

---

## Type-Safe Value Binding
Automatically convert and assign option values using `BindTo()`.

Basic example:
```cpp
int count = 0; float rate = 0.0f; bool verbose = false; std::string file;
auto& root = parser.GetAppCommandDecl();
root.AddOption(OptionType::Variable, "count", 'c').BindTo(count);
root.AddOption(OptionType::Variable, "rate", 'r').BindTo(rate);
root.AddOption(OptionType::Variable, "verbose", 'v').BindTo(verbose);
root.AddOption(OptionType::Parameter, "filename").BindTo(file);
parser.ParseArgs(argc, argv);
```

Supported (default converter): integral, floating, bool (true/false/1/0/yes/no/on/off), char (first char), std::string.

Custom converter:
```cpp
enum class LogLevel { Debug, Info, Warning, Error };
struct LogLevelConv {
    LogLevel operator()(const std::string& s) const {
        if (s=="debug") return LogLevel::Debug;
        if (s=="info") return LogLevel::Info;
        if (s=="warning") return LogLevel::Warning;
        if (s=="error") return LogLevel::Error;
        throw SyntaxException(SyntaxError::InvalidValue, "Invalid log level", s);
    }
};
LogLevel level;
root.AddOption(OptionType::Variable, "log-level").BindTo(level, LogLevelConv{});
```

Errors during conversion raise `SyntaxException`.

### Supported Types (Default Converter)
* Integral types: `int`, `unsigned int`, `long`, `unsigned long`, `long long`, `unsigned long long`
* Floating types: `float`, `double`
* Boolean: accepts `true/false`, `1/0`, `yes/no`, `on/off`
* Character: `char` (first character of provided value)
* Strings: `std::string`

### Custom Converters (Additional Examples)
Uppercasing converter:
```cpp
struct UppercaseConverter {
    std::string operator()(const std::string& s) const {
        std::string r = s; std::transform(r.begin(), r.end(), r.begin(), ::toupper); return r;
    }
};
std::string name;
root.AddOption(OptionType::Variable, "name").BindTo(name, UppercaseConverter{});
```

Enum converter (already shown above for `LogLevel`) demonstrates throwing `SyntaxException` with `SyntaxError::InvalidValue` for invalid input.

### Conversion Error Example
```cpp
int number;
root.AddOption(OptionType::Variable, "number").BindTo(number);
// Command: myapp --number=abc (or "--number abc")
// Throws: SyntaxException (InvalidValue) token "abc"
```

---

## Global Options & Context
Add once via `AddGlobalOption` and visible in all blocks.
```cpp
parser.AddGlobalOption(OptionType::Switch, "verbose", 'v');
parser.AddGlobalOption(OptionType::Variable, "config", 'c');
```
Context-aware usage:
```cpp
if (parser.IsGlobalOptionSet("verbose")) {
    size_t idx = parser.GetGlobalOptionBlockIndex("verbose");
    const auto& block = parser.GetCommandBlock(idx);
    // differentiate build vs test vs root
}
```
Advanced pattern using unique IDs:
```cpp
enum class ContextId { Root, Build, Test, Deploy };
root.SetUniqueId(ContextId::Root);
auto& build = root.AddSubCommand("build"); build.SetUniqueId(ContextId::Build);
// ... after parse
if (parser.IsGlobalOptionSet("dry-run")) {
    size_t i = parser.GetGlobalOptionBlockIndex("dry-run");
    switch (parser.GetCommandBlock(i).GetDecl().GetUniqueId<ContextId>()) {
        case ContextId::Build: /* build dry-run */ break;
        case ContextId::Deploy: /* deploy dry-run */ break;
        default: break;
    }
}
```

Benefits:
* Unified semantics (`--verbose` everywhere)
* Origin-aware behavior (context-specific meanings)
* No option duplication between subcommands

### Advanced Context-Based Behavior
Verbose interpretation per command:
```cpp
if (parser.IsGlobalOptionSet("verbose")) {
    size_t ctxIdx = parser.GetGlobalOptionBlockIndex("verbose");
    const auto& ctxBlock = parser.GetCommandBlock(ctxIdx);
    if (ctxBlock.GetDecl().GetName() == "build") {
        // build-specific verbose level
    } else if (ctxBlock.GetDecl().GetName() == "test") {
        // test-specific verbose level
    } else {
        // root-level generic verbose
    }
}
```

Using unique IDs for richer dispatch (extended dry-run example):
```cpp
enum class CommandContext { Root, Build, Test, Deploy };
root.SetUniqueId(CommandContext::Root);
auto& build = root.AddSubCommand("build").SetUniqueId(CommandContext::Build);
auto& test  = root.AddSubCommand("test").SetUniqueId(CommandContext::Test);
auto& deploy= root.AddSubCommand("deploy").SetUniqueId(CommandContext::Deploy);
parser.AddGlobalOption(OptionType::Switch, "dry-run");
auto numBlocks = parser.ParseArgs(argc, argv);
if (parser.IsGlobalOptionSet("dry-run")) {
    size_t origin = parser.GetGlobalOptionBlockIndex("dry-run");
    switch (parser.GetCommandBlock(origin).GetDecl().GetUniqueId<CommandContext>()) {
        case CommandContext::Build: /* configure build dry-run */ break;
        case CommandContext::Deploy: /* configure deploy dry-run */ break;
        case CommandContext::Root:   /* general simulation */ break;
        default: break;
    }
}
```

Configuration path semantics vary by context:
```cpp
if (parser.IsGlobalOptionSet("config")) {
    std::string path = parser.GetGlobalOptionValue("config");
    size_t idx = parser.GetGlobalOptionBlockIndex("config");
    const auto& block = parser.GetCommandBlock(idx);
    if (block.GetDecl().GetName() == "build") { /* load build config */ }
    else if (block.GetDecl().GetName() == "test") { /* load test config */ }
    else { /* load general config */ }
}
```

---

## Auto-Help & Help Generation
Enable once:
```cpp
parser.EnableAutoHelp("help", 'h');
parser.SetAutoHelpDescription("Display comprehensive usage information and examples");
```
Output stream control:
```cpp
std::ostringstream buf; parser.EnableAutoHelp("help", 'h', buf);
```
Example outputs:
```
myapp --help
Usage: myapp [--help]
  --help, -h   Display comprehensive usage information and examples
  build        Build the project
  test         Run tests
```
Subcommand:
```
myapp build --help
Usage: myapp build [--help] [--verbose] [--target <value>] [<project>]
```
Programmatic help generation (after parse):
```cpp
std::string help = parser.GetHelpString();            // active block
std::string rootHelp = parser.GetHelpString(0);       // root block
```

### Output Stream Configuration
Help output can be redirected to any `std::ostream`:
```cpp
std::ostringstream buffer; parser.EnableAutoHelp("help", 'h', buffer); // capture
std::ofstream file("help.txt"); parser.EnableAutoHelp("help", 'h', file); // file
parser.EnableAutoHelp("help", 'h', std::cerr); // stderr
```
Use cases:
* Unit testing (assert on buffer contents)
* Logging systems (pipe to log stream)
* Persisting generated help docs

### Context-Sensitive Examples
```
myapp --help
Usage: myapp [--help]
```
```
myapp build --help
Usage: myapp build [--help] [--verbose] [--target <value>] [<project>]
```
```
git remote add --help
Usage: git remote add [--help] <name> <url>
```

---

## Error Handling
Two distinct exception types:
* `ApiException` (developer misuse): duplicate names, invalid alias on parameter, missing unique ID access, etc.
* `SyntaxException` (user input): unknown option, missing value, invalid value, too many parameters, unexpected argument.

Example user error handling:
```cpp
try { parser.ParseArgs(argc, argv); }
catch (const SyntaxException& e) {
    std::cout << "Error: " << e.GetMessage() << "\n";
    if (!e.GetToken().empty()) std::cout << "Problem with token: '" << e.GetToken() << "'\n";
}
```
Sample messages:
```
Error: Unknown option
Error: Option '--target' requires a value
Error: Unexpected argument
```

### Detailed User Error Examples
Unknown option:
```shell
$ mybuildtool --invalid-option
Error: Unknown option
```
Missing variable value:
```shell
$ mybuildtool build --target
Error: Option '--target' requires a value
```
Invalid command structure / unexpected argument:
```shell
$ mybuildtool build invalid-subcommand
Error: Unexpected argument
```
Token identification:
```cpp
try { parser.ParseArgs(argc, argv); }
catch (const SyntaxException& e) {
    std::cout << "Error: " << e.GetMessage() << "\n";
    if (!e.GetToken().empty()) std::cout << "Problem with token: '" << e.GetToken() << "'\n";
}
```
Example output:
```
$ mybuildtool --target=value  # if delimiter mode not enabled
Error: Unexpected argument
Problem with token: '--target=value'
```

---

## Advanced Topics
### Variable Delimiters
Choose once per parser:
```cpp
CommandParser p1("app");                           // Whitespace
CommandParser p2("app", VariableDelimiter::Equals); // --name=value
CommandParser p3("app", VariableDelimiter::Colon);  // --name:value
```
Whitespace form always remains valid.
Additional notes:
* A single delimiter policy per parser promotes consistent UX
* Packed form works for both long (`--name=value`) and short (`-n=value`) variants
* Traditional whitespace form always accepted for backward compatibility
* Switches never accept values (e.g., `--verbose=true` triggers syntax error)
* Mixed usage allowed: `--output=file -v --mode release`

### Domains (Value Sets)
```cpp
root.AddOption(OptionType::Variable, "mode")
    .SetDomain({"debug", "release", "profile"});
```
Invalid value triggers `SyntaxException(SyntaxError::InvalidValue)`.

### Unique IDs
Attach enums to descriptors for switch-based dispatch:
```cpp
enum class CmdId { Root, Build, Test };
root.SetUniqueId(CmdId::Root);
auto& build = root.AddSubCommand("build"); build.SetUniqueId(CmdId::Build);
// after parse
switch (parser.GetCommandBlock(count-1).GetDecl().GetUniqueId<CmdId>()) {
    case CmdId::Build: /* ... */ break;
    default: break;
}
```

### Custom Converters
Provide functor with `T operator()(const std::string&)` and throw `SyntaxException` for invalid input.

---

## Examples
### Git-like Structure
```cpp
CommandParser parser("git");
auto& git = parser.GetAppCommandDecl();
auto& remote = git.AddSubCommand("remote").SetDescription("Manage remote repositories");
auto& add = remote.AddSubCommand("add").SetDescription("Add a remote repository");
add.AddOption(OptionType::Parameter, "name").SetDescription("Remote name");
add.AddOption(OptionType::Parameter, "url").SetDescription("Remote URL");
```

### Command Identification (Unique IDs)
```cpp
enum class MyCommandId { Root, Build, Test, Deploy };
CommandParser parser("myapp");
auto& root = parser.GetAppCommandDecl(); root.SetUniqueId(MyCommandId::Root);
auto& build = root.AddSubCommand("build"); build.SetUniqueId(MyCommandId::Build);
auto& test  = root.AddSubCommand("test");  test.SetUniqueId(MyCommandId::Test);
auto& deploy= root.AddSubCommand("deploy");deploy.SetUniqueId(MyCommandId::Deploy);
auto numBlocks = parser.ParseArgs(argc, argv);
const auto& active = parser.GetCommandBlock(numBlocks - 1);
switch (active.GetDecl().GetUniqueId<MyCommandId>()) {
    case MyCommandId::Build: /* build */ break;
    case MyCommandId::Test:  /* test  */ break;
    default: break;
}
```

---

## Complete API Reference
The following consolidates public interfaces (duplicates removed for brevity). See earlier sections for usage patterns.

### CommandParser
```cpp
CommandParser(const std::string& appName, VariableDelimiter delim = VariableDelimiter::Whitespace);
CommandDecl& GetAppCommandDecl();
const CommandDecl& GetAppCommandDecl() const;
OptionDecl& AddGlobalOption(OptionType type, const std::string& name, char alias = 0);
size_t ParseArgs(int argc, const char* argv[]);
size_t GetNumCommandBlocks() const;
const CommandBlock& GetCommandBlock(size_t index) const;
bool IsGlobalOptionSet(const std::string& name) const;
const std::string& GetGlobalOptionValue(const std::string& name) const;                 // first occurrence; throws if unset/no value
const std::string& GetGlobalOptionValue(const std::string& name, size_t index) const;  // indexed occurrence
size_t GetGlobalOptionValueCount(const std::string& name) const;                       // number of occurrences
size_t GetGlobalOptionBlockIndex(const std::string& name) const;                       // block index of last occurrence
size_t GetGlobalOptionBlockIndex(const std::string& name, size_t index) const;         // block index of indexed occurrence
void EnableAutoHelp(const std::string& optionName, char alias, std::ostream& out = std::cout);
void SetAutoHelpDescription(const std::string& description);
void DisableAutoHelp();
bool WasAutoHelpRequested() const;
std::string GetHelpString() const;                 // active (rightmost) block
std::string GetHelpString(size_t commandBlockIndex) const; // explicit index
```

### CommandDecl
```cpp
OptionDecl& AddOption(OptionType type, const std::string& name);
OptionDecl& AddOption(OptionType type, const std::string& name, char alias);  // no alias for parameters
CommandDecl& AddSubCommand(const std::string& name);
CommandDecl& SetDescription(const std::string& description);
template<typename T> void SetUniqueId(T id);
template<typename T> T GetUniqueId() const;            // throws if unset/type mismatch
bool HasUniqueId() const;
const std::string& GetName() const;
const std::string& GetDescription() const;
```

### CommandBlock
```cpp
bool IsOptionSet(const std::string& name) const;
const std::string& GetOptionValue(const std::string& name) const;                   // first occurrence; throws if missing
const std::string& GetOptionValue(const std::string& name, const std::string& defaultValue) const;
const std::string& GetOptionValue(const std::string& name, size_t index) const;     // indexed occurrence
size_t GetOptionValueCount(const std::string& name) const;                           // number of occurrences
const CommandDecl& GetDecl() const;
```

### OptionDecl
```cpp
OptionDecl& SetDescription(const std::string& description);
OptionDecl& SetDomain(const std::vector<std::string>& domain);
OptionDecl& AllowMultiple(bool allow = true);            // permit multiple occurrences for this option
OptionType GetType() const;
const std::string& GetName() const;
const std::string& GetDescription() const;
char GetAlias() const;                // 0 if none
const std::vector<std::string>& GetDomain() const;
template<typename T, typename Converter = DefaultConverter<T>>
OptionDecl& BindTo(T& variable, Converter converter = Converter{});
bool AllowsMultiple() const;                                  // query capability
```

### Enums

```cpp
enum class OptionType { Switch, Variable, Parameter };
enum class VariableDelimiter { Whitespace, Equals, Colon };
enum class ApiError {
    None,
    OutOfMemory,
    DuplicateCommandBlock,
    DuplicateOption,
    UniqueIdNotAssigned,
    OptionNotFound,
    ParameterNotFound,
    InvalidOptionType,
    InvalidUniqueIdType,
    OutOfRange,
};

enum class SyntaxError {
    None,
    UnknownOption,
    MissingVariableValue,
    UnexpectedArgument,
    TooManyParameters,
    InvalidValue,
    OptionNotSet,
    InvalidAlias
};
```

### Exceptions
```cpp
class ApiException {
public:
    const std::string& GetMessage() const;
    ApiError GetError() const;
};
class SyntaxException {
public:
    const std::string& GetMessage() const;
    const std::string& GetToken() const;
    SyntaxError GetError() const;
};
```

---

<!-- Duplicate Requirements & License sections removed to avoid redundancy -->



