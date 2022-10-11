/*** includes ***/
#define _DEFAULT_SOURCE
#define _BSD_SOURCE
#define _GNU_SOURCE

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <termio.h>
#include <ctype.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <time.h>
#include <stdarg.h>
#include <fcntl.h>

#include "config.h"
#include "stacks.h"
#include "erow.h"

/*** Definitions ***/
#define CTRL_KEY(k) ((k) & 0x1f)
#define MICHAEL_IDE_VER "1.0"

enum editorKey {
    BACKSPACE = 127,
    ARROW_UP = 1000,
    ARROW_LEFT,
    ARROW_DOWN,
    ARROW_RIGHT,
    CTRL_LEFT,
    CTRL_RIGHT,
    CTRL_UP,
    CTRL_DOWN,
    DEL_KEY,
    HOME_KEY,
    END_KEY,
    PAGE_UP,
    PAGE_DOWN,
    F1_FUNCTION_KEY,
    F2_FUNCTION_KEY,
    F3_FUNCTION_KEY,
    F4_FUNCTION_KEY,
    F5_FUNCTION_KEY,
    F6_FUNCTION_KEY,
    F7_FUNCTION_KEY,
    F8_FUNCTION_KEY,
    F9_FUNCTION_KEY,
    F10_FUNCTION_KEY,
    ALT_S,
    ALT_F
};

typedef enum windowType {
    TEXT_EDITOR = 1,
    DATA_ARRAY,
    OUTPUT
} windowType;

typedef enum topMode {
    EDIT = 1,
    DEBUG,
    EXECUTE
} topMode;

/*** brainfuck logic ***/
typedef enum debuggerMode {
    PAUSED = 0,
    STEP_BY_STEP,
    CONTINUE,
    STEPPING_OUT,
    EXECUTION_ENDED
} debuggerMode;

typedef struct brainFuckModule {
    unsigned char * dataArray;
    int arraySize;
    int arrayIndex;

    debuggerMode debugMode;

    int instX, instY;
    long long instCounter;
    
    coordStack bracketStack;
    bool regenerateStack;

    char* errorMsg;
} brainFuckModule;

bool resetBrainfuck(bool initializing, brainFuckModule* this);
void processBrainFuck(brainFuckModule* this);
void instForward(); //igore comments
void brainfuckDie(char* error, brainFuckModule* this);
void regenerateBracketStack(int stopPointX, int stopPointY, brainFuckModule* this);
bool brainfuckGetByte(unsigned char * dataPtr, brainFuckModule * this);

/*** Window ***/
typedef struct window {
    windowType type;
    int startPosX, startPosY;
    int cx, cy;
    int rx;
    int numRows;
    int numCells;
    int windowRows;
    int windowCols;
    int startRow, startCol;
    erow* row; //dynamic array
    int dirty;
} window;

void windowReset(windowType givenType, window* this);

// row-based operation
void windowInsertRow(int at, char*s, size_t len, window* this);
void windowDelRow(int at, window* this);
int windowCxToRx(erow* row, int cx);
int dataArrayCxToIndex();

// window operation
void windowInsertChar(int c, window* this, bool saveUndo);
void windowInsertNewLine(window* this, bool saveUndo);
void windowDelChar(window* this, bool saveUndo);

void windowScroll(window* this);
void dataArrayScroll(window* this);
void windowMoveCursor(int key, window* this);
void dataArrayMoveCursor(int key, window* this); // basically virtual func of above

void editorUndo();
void editorFind();
void editorFindCallback(char * querry, int key);
char * stringToLowerCase(char * source); // need to free returned buffer
bool editorLoopValidityCheck(window * this); // return if it's valid

// file i/o 
void editorOpen(char* fileName);
char* windowRowToString(int* bufLen, window* this);
void editorSave();

/*** Global Input & Output ***/
void processKeypress(void);
char* promptInput(char* prompt, int inputSizeLimit , void (*callback)(char *, int)); 
    // Need to free the returned buffer. -1 for no limit. Callback func get current buffer and new key after each key stroke

typedef struct abuf {
    char* b;
    int len;
} abuf;

#define ABUF_INIT {NULL, 0}
void abAppend(struct abuf* ab, const char* s, int len);
void abFree(struct abuf* ab) {free(ab->b);}

void globalRefreshScreen(void);
void setGlobalCursor(abuf* ab, int x, int y); //set cursor to arbitary position, no rule checking
void drawEditor(abuf* ab);
void drawBorder(abuf* ab);
void drawDataArray(abuf* ab);
void drawOutput(abuf* ab);
void drawStatusBar(abuf* ab);
void drawMessageBar(abuf* ab);
void setStatusMessage(const char* fmt, ...);

/*** Global environmnet ***/
struct globalEnvironment {
    // information
    struct termios orig_termio;
    int fullScreenRows;
    int fullScreenCols;
    
    //windows
    window E;
    window O;
    window dataArray;
    brainFuckModule B;

    char* fileName;
    char statusMsg [500]; // bottleneck for status message length
    time_t statusMsg_time;

    windowType activeWindow;
    window* activeWindowPtr;
    topMode currentMode;

    undoStack editorUndoStack;

    bool highlightBF;
    bool showArraryAsChar;
    bool caseMatchSearch;
};

void globalInit(void);
void selectSyntaxHighlight();

// terminal
void enableRawMode(void);
void disableRawMode(void);
void die(const char* s);
int readKey(void); // add support for new special keys here
int getWindowSize (int* rows, int* cols);
int getCursorPosition(int* rows, int* cols);

// managing modes and windows
void modeSwitcher(topMode nextMode);
void activeWindowSwicher(windowType nextWindow);
void updateWindowSizes();

/*** Global Struct declaration ***/
struct globalEnvironment G;

int main(int argc, char* argv[]) {
    enableRawMode();
    globalInit();
    // editorOpen("demo/mandelbrot.bf"); //for testing in VScode
    if (argc >= 2){ //command would be kilo fileName, kilo would be first argument
        editorOpen(argv[1]);
    }

    setStatusMessage("F1 = Help | Ctrl-S = Save | Ctrl-Q = Quit | Ctrl-F = Find | Ctrl-Z = Undo | F5 = Debug");

    while (1) {
        globalRefreshScreen();
        int storedDirty = G.E.dirty;
        processKeypress();
        if (G.E.dirty != storedDirty) { //not sure if this is better than keeping it all in processKeyPress
            G.B.regenerateStack = true; 
        }
    }

    return 0;
}

/*** Barinfuck logic functions ***/
bool resetBrainfuck(bool initializing, brainFuckModule* this){
    this->arrayIndex = 0;
    
    this->debugMode = PAUSED;

    this->instCounter = 0;

    this->instX = 0;
    this->instY = 0;

    if(!coordStackInit(initializing, &this->bracketStack) ) {
        return false;
    };

    this->regenerateStack = false;

    this->errorMsg = NULL;

    this->arraySize = BRAINFUCK_ARRAY_START_SIZE;
    if (!initializing) {
        free(this->dataArray);
    }
    this->dataArray = malloc(BRAINFUCK_ARRAY_START_SIZE);
    if (!this->dataArray) { // this really shouldn't happen
        this->arraySize = 0;
        return false;
    }
    memset(this->dataArray, 0, BRAINFUCK_ARRAY_START_SIZE);
    return true;
}

