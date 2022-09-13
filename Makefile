CC=gcc
CFLAGS=-I.

michaelBfIDE: michaelBfIDE.o stacks.o erow.o
	${CC} -o michaelBfIDE michaelBfIDE.o stacks.o erow.o -Wall -Wextra -pedantic
