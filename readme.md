# Introduction

This is a work-in-progress. Most of the primary features are functional, but quite a bit of polish is still needed.

InCommand is a command line parsing utility. Command lines are expected to have the following syntax:

```
app-name [command [command[...]]] [options...]
```

# Arguments

Arguments are the space-delimitted strings in a command line. Typically, these are passed into the main function in a program's main function as 'argv'.

```C
int main(int argc, const char *argv[])
```

Arguments are either commands or options, with commands appearing before any options in the argument list.

---

## Command Arguments

A sequence of command arguments describes a command context. If no command arguments are given the default command is active. A command context is declared as either a subcontext of the default command context or of another declared command context.

Command arguments must be at the start of the argument list, preceding any option arguments. Only one command context is active in a given command line.

Example:

```
foo.exe subcom-A subcom-B-of-A subcom-C-of-B-of-A --switch-on-subcom-C-of-B-of-A
```

---

## Parameter Options

Parameter options contain only a value. Typically, parameter arguments do not start with '--' or '-', as these give the appearance of variable or switch options.

Any option argument that doesn't begin with '--' or '-' is assumed to be a parameter value. Values are assigned to parameters in the order they are declared. An error is produced if there are more parameters in the argument list than declared parameters.

Example:

```
foo.exe myfile1.foo myfile2.foo
```

---

## Variable and Switch Options

Syntactically, variable and switch option arguments use '--' followed by the name of the option. Optionally, a single letter short-form label may be declared, which is preceded by a single hyphen '-'.

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