void processBrainFuck(brainFuckModule* this) {
    //validate inst
    if (this->instY >= G.E.numRows || this->debugMode == EXECUTION_ENDED) {
        this->debugMode = EXECUTION_ENDED;

        if (this->errorMsg) {
            setStatusMessage("Execution finished. \x1b[7;31mError: %s\x1b[0m\x1b[7m Press F8 to restart, F9 to quit to editor", this->errorMsg);
        }
        else {
            if (this->regenerateStack) {
                regenerateBracketStack(this->instX, this->instY, this);
            }

            if (coordStackIsEmpty(&this->bracketStack)) {
                setStatusMessage("Execution finished. Press F8 to restart, F9 to quit to editor");
            }
            else {
                G.E.cx = coordStackTop(&this->bracketStack).x;
                G.E.cy = coordStackTop(&this->bracketStack).y;
                brainfuckDie("Opening bracket not closed.", this);
            }
        }
        return;
    }

    if (this->instX >= G.E.row[this->instY].size) {
        // This gets triggered on empty line. Can also happen when user delete stuff in run time
        instForward();
        return;
    }

    char curInst = G.E.row[this->instY].chars[this->instX];

    //validate data cell
    if (this->arrayIndex < 0 || this->arrayIndex >= this->arraySize) {
        this->debugMode = EXECUTION_ENDED;
        // preseumably there should be an error message set already
        return;
    }

    unsigned char* dataPtr = &(this->dataArray[this->arrayIndex]);

    switch(curInst){
        case '>':
            if (this->arrayIndex + 1 >= this->arraySize ) {
                // allocate more memory
                unsigned char * temp = realloc(this->dataArray, this->arraySize + BRAINFUCK_ARRAY_INCREASE_INCREMENT);
                if (temp) {
                    this->dataArray = temp;
                    this->arraySize += BRAINFUCK_ARRAY_INCREASE_INCREMENT;
                    memset(&this->dataArray[this->arrayIndex + 1], 0, BRAINFUCK_ARRAY_INCREASE_INCREMENT);
                    this->debugMode = PAUSED;
                    setStatusMessage("%d more bytes allocated for brainfuck", BRAINFUCK_ARRAY_INCREASE_INCREMENT);
                }
            }

            if (this->arrayIndex + 1 >= this->arraySize ) {
                brainfuckDie("Failed to allocate more memory for brainfuck.", this);
                return;
            }
            else {
                this->arrayIndex++;
                instForward();
            }
            break;
        case '<':
            if (this->arrayIndex - 1 < 0) {
                brainfuckDie("Attemped to go out of array's lower bound.", this);
                return;
            }
            else {
                this->arrayIndex--;
                instForward();
            }
            break;
        case '+':
            (*dataPtr)++;
            instForward();
            break;
        case '-':
            (*dataPtr)--;
            instForward();
            break;
        case '.':
            switch (*dataPtr) {
                case '\n': //ASCII 10
                    windowInsertNewLine(&G.O, false);
                    globalRefreshScreen();
                    break;
                case 8: //backspace symbol
                case BACKSPACE:
                    windowDelChar(&G.O, false);
                    break;
                default:
                    if ((*dataPtr > 31 && *dataPtr < 127) || *dataPtr =='\t') { //tab is ASCII 9
                        windowInsertChar(*dataPtr, &G.O, false);
                    }
                    break;
            }
            instForward();
            break;
        case ',': 
            {
                if (brainfuckGetByte(dataPtr, this)) {
                    instForward();
                }
            }
            break;
        case '[':
            if(*dataPtr){
                //add bracket loc to stack
                if (!coordStackPush(this->instX, this->instY, &this->bracketStack)) {
                    brainfuckDie("Loop stack overflow.", this);
                }
                instForward();
            }
            else{
                //skip till matching closing bracket
                coordStack stackForSkippping;
                coordStackInit(true, &stackForSkippping);
                //traverse inst till condition met
                instForward();
                bool skipping = true;
                while (skipping) {
                    if (this->instY >= G.E.numRows) {
                        skipping = false;
                        brainfuckDie("Missing closing bracket.", this);
                        break;
                    }
                    if (this->instX >= G.E.row[this->instY].size) {
                        instForward();
                        continue;
                    }
                    char curInst = G.E.row[this->instY].chars[this->instX];

                    if (curInst == '[') {
                        if (!coordStackPush(this->instX, this->instY, &stackForSkippping)) {
                            brainfuckDie("Loop stack full.", this);
                        }
                        instForward();
                    }
                    else if (curInst == ']') {
                        if (coordStackIsEmpty(&stackForSkippping)) {
                            instForward();
                            skipping = false;
                            break;
                        }
                        else {
                            coordStackPop(&stackForSkippping);
                            instForward();
                        }
                    }
                    else {
                        instForward();
                    }
                }
                coordStackFree(&stackForSkippping);
            }
            break;
        case ']':
            if(*dataPtr){
                if (this->regenerateStack) {
                    regenerateBracketStack(this->instX, this->instY, this);
                }
                
                //go back to last open bracket
                if (!coordStackIsEmpty(&this->bracketStack)) {
                    this->instX = coordStackTop(&this->bracketStack).x;
                    this->instY = coordStackTop(&this->bracketStack).y;
                    instForward(); //don't execute the open bracket again
                }
                else {
                    brainfuckDie("Cannot find opening bracket.", this);
                }
            }
            else{
                //exit loop
                coordStackPop(&this->bracketStack);
                instForward();
            }
            break;
        case '?':
            if (G.currentMode == DEBUG) {
                this->debugMode = PAUSED;
                this->instCounter = 0;
            }
            instForward();
            break;
        case '#': // comment out rest of line
            this->instX = G.E.row[this->instY].size;
            instForward();
            break;
        default:
            instForward();
            break;
    }

    // scroll screen if needed
    if (this->instY >= G.E.startRow + G.E.windowRows || this->instY < G.E.startRow) {
        G.E.cy = this->instY;
    }
    if (this->instY >= G.E.numRows) {
        G.E.cy = G.E.numRows;
    }
    if (this->instX >= G.E.startCol + G.E.windowCols || this->instX <= G.E.startCol) {
        G.E.cx = this->instX;
    }
    
    int curRowInArray = G.B.arrayIndex / G.dataArray.numCells;
    if (curRowInArray >= G.dataArray.startRow + G.dataArray.windowRows || curRowInArray < G.dataArray.startRow) {
        G.dataArray.cy = curRowInArray;
    }

    if (this->debugMode == STEP_BY_STEP) this->debugMode = PAUSED;

    // auto-breakpoint request system
    if (this->debugMode == PAUSED) {
        this->instCounter = 0;
    }
    else {
        ++(this->instCounter);
        if (this->instCounter > BREAK_POINT_REQUEST_THRESHOLD) {
            this->instCounter = 0;
            this->debugMode = PAUSED;
            setStatusMessage("Interpreter executed too long (>%.1E instructions). Breakpoint requested", BREAK_POINT_REQUEST_THRESHOLD);
        }
    }
    
}

void instForward() { //igores comments
    char nextChar = '\0';
    while (nextChar != '+' && nextChar != '-' && nextChar != '>' && nextChar != '<' &&
            nextChar != '.' && nextChar != ',' && nextChar != '[' && nextChar != ']' && nextChar != '?') {
        erow *row = (G.B.instY >= G.E.numRows) ? NULL : & G.E.row[G.B.instY];
        //there is a - 1 because we don't need it to go 1 space outside the current line
        if (row && G.B.instX < row->size - 1 ) {
            G.B.instX++;
        }
        else if (row && G.B.instX >= row->size - 1) {
            G.B.instY++;
            G.B.instX = 0;
        }

        erow *nextRow = (G.B.instY >= G.E.numRows) ? NULL : & G.E.row[G.B.instY];
        if (nextRow == NULL) return;
        if (G.B.instX < nextRow->size) {
            nextChar = nextRow->chars[G.B.instX];
            if (nextChar == '#') {
                G.B.instX = nextRow->size;
            }
        }
        else {
            nextChar = '\0';
        }
    }
}

void brainfuckDie(char* error, brainFuckModule* this) {
    this->debugMode = EXECUTION_ENDED;
    setStatusMessage("\x1b[7;31mError: %s\x1b[0m\x1b[7m", error);
    this->errorMsg = error;
    globalRefreshScreen();
}

void regenerateBracketStack(int stopPointX, int stopPointY, brainFuckModule* this) {
    if (!this->regenerateStack) return;

    coordStackInit(false, &this->bracketStack);
    this->instX = 0;
    this->instY = 0;
    
    //traverse till we reach the closing bracket
    bool skipping = true;
    while (!(this->instY == stopPointY && this->instX == stopPointX)) {
        //validate inst just in case
        if (this->instY >= G.E.numRows) {
            return;
        }
        if (this->instX >= G.E.row[this->instY].size) {
            instForward();
            continue;;
        }

        char curInst = G.E.row[this->instY].chars[this->instX];
        if (curInst == '[') {
            if (!coordStackPush(this->instX, this->instY, &this->bracketStack)) {
                brainfuckDie("Loop stack full.", this);
            }
        }
        else if (curInst == ']') {
            coordStackPop(&this->bracketStack); // don't check if ] is valid
        }
        instForward();
    }
    this->regenerateStack = false;
}

bool brainfuckGetByte(unsigned char * dataPtr, brainFuckModule * this) {
    char * userInput = NULL;
    bool validInput = false;
    while (!validInput) {
        userInput = promptInput("Enter value for selected cell (8-bit alphanumeric only, ESC to cancel): %s", 3, NULL); 
        if(userInput) {
            if (userInput[0] >= 48 && userInput[0] <= 57) {
                // First char is number
                int potentialNum = atoi(userInput);
                if(potentialNum == 0 && (userInput[0] != '0')) {
                    //shouldn't ever happen
                    validInput = false;
                    free(userInput);
                    userInput = NULL;
                    setStatusMessage("Invalid input");
                    globalRefreshScreen();
                    readKey();
                }
                else if (potentialNum > 255) {
                    validInput = false;
                    free(userInput);
                    userInput = NULL;
                    setStatusMessage("Invalid input -- Number exceeded 8-bit limit");
                    globalRefreshScreen();
                    readKey();
                }
                else {
                    validInput = true;
                    *dataPtr = potentialNum;
                }
            }
            else {
                *dataPtr = userInput[0];
                validInput = true;
            }
        }
        else {
            //user canceled. So just go back to editor and pause without doing anything
            this->debugMode = PAUSED;
            return false;
        }
    }

    free(userInput);
    userInput = NULL;
    return true;
}

/*** Window ***/
void windowReset(windowType givenType, window* this) {
    this->type = givenType;
    if (this->type != DATA_ARRAY) {
        for (int i = 0; i < this->numRows; i++) {
            rowFree(&this->row[i]);
        }
    }
    free(this->row);
    this->row = NULL;

    this->cx = 0;
    this->cy = 0;
    this->rx = 0;
    this->startRow = 0;
    this->startCol = 0;
    this->numRows = 0;
    this->dirty = 0;
}

