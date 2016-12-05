# libc++ bad header visibility specifiers checker

This is a simple clang tool to check for incorrect visibility specifiers
in libc++ headers. If we have a class marked with default visibility, we
want all templated member functions whose definitions are outside the
class declaration to be marked inline and/or hidden visibility, so that
other libraries using the headers and wishing to control their
visibility (using `-fvisibility=hidden -fvisibility-inlines-hidden`) do
not export implicit instantiations of those template member functions.
The tool finds member functions which are not marked appropriately.

This tool was mostly a personal exercise to learn clang tooling
infrastructure; I'm uploading it in case anyone else wants to double
check my logic or build something similar. It's heavily based on the
[clang AST visitor example](http://clang.llvm.org/docs/RAVFrontendAction.html)
and [Eli Bendersky's samples](https://github.com/eliben/llvm-clang-samples).

## Usage

You must have a source checkout of llvm, clang, and libc++, and a build
of llvm (the libraries and llvm-config) and the clang libraries. Edit
the variables at the start of the Makefile to point to the appropriate
paths on your system. You must also have compiled lit and FileCheck to
run the tests.

Run `make bad-visibility-finder` to compile the tool, and run it using
`./bad-visibility-finder source.cpp --`. Run `make check` to run tests
and `make run-bad-visibility-finder` (or just `make`) to run the checker
against all libc++ headers.

I've tested on macOS 10.12 and CentOS 7 against tip of tree llvm as of
4th December 2016. All bets are off on other systems.
