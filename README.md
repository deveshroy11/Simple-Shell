# Simple Shell

A basic Unix-like command-line shell implemented in C. It supports:

- Command execution (`ls`, `cat`, etc.)
- Background process execution using `&`
- Command history
- Pipeline support using `|`
- Custom signal handling for `Ctrl+C` (SIGINT)

---


To build the shell:

Compilation:
gcc -o simple_shell simple_shell.c
Usage:
./simple_shell