// row-based operations
void windowInsertRow(int at, char* s, size_t len, window* this) {
    if (at < 0 || at > this->numRows) return;
    //allocate memory for a new row
    erow* temp = realloc(this->row, sizeof(erow) * (this->numRows + 1));
    if (temp == NULL) die("windowInserRow");
    this->row = temp;
    memmove(&this->row[at + 1], &this->row[at], sizeof(erow) * (this->numRows - at));

    this->row[at].size = len;
    this->row[at].chars = (char*) malloc(len + 1);
    memcpy(this->row[at].chars, s, len);
    this->row[at].chars[len] = '\0'; //we do this manually cuz we stripped off the end of the line (\r\n)

    this->row[at].rsize = 0;
    this->row[at].render = NULL;
    rowUpdate(&(this->row[at]));

    this->numRows++;
    this->dirty++;
}

void windowDelRow(int at, window* this) {
    if (at < 0 || at >= this->numRows) return;
    rowFree(&this->row[at]);
    memmove(&this->row[at], &this->row[at + 1], sizeof(erow) * (this->numRows - at - 1));
    this->numRows--;
}

int windowCxToRx(erow* row, int cx) {
    int rx = 0;
    for (int j = 0; j < cx; j++) { //check everything before cx for tabs
        if (row->chars[j] == '\t') {
            rx += (TAB_STOP - 1) - (rx % TAB_STOP); 
            //(TAB_STOP-1) and the rx++ gives the full tab, then -(rx%TAB_STOP) cuz we don't always need a full tab
            //remmber that each tab is just 1 char in text file, this calculate how many spaces that tab actually should be
        } 
        rx++;
    }
    return rx;
}

int dataArrayCxToIndex() {
    return (G.dataArray.cy * G.dataArray.numCells + G.dataArray.cx);
}

// window operations
void windowInsertChar(int c, window* this, bool saveUndo) {
    if (this->cy == this->numRows) {
        windowInsertRow(this->numRows,"", 0, this);
    }
    rowInsertChar(&this->row[this->cy], this->cx, c);

    if (G.currentMode == DEBUG && this->type == TEXT_EDITOR) {
        if (this->cy == G.B.instY && this->cx <= G.B.instX) {
            G.B.instX++;
        }
    }

    this->cx++;
    this->dirty++;

    if (saveUndo) {
        undoStackPush(G.E.cx, G.E.cy, ADDITION, '\0', &G.editorUndoStack);
    }
}

void windowInsertNewLine(window* this, bool saveUndo) {
    erow* row = &this->row[this->cy];
    if (this->cx == 0) {
        windowInsertRow(this->cy, "", 0, this);
    }
    else {
        windowInsertRow(this->cy + 1, &row->chars[this->cx], row->size - this->cx, this);
        row = &this->row[this->cy]; //in case realloc change address of things
        row->size = this->cx;
        row->chars[row->size] = '\0';
        rowUpdate(row);
    }
    
    // auto-indent
    int numTab = 0;
    if (AUTO_INDENT && this->type == TEXT_EDITOR) {
        for (int i = 0; i < this->cx; i++) { //check everything before cx for tabs
            if (row->chars[i] == '\t') {
                numTab++;
            }
            else {
                break;
            }
        }
    }

    if (G.currentMode == DEBUG && this->type == TEXT_EDITOR) {
        if (this->cy == G.B.instY && this->cx <= G.B.instX) {
            G.B.instX -= this->cx;
            G.B.instY++;
        }
        else if (this->cy < G.B.instY) {
            G.B.instY++;
        }
    }

    this->cy++;
    this->cx = 0;

    if (saveUndo) {
        undoStackPush(G.E.cx, G.E.cy, LINE_BREAK, '\0', &G.editorUndoStack);
    }

    while (AUTO_INDENT && numTab) {
        windowInsertChar('\t', this, true);
        numTab--;
    }
}

void windowDelChar(window* this, bool saveUndo) {
    if (this->cy == this->numRows) return;
    if (this->cx == 0 && this->cy == 0) return;

    if (this->cx > 0) {
        char delChar = this->row[this->cy].chars[this->cx - 1];
        rowDelChar(&this->row[this->cy], this->cx - 1);

        if (G.currentMode == DEBUG && this->type == TEXT_EDITOR) {
            if (this->cy == G.B.instY && this->cx <= G.B.instX) {
                G.B.instX--;
            }
        }

        this->cx--;

        if (saveUndo) {
            undoStackPush(G.E.cx, G.E.cy, DELETION, delChar, &G.editorUndoStack);
        }
    }
    else {
        if (G.currentMode == DEBUG && this->type == TEXT_EDITOR) {
            if (this->cy == G.B.instY) {
                G.B.instX += this->row[this->cy - 1].size;
                G.B.instY--;
            }
            else if (this->cy < G.B.instY) {
                G.B.instY--;
            }
        }

        this->cx = this->row[this->cy - 1].size;
        rowAppendString(&this->row[this->cy - 1], this->row[this->cy].chars, this->row[this->cy].size);
        windowDelRow(this->cy, this);
        this->cy--;

        if (saveUndo) {
            undoStackPush(G.E.cx, G.E.cy, LINE_JOIN, '\0', &G.editorUndoStack);
        }
    }
    this->dirty++;
}

void windowScroll(window* this) {
    if (this->type == DATA_ARRAY) {
        dataArrayScroll(this);
        return;
    }

    //first process tabs. Each time E.cx get change, we will get to here, and then calculate the correct E.rx to show
    this->rx = 0;
    if (this->cy < this->numRows) {
        this->rx = windowCxToRx(&this->row[this->cy], this->cx);
    }

    //now is actual scrolling
    if (this->cy < this->startRow) {
        this->startRow = this->cy;
    }
    if (this->cy >= this->startRow + this->windowRows) {
        this->startRow = this->cy - this->windowRows + 1;
    }

    if (this->rx < this->startCol) {
        this->startCol = this->rx;
    }
    if (this->rx >= this->startCol + this->windowCols) {
        this->startCol = this->rx - this->windowCols + 1;
    }
}

void dataArrayScroll(window* this) {
    if (this-> type != DATA_ARRAY) return;

    if (this->cy < this->startRow) {
        this->startRow = this->cy;
    }
    if (this->cy >= this->startRow + this->windowRows) {
        this->startRow = this->cy - this->windowRows + 1;
    }

    G.dataArray.rx = G.dataArray.cx * 4 + 1; 
}

void windowMoveCursor(int key, window* this){
    if (this->type == DATA_ARRAY) {
        dataArrayMoveCursor(key, this);
        return;
    }

    erow* row = (this->cy >= this->numRows) ? NULL : & this->row[this->cy];

    switch (key) {
        case ARROW_UP:
            if (this->cy > 0) this->cy--;
            break;
        case ARROW_LEFT:
            if (this->cx != 0) {
                this->cx--;
            } 
            else if (this->cy > 0) {
                this->cy--;
                this->cx = this->row[this->cy].size;
            }
            break;
        case ARROW_DOWN:
            if (this->cy < this->numRows) this->cy++;
            break;
        case ARROW_RIGHT:
            if (row && this->cx < row->size ) {
                this->cx++;
            }
            else if (this->cy < this->numRows && this->cx == row->size) {
                this->cy++;
                this->cx = 0;
            }
            break;
    }
    //correct cursor if needed
    row = (this->cy >= this->numRows) ? NULL : & this->row[this->cy];
    int rowLen = row ? row->size : 0;
    if (this->cx > rowLen) {
        this->cx = rowLen;
    }
}

void dataArrayMoveCursor(int key, window* this){ //this of this as virtual func of above
    if (this->type != DATA_ARRAY) return;

    switch (key) {
        case ARROW_UP:
            if (this->cy > 0) this->cy--;
            break;
        case ARROW_LEFT:
            if (this->cx != 0) {
                this->cx--;
            } 
            else if (this->cy > 0) {
                this->cy--;
                this->cx = this->numCells - 1;
            }
            break;
        case ARROW_DOWN:
            if (this->cy < this->numRows - 1) this->cy++;
            break;
        case ARROW_RIGHT:
            if (this->cx < this->numCells - 1 ) {
                this->cx++;
            }
            else if (this->cy < this->numRows - 1 && this->cx == this->numCells - 1) {
                this->cy++;
                this->cx = 0;
            }
            break;
    }

    if (this->cx > this->numCells - 1) {
        this->cx = this->numCells - 1;
    }
}

void editorUndo() {
    if (G.activeWindow != TEXT_EDITOR) return;

    if (!undoStackIsEmpty(&G.editorUndoStack)) {
        undoStruct actionToUndo = undoStackPop(&G.editorUndoStack);
        //do a check here, just in case
        G.E.cx = actionToUndo.x;
        G.E.cy = actionToUndo.y;
        
        switch (actionToUndo.action) {
            case ADDITION:
                windowDelChar(&G.E, false);
                setStatusMessage("Undid addition");
                break;
            case LINE_BREAK:
                //might want to check if cx is 0
                windowDelChar(&G.E, false);
                setStatusMessage("Undid line break");
                break;
            case DELETION:
                windowInsertChar(actionToUndo.delChar, &G.E, false);
                setStatusMessage("Undid deletion");
                break;
            case LINE_JOIN:
                windowInsertNewLine(&G.E, false);
                setStatusMessage("Undid line join");
                break;
        }
    }
    else {
        setStatusMessage("Nothing to undo");
    }
}

