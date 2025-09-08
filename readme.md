# InCommand

**InCommand** is a modern, hierarchical command-line interface (CLI) argument parser for C++ applications. It provides a clean, type-safe API for defining complex command structures with subcommands, options, parameters, and aliases.

---

## Features

**Hierarchical Command Structure**: Support for nested command blocks (e.g., `git remote add origin url`)  
**Multiple Option Types**: Switches, variables, and positional parameters  
**Type-Safe Value Binding**: Template method for automatic type conversion and variable binding  
**Variable Delimiters**: Support for packed syntax (`--name=value`) alongside traditional whitespace format  
**Alias Support**: Single-character aliases with switch grouping (`-vqh`)  
**API Validation**: Prevents logical errors (e.g., parameters with aliases) at setup time  
**Syntax Error Reporting**: Detailed error messages with problematic token identification  

---

### Basic Usage

```cpp
InCommand::CommandParser parser("myapp");
auto& rootCmd = parser.GetRootCommandBlockDesc();
rootCmd.SetDescription("My application");

// Declare subcommands
auto& buildCmd = rootCmd.DeclareSubCommandBlock("build");
buildCmd.SetDescription("Build the project");

auto& testCmd = rootCmd.DeclareSubCommandBlock("test");
testCmd.SetDescription("Run tests");
```

---

## Unique Variable Delimiter Support

InCommand supports three delimiter styles for variable assignment:

- Traditional: `--name value` 
- Equals: `--name=value`
- Colon: `--name:value`

This feature makes InCommand particularly suitable for applications that need to match existing CLI tool conventions or provide users with their preferred syntax style.

---

## Command Syntax and Structure

### Command Blocks

A **command block** represents a complete command context with its own set of options. Command blocks can be nested to create hierarchical command structures like `git remote add` or `docker container run`.

```shell
app-name [options] [command-block [options] [command-block [options]]...]
```

This shows the hierarchical nature: the outermost command block (`app-name`) can contain nested command blocks, each with their own options. The `...` indicates this nesting can continue to arbitrary depth.

Examples:
- `myapp --verbose` (outermost command block with switch)
- `myapp build --target release project.vcxproj` (nested command block with variable and parameter)
- `git remote add origin https://github.com/user/repo.git` (multiple nested command blocks)
  - myapp
  - remote
  - add
  - origin

#### Command Block Hierarchy

Command blocks nest to create subcommand structures. Each level represents a new command context:

```text
Outermost Command Block: "git"
├── Switch: --version
├── Variable: --config-file (-c)
└── Sub Command Block: "remote"
    ├── Switch: --verbose (-v)
    └── Sub Command Block: "add"
        ├── Parameter: name
        └── Parameter: url
```

### Option Types

InCommand supports three distinct option types, each with different syntax rules:

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

### Parsing Rules

1. **Option Precedence**: Options are processed left-to-right
2. **Switch Grouping**: Multiple single-char switch aliases can be grouped: `-abc` = `-a -b -c`
3. **Variable Separation**: Variables use space separation: `--key value`
4. **Parameter Order**: Parameters fill in declaration order regardless of position
5. **Subcommand Navigation**: Each subcommand creates a new command block context
6. **Alias Restrictions**: Only switches can be grouped; variables require individual specification

**Example breakdown**:

```shell
git remote add origin https://github.com/user/repo.git
```

1. **"git"** - Outermost command block
2. **"remote"** - First subcommand (creates new command block)
3. **"add"** - Second subcommand (creates third command block)  
4. **"origin"** - First parameter of the "add" command block
5. **"https://..."** - Second parameter of the "add" command block

Each command block has its own independent set of options and parameters. The `git` block might have `--version`, the `remote` block might have `--verbose`, and the `add` block takes the repository name and URL as parameters.

---

## Quick Start

### Basic Usage

