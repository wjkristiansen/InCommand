# Introduction

This is a work-in-progress and is not yet functional.

InCommand is a command line parsing utility. Command lines are expected to have the following syntax:

```
app-name [subcommand [subcommand[...]]] [options...]
```

# Arguments

Arguments are the space-delimitted strings in a command line. Typically, these are passed into the main function in a program's main function as 'argv'.

```C
int main(int argc, const char *argv[])
```

Arguments are either subcommands or options, with subcommands appearing before any options in the argument list.

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

## Non-Keyed Options

Non-Keyed options contain only a value string. Typically, non-keyed arguments do not start with '--' or '-', as these give the appearance of keyed options.

Any option argument that doesn't match a keyed option is assumed to be a non-keyed option. An error occurs if there are not enough non-keyed options declared to contain matching arguments on the command line.

Example:

```
foo.exe myfile1.foo myfile2.foo
```

---

## Keyed Options

Syntactically, keyed option arguments use '--' followed by the name of the option. Optionally, a single letter short-form argument may be declared, which is preceded by a single hyphen '-'.

Example
```
foo.exe --long-option-name
```

or

```
foo.exe -l
```

### Switch Options

Switch options are boolean values considered to be 'on' when present in the options list. 

Example:

```
foo.exe --reset
```

### Variable Options

Variable arguments are name/value string pairs. The value string is taken from the next argument in the argument list. A default value is assigned if the variable is not present in the argument list.

Example:

```
foo.exe --file hello.txt
```

Variable options may be constrained to a pre-declared domain of values. An error results if an assigned value is not in the domain.

Example:

Assume Variable 'color' is declared as constrained to the set ['red', 'green', 'blue']

```
foo.exe --color red
```
