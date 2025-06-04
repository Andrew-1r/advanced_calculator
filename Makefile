# REF: Assistance from ChatGPT for changing Makefile to include tinyexpr library from conversation 21/3/25 at 10:09am.

# Define the compiler.
CC = gcc

# Define the flags for compilation.
CFLAGS = -Wall -pedantic -std=gnu99 -Wextra -I/local/courses/csse2310/include

# Define additional flags for debugging purposes.
DEBUG = -g

# Specify the default target to run.
.DEFAULT_GOAL := uqexpr

# Specify which targets do not generate output files.
.PHONY: debug clean

# The debug target will update compile flags then compile program.
debug: CFLAGS += $(DEBUG)
debug: uqexpr

# uqexpr.o is the target and uqexpr.c is the dependency.
uqexpr.o: uqexpr.c
	$(CC) $(CFLAGS) -c $^ -o $@

# uqexpr is the target and uqexpr.o is the dependency.
uqexpr: uqexpr.o
	$(CC) $(CFLAGS) $^ -o $@ -L/local/courses/csse2310/lib -ltinyexpr -lm

# Remove object and binary files.
clean:
	rm -f uqexpr *.o

.PHONY: all clean
