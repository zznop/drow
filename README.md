# drow [![Build Status](https://travis-ci.org/zznop/ich.svg?branch=master)](https://travis-ci.org/zznop/drow)

![demo ich](drow.gif)

## Description

drow is a command-line utility for applying patches to ELF files (post-build) and hooking the ELF header `e_entry` to
get execution at runtime.

## How does it work?

### Code Injection

drow takes an unmodified ELF executable as input and outputs a patched ELF executable containing injected code. It patches
the ELF by taking the following steps:

1. Map in the umodified target ELF
2. Map in the user-supplied payload/patch blob
3. Locate the first executable segment by parsing program headers
4. Locate the last section in the executable segment by parsing section headers
5. Expand the section header's `sh_size` and program header's `p_memsz`/`p_filesz` by the size of the user-supplied payload
6. Fixup section headers' `sh_offset`'s and program headers' `p_offset`'s (move down sections and segments to make room
   for the payload)
7. Modify offsets in the ELF header
8. Create a new ELF containing the injected patch in and modified ELF headers

### Entry Hooking

drow modifies the `e_entry` (the ELF entrypoint) to point to a injected "stager stub" that calls into the payload. It
is intended for payloads to be written to return to the stager after initialization. The stager then tailcalls into the
real `_start` routine.

## Building

Requires installing `gcc` and `scons` packages. Simply run `scons` from the root of the directory.

## Other Information

In addition to building drow, this project also builds a x86-64 payload named `rappers_delight.bin` that simply outputs
Rapper's Delight lyrics to stdout. This can be used for testing. Currently, drow only works with ELF64 files targetting
x86-64.

