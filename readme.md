# Introduction

This is a work in progress. Most of the primary features are functional, but quite a bit of polish is still needed.

InCommand is a command line interface (CLI) argument processor. Command expressions are expected to have the following syntax:

```
app-name [[<category>] [<options>...]...]
```

---

## Definition of Terms

### Command expression

The ordered collection of command category and option arguments.

### Arguments

Arguments are the elements of a command expression representing individual commands, variables, switches, or parameters

### Category Arguments

Category arguments provide context used to describe a specific command action or subject. Categories are related hierarchically, allowing some command expressions to have arbitrarily deep category levels. However, in most cases, only one or two category levels are needed. Every command has a top-level implicit category. All other categories are declared as a child of either the implicit top-level category or another already-declared category.

A typical example of the use of sub-categories is an app command that take an action as the first argument:

`cmake --version` <- Only uses an implicit top-level category.
`apt list --installed` <- `list` is a sub-category of the implicit top-level category.
`git config list` <- `config` is a sub-category of the implicit top-level category and `list` is a sub-category of `config`.
`git submodule init` <- `submodule` is a sub-category of the implicit top-level category and `init` is a sub-category of `submodule`.

### Option Arguments

Options set command attributes within a category. There are three types of option arguments:

- Variable
  - A name/value pair.
    - E.g. `copy --source myinputfile.txt --dest myoutputfile.txt`
  - The values can optionally be constrained to a specified domain of values.
- Switch
  - A boolean variable set to `true` if present in the command expression.
    - `spellcheck --verbose myinputfile.txt`
- Parameters
  - Arbitrary inputs to a command. Parameters have a well defined order and are not preceded by `--` or `-`.
    - E.g. `copy myinputfile.txt myoutputfile.txt`

Each category can have zero or more declared options.

### Command reader

The command reader parses directives and options.

### Examples

``` sh
rocket # No command argument so the default base-level command is used
rocket launch # `launch` command, which is technically a subcommand of the default base-level command
rocket fuel refill # `refill` is a subcommand of `fuel`
rocket fuel dump # `dump` is another subcommand of `fuel`
```

### Variable Options

Variable options are prefixed with `--` (long form) or `-` (short form). Variable options represent a name/value pair and are expressed as `--<name> [value]`. For example:

``` sh
rocket launch --destination mars
```

All variable options are declared with a name. Variable options may optionally be given a single-character (short form) name. Short form switch arguments are preceded by a single `-` instead of `--`. For example:

``` sh
rocket fuel refill --type oxygen
rocket fuel refill --t hydrogen
```

Values can be optionally constrained to a pre-declared set. Example:

``` sh
# Okay:
rocket fuel refill --type oxygen 
rocket fuel refill --type hydrogen

# Not okay assuming `bananas` is not declared as a valid `type` variable for the `refill` subcommand under `fuel`:
rocket fuel refill --type bananas
```

### Switch Options

Switch options are similar to variable options except they do not take a value argument. Instead, switch options are treated as `true` if they are present in the argument list.

Example:

``` sh
rocket fuel refill --all
```

### Parameter Options

Parameter option arguments have no prefix like `--` or `-`. Multiple parameter options are recorded in the order they appear in a command expression. Any argument not recognized as a sub-category is treated as a parameter argument.

Parameter option arguments behave exactly like variable arguments except that they are not referenced by name in the command expression.

Example:

``` sh
rocket rename Enterprise
```
