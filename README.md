# Slash

## Support for APMs

An APM is a dynamic library that may define slash command that then appear as an extension to the set of commands defined by the application, like CSH.

The design requires a minimum of changes in the APM, and the solution is backwards compatible, such that no changes are required in the application, as opposed to the current way of defining commands.

### How it works

The slash_commands are added in an ELF section, such that they appear as a convenient contiguous array of structs. The linker defines start and stop variables as the first and last entries in the section. This allows for iterating through all commands as part of operations like find and help. The current section name is slash. 

In an APM the section name cannot be the same, i.e. slash, as the start and stop variables will be external variables with same names as in the application, such that they refer to the section in the application leaving no reference to the commands defined in the APM.

The solution is to assign separate section names in application and APMs. The name must be unique, so good practice is to use the APM name that must anyway be unique among APM that can be used simultaneously. It is good practice to prefix this by "slash_", as the sections are the clearly identifiable, and they are not mixed up with sections for other purposes, like poarameters. 

As a common container of all commands defined in application and APMs, a pointer to a slash_command struct is added to the slash struct and to each slash_command struct. This is used to build a single-linked list of the commands. This is done by the slash_command_list_add(start, stop) function that must be called once per APM to add the commands defined in its separate command section.

Any order of the command list works, so it is sorted alphabetically, which makes help produce a sorted list.

### How to define commands in an APM

Some where is the code, likely in a main source file, the initialization macro must be used:

```
SLASH_SECTION_INIT(name)
```
This declares the above mentioned start and stop variables that are required by slash operations, but are not used explicitly in the APM. It also declares a function that returns pointers to the start and stop commands that are passed to the slash_command_list_add() function when loading the APM.

A command is currently created using the macro

```
slash_command(name, func, args, help)
```
This created a slash_command struct located in the section called slash.
This still works for an application, so no changes to command definition is required in for instance csh.

An APM that requires a separate section name uses this:

```
slash_sec_command(section, name, func, args, help)
```
The additional argument, section, specified the name of the section in which the command struct is placed.

This applies to all macros for sub commands etc.