void editorFind() {
    int saved_cx = G.E.cx;
    int saved_cy = G.E.cy;
    int saved_startCol = G.E.startCol;
    int saved_startRow = G.E.startRow;

    char * querry = promptInput("Search: %s (Use ESC/Arrows/Enter)", -1, editorFindCallback);
    if (querry) {
        free(querry);
    }
    else if (CANCEL_SEARCH_RESTORE_CURSOR) { // canceled search
        G.E.cx = saved_cx;
        G.E.cy = saved_cy;
        G.E.startCol = saved_startCol;
        G.E.startRow = saved_startRow;
    }
}

void editorFindCallback(char * querry, int key) {
    static int last_match = -1; // for directional search
    static int direction = 1;

    if (key == '\x1b' || key == '\r' || key == '\n') {
        last_match = -1;
        direction = 1;
        return;
    }
    else if (key == ARROW_RIGHT || key == ARROW_DOWN) {
        direction = 1;
    } 
    else if (key == ARROW_LEFT || key == ARROW_UP) {
        direction = -1;
    }
    else {
        last_match = -1;
        direction = 1;
    }

    if (last_match == -1) direction = 1;
    int current = last_match;
    
    if (querry && querry[0]) { // don't do this if nothing is typed
        for (int i = 0; i < G.E.numRows; i++) {
            current += direction;
            if (current <= -1) {
                current = G.E.numRows - 1;
            }
            else if (current >= G.E.numRows) {
                current = 0;
            }

            erow * row = &G.E.row[current];

            char * lowerQuerry = NULL;
            char * lowerRow = NULL;
            if (!G.caseMatchSearch) {
                lowerQuerry = stringToLowerCase(querry);
                lowerRow = stringToLowerCase(row->chars);
            }

            // char * match = strstr(row->chars, querry);
            char * match = G.caseMatchSearch ? strstr(row->chars, querry) : strstr(lowerRow, lowerQuerry);

            if (match) {
                last_match = current;
                G.E.cy = current;
                G.E.cx = G.caseMatchSearch? match - row->chars : match - lowerRow;

                G.E.startRow = G.E.numRows; // this way windowScroll will put matched string on top of window
                if (G.E.cx >= G.E.startCol + G.E.windowCols) {
                    G.E.startCol = G.E.row[i].size;
                }

                if (lowerQuerry) free(lowerQuerry);
                if(lowerRow) free(lowerRow);
                break;
            }

            if (lowerQuerry) free(lowerQuerry);
            if(lowerRow) free(lowerRow);
        }
    }

}

char * stringToLowerCase(char * source) {
    int length = strlen(source);
    char * lowerCaseVer = malloc(length + 1);
    if (lowerCaseVer) {
        for (int i = 0; i < length; i++) {
            lowerCaseVer[i] = tolower(source[i]);
        }
        lowerCaseVer[length] = '\0'; 
        return lowerCaseVer;
    }
    else {
        return source;
    }
}

bool editorLoopValidityCheck(window * this) { // this can maybe replace regenerate brainfuck stack? 
    coordinate returnCoord = {-1, -1};
    if (this->type != TEXT_EDITOR) return false;
    if (PERFORM_VALIDITY_CHECK == 0) return true;

    coordStack errorCheckStack;
    coordStackInit(true, &errorCheckStack);

    for (int i = 0; i < this->numRows; i++) {
        erow * row = &this->row[i];
        for (int j = 0; j < row->size; j++) {
            if (row->chars[j] == '[') {
                coordStackPush(j, i, &errorCheckStack);
            }
            else if (row->chars[j] == ']') {
                if (coordStackIsEmpty(&errorCheckStack)) {
                    this->cx = j;
                    this->cy = i;
                    coordStackFree(&errorCheckStack);
                    setStatusMessage("\x1b[7;31mError: Cannot find opening bracket for {%d,%d}.\x1b[7m", j, i);
                    return false;
                }
                else {
                    coordStackPop(&errorCheckStack);
                }
            }
            else if (row->chars[j] == '#') {
                break;
            }
        }
    }
    
    if (!coordStackIsEmpty(&errorCheckStack)) {
        this->cx = coordStackTop(&errorCheckStack).x;
        this->cy = coordStackTop(&errorCheckStack).y;
        setStatusMessage("\x1b[7;31mError: {%d,%d} has no closing bracket.\x1b[7m", this->cx, this->cy);
        coordStackFree(&errorCheckStack);
        return false;
    }
    else {
        coordStackFree(&errorCheckStack);
        return true;
    }
}

// file i/o
void editorOpen(char* fileName) {
    free(G.fileName);
    G.fileName = strdup(fileName);
    selectSyntaxHighlight();

    FILE *fp = fopen(fileName, "r");
    if (!fp) die("fopen");
    char* line = NULL;
    size_t lineCap = 0; //cuz getline can be more lineCap than needed
    ssize_t lineLen; 
    
    while ((lineLen = getline(&line, &lineCap, fp)) != -1) {
        while (lineLen > 0 && (line[lineLen-1] == '\n' || line[lineLen-1] == '\r') ) { //strip off change line
            lineLen--;
        }
        windowInsertRow(G.E.numRows, line, lineLen, &G.E);
    }

    free(line);
    fclose(fp);
    G.E.dirty = 0;
}

char* windowRowToString(int* bufLen, window* this) {
    int totalLen = 0;
    int j;
    for (j = 0; j < this->numRows; j++) {
        totalLen += this->row[j].size + 1;  //+ 1 is for \n
    }
    if (bufLen != NULL)*bufLen = totalLen;

    char* buf = malloc(totalLen);
    char* p = buf;
    for (j = 0; j < this->numRows; j++) {
        memcpy(p, this->row[j].chars, this->row[j].size);
        p += this->row[j].size;
        *p = '\n';
        p++;
    }
    return buf;
}

void editorSave() {
    if (G.fileName == NULL) {
        G.fileName = promptInput("Save as (ESC to cancel): %s", 255, NULL);
        if (G.fileName == NULL) {
            setStatusMessage("Save aborted");
            return;
        }
        else if (strstr(G.fileName, "/")) {
            setStatusMessage("\x1b[7;31mError: file name cannot contarin '/'. Save aborted\x1b[7m");
            G.fileName = NULL;
            return;
        }
        selectSyntaxHighlight();
    }

    int len;
    char* buf = windowRowToString(&len, &G.E);

    int fd = open("michaelBFIDE_SAVE_BUFFER", O_RDWR | O_CREAT, 0644);  //RDWR for read & write, CREAT means create file if not exist, 0644 set permission
    if (fd != -1) {
        if (ftruncate(fd, len) != -1) {
        //ftruncate changes file sizze of len. Doing this instead of reseting entire file means we lose less stuff if write() fail. Better soln would be to use a buffer file 
            if (write(fd, buf, len) == len) {
                free(buf);
                close(fd);
                remove(G.fileName); //gcc does this automatically for rename(), Visual Studio doesn't
                if (rename("michaelBFIDE_SAVE_BUFFER", G.fileName) != -1) {
                    G.E.dirty = 0;
                    setStatusMessage("%d bytes written to disk", len);
                    return;
                }
                else {
                    setStatusMessage("\x1b[7;31mSave buffer cannot be renamed.\x1b[7m Error code: %s", strerror(errno));
                    return;
                }
            }
        }
        close(fd);
    }

    free(buf);
    setStatusMessage("\x1b[7;31mCan't save!\x1b[7m I/O errror: %s", strerror(errno));
}

