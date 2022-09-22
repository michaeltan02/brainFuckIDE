CC=gcc
CFLAGS=-I.

michaelBfIDE: michaelBfIDE.o stacks.o erow.o
	${CC} -o michaelBfIDE michaelBfIDE.o stacks.o erow.o -Wall -Wextra -pedantic

michaelBfIDE.o: michaelBfIDE.c config.h
	${CC} -c michaelBfIDE.c
	
stacks.o: stacks.c config.h
	${CC} -c stacks.c

erow.o: erow.c config.h
	${CC} -c erow.c