```cpp
#include "InCommand.h"
using namespace InCommand;

int main(int argc, const char *argv[])
{
    // Create InCommand parser - it owns the root command descriptor
    CommandParser parser("mybuildtool");
    auto& rootCmdDesc = parser.GetRootCommandBlockDesc();
    rootCmdDesc.SetDescription("My awesome application");

    // Note: Help options are automatically declared - no manual setup needed!

    // Declare a subcommand with cascading style
    CommandBlockDesc& buildCmdDesc = rootCmdDesc.DeclareSubCommandBlock("build");
    buildCmdDesc.SetDescription("Build the project");
    buildCmdDesc.DeclareOption(OptionType::Switch, "verbose", 'v')
        .SetDescription("Enable verbose output");
    buildCmdDesc.DeclareOption(OptionType::Variable, "target")
        .SetDescription("Build target");
    buildCmdDesc.DeclareOption(OptionType::Parameter, "project")
        .SetDescription("Project file");

    try
    {
        // ParseArgs returns the last CommandBlock in the chain that was parsed
        const CommandBlock* result = parser.ParseArgs(argc, argv);
        
        // Check which command was parsed
        if (result->GetDesc().GetName() == "build")
        {
            bool verbose = result->IsOptionSet("verbose");
            std::string target = result->GetOptionValue("target", "debug");
            std::string project = result->GetParameterValue("project");
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

### Variable Assignment Delimiters

InCommand supports flexible variable assignment syntax through the `VariableDelimiter` enum. Each parser instance uses a single delimiter format for consistency:

```cpp
// Whitespace delimiter (traditional format)
CommandParser parser("myapp", VariableDelimiter::Whitespace);
// or simply: CommandParser parser("myapp");  // defaults to Whitespace

// Equals delimiter 
CommandParser equalsParser("myapp", VariableDelimiter::Equals);

// Colon delimiter
CommandParser colonParser("myapp", VariableDelimiter::Colon);

auto& rootCmdDesc = equalsParser.GetRootCommandBlockDesc();
rootCmdDesc.DeclareOption(OptionType::Variable, "output", 'o')
    .SetDescription("Output file");
rootCmdDesc.DeclareOption(OptionType::Switch, "verbose", 'v')
    .SetDescription("Verbose mode");

// With VariableDelimiter::Equals, these formats work:
// myapp --output=file.txt -v
// myapp -o=file.txt -v
// myapp --output file.txt -v  (traditional format still works)
```

**Delimiter Options:**

- **`VariableDelimiter::Whitespace`** - Traditional space-separated format (`--name value`)
- **`VariableDelimiter::Equals`** - Packed equals format (`--name=value`, `-n=value`)  
- **`VariableDelimiter::Colon`** - Packed colon format (`--name:value`, `-n:value`)

**Key Features:**
- **Single delimiter per parser** - Each parser instance uses one consistent format
- **Type-safe enum** - Prevents invalid delimiter configurations at compile time
- **Long and short options** - Delimiter format works with both `--name=value` and `-n=value`
- **Error validation** - Switches cannot have values (`--verbose=true` throws `SyntaxException`)
- **Backward compatibility** - Traditional space-separated format always works alongside delimiter format
- **Mixed usage** - Can mix delimiter and space-separated formats in the same command line

---

## Type-Safe Value Binding

InCommand provides a template method `BindTo()` for automatic type conversion and variable binding. When options are parsed, values are automatically converted from strings to the target variable type.

### Basic Example

```cpp
#include "InCommand.h"

