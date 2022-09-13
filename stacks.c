#include "stacks.h"

/*** Coordinate struct and stack ***/
void coordStackInit(coordStack* this) {
    this->top = -1;
    this->size = 100;
    //make this dynamic later
}

bool coordStackPush(int x, int y, coordStack* this) {
    this->top++;
    if (this->top < this->size) {
        this->stackArray[this->top].x = x;
        this->stackArray[this->top].y = y;
    }
    else {
        //allocate more memory when dynamic
        //brainfuckDie("Loop stack full.", &G.B);
        return false;
    }
}

void coordStackPop(coordStack* this) {
    if (this->top > -1) {
        this->top--;
    }
}

coordinate coordStackTop(coordStack* this) {
    if (this->top > -1 && this->top < this->size) {
        return (this->stackArray[this->top]);
    }
    else {
        //brainfuckDie("Tried to pop empty stack. Returned {0,0}.", &G.B);
        coordinate invalidCoord = {-1,-1};
        return invalidCoord;
    }
}

bool coordStackIsEmpty(coordStack* this) {
    return (this->top == -1);
}

/*** undo stack ***/
void undoStackInit(undoStack* this) {
    this->top = -1;
}

void undoStackPush(int x, int y, editorAction action, char delChar, undoStack* this) {
    this->top++;
    if (this->top >= UNDO_STACK_SIZE) {
        if (UNDO_STACK_SIZE % 2) { //size is odd
            memmove(this->stackArray, &(this->stackArray[UNDO_STACK_SIZE / 2 + 1]), (UNDO_STACK_SIZE / 2) * sizeof(undoStruct));
        }
        else { // size is even
            memmove(this->stackArray, &(this->stackArray[UNDO_STACK_SIZE / 2]), (UNDO_STACK_SIZE / 2) * sizeof(undoStruct));
        }
        this->top = UNDO_STACK_SIZE / 2;
    }

    this->stackArray[this->top].x = x;
    this->stackArray[this->top].y = y;
    this->stackArray[this->top].action = action;
    this->stackArray[this->top].delChar = delChar;
}

undoStruct undoStackPop(undoStack* this) {
    undoStruct returnStruct = {-1,-1,ADDITION,'\0'}; // just to have a default
    if (this->top > -1) {
        returnStruct = this->stackArray[this->top];
        this->top--;
    }
    return returnStruct;
}

bool undoStackIsEmpty(undoStack* this) {
    return (this->top == -1);
}