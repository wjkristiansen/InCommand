# Introduction

This is a work in progress. Most of the primary features are functional, but quite a bit of polish is still needed.

InCommand is a command line interface (CLI) argument processor. Command lines are expected to have the following syntax:

```
app-name [command args...] [parameter args...]
```

---

## Definition of Terms

### Arguments

Arguments are the elements of a command representing individual action or parameter arguments.

### Command Arguments

Arguments that represent the action to be taken.

### Parameter Arguments

Parameter arguments are given after command arguments. Parameters can either be switch parameters or input parameters.

### Command reader

The command reader reads command and parameter arguments, sets variables, and provides the top-level command scope.

### Command Scope

A command scope encapsulates a group of nested command arguments. Complex command arguments can be structured hierarchically. This can be useful if commands require context or categorization. All command arguments are declared under an existing command scope. The command reader provides the root command scope.

For example:

``` sh
rocket launch
rocket fuel refill
rocket fuel dump
```

Command arguments must be first in the argument list, preceding any parameter arguments. Only one action is active in a given command. Note that `planets` itself represents a valid command scope.

### Switch Parameters

Switch parameters are prefixed with `--` (long form) or `-` (short form). Switch parameters represent a name/value pair and are expressed as `--<name> [value]`. For example:

``` sh
rocket launch --destination mars
```

All switch parameters are declared with a name. Switch parameters may optionally be given a single-character (short form) name. Short form switch arguments are preceded by a single `-` instead of `--`. For example:

``` sh
rocket fuel refill --type oxygen
rocket fuel refill -t hydrogen
```

Values can be optionally constrained to a pre-declared set. Example:

``` sh
# Okay:
rocket fuel refill -t oxygen 
rocket fuel refill -t hydrogen

# Not okay:
rocket fuel refill -t bananas
```

Boolean switch parameters do not take a value argument. Instead, boolean parameters are treated as `true` if they are present in the argument list.

Example:

``` sh
rocket fuel refill --all
```

### Input Parameters

Input parameter arguments have no prefix like `--` or `-`. Input parameters can still be named to aid in usage output.

Example:

``` sh
rocket rename Enterprise
```