/*** Global Input & Output ***/
// input
void processKeypress() {
    static int curQuitTimes = QUIT_TIMES;   //preserve value even after going out of scope. Don't get re-initialized
    
    int c = readKey();

    switch (c) {
        case '\r':
            if (G.activeWindow == TEXT_EDITOR) {
                windowInsertNewLine(&G.E, true);
                G.B.regenerateStack = true;
            }
            break;
        case CTRL_KEY('q'):
            if (G.E.dirty && curQuitTimes > 0) {
                setStatusMessage("Warning!!! File has unsaved changes. Press Ctrl-Q %d more times to quit.", curQuitTimes);
                curQuitTimes--;
                return;
            }
            write(STDOUT_FILENO, "\x1b[2J", 4); 
            write(STDOUT_FILENO, "\x1b[H", 3);
            exit(0);
            break;
        case CTRL_KEY('s'):
            editorSave();
            break;
        case ALT_S: 
            {
                // save-as
                char * backUpName = G.fileName;
                G.fileName = NULL;
                editorSave();
                if (G.fileName == NULL) {
                    G.fileName = backUpName;
                }
                selectSyntaxHighlight();
            }
            break;
        case CTRL_KEY('p'):
            //manually set brainfuck inst ptr to cursor location
            if (G.currentMode == DEBUG && G.B.debugMode != EXECUTION_ENDED) {
                G.B.instX = G.E.cx;
                G.B.instY = G.E.cy;
                setStatusMessage("Instruction jumped to (%d, %d)", G.B.instX, G.B.instY);
                regenerateBracketStack(G.B.instX, G.B.instY, &G.B);
            }
            break;
        case CTRL_KEY('c'):
            {
                // set data array cell of cursor
                if (G.activeWindow == DATA_ARRAY) {
                    int cursorIndex = dataArrayCxToIndex();
                    if (cursorIndex < G.B.arraySize) {
                        if (brainfuckGetByte(&G.B.dataArray[cursorIndex], &G.B)) {
                            setStatusMessage("Set cell %d to %d", cursorIndex, G.B.dataArray[cursorIndex]);
                        }
                    }
                }
            }
            break;
        case CTRL_KEY('j'):
            // snap cursor to where they shoudl be
            if (G.currentMode != EDIT) {
                G.E.cx = G.B.instX;
                G.E.cy = G.B.instY;

                if (G.B.arrayIndex == 0) {
                    G.dataArray.cy = 0;
                    G.dataArray.cx = 0;
                }
                else if (G.B.arrayIndex && G.dataArray.numCells) {
                    G.dataArray.cy = G.B.arrayIndex / G.dataArray.numCells;
                    G.dataArray.cx = G.B.arrayIndex % G.dataArray.numCells;
                }

                setStatusMessage("Snapped cursors to current inst/cell");
            }
            break;
        case CTRL_KEY('w'):
            //swiching active window
            if (G.currentMode != EDIT) {
                switch (G.activeWindow) {
                    case TEXT_EDITOR:
                        activeWindowSwicher(OUTPUT);
                        setStatusMessage("Switched to output window");
                        break;
                    case OUTPUT:
                        activeWindowSwicher(DATA_ARRAY);
                        setStatusMessage("Switched to data array window");
                        break;
                    case DATA_ARRAY:
                        activeWindowSwicher(TEXT_EDITOR);
                        setStatusMessage("Switched to editor window");
                        break;
                }
            }
            break;
        case CTRL_KEY('z'):
            if (G.activeWindow == TEXT_EDITOR) {
                editorUndo();
            }
            break;
        case CTRL_KEY('f'):
            // search
            if (G.activeWindow == TEXT_EDITOR) {
                G.caseMatchSearch = false;
                editorFind();
            }
            break;
        case ALT_F:
            // search
            if (G.activeWindow == TEXT_EDITOR) {
                G.caseMatchSearch = true;
                editorFind();
            }
            break;
        case CTRL_KEY('n'):
            if (G.currentMode != EDIT) {
                G.showArraryAsChar = G.showArraryAsChar ? 0 : 1;
            }
            break;
        case PAGE_UP:
        case PAGE_DOWN: 
            {
                //move to top/bottom of page, then scroll an entire page
                if (c == PAGE_UP) {
                    G.activeWindowPtr->cy = G.activeWindowPtr->startRow;
                }
                else if (c == PAGE_DOWN) {
                    G.activeWindowPtr->cy = G.activeWindowPtr->startRow + G.activeWindowPtr->windowRows - 1;
                    if (G.activeWindowPtr->cy > G.activeWindowPtr->numRows) G.activeWindowPtr->cy = G.activeWindowPtr->numRows;
                }

                for (int times = G.activeWindowPtr->windowRows - 1; times > 0; times--) {
                    windowMoveCursor( (c == PAGE_UP) ? ARROW_UP : ARROW_DOWN, G.activeWindowPtr);
                }
            }
            break;
        case HOME_KEY:
            G.activeWindowPtr->cx = 0;
            break;
        case END_KEY:
            if (G.activeWindow == TEXT_EDITOR || G.activeWindow == OUTPUT) {
                if (G.activeWindowPtr->cy < G.activeWindowPtr->numRows) {
                    G.activeWindowPtr->cx = G.activeWindowPtr->row[G.activeWindowPtr->cy].size;
                }
            }
            else if (G.activeWindow == DATA_ARRAY) {
                G.activeWindowPtr->cx = G.activeWindowPtr->numCells - 1;
            }
            break;
        case BACKSPACE:
        case CTRL_KEY('h'):
        case DEL_KEY:
            if (G.activeWindow == TEXT_EDITOR) {
                if (c == DEL_KEY) windowMoveCursor(ARROW_RIGHT, &G.E);
                windowDelChar(&G.E, true);
                G.B.regenerateStack = true; 
            }
            break;
        case ARROW_UP:
        case ARROW_LEFT:
        case ARROW_DOWN:
        case ARROW_RIGHT:
            windowMoveCursor(c, G.activeWindowPtr);
            break;
        case CTRL_UP:
        case CTRL_DOWN:
            break;
        case CTRL_LEFT:
            while (G.activeWindow == TEXT_EDITOR || G.activeWindow == OUTPUT) {
                windowMoveCursor(ARROW_LEFT, G.activeWindowPtr);
                if (G.activeWindowPtr->cy < G.activeWindowPtr->numRows) {
                    char curChar = G.activeWindowPtr->row[G.activeWindowPtr->cy].chars[G.activeWindowPtr->cx - 1];  //there are so many edge cases and stuff to check, like "", &&
                    if (curChar == ' ' || curChar == '(' ||
                        curChar == ')' || curChar == '[' ||
                        curChar == ']' || curChar == '.' ||
                        curChar == '#' || curChar == '"' ||
                        curChar == '\t' ||
                        G.activeWindowPtr->row[G.activeWindowPtr->cy].chars[G.activeWindowPtr->cx] == '"'||
                        G.activeWindowPtr->cx == 0) break;
                }
                else {
                    break;
                }
            }
            break;
        case CTRL_RIGHT:
            while (G.activeWindow == TEXT_EDITOR || G.activeWindow == OUTPUT){
                windowMoveCursor(ARROW_RIGHT, G.activeWindowPtr);
                if (G.activeWindowPtr->cy < G.activeWindowPtr->numRows) {
                    char curChar = G.activeWindowPtr->row[G.activeWindowPtr->cy].chars[G.activeWindowPtr->cx];
                    if (curChar == ' ' || curChar == '(' ||
                        curChar == ')' || curChar == '[' ||
                        curChar == ']' || curChar == '.' ||
                        curChar == '&' || curChar == ',' ||
                        curChar == ':' || curChar == '"' ||
                        curChar == '\t' ||
                        (G.activeWindowPtr->row[G.activeWindowPtr->cy].chars[G.activeWindowPtr->cx-1] == '/' && G.activeWindowPtr->row[G.activeWindowPtr->cy].chars[G.activeWindowPtr->cx-2] == '/')||
                        G.activeWindowPtr->row[G.activeWindowPtr->cy].chars[G.activeWindowPtr->cx-1] == '"'||
                        G.activeWindowPtr->cx == G.activeWindowPtr->row[G.activeWindowPtr->cy].size ) break;
                }
                else {
                    break;
                }
            }
            break;
        case F1_FUNCTION_KEY:
            if (G.currentMode == EDIT) {
                setStatusMessage("HELP: Ctrl-S = Save | Ctrl-Q = Quit | Ctrl-F = Find | Ctrl-Z = Undo | F5 = Debug | F6 = Execute");
            }
            else {
                setStatusMessage("HELP: F5 = Continue | F6 = Step Into | F7 = Step Out | F8 = Restart | F9 = Stop | Ctrl-N = Toggle | Ctrl-J = Snap | Ctrl-C = Set Cell | Ctrl-P = Set Inst");
            }
            break;
        case F2_FUNCTION_KEY:
        case F3_FUNCTION_KEY:
        case F4_FUNCTION_KEY:
            break;
        case F5_FUNCTION_KEY:
            if (G.currentMode == EDIT) {
                // enter debug mode
                if (editorLoopValidityCheck(&G.E)) {
                    if (resetBrainfuck(false, &G.B)) {
                        modeSwitcher(DEBUG);
                        setStatusMessage("F1 = Help | F5 = Continue | F6 = Step Into | F7 = Step Out | F8 = Restart | F9 = Stop | Ctrl-N = Toggle | Ctrl-J = Snap | Ctrl-C = Set Cell | Ctrl-P = Set Inst");
                    }
                    else {
                        setStatusMessage("Cannot allocate starting memory for brainfuck");
                    }
                }
            }
            else if (G.currentMode != EDIT) {
                //continue
                if (G.B.debugMode == EXECUTION_ENDED) {
                    processBrainFuck(&G.B);
                }
                else {
                    G.B.debugMode = CONTINUE;
                    globalRefreshScreen();
                    while (G.B.debugMode == CONTINUE) {
                        processBrainFuck(&G.B);
                    }
                }
            }
            break;
        case F6_FUNCTION_KEY:
            if (G.currentMode == EDIT) {
                // enter execute mode
                if (editorLoopValidityCheck(&G.E)) {
                    resetBrainfuck(false, &G.B);
                    modeSwitcher(EXECUTE);
                    G.B.debugMode = CONTINUE;
                    globalRefreshScreen();
                    while (G.B.debugMode == CONTINUE) {
                        processBrainFuck(&G.B);
                    }
                }
            }
            else if (G.currentMode == DEBUG) {
                // step into
                if (G.B.debugMode != EXECUTION_ENDED) {
                    G.B.debugMode = STEP_BY_STEP;
                }
                processBrainFuck(&G.B);
            }
            break;
        case F7_FUNCTION_KEY:
            {
                //step out
                if (G.currentMode != EDIT  && G.B.debugMode == EXECUTION_ENDED) {
                    processBrainFuck(&G.B); //just to display error message
                }
                else if (G.currentMode == DEBUG  && G.B.debugMode != EXECUTION_ENDED) {
                    if (G.B.regenerateStack) {
                        regenerateBracketStack(G.B.instX, G.B.instY, &G.B);
                    }
                    bool inALoop = !coordStackIsEmpty(&G.B.bracketStack);
                    globalRefreshScreen();
                    if (inALoop) {
                        int curStackSize = G.B.bracketStack.top;
                        G.B.debugMode = STEPPING_OUT;
                        globalRefreshScreen();
                        while (G.B.debugMode == STEPPING_OUT && G.B.bracketStack.top != curStackSize - 1) {
                            processBrainFuck(&G.B);
                        }
                        G.B.debugMode = PAUSED;
                    }
                    else {
                        setStatusMessage("Not in loop");
                    }
                }
            }
            break;
        case F8_FUNCTION_KEY:
            //restart
            if (G.currentMode != EDIT) {
                G.E.cx = 0;
                G.E.cy = 0;
                resetBrainfuck(false, &G.B);
                windowReset(OUTPUT, &G.O);
                windowReset(DATA_ARRAY, &G.dataArray);
                setStatusMessage("");
                globalRefreshScreen();
            }
            break;
        case F9_FUNCTION_KEY:
            // quit to editor
            activeWindowSwicher(TEXT_EDITOR);
            if (G.currentMode != EDIT) {
                modeSwitcher(EDIT);
                resetBrainfuck(false, &G.B);
                windowReset(OUTPUT, &G.O);
                windowReset(DATA_ARRAY, &G.dataArray);
                setStatusMessage("");
            }
            break;
        case F10_FUNCTION_KEY:
            break;
        case CTRL_KEY('l'): 
        case '\x1b':
            break;
        //bracket auto-complete
        case '(':
        case '{':
        case '[': {
            if (G.activeWindow == TEXT_EDITOR) {
                windowInsertChar(c, &G.E, true);
                char curChar = G.E.row[G.E.cy].chars[G.E.cx];
                //if cx is at end of row, curChar would be '\0'
                if (AUTO_COMPLETE_BRACKET && (curChar == '\0' || curChar == ' ' || curChar == ')' || curChar == ']') ) {
                    switch (c) {
                        case '(':
                            windowInsertChar(')', &G.E, true);
                            break;
                        case '{':
                            windowInsertChar('}', &G.E, true);
                            break;
                        case '[':
                            windowInsertChar(']', &G.E, true);
                            break;
                        case '"':
                        case 39:
                            if (curChar == c) {
                                windowMoveCursor(ARROW_RIGHT, &G.E);
                            }
                            else {
                                windowInsertChar(c, &G.E, true);
                            }
                            break;
                    }
                    windowMoveCursor(ARROW_LEFT, &G.E);
                }
                G.B.regenerateStack = true; 
            }
            break;
        }
        case ')':
        case '}':
        case ']': {
            if (G.activeWindow == TEXT_EDITOR) {
                if (AUTO_COMPLETE_BRACKET && G.E.cy < G.E.numRows) {
                    char curChar = G.E.row[G.E.cy].chars[G.E.cx];
                    if (curChar == c) {
                        windowMoveCursor(ARROW_RIGHT, &G.E);
                    }
                    else {
                        windowInsertChar(c, &G.E, true);
                    }
                }
                else {
                    windowInsertChar(c, &G.E, true);
                }
                G.B.regenerateStack = true; 
            }
            break;
        }
        case '"':
        case 39: {    // this char is '
            if (G.activeWindow == TEXT_EDITOR) {
                if (AUTO_COMPLETE_BRACKET && G.E.cy < G.E.numRows) {
                    char curChar = G.E.row[G.E.cy].chars[G.E.cx];
                    if (curChar == c) {
                        windowMoveCursor(ARROW_RIGHT, &G.E);
                    }
                    else if (curChar == '\0' || curChar == ' ' || curChar == ')' || curChar == ']') {
                        windowInsertChar(c, &G.E, true);
                        windowInsertChar(c, &G.E, true);
                        windowMoveCursor(ARROW_LEFT, &G.E);
                    }
                    else {
                            windowInsertChar(c, &G.E, true);
                    }
                }
                else {
                    windowInsertChar(c, &G.E, true);
                }
                G.B.regenerateStack = true; 
            }
            break;
        }
        default:
            if (G.activeWindow == TEXT_EDITOR) {
                if ( (c > 31 && c < 127) || c == '\t') {
                    windowInsertChar(c, &G.E, true);
                    G.B.regenerateStack = true; 
                }
            }
            break;
    }
    curQuitTimes = QUIT_TIMES;
}

