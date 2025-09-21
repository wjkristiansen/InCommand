# DEFENSIVE_DISCLOSURE.md

**Title**: Declarative CLI Architecture with Parsed Command Chain and Schema-Driven Help Generation

**Author**: Bill Kristiansen  
**Date of Public Disclosure**: September 17, 2025  
**Repository**: [InCommand](https://github.com/wjkristiansen/InCommand)

---

## Purpose

This document serves as a formal defensive publication of the architectural and functional design of the *InCommand* C++ command-line interface (CLI) parsing framework. Its intent is to establish prior art and prevent future patent claims on the core concepts and mechanisms described herein.

---

## Abstract

InCommand introduces a novel CLI parsing model that separates **declarative schema definition** from **parsed execution**, enabling introspective tooling, scoped option inheritance, and automatic help generation. Unlike traditional CLI libraries that rely on procedural setup or flat argument parsing, InCommand models command-line input as a structured, hierarchical schema and a linear parsed command chain.

---

## Key Innovations

### 1. **Declarative Schema Tree (`CommandDecl`)**
- CLI structure is defined as a tree of command blocks, each with scoped options and subcommands.
- Options are declared with metadata, domain constraints, and type-safe bindings via `BindTo()`.

### 2. **Parsed Command Chain (`CommandBlock`)**
- User input is parsed into a linear chain of command blocks, reflecting the execution path.
- Each parsed block inherits context and options from its predecessors.

### 3. **Global Option Inheritance with Local Context**
- Options declared in the parser-level schema are automatically visible to all subcommands.
- Supports semantic layering, where global options are structurally shared but functionally scoped.
  - Interpretation may vary depending on the parsed command block in which they are accessed.
  - This allows global options to be context-sensitive, enabling behaviors like:
    - `--verbose` triggering different logging levels in different subcommands.
    - `--config` loading scoped configuration relevant to the current command path
- No duplication or manual propagation required.

### 4. **Schema-Driven Help Generation**
- Help output is derived directly from the declarative schema.
- Includes option descriptions, constraints, and visibility based on scope.

### 5. **Type-Safe Option Binding**
- Developers can bind options directly to variables using `OptionDecl::BindTo()`.
- Supports domain-constrained values and automatic conversion.

---

## Example

```cpp
CommandParser parser("rocket");

parser.GetAppCommandDecl().AddOption(OptionType::Switch, {"verbose", "v"})
      .SetDescription("Enable verbose output");

int fuelAmount = 0;
auto& fuel = parser.GetAppCommandDecl().AddSubcommand("fuel");
fuel.AddOption(OptionType::Parameter, {"amount"})
    .SetDescription("Amount of fuel to add")
    .BindTo(fuelAmount);
```
