# Introduction

This is a work-in-progress and is not yet functional.

InCommand is a command line parsing utility. Command lines are expected to have the following syntax:

```
app-name [subcommand [subcommand[...]]] [parameters...]
```

# Arguments

Arguments are the space-delimitted strings in a command line. Typically, these are passed into the main function in a program's main function as 'argv'.

```C
int main(int argc, const char *argv[])
```

---

## Subcommand Arguments

The presence of a subcommand sets the command scope to the given subcommand. Once a subcommand is specified, the subsequent command arguments are considered part of that command scope.

Subcommands must be given at the beginning of a command scope. No more than one subcommand will be parsed per command scope.

Subcommands must be declared before parsing. Subcommands can be chained, meaning at any given subcommand can declare its own set of subcommands. No more than one subcommand may be used for each subcommand level.

Example:

```
foo.exe subcom-A subcom-B-of-A subcom-C-of-B-of-A --switch-on-subcom-C-of-B-of-A
```

---

## Parameters

Parameter arguments are parsed by the active command scope. Parameter arguments can be one of the following types:

- Anonymous
- Switch
- Variable
- OptionsVariable

---

## Anonymous Parameters

Anonymous parameters are strings unassociated with any specific named parameters. The number of anonymous parameters must be declared before parsing. Anonymous parameter strings are indexed in the order they appears in the parameters list.

Any parameter argument that doesn't match a declared parameter is assumed to be an anonymous parameter. An error occurs if the number of anonymous parameters exceeds the number of declared number of parameters.

Example:

```
foo.exe myfile1.foo myfile2.foo
```

---

## Declared Parameters

Syntactically, declared parameters use a "--" prefix by default, though alternative prefixes may be defined.

### Switch Parameters

Switch parameters are boolean values considered to be 'true' when present in the parameters list. 

Example:

```
foo.exe --reset
```

### Variable Parameters

Variable arguments are name/value string pairs. The value string is taken from the next argument in the argument list. A default value is assigned if the variable is not present in the argument list.

Example:

```
foo.exe --file hello.txt
```

### OptionsVariable Parameters

OptionsVariable arguments are variables with a declared set of valid strings. The value string is taken from the next argument in the argument list. A default value is assigned if the variable is not present in the argument list.

Example:

Assume OptionsVariable 'color' may be one of ['red', 'green', 'blue']

```
foo.exe --color red
```
