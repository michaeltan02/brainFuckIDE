#pragma once
/*** undo stack, coordinate stack, and their corresponding structs ***/
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "config.h"

#define COORDSTACK_START_SIZE 100
#define COORDSTACK_INCREMENT 100

/*** Coordinate struct and stack ***/
typedef struct coordinate {
    int x;
    int y;
} coordinate;

typedef struct coordStack {
    int top;
    int size;
    coordinate * stackArray;
} coordStack;

// best to seperate init and reset
bool coordStackInit(bool initializing, coordStack* this);
void coordStackFree(coordStack* this);
bool coordStackPush(int x, int y, coordStack* this); // return true on sucess, false on failure
void coordStackPop(coordStack* this);
coordinate coordStackTop(coordStack* this); // return {-1,-1} if stack empty
bool coordStackIsEmpty(coordStack* this);

/*** Undo struct and stack ***/
typedef enum editorAction {
    ADDITION = 0,
    DELETION,
    LINE_BREAK,
    LINE_JOIN
} editorAction;

typedef struct undoStruct {
    int x;
    int y;
    editorAction action;
    char delChar;
} undoStruct;

typedef struct undoStack {
    int top;
    undoStruct stackArray[UNDO_STACK_SIZE];
} undoStack;

void undoStackInit(undoStack* this);
void undoStackPush(int x, int y, editorAction action, char delChar, undoStack* this);
undoStruct undoStackPop(undoStack* this); //return {-1,-1,0,0} if stack empty
bool undoStackIsEmpty(undoStack* this);