char* promptInput(char * prompt, int inputSizeLimit , void (*callback)(char *, int)){
    size_t bufSize = 128;
    char * buf = malloc(bufSize);   //our dynamic input buffer

    size_t bufLen = 0;
    buf[0] = '\0';

    while (1) {
        setStatusMessage(prompt, buf);
        globalRefreshScreen();

        int c = readKey();
        if (c == DEL_KEY || c == CTRL_KEY('h') || c == BACKSPACE) { //presed backspace
            if (bufLen != 0) {
                bufLen--;
                buf[bufLen] = '\0';
            }
        }
        else if (c == '\x1b') {  //pressed esc
            setStatusMessage("");
            if (callback) callback(buf, c);
            free(buf);
            return NULL;
        }
        else if (c == '\r') {   //pressed enter
            if (bufLen != 0) {
                setStatusMessage("");
                if (inputSizeLimit != -1 && bufLen > inputSizeLimit) {
                    setStatusMessage("Input size limit (%d characters) reached. Please delete some characters", inputSizeLimit);
                    globalRefreshScreen();
                    readKey();
                }
                else {
                    if (callback) callback(buf, c);
                    return buf;
                }
            }
        }
        else if (!iscntrl(c) && c < 128) {  //entered valid char
            if (bufLen == bufSize - 1) {    //expand buffer if needed
                bufSize *= 2;
                buf = realloc(buf, bufSize);
            }
            buf[bufLen] = c;
            bufLen++;
            buf[bufLen] = '\0';
        }

        if ( (c > 31) && callback ) callback(buf, c); // excluded control char
    }
}

// output
void abAppend(struct abuf * ab, const char * s, int len) {
    char * new = (char *) realloc(ab->b, ab->len + len);
    if (new == NULL) return;

    memcpy(&new[ab->len], s, len);
    ab->b = new;
    ab->len += len;
}

void globalRefreshScreen(){
    windowScroll(&G.E);
    if (G.currentMode != TEXT_EDITOR) { 
        windowScroll(&G.dataArray);
        windowScroll(&G.O);
    }

    struct abuf ab = ABUF_INIT;

    abAppend(&ab, "\x1b[?25l", 6); //hide cursor
    
    drawEditor(&ab);
    if (G.currentMode != EDIT) {
        drawBorder(&ab);
        drawDataArray(&ab);
        drawOutput(&ab);
    }
    drawStatusBar(&ab);
    drawMessageBar(&ab);
    
    int activeWindowX = G.activeWindowPtr->rx - G.activeWindowPtr->startCol + 1 + G.activeWindowPtr->startPosX;
    int activeWindowY = G.activeWindowPtr->cy - G.activeWindowPtr->startRow + 1 + G.activeWindowPtr->startPosY;
    setGlobalCursor(&ab, activeWindowX, activeWindowY);
    
    abAppend(&ab, "\x1b[?25h", 6);
    write(STDOUT_FILENO, ab.b, ab.len);
    abFree(&ab);

}

void setGlobalCursor(struct abuf * ab, int x, int y) {
    char buf[32];
    snprintf(buf, sizeof(buf),"\x1b[%d;%dH", y, x);
    abAppend(ab, buf, strlen(buf));
}

