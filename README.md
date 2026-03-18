# The-Expression-tree
# Description
This C program reads a sequence of commands from an input file (input.txt), processes arithmetic expressions in infix, prefix (Polish), or postfix (Reverse Polish) notation, and outputs the results to output.txt. It supports integer arithmetic, variables, and a set of operators including addition, subtraction, multiplication, division, modulo, exponentiation, unary minus, and factorial. The program also tracks dynamic memory allocations (malloc, realloc, free) and writes a memory usage report to memstat.txt.

# Features
Multiple notations: Parse and evaluate expressions in infix, prefix (load_prf), and postfix (load_pst) forms.

Arithmetic operators: +, -, *, /, %, ^ (exponentiation), unary minus, and factorial !.

Integer arithmetic: Operates on int values with overflow checking for addition, subtraction, multiplication, exponentiation, and factorial.

Variable support: Single‑letter variables (a–z) can be assigned values in the eval command.

Memory tracking: Counts calls to malloc, realloc, and free; writes statistics to memstat.txt.

Command‑driven input: Processes commands from input.txt line by line.

Error handling: Detects invalid expressions, missing variables, division by zero, overflow, and syntax errors, printing appropriate messages (incorrect, not_loaded, error).

# Requirements
C compiler with support for the C99 standard (or later).

Standard C library.

# Installation
Clone the repository or download the source file lab_expr.c.

Compile the program using a C compiler. For example, with gcc:

bash
gcc -o expr_parser lab_expr.c -std=c99
Prepare an input.txt file with the desired commands (see Usage below).

Run the program:

bash
./expr_parser
Results will be written to output.txt and memory statistics to memstat.txt.

# License
This project is provided for educational purposes. No explicit license is specified. The code may be used and modified at your own risk.

