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

Command arguments describe the action that a command needs to perform.

### Command Context

A command context encapsulates a given command argument and defines the parameter rules for the context. Command contexts may be nested to support rich command syntax. For example, command context nesting can be useful for creating command categories:

``` sh
fooApp config reset
```

In this case, the command argument 'reset' is nested with the command argument 'config'.

Command arguments must be at the start of the argument list, preceding any parameter arguments. Only one action is active in a given command.

### Parameter Arguments

Parameter arguments are given after command arguments. Parameter arguments can either be options or inputs.

### Option Parameter Arguments

Option argument strings are prefixed with `--`. Option argument types and values are constrained by the rules set in a declared command context.

Option parameters are typed. Boolean options are `true` if they are present in the command arguments.

Example:

``` sh
fooApp config reset --force
```

All other option parameters must be immediately followed by an input value of a matching type.

Example:

``` sh
fooApp config reset --mode quick
```

If needed, option parameter values can be constrained to a limited domain of input values.

Example:

Assume option 'color' is declared as constrained to the set ['red', 'green', 'blue']

``` sh
fooApp --color red
```

InCommand supports optional short-form single letter aliases for option parameter arguments. Short-form options are preceded by `-` instead of `--`.

Example:

``` sh
fooApp --long-option-name
fooApp -l
```

### Input Parameter Arguments

Input parameter arguments have no prefix like `--` or `-`. The number if input parameters is constrained by the associate command context or option parameter rules.

Example:

``` sh
fooApp merge myfile1.foo myfile2.foo --outfile mergedfile.foo
```