void drawEditor(struct abuf * ab) {
    abAppend(ab, "\x1b[H", 3);  //moves cursor position to 0,0
    for (int y = 0; y < G.E.windowRows; y++){
        int fileRow = G.E.startRow + y;
        if (fileRow >= G.E.numRows) {
            if (G.E.numRows == 0 && y == G.E.windowRows / 3) {  //welcome message
                char welcome[80];
                int welcomelen = snprintf(welcome, sizeof(welcome), "Michael's Brainfuck IDE -- version %s", MICHAEL_IDE_VER);

                int padding = (G.E.windowCols - welcomelen) / 2;
                if (padding != 0) {
                    abAppend(ab, "~", 1);
                    padding--;
                }
                for (int i = padding; i > 0; i--) abAppend(ab, " ", 1);

                if (welcomelen > G.E.windowCols) welcomelen = G.E.windowCols;
                abAppend(ab, welcome, welcomelen);
            }
            else {
                abAppend(ab, "~", 1);
            }
        }
        else {
            int len = G.E.row[fileRow].rsize - G.E.startCol;
            if (len < 0) len = 0; // just display nothing
            if (len > G.E.windowCols) len = G.E.windowCols;

            // basic syntax highlighting
            char* lineToPrint = &G.E.row[fileRow].render[G.E.startCol];
            int rxOfInst = windowCxToRx(&G.E.row[fileRow] , G.B.instX);
            for (int j = 0; j < len; j++) { // on window, this doesn't print last letter in the row for snme reason
                if (G.highlightBF) {
                    switch(lineToPrint[j]){
                        case '>':
                        case '<':
                            //cyan();
                            abAppend(ab, "\x1b[36m", 5);
                            break;
                        case '+':
                        case '-':
                            //white();
                            abAppend(ab, "\x1b[37m", 5);
                            break;
                        case '.':
                        case ',':
                            //purple();
                            abAppend(ab, "\x1b[35m", 5);
                            break;
                        case '[':
                        case ']':
                            //yellow();
                            abAppend(ab, "\x1b[33m", 5);
                            break;
                        case '?':
                            //red();
                            abAppend(ab, "\x1b[31m", 5);
                            break;
                        default:
                            //blue();
                            abAppend(ab, "\x1b[34m", 5);
                            break;
                    }
                }
                if (G.currentMode == DEBUG && fileRow == G.B.instY && rxOfInst == G.E.startCol + j) {
                    abAppend(ab, "\x1b[7;33m", 7);
                }
                abAppend(ab, &lineToPrint[j], 1);
                abAppend(ab, "\x1b[0m", 4);
            }
        }

        abAppend(ab, "\x1b[K", 3);  // clear line right of cursor
        abAppend(ab, "\r\n", 2);
    }
}

void drawBorder(struct abuf * ab){
    setGlobalCursor(ab, G.E.windowCols + 1, 0);
    for (int y = 0; y < G.E.windowRows; y++) {
        abAppend(ab, "|\x1b[D\x1b[B", 7);
    }
    setGlobalCursor(ab, 0, G.E.windowRows + 1);
    for (int x = 0; x < G.fullScreenCols; x++) {
        abAppend(ab, "=", 1);
    }
}

void drawDataArray(struct abuf * ab){
    setGlobalCursor(ab, G.dataArray.startPosX, G.dataArray.startPosY);

    char buf[10];
    
    int cellToHighlight = G.B.arrayIndex;
    int index = G.dataArray.startRow * G.dataArray.numCells;

    for(int i = 0; i < G.dataArray.windowRows; i++) {
        for (int j = 0; j < G.dataArray.numCells; j++) {
            if (index == cellToHighlight) {
                abAppend(ab, "\x1b[7;33m", 7);
            }

            if (index < G.B.arraySize) {
                if (G.showArraryAsChar) {
                    if (G.B.dataArray[index] > 31 && G.B.dataArray[index] < 127) {
                        if (G.B.dataArray[index] >= 48 && G.B.dataArray[index] <= 57)
                            snprintf(buf, sizeof(buf)," #%c|", G.B.dataArray[index]);
                        else {
                            snprintf(buf, sizeof(buf),"  %c|", G.B.dataArray[index]);
                        }
                    }
                    else if (G.B.dataArray[index] == 10) {
                        snprintf(buf, sizeof(buf)," \\n|");
                    }
                    else if (G.B.dataArray[index] == 8 || G.B.dataArray[index] == 127) {
                        snprintf(buf, sizeof(buf)," BS|");
                    }
                    else if (G.B.dataArray[index] == 9) {
                        snprintf(buf, sizeof(buf)," \\t|");
                    }
                    else {
                        snprintf(buf, sizeof(buf),"%3d|", G.B.dataArray[index]);
                    }
                }
                else {
                    snprintf(buf, sizeof(buf),"%3d|", G.B.dataArray[index]);
                }
            }
            else {
                snprintf(buf, sizeof(buf),"   |");
            }
            abAppend(ab, buf, strlen(buf));

            if (index == cellToHighlight) {
                abAppend(ab, "\x1b[m", 3);
            }

            index++;
        }
        //clear right of screen, then move down
        abAppend(ab, "\x1b[K", 3);
        setGlobalCursor(ab, G.dataArray.startPosX, i + 2); //screen is actually 1-based. 0 is just default value, which just happens to move start of line/col
    }
}

void drawOutput(struct abuf * ab) {
    setGlobalCursor(ab, G.O.startPosX, G.O.startPosY);
    for (int y = -1; y < G.O.windowRows; y++){
        int outRow = G.O.startRow + y;
        if (y == -1) { // "Output:" in bold (use -1 so we don't mess up outRow)
            abAppend(ab, "\x1b[1;37m", 7);
            abAppend(ab, "Output:\x1b[0m", 11);
        }
        else {
            if (outRow < G.O.numRows) {
                // manually highlight cursor here if it's not the active window
                int len = G.O.row[outRow].rsize - G.O.startCol;
                if (len < 0) len = 0;
                if (len > G.O.windowCols) len = G.O.windowCols;
                // abAppend(ab, &G.O.row[outRow].render[G.O.startCol], len);
                // highlights where cursor should be
                char* lineToPrint = &G.O.row[outRow].render[G.O.startCol];
                G.O.rx = windowCxToRx(&G.O.row[outRow], G.O.cx);
                for (int j = 0; j < len; j++) {
                    if (G.activeWindow != OUTPUT && outRow == G.O.cy && G.O.startCol + j == G.O.rx) {
                        abAppend(ab, "\x1b[7m", 4); // when cursor is be on top a char
                    }
                    abAppend(ab, &lineToPrint[j], 1);
                    abAppend(ab, "\x1b[0m", 4);
                }
                if (G.activeWindow != OUTPUT && outRow == G.O.cy && len == G.O.rx) {
                    abAppend(ab, "\x1b[7m \x1b[0m" , 9); // when cursor is after end of line
                }
            }
            else {
                if (G.activeWindow != OUTPUT && outRow == G.O.cy) {
                    abAppend(ab, "\x1b[7m~\x1b[0m" , 9); // when cursor is moved to a tilde
                }
                else {
                    abAppend(ab, "~", 1);
                }
                
            }
        }
        abAppend(ab, "\x1b[K\r\n", 5);  // clear line right of cursor
    }
}

void drawStatusBar(struct abuf * ab) {
    setGlobalCursor(ab, 0, G.fullScreenRows - 1);
    
    abAppend(ab, "\x1b[7m", 4); // invert color, will also use for selection
    // file name
    char status[80], rightStatus[80];
    int len = snprintf(status, sizeof(status), "%.20s - %d lines %s", (G.fileName ? G.fileName : "No Name"), G.E.numRows, (G.E.dirty ? "(modified)" : "") );
    if (len > G.fullScreenCols) len = G.fullScreenCols;
    abAppend(ab, status, len);

    int rightLen;
    if (G.currentMode == EDIT) {
        rightLen = snprintf(rightStatus, sizeof(rightStatus), "%d/%d", G.E.cy + 1 , G.E.numRows);
    }
    else {
        char * mode = NULL;
        switch (G.B.debugMode) {
            case PAUSED:
                mode = "Paused";
                break;
            case STEP_BY_STEP:
                mode = "Step by Step";
                break;
            case CONTINUE:
                mode = "Continue";
                break;
            case STEPPING_OUT:
                mode = "Step Out";
                break;
            case EXECUTION_ENDED:
                mode = G.B.errorMsg ? "Brainfucked" : "Finished";
                break;
        }
        rightLen = snprintf(rightStatus, sizeof(rightStatus), 
                    "Line %d/%d | Debugger Mode: %s | Cell %d/%d", G.E.cy + 1 , G.E.numRows, mode, 
                    G.activeWindow == DATA_ARRAY ? dataArrayCxToIndex() : G.B.arrayIndex, G.B.arraySize);
    }
    //fill rest of screen with spaces for the white bar effect
    while (len < G.fullScreenCols) {
        if (G.fullScreenCols - len == rightLen) {
            abAppend(ab, rightStatus, rightLen);
            break;
        }
        else {
            abAppend(ab, " ", 1);
            len++;
        }
    }
    abAppend(ab, "\x1b[m", 3);
    abAppend(ab, "\r\n", 2);
}

void drawMessageBar(struct abuf * ab) {
    abAppend(ab, "\x1b[7m", 4);

    abAppend(ab, "\x1b[K", 3);
    int msgLen = strlen(G.statusMsg);
    if (msgLen > G.fullScreenCols) msgLen = G.fullScreenCols;
    time_t curTime = time(NULL) - G.statusMsg_time;
    if ( ( (time(NULL) - G.statusMsg_time) < 5 ) && msgLen) {
        abAppend(ab, G.statusMsg, msgLen);
    }
    abAppend(ab, "\x1b[m", 3);
}

void setStatusMessage(const char * fmt, ...) {
    //This is how you do function with varied amount of argument (variadic) like printf, 
    va_list ap;
    va_start(ap, fmt); //initalizes ap with start of variable list
    //nomrally you'd then use va_arg(va_list, <varl type>) to access next argument, but we just use vsnprintf to automatically parse fmt in the printf way
    vsnprintf(G.statusMsg, sizeof(G.statusMsg), fmt, ap);
    va_end(ap); //clean up for va_start
    G.statusMsg_time = time(NULL); //current time, in seconds since midnight Jan 1, 1970
}

