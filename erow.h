#pragma once
/*** erow struct, a dynamic row of text, supports tabs and syntax highlighting ***/
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "config.h"

typedef struct erow {
    int size;
    char * chars;
    int rsize;
    char * render;
} erow;

void rowInsertChar(erow * this, int at, int c);
void rowDelChar(erow* this, int at);
void rowAppendString(erow* this, char* s, size_t len);
void rowUpdate(erow* this);
void rowFree(erow* this);
