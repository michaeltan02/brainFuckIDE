CC=gcc
CFLAGS=-I.

michaelBfIDE: michaelBfIDE.o stacks.o
	${CC} -o michaelBfIDE michaelBfIDE.o stacks.o -Wall -Wextra -pedantic