/*** Global environmnet ***/
void globalInit() {
    resetBrainfuck(true, &G.B);
    windowReset(OUTPUT, &G.O);
    windowReset(TEXT_EDITOR, &G.E);
    windowReset(DATA_ARRAY, &G.dataArray);

    G.currentMode = EDIT;
    updateWindowSizes();

    G.fileName = NULL;
    G.statusMsg[0] = '\0';
    G.statusMsg_time = 0;
    G.activeWindow = TEXT_EDITOR;
    G.activeWindowPtr = &G.E;

    undoStackInit(&(G.editorUndoStack));

    G.highlightBF = false;
    G.showArraryAsChar = false;
    G.caseMatchSearch = false;
}

void selectSyntaxHighlight() {
    if (G.fileName == NULL) return;

    if (strstr(G.fileName, ".bf") || strstr(G.fileName, ".b")) {
        G.highlightBF = true;
    }
}

// terminal
void enableRawMode() {
    if (tcgetattr(STDIN_FILENO, &G.orig_termio) == -1) {
        die("tcgetattr");
    }
    atexit(disableRawMode); // it's cool that this can be placed anywhere

    struct termios raw = G.orig_termio;
    raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    raw.c_oflag &= ~(OPOST);
    raw.c_cflag |= (CS8);
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);

    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 1;    // adds a timeout to read (in 0.1s)

    if(tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) { // TCSAFLUCH defines how the change is applied
        die("tcsetattr"); 
    }
}

void disableRawMode(void) {
    if(tcsetattr(STDIN_FILENO, TCSAFLUSH, &G.orig_termio) == -1) {
        die("tcsetattr");   
    }
}

void die(const char* s) { //error handling
    write(STDOUT_FILENO, "\x1b[2J", 4);
    write(STDOUT_FILENO, "\x1b[H", 3);
    perror(s);
    write(STDOUT_FILENO, "\r", 1);
    exit(1);
}

int readKey(void) {
    int nread;
    char c;
    int oldWidth, oldLength;
    while ((nread = read(STDIN_FILENO, &c, 1)) != 1) {
        if(nread == -1 && errno != EAGAIN) die("read");
        //experiement with auto updating screen when resizing happen
        oldWidth = G.fullScreenCols;
        oldLength = G.fullScreenRows;
        updateWindowSizes();
        if (G.fullScreenCols != oldWidth || G.fullScreenRows != oldLength) {
            globalRefreshScreen();
        }
    }
    if (c == '\x1b') {
        char seq[5];
        if (read(STDIN_FILENO, &seq[0], 1) != 1) return '\x1b';
        
        switch(seq[0]) {
            case 's': return ALT_S;
            case 'f': return ALT_F;
        }
        // if (seq[0] == 's') return ALT_S;

        if (read(STDIN_FILENO, &seq[1], 1) != 1) return '\x1b';

        if (seq[0] == '[') {
            if (seq[1] >= '0' && seq[1] <= '9') {
                if (read(STDIN_FILENO, &seq[2], 1) != 1) return '\x1b';
                if (seq[2] == '~') {
                    switch (seq[1]) {
                        case '1': return HOME_KEY;
                        case '3': return DEL_KEY;
                        case '4': return END_KEY;
                        case '5': return PAGE_UP;
                        case '6': return PAGE_DOWN;
                        case '7': return HOME_KEY;
                        case '8': return END_KEY;
                    }
                }
                else if (seq[2] == ';') {
                    if (read(STDIN_FILENO, &seq[3], 2) != 2) return '\x1b'; //this means something went wrong
                    if (seq[3] == '5') { //ctrl key pressed, seq is esc[1;5C, C is from arrow key, 1 is unchanged default keycode (used by page up/down and delete), 5 is modifier meaning ctrl
                                        //https://en.wikipedia.org/wiki/ANSI_escape_code#:~:text=0%3B%0A%7D-,Terminal%20input%20sequences,-%5Bedit%5D
                        switch (seq[4]) {
                            case 'A': return CTRL_UP;
                            case 'B': return CTRL_DOWN;
                            case 'C': return CTRL_RIGHT;
                            case 'D': return CTRL_LEFT;
                        }
                    }
                }
                else if (read(STDIN_FILENO, &seq[3], 1) == 1) {
                    if (seq[3] == '~' && seq[1] == '1') {
                        switch (seq[2]) {
                            case '5': return F5_FUNCTION_KEY;
                            case '7': return F6_FUNCTION_KEY;
                            case '8': return F7_FUNCTION_KEY;
                            case '9': return F8_FUNCTION_KEY;
                        }
                    }
                    else if (seq[3] == '~' && seq[1] == '2' && seq[2] == '0') {
                        return F9_FUNCTION_KEY;
                    }
                }
            }
            else {
                switch (seq[1]) {
                    case 'A': return ARROW_UP;
                    case 'B': return ARROW_DOWN;
                    case 'C': return ARROW_RIGHT;
                    case 'D': return ARROW_LEFT;
                    case 'H': return HOME_KEY;
                    case 'F': return END_KEY;
                }
            }
        }
        else if (seq[0] == '0') {
            switch (seq[1]) {
                case 'H': return HOME_KEY;
                case 'F': return END_KEY;
            }
        }
        else if (seq[0] == 'O') {
            switch (seq[1]) {
                case 'P': return F1_FUNCTION_KEY;
                case 'Q': return F2_FUNCTION_KEY;
                case 'R': return F3_FUNCTION_KEY;
                case 'S': return F4_FUNCTION_KEY;
                case 't': return F5_FUNCTION_KEY;
                case 'u': return F6_FUNCTION_KEY;
                case 'v': return F7_FUNCTION_KEY;
                case 'l': return F8_FUNCTION_KEY;
                case 'w': return F9_FUNCTION_KEY;
                case 'x': return F10_FUNCTION_KEY;
            }
        }

        return '\x1b';
    }
    else {
        return c;
    }
}

int getWindowSize (int* rows, int* cols) {
    struct winsize ws;
    if(ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
        if (write(STDOUT_FILENO, "\x1b[999C\x1b[999B", 12) != 12) return -1;  //try move cursor a ton
        return getCursorPosition(rows, cols);  
    }
    else {
        *rows = ws.ws_row;
        *cols = ws.ws_col;
        return 0;
    }
}

int getCursorPosition(int* rows, int* cols) {
    char buf[32];
    unsigned int i = 0;
    if (write(STDOUT_FILENO, "\x1b[6n", 4) != 4) return -1; //ask terminal for cursor loc

    for (i = 0; i < sizeof(buf) - 1; i++) {
        if (read(STDIN_FILENO, &buf[i], 1) != 1) break;
        if (buf[i] == 'R') break;
    }
    buf[i] = '\0';
    //printf("\r\n%s\r\n", &buf[1]);
    if (buf[0] != '\x1b' || buf[1] != '[') return -1;
    if (sscanf(&buf[2], "%d;%d", rows, cols) !=2) return -1;
}


// modes and windows
void modeSwitcher(topMode nextMode) {
    G.currentMode = nextMode;
    write(STDOUT_FILENO, "\x1b[2J", 4);    //clear entire screen
    updateWindowSizes();
    globalRefreshScreen();
}

void activeWindowSwicher(enum windowType nextWindow) {
    G.activeWindow = nextWindow;
    switch (nextWindow) {
        case TEXT_EDITOR:
            G.activeWindowPtr = &G.E;
            break;
        case DATA_ARRAY:
            G.activeWindowPtr = &G.dataArray;
            break;
        case OUTPUT:
            G.activeWindowPtr = &G.O;
            break;
    }
}

void updateWindowSizes() { //Chaning window size in debug mode has some funky bugs
    if (getWindowSize(&G.fullScreenRows, &G.fullScreenCols) == -1) {
        die("getWindowSize");
    }

    G.E.startPosX = 0;
    G.E.startPosY = 0;
    switch (G.currentMode) {
        case EDIT:
            G.E.windowCols = G.fullScreenCols;
            G.E.windowRows = G.fullScreenRows - 2;
            break;
        case DEBUG:
        case EXECUTE:
            // G.E.windowCols = (G.fullScreenCols / 2) - 2;
            G.E.windowCols = G.fullScreenCols * EDITOR_WIDTH_IN_DEBUG_MODE - 2;
            // G.E.windowRows = (G.fullScreenRows * 3) / 5 - 2;
            G.E.windowRows = G.fullScreenRows * EDITOR_HEIGHT_IN_DEBUG_MODE - 2;
            break;
    }

    //this doesn't really depend on the mode. They don't get printed in EDIT mode anyway
    if (G.currentMode != EDIT) {
        G.dataArray.startPosX = G.E.windowCols + 2;
        G.dataArray.startPosY = 0;
        G.dataArray.windowCols = G.fullScreenCols - G.E.windowCols - 1;
        G.dataArray.windowRows = G.E. windowRows;

        if (G.dataArray.windowCols)
            G.dataArray.numCells = G.dataArray.windowCols / 4;
        if (G.dataArray.numCells)
            G.dataArray.numRows = G.B.arraySize / G.dataArray.numCells + 1;

        G.O.startPosX = 0;
        G.O.startPosY = G.E.windowRows + 2;
        G.O.windowCols = G.fullScreenCols;
        G.O.windowRows = G.fullScreenRows - G.E.windowRows - 4; //1 for border, 2 for status bar/message , 1 for the Output line
    }
}