int main(int argc, const char* argv[])
{
    CommandParser parser("myapp");
    auto& rootCmd = parser.GetRootCommandBlockDesc();
    
    // Variables to bind to
    int count = 10;
    float rate = 1.5f;
    bool verbose = false;
    std::string filename;
    
    // Declare options with type-safe bindings
    rootCmd.DeclareOption(OptionType::Variable, "count", 'c')
           .BindTo(count)
           .SetDescription("Number of items");
           
    rootCmd.DeclareOption(OptionType::Variable, "rate", 'r')
           .BindTo(rate)
           .SetDescription("Processing rate");
           
    rootCmd.DeclareOption(OptionType::Variable, "verbose", 'v')
           .BindTo(verbose)
           .SetDescription("Enable verbose output");
           
    rootCmd.DeclareOption(OptionType::Parameter, "filename")
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

rootCmd.DeclareOption(OptionType::Variable, "name")
       .BindTo(name, UppercaseConverter{});
       
rootCmd.DeclareOption(OptionType::Variable, "log-level")
       .BindTo(logLevel, LogLevelConverter{});
```

### Error Handling

Type conversion errors automatically throw `SyntaxException`:

```cpp
int number;
rootCmd.DeclareOption(OptionType::Variable, "number").BindTo(number);

// Command: myapp --number abc
// Result: SyntaxException with message "Invalid value for numeric type (token: abc)"
```

### Method Signature

```cpp
template<typename T, typename Converter = DefaultConverter<T>>
OptionDesc& BindTo(T& variable, Converter converter = Converter{});
```

**Parameters:**
- `T` - Target type (automatically deduced from variable reference)
- `Converter` - Conversion functor (defaults to `DefaultConverter<T>`)

**Returns:** Reference to `OptionDesc` for method chaining

**Constraints:**
- Only `Variable` and `Parameter` options support binding
- `Switch` options cannot be bound (throws `ApiException`)

---

## Core API

This section provides comprehensive documentation for all public classes and methods in InCommand.

### Class Overview

- **CommandParser** - Main entry point, owns the command structure
- **CommandBlockDesc** - Describes commands and options (tree structure)
- **CommandBlock** - Parsed command results (linked chain)
- **OptionDesc** - Describes individual options, supports method chaining
- **Enums** - OptionType, VariableDelimiter, ApiError, SyntaxError
- **Exceptions** - ApiException, SyntaxException

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

#### `CommandParser::GetRootCommandBlockDesc()`

```cpp
CommandBlockDesc& GetRootCommandBlockDesc()
const CommandBlockDesc& GetRootCommandBlockDesc() const
```

Returns a reference to the root command block descriptor. Use this to configure your application's commands and options.

#### `CommandParser::DeclareGlobalOption()`

```cpp
OptionDesc& DeclareGlobalOption(OptionType type, const std::string& name, char alias = 0)
```

Declares a global option that is available to all command blocks in the hierarchy. Global options can be specified at any level of the command chain and are accessible from any command block after parsing.

**Parameters:**
- `type`: The option type (Switch, Variable, or Parameter)
- `name`: The option name
- `alias`: Optional single-character alias (0 for no alias)

**Returns:** Reference to the created OptionDesc for method chaining

**Example:**
```cpp
parser.DeclareGlobalOption(OptionType::Switch, "verbose", 'v')
      .SetDescription("Enable verbose output");
// Usage: myapp --verbose build  OR  myapp build --verbose
```

#### `CommandParser::ParseArgs()`

```cpp
const CommandBlock* ParseArgs(int argc, const char* argv[])
```

Parses command-line arguments and returns the last (most specific) command block that was parsed. Throws `SyntaxException` if parsing fails.

#### `CommandParser::GetNumCommandBlocks()`

```cpp
size_t GetNumCommandBlocks() const
```

Returns the number of command blocks parsed from `ParseArgs()`.

#### `CommandParser::GetCommandBlock()`

```cpp
const CommandBlock &GetCommandBlock(size_t index) const
```

Returns the command block at the requested `index`. Throws `SyntaxException` if no command blocks have been parsed.

#### Global Option Access Methods

```cpp
bool IsGlobalOptionSet(const std::string& name) const
```

Returns `true` if the named global option was set anywhere in the command chain during parsing.

```cpp
const std::string& GetGlobalOptionValue(const std::string& name) const
```

Returns the value of a global variable or parameter option. Throws `ApiException` if the option doesn't exist or has no value.

```cpp
size_t GetGlobalOptionContext(const std::string& name) const
```

Returns the command block indexwhere the global option was set. Throws `ApiException` if the global option was not set.

#### `CommandParser::SimpleUsageString()`

```cpp
std::string SimpleUsageString() const
```

Generates a basic usage string for the app-level command block showing the application name followed by its options and parameters. Does not include the command description or detailed option explanations.

#### `CommandParser::OptionDetailsString()`

```cpp
std::string OptionDetailsString() const
```

Generates detailed help text for all options defined on the root command. Shows option names, aliases, descriptions, and domains (if applicable). Does not include usage line or command description.

#### `CommandParser::GetHelpString()`

```cpp
std::string GetHelpString() const
```

Generates complete help text for the root command by combining the command description (if set), usage string, and option details. This is the recommended method for displaying comprehensive help information to users.

### CommandBlockDesc

Describes a command or subcommand with its available options. Represents the tree structure of your command hierarchy.

#### `CommandBlockDesc::DeclareOption()` - Basic Form

```cpp
OptionDesc& DeclareOption(OptionType type, const std::string& name)
```

Declares an option without an alias. Returns a reference to the newly created OptionDesc.
- **type**: `OptionType::Switch`, `OptionType::Variable`, or `OptionType::Parameter`
- **name**: The option name (e.g., "verbose", "output", "input-file")

#### `CommandBlockDesc::DeclareOption()` - With Alias

```cpp
OptionDesc& DeclareOption(OptionType type, const std::string& name, char alias)
```

Declares an option with a single-character alias. **Parameters cannot have aliases** and will throw `ApiException`.
- **type**: `OptionType::Switch` or `OptionType::Variable` (NOT `Parameter`)
- **name**: The option name
- **alias**: Single character alias (e.g., 'v', 'o', 'h')

#### `CommandBlockDesc::DeclareSubCommandBlock()`

```cpp
CommandBlockDesc& DeclareSubCommandBlock(const std::string& name)
```

Declares a new subcommand. Returns a reference to the new command block descriptor for configuration.

#### `CommandBlockDesc::SetDescription()`

```cpp
CommandBlockDesc& SetDescription(const std::string& description)
```

Sets the description for this command. Used in help generation. Returns `*this` for method chaining.

#### `CommandBlockDesc::SetUniqueId()`

```cpp
template<typename T>
void SetUniqueId(T id)
```

Assigns a user-defined ID to this command block descriptor. The ID can be any type (typically an enum) and is useful for switch-based command handling in your application logic.

#### `CommandBlockDesc::GetUniqueId()`

```cpp
template<typename T>  
T GetUniqueId() const
```

Retrieves the previously assigned unique ID. Throws `ApiException` if no ID has been set or the type doesn't match the stored ID type.

#### `CommandBlockDesc::HasUniqueId()`

```cpp
bool HasUniqueId() const
```

Returns `true` if a unique ID has been assigned to this command block descriptor, `false` otherwise.

#### `CommandBlockDesc::GetName()`

```cpp
const std::string& GetName() const
```

Returns the name of this command block (the command word that users type).

#### `CommandBlockDesc::GetDescription()`

```cpp
const std::string& GetDescription() const
```

Returns the description text for this command block. Empty string if no description has been set.

#### Help Generation Methods

#### `CommandBlockDesc::SimpleUsageString()`

```cpp
std::string SimpleUsageString() const
```

Generates a basic usage line showing the command name followed by its options and parameters. Does not include the command description or detailed option explanations.

#### `CommandBlockDesc::OptionDetailsString()`

```cpp
std::string OptionDetailsString() const
```

Generates detailed help text for all options defined on this command. Shows option names, aliases, descriptions, and domains (if applicable). Does not include usage line or command description.

#### `CommandBlockDesc::GetHelpString()`

```cpp
std::string GetHelpString() const
```

Generates complete help text by combining the command description (if set), usage string, and option details. This is the recommended method for displaying comprehensive help information to users.

#### `CommandBlockDesc::GetHelpStringWithPath()`

```cpp
std::string GetHelpStringWithPath(const std::string& commandPath) const
```

Generates complete help text with a custom command path prefix. Useful for generating help for nested subcommands with their full command path.

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

Returns the value of a variable or parameter. Throws `ApiException` if the option doesn't exist or has no value.

#### `CommandBlock::GetOptionValue()` - With Default

```cpp
const std::string& GetOptionValue(const std::string& name, const std::string& defaultValue) const
```

Returns the value of a variable or parameter, or the provided default value if the option wasn't set.

#### `CommandBlock::IsParameterSet()`

```cpp
bool IsParameterSet(const std::string& name) const
```

Returns `true` if the named parameter has a value. Semantically equivalent to `IsOptionSet()` but more clear for positional arguments.

#### `CommandBlock::GetParameterValue()` - Basic Form

```cpp
const std::string& GetParameterValue(const std::string& name) const
```

Returns the value of a parameter. Throws `ApiException` if the parameter doesn't exist or has no value.

#### `CommandBlock::GetParameterValue()` - With Default

```cpp
const std::string& GetParameterValue(const std::string& name, const std::string& defaultValue) const
```

Returns the value of a parameter, or the provided default value if the parameter wasn't set.

#### `CommandBlock::GetPrevCommandBlock()`

```cpp
CommandBlock* GetPrevCommandBlock() const
```

Returns the previous (more general) command block in the parsed chain, moving toward the root command. Returns `nullptr` if this is the root command block.

#### `CommandBlock::GetNextCommandBlock()`

```cpp
CommandBlock* GetNextCommandBlock() const
```

Returns the next (more specific) command block in the parsed chain, moving toward subcommands. Returns `nullptr` if this is the most specific command block.

#### `CommandBlock::GetPrevCommandBlock()`

```cpp
CommandBlock* GetPrevCommandBlock() const
```

Returns the previous (more general) command block in the parsed chain, moving toward the root command. Returns `nullptr` if this is the root command block.

#### `CommandBlock::GetNextCommandBlock()`

```cpp
CommandBlock* GetNextCommandBlock() const
```

Returns the next (more specific) command block in the parsed chain, moving toward subcommands. Returns `nullptr` if this is the most specific command block.

#### `CommandBlock::GetDesc()`

```cpp
const CommandBlockDesc& GetDesc() const
```

Returns a reference to the command block descriptor that was used to create this parsed command block.

---

### OptionDesc

Describes an individual option (switch, variable, or parameter). Returned by `DeclareOption()` methods.

#### `OptionDesc::SetDescription()`

```cpp
OptionDesc& SetDescription(const std::string& description)
```

Sets the description for this option. Used in help generation. Returns `*this` for method chaining.

#### `OptionDesc::SetDomain()`

```cpp
OptionDesc& SetDomain(const std::vector<std::string>& domain)
```

Restricts the option to a specific set of valid values. Useful for variables with limited choices.

#### `OptionDesc::SetUniqueId()`

```cpp
template<typename T>
void SetUniqueId(T id)
```

Assigns a user-defined ID to this option descriptor. The ID can be any type (typically an enum) and is useful for option identification in your application logic.

#### `OptionDesc::GetUniqueId()`

```cpp
template<typename T>
T GetUniqueId() const
```

Retrieves the previously assigned unique ID. Throws `ApiException` if no ID has been set or the type doesn't match the stored ID type.

#### `OptionDesc::HasUniqueId()`

```cpp
bool HasUniqueId() const
```

Returns `true` if a unique ID has been assigned to this option descriptor, `false` otherwise.

#### `OptionDesc::GetType()`

```cpp
OptionType GetType() const
```

Returns the type of this option (`OptionType::Switch`, `OptionType::Variable`, or `OptionType::Parameter`).

#### `OptionDesc::GetName()`

```cpp
const std::string& GetName() const
```

Returns the name of this option as specified when it was created.

#### `OptionDesc::GetDescription()`

```cpp
const std::string& GetDescription() const
```

Returns the description text for this option. Empty string if no description has been set.

#### `OptionDesc::GetAlias()`

```cpp
char GetAlias() const
```

Returns the single-character alias for this option. Returns 0 if no alias has been set.

#### `OptionDesc::GetDomain()`

```cpp
const std::vector<std::string>& GetDomain() const
```

Returns the list of valid values for this option. Empty vector if no domain restrictions have been set.

#### `OptionDesc::BindTo()`

```cpp
template<typename T, typename Converter = DefaultConverter<T>>
OptionDesc& BindTo(T& variable, Converter converter = Converter{})
```

Binds the option's parsed value to a typed variable with automatic type conversion. When the command line is parsed, the string value is converted to the target type and assigned to the variable. Returns `*this` for method chaining.

**Parameters:**
- `T` - Target type (automatically deduced from variable reference)
- `Converter` - Conversion functor (defaults to `DefaultConverter<T>`)

**Constraints:**
- Only `Variable` and `Parameter` options support binding
- `Switch` options cannot be bound (throws `ApiException`)

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
rootCmdDesc.DeclareOption(OptionType::Switch, "verbose", 'v');
// Usage: myapp --verbose  or  myapp -v  or  myapp -vq (grouped)
```

#### Variable Options

Options that require a value argument.
```cpp
rootCmdDesc.DeclareOption(OptionType::Variable, "output", 'o');
// Usage: myapp --output file.txt  or  myapp -o file.txt
```

#### Parameter Options

Positional arguments without option prefixes. Order matters and they are consumed in declaration order.
```cpp
rootCmdDesc.DeclareOption(OptionType::Parameter, "input-file");
// Usage: myapp input.txt
// NOTE: Parameters cannot have aliases
```

#### Alias Rules

- Only **switches** and **variables** can have aliases
- Only **switches** can be grouped (`-vq` = `-v -q`)
- **Parameters** cannot have aliases (will throw `ApiException`)
- **Variables** cannot be part of grouped aliases

---

## Enums and Types

### OptionType

Specifies the type of option when calling `DeclareOption()`.

```cpp
enum class OptionType
{
    Switch,     // Boolean flag (--verbose, -v)
    Variable,   // Option with value (--output file.txt, -o file.txt)  
    Parameter   // Positional argument (input.txt)
}
```

### VariableDelimiter

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

### ApiError

Error codes for `ApiException`. Indicates developer API misuse.

```cpp
enum class ApiError
{
    None,                    // No error
    OutOfMemory,            // Memory allocation failed
    DuplicateCommandBlock,  // Command name already exists
    DuplicateOption,        // Option name or alias already exists
    UniqueIdNotAssigned,    // Trying to get unassigned unique ID
    OptionNotFound,         // Referenced option doesn't exist
    ParameterNotFound,      // Referenced parameter doesn't exist
    InvalidOptionType       // Invalid operation for option type (e.g., alias on parameter)
}
```

### SyntaxError

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

## Exception Types

InCommand provides two distinct exception types for different error scenarios:

### ApiException

Thrown when developers misuse the API during command setup. These are programming errors that should be caught during development.

#### `ApiException::GetMessage()`

```cpp
const std::string& GetMessage() const
```

Returns a human-readable description of the API misuse error.

#### `ApiException::GetErrorCode()`

```cpp
ApiError GetErrorCode() const
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
    auto& rootCmdDesc = parser.GetRootCommandBlockDesc();
    rootCmdDesc.DeclareOption(OptionType::Switch, "help");
    rootCmdDesc.DeclareOption(OptionType::Switch, "help"); // Duplicate!
    
    // This also throws ApiException:
    rootCmdDesc.DeclareOption(OptionType::Parameter, "file", 'f'); // Parameters can't have aliases!
}
catch (const ApiException& e)
{
    std::cout << "API Error: " << e.GetMessage() << std::endl;
    // Handle programming error - fix your code!
}
```

### SyntaxException

Thrown when end-users provide invalid command-line syntax. These represent user input errors, not programming mistakes.

#### `SyntaxException::GetMessage()`

```cpp
const std::string& GetMessage() const
```

Returns a human-readable description of the syntax error.

#### `SyntaxException::GetToken()`

```cpp
const std::string& GetToken() const
```

Returns the specific command-line token that caused the error, if available. May be empty for some error types.

#### `SyntaxException::GetErrorCode()`

```cpp
SyntaxError GetErrorCode() const
```

Returns the specific `SyntaxError` enum value indicating the type of syntax error that occurred.

#### Common Scenarios

- **Unknown Options**: User provides unrecognized flags
- **Missing Values**: Variables require values but none provided
- **Invalid Values**: Values outside defined domains
- **Unexpected Arguments**: Extra parameters or unknown subcommands
- **Parsing Errors**: Malformed option syntax

```cpp
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

## Help Generation and Error Reporting

#### Complete Help with Description, Usage, and Options

The recommended way to generate help output is to use the `GetHelpString()` method on `CommandParser`:

```cpp
std::cout << rootCmdDesc.GetHelpString();
```

This prints the command description (if set), usage string, and option details in a single, formatted output. For subcommands, use the corresponding descriptor:

```cpp
std::cout << buildCmdDesc.GetHelpString();
```

**Sample Output:**

```shell
Sample application demonstrating InCommand
Usage: sample [--help] [add] [mul]
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

#### Simple Usage Strings

```cpp
std::string usage = parser.SimpleUsageString();
```

**Output:**

```shell
mybuildtool [--help] [build] [--verbose] [--target <value>] [<project>]
```

#### Detailed Help with Descriptions

```cpp
std::string details = parser.OptionDetailsString();
```

**Output:**

```shell
  --help, -h                  Show help information
  build                       Build the project
  --verbose, -v               Enable verbose output
  --target                    Build target
  <project>                   Project file
```

#### Complete Help Integration

Most applications combine both for comprehensive help:

```cpp
if (result->IsOptionSet("help"))
{
    std::cout << parser.GetHelpString() << std::endl;
    return 0;
}
```

**User Experience:**

```shell
$ mybuildtool --help
Usage: mybuildtool [--help] [build] [--verbose] [--target <value>] [<project>]

Options:
  --help, -h                  Show help information
  build                       Build the project
  --verbose, -v               Enable verbose output
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
Usage: mybuildtool [--help] [build]

# Subcommand help  
$ mybuildtool build --help
Usage: mybuildtool build [--verbose] [--target <value>] [<project>]

Options:
  --verbose, -v               Enable verbose output
  --target                    Build target
  <project>                   Project file (required)
```

---

## API Design Benefits

### Ownership Model

The `CommandParser` uses RAII principles and owns the command structure internally, eliminating the need for developers to manage separate descriptor objects:

```cpp
// ✅ Clean: Parser owns everything
CommandParser parser("myapp");
auto& rootCmdDesc = parser.GetRootCommandBlockDesc();
```

### Fluent Interface

Method chaining enables elegant, readable command definitions:

```cpp
rootCmdDesc.DeclareOption(OptionType::Switch, "verbose", 'v')
    .SetDescription("Enable verbose output")
    .SetDomain({"low", "medium", "high"});  // if applicable

// Cascading for command block setup too
buildCmdDesc.SetDescription("Build the project")
            .SetUniqueId(CommandId::Build);
```

### Type Safety

API validation prevents logical errors at setup time rather than runtime:

```cpp
// ✅ This works - switches can have aliases
rootCmdDesc.DeclareOption(OptionType::Switch, "verbose", 'v');

// ❌ This throws ApiException - parameters can't have aliases  
rootCmdDesc.DeclareOption(OptionType::Parameter, "filename", 'f');
//   → "Parameters cannot have aliases - use DeclareOption(type, name) instead"
```

---

## Advanced Examples

### Git-like Command Structure

```cpp
CommandParser parser("git");
auto& gitCmdDesc = parser.GetRootCommandBlockDesc();

// git remote
auto& remoteCmdDesc = gitCmdDesc.DeclareSubCommandBlock("remote");
remoteCmdDesc.SetDescription("Manage remote repositories");

// git remote add
auto& addCmdDesc = remoteCmdDesc.DeclareSubCommandBlock("add");
addCmdDesc.SetDescription("Add a remote repository")
           .DeclareOption(OptionType::Parameter, "name")
               .SetDescription("Remote name");
addCmdDesc.DeclareOption(OptionType::Parameter, "url")
    .SetDescription("Remote URL");

// Usage: git remote add origin https://github.com/user/repo.git
```

### Command Identification with Unique IDs

Unique, user-defined type IDs can be assigned to Command Block, allowing developers to use `switch` blocks to trivially walk the command chain.

```cpp
enum class MyCommandId { Root, Build, Test, Deploy };

CommandParser parser("myapp");
auto& rootCmdDesc = parser.GetRootCommandBlockDesc();
rootCmdDesc.SetUniqueId(MyCommandId::Root);

auto& buildCmdDesc = rootCmdDesc.DeclareSubCommandBlock("build");
buildCmdDesc.SetUniqueId(MyCommandId::Build);

auto& testCmdDesc = rootCmdDesc.DeclareSubCommandBlock("test");
testCmdDesc.SetUniqueId(MyCommandId::Test);

auto& deployCmdDesc = rootCmdDesc.DeclareSubCommandBlock("deploy");
deployCmdDesc.SetUniqueId(MyCommandId::Deploy);

const CommandBlock* active = parser.ParseArgs(argc, argv);
switch (active->GetDesc().GetUniqueId<MyCommandId>())
{
    case MyCommandId::Root: /* handle root command */ break;
    case CommandId::Test:  /* handle test */  break;
    case CommandId::Build: /* handle build */ break;
    case CommandId::Deploy:  /* handle deploy */  break;
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

[Add your license information here]
