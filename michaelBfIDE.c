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

/*** Definitions ***/
#define CTRL_KEY(k) ((k) & 0x1f)
#define MICHAEL_IDE_VER "0.9"
#define UNDO_STACK_SIZE 200

// User Tweakable Parameters
#define TAB_STOP 4
#define QUIT_TIMES 2

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
    ALT_S
};

typedef enum windowType {
    TEXT_EDITOR = 1,
    DATA_ARRAY,
    OUTPUT
} windowType;

typedef enum topMode {
    EDIT = 1,
    DEBUG,
    EXECUTE //this is actually needed. Make it convert things to string, and then execute. That might be faster
} topMode;

typedef enum debuggerMode {
    PAUSED = 0,
    STEP_BY_STEP,
    CONTINUE,
    STEPPING_OUT,
    EXECUTION_ENDED
} debuggerMode;

//brainfuck stuff
typedef struct coordinate {
    int x;
    int y;
} coordinate;

typedef struct coordStack {
    int top;
    int size;
    coordinate stackArray[100];
} coordStack;

//stack member functions
void coordStackInit(coordStack* this);
bool coordStackPush(int x, int y, coordStack* this);
void coordStackPop(coordStack* this);
coordinate coordStackTop(coordStack* this); //return {0,0} if stack empty
bool coordStackIsEmpty(coordStack* this);

//brainfuck logic
typedef struct brainFuckModule {
    unsigned char dataArray[30000];
    int arraySize;
    int arrayIndex;

    debuggerMode debugMode;

    int instX, instY;
    long long instCounter;
    
    coordStack bracketStack;
    bool regenerateStack;

    char* errorMsg;
} brainFuckModule;

//brainFuck logic member functions
void resetBrainfuck(brainFuckModule* this);
void processBrainFuck(brainFuckModule* this);
void instForward(); //igore comments
void brainfuckDie(char* error, brainFuckModule* this);
void regenerateBracketStack(int stopPointX, int stopPointY, brainFuckModule* this);

//Windows and I/O
//append buffer (dynamic string)
struct abuf {
    char* b;
    int len;
};

#define ABUF_INIT {NULL, 0}
void abAppend(struct abuf* ab, const char* s, int len);
void abFree(struct abuf* ab) {free(ab->b);}

typedef struct erow {
    int size;
    char* chars;
    int rsize;
    char* render;
} erow;

typedef struct window {
    windowType type;
    int startPosX, startPosY;
    int cx, cy;
    int rx;
    int numRows;
    int numCells;
    int windowRows;
    int windowCols;
    int startRowOrCell, startCol;
    erow* row; //dynamic array
    int dirty;
} window;

//window member functions
void resetWindow(windowType givenType, window* this);

    //row operation
void windowInsertRow(int at, char*s, size_t len, window* this);
void windowUpdateRow(erow* row);
int windowRowCxToRx(erow* row, int cx);
void windowRowInsertChar(erow* row, int at, int c);
void windowRowDelChar(erow* row, int at);
void freeRow(erow* row);
void windowDelRow(int at, window* this);
void windowRowAppendString(erow* row, char* s, size_t len);

    //window operation
void windowInsertChar(int c, window* this, bool saveUndo);
void windowInsertNewLine(window* this, bool saveUndo);
void windowDelChar(window* this, bool saveUndo);
void editorUndo();

    //file i/o
void editorOpen(char* fileName);
char* windowRowToString(int* bufLen, window* this);
void editorSave();

    //input
void processKeypress(void);
void moveCursorChecking(int key, window* this);
void moveCursorCheckingDatayArray(int key, window* this); // basically virtual func of above
char* promptInput(char* prompt, int inputSizeLimit); //our own scanf (-1 for no limit), need to free the returned buffer

    //output
void windowScroll(window* this);
void dataArrayScroll(window* this);
void globalRefreshScreen(void);
void drawEditor(struct abuf* ab);
void drawBorder(struct abuf* ab);
void drawDataArray(struct abuf* ab);
void drawOutput(struct abuf* ab);
void drawStatusBar(struct abuf* ab);
void drawMessageBar(struct abuf* ab);
void setStatusMessage(const char* fmt, ...);
void basicMoveCursor(struct abuf* ab, int x, int y); //move cursor to arbitary position, no rule checking

// Undo struct and stack
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

//stack member functions
void undoStackInit(undoStack* this);
void undoStackPush(int x, int y, editorAction action, char delChar, undoStack* this);
undoStruct undoStackPop(undoStack* this); //return {0,0,0,0} if stack empty
bool undoStackIsEmpty(undoStack* this);

struct globalEnvironment {
    //information
    struct termios orig_termio;
    int fullScreenRows;
    int fullScreenCols;
    
    //windows
    window E;
    window O;
    window dataArray;
    brainFuckModule B;

    char* fileName;
    char statusMsg [255]; //bottleneck for status message length
    time_t statusMsg_time;

    windowType activeWindow;
    window* activeWindowPtr;
    topMode currentMode;

    undoStack editorUndoStack;
};

//Global environmnet member functions
void globalInit(void);

    //terminal
void enableRawMode(void);
void disableRawMode(void);
void die(const char* s);
int readKey(void);
int getWindowSize (int* rows, int* cols);
int getCursorPosition(int* rows, int* cols);

    //modes and windows
void modeSwitcher(topMode nextMode);
void activeWindowSwicher(windowType nextWindow);
void updateWindowSizes();

// I can re-factor this global struct to be declared in main and pass it to functions, but that provides no real advantage for a one-person project of this size
struct globalEnvironment G;

int main(int argc, char* argv[]) {
	enableRawMode();
    globalInit();
    //editorOpen("mandelbrot.bf"); //for testing in VScode
    if (argc >= 2){ //command would be kilo fileName, kilo would be first argument
        editorOpen(argv[1]);
    }

    setStatusMessage("HELP: Ctrl-S = Save | Ctrl-Q = quit");

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

//terminal
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
        if (seq[0] == 's') return ALT_S;
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

//row operation
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
    windowUpdateRow(&(this->row[at]));

    this->numRows++;
    this->dirty++;
}

void windowUpdateRow(erow* row) {
    int j;
    int tabs = 0;
    int extraSpace = 0;
    //find number of tab
    for (j = 0; j < row->size; j++) {
        if (row->chars[j] == '\t') tabs++;
    }
    extraSpace += tabs * (TAB_STOP - 1);

    free (row->render);
    row->render = (char*) malloc(row->size + extraSpace + 1);

    int idx = 0;
    for (j = 0; j < row->size; j++) {
        if (row->chars[j] == '\t') {
            row->render[idx++] = ' ';
            while (idx % TAB_STOP != 0) {
                row->render[idx++] = ' ';
            }
        }
        else {
            row->render[idx] = row->chars[j];
            idx++;
        }
    }
    row->render[idx] = '\0';
    row->rsize = idx;
}

int windowRowCxToRx(erow* row, int cx) {
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

void windowRowInsertChar(erow* row, int at, int c) {
    if (at < 0 || at > row->size) at = row->size; //validate at just in case
    char* temp = realloc(row->chars, row->size + 2); //2 cuz the inserted char, and cuz size doesn't include null terminator
    if (temp == NULL) die("windowRowInsertChar");
    row->chars = temp;

    memmove(&row->chars[at + 1], &row->chars[at], row->size - at + 1); //+1 for null terminator
    row->size++;
    row->chars[at] = c;
    windowUpdateRow(row);
}

void windowRowDelChar(erow* row, int at) {
    if (at < 0 || at >= row->size) return;
    memmove(&row->chars[at], &row->chars[at + 1], row->size - at);
    row->size--;
    windowUpdateRow(row);
}

void freeRow(erow* row) {
    free (row->chars);
    free (row->render);
}

void windowDelRow(int at, window* this) {
    if (at < 0 || at >= this->numRows) return;
    freeRow(&this->row[at]);
    memmove(&this->row[at], &this->row[at + 1], sizeof(erow) * (this->numRows - at - 1));
    this->numRows--;
}

void windowRowAppendString(erow * row, char* s, size_t len) {
    row->chars = (char*) realloc(row->chars, row->size + len + 1);
    memcpy(&row->chars[row->size], s, len);
    row->size += len;
    row->chars[row->size] = '\0';
    windowUpdateRow(row);
}

void windowInsertChar(int c, window* this, bool saveUndo) {
    if (this->cy == this->numRows) {
        windowInsertRow(this->numRows,"", 0, this);
    }
    windowRowInsertChar(&this->row[this->cy], this->cx, c);

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
        windowUpdateRow(row);
    }
    
    // auto-indent
    int numTab = 0;
    if (this->type == TEXT_EDITOR) {
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

    while (numTab) {
        windowInsertChar('\t', this, true);
        numTab--;
    }
}

void windowDelChar(window* this, bool saveUndo) {
    if (this->cy == this->numRows) return;
    if (this->cx == 0 && this->cy == 0) return;

    if (this->cx > 0) {
        char delChar = this->row[this->cy].chars[this->cx - 1];
        windowRowDelChar(&this->row[this->cy], this->cx - 1);

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
        windowRowAppendString(&this->row[this->cy - 1], this->row[this->cy].chars, this->row[this->cy].size);
        windowDelRow(this->cy, this);
        this->cy--;

        if (saveUndo) {
            undoStackPush(G.E.cx, G.E.cy, LINE_JOIN, '\0', &G.editorUndoStack);
        }
    }
    this->dirty++;
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

//file i/o
void editorOpen(char* fileName) {
    free(G.fileName);
    G.fileName = strdup(fileName);

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
        G.fileName = promptInput("Save as (ESC to cancel): %s", 255);
        if (G.fileName == NULL) {
            setStatusMessage("Save aborted");
            return;
        }
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
                    setStatusMessage("Save buffer cannot be renamed. Error code: %s", strerror(errno));
                    return;
                }
            }
        }
        close(fd);
    }

    free(buf);
    setStatusMessage("Can't save! I/O errror: %s", strerror(errno));
}

//input
void processKeypress() {
    static int curQuitTimes = QUIT_TIMES;   //preserve value even after going out of scope. Don't get re-initialized
    
    int c = readKey();

    switch (c) {
        case '\r':
            if (G.activeWindow == TEXT_EDITOR) {
                windowInsertNewLine(&G.E, true);
                G.B.regenerateStack = true;
                //coordStackPush(G.E.cx, G.E.cy, &G.editorUndoStack);

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
            }
            break;
        case CTRL_KEY('p'): // need change for pause
            //manually set brainfuck inst ptr to cursor location
            if (G.currentMode == DEBUG && G.B.debugMode != EXECUTION_ENDED) {
                G.B.instX = G.E.cx;
                G.B.instY = G.E.cy;
                setStatusMessage("Instruction jumped to (%d, %d)", G.B.instX, G.B.instY);
                regenerateBracketStack(G.B.instX, G.B.instY, &G.B);
            }
            break;
        case CTRL_KEY('w'):
            //swiching active window
            if (G.currentMode != EDIT) {
                switch (G.activeWindow) {
                    case TEXT_EDITOR:
                        activeWindowSwicher(OUTPUT);
                        break;
                    case OUTPUT:
                        activeWindowSwicher(DATA_ARRAY);
                        break;
                    case DATA_ARRAY:
                        activeWindowSwicher(TEXT_EDITOR);
                        break;
                }
            }
            break;
        case CTRL_KEY('z'):
            if (G.activeWindow == TEXT_EDITOR) {
                editorUndo();
            }
            break;
        case PAGE_UP:
        case PAGE_DOWN: 
            {
                //move to top/bottom of page, then scroll an entire page
                if (c == PAGE_UP) {
                    G.E.cy = G.E.startRowOrCell;
                }
                else if (c == PAGE_DOWN) {
                    G.E.cy = G.E.startRowOrCell + G.E.windowRows - 1;
                    if (G.E.cy > G.E.numRows) G.E.cy = G.E.numRows;
                }

                for (int times = G.E.windowRows - 1; times > 0; times--) {
                    moveCursorChecking( (c == PAGE_UP) ? ARROW_UP : ARROW_DOWN, &G.E);
                }
            }
            break;
        case HOME_KEY:
            G.E.cx = 0;
            break;
        case END_KEY:
            if (G.E.cy < G.E.numRows) {
                G.E.cx = G.E.row[G.E.cy].size;
            }
            break;
        case BACKSPACE:
        case CTRL_KEY('h'):
        case DEL_KEY:
            if (G.activeWindow == TEXT_EDITOR) {
                if (c == DEL_KEY) moveCursorChecking(ARROW_RIGHT, &G.E);
                windowDelChar(&G.E, true);
                G.B.regenerateStack = true; 
            }
            break;
        case ARROW_UP:
        case ARROW_LEFT:
        case ARROW_DOWN:
        case ARROW_RIGHT:
            if (G.activeWindow == TEXT_EDITOR || G.activeWindow == OUTPUT) {
                moveCursorChecking(c, G.activeWindowPtr);
            }
            else if (G.activeWindow == DATA_ARRAY) {
                moveCursorCheckingDatayArray(c, G.activeWindowPtr);
            }
            break;
        case CTRL_UP:
        case CTRL_DOWN:
            break;
        case CTRL_LEFT:
            while (1) {
                moveCursorChecking(ARROW_LEFT, &G.E);
                if (G.E.cy < G.E.numRows) {
                    char curChar = G.E.row[G.E.cy].chars[G.E.cx - 1];  //there are so many edge cases and stuff to check, like "", &&
                    if (curChar == ' ' || curChar == '(' ||
                        curChar == ')' || curChar == '[' ||
                        curChar == ']' || curChar == '.' ||
                        curChar == '#' || curChar == '"' ||
                        curChar == '\t' ||
                        G.E.row[G.E.cy].chars[G.E.cx] == '"'||
                        G.E.cx == 0) break;
                }
                else {
                    break;
                }
            }
            break;
        case CTRL_RIGHT:
            while (1){
                moveCursorChecking(ARROW_RIGHT, &G.E);
                if (G.E.cy < G.E.numRows) {
                    char curChar = G.E.row[G.E.cy].chars[G.E.cx];
                    if (curChar == ' ' || curChar == '(' ||
                        curChar == ')' || curChar == '[' ||
                        curChar == ']' || curChar == '.' ||
                        curChar == '&' || curChar == ',' ||
                        curChar == ':' || curChar == '"' ||
                        curChar == '\t' ||
                        (G.E.row[G.E.cy].chars[G.E.cx-1] == '/' && G.E.row[G.E.cy].chars[G.E.cx-2] == '/')||
                        G.E.row[G.E.cy].chars[G.E.cx-1] == '"'||
                        G.E.cx == G.E.row[G.E.cy].size ) break;
                }
                else {
                    break;
                }
            }
            break;
        case F1_FUNCTION_KEY:
        case F2_FUNCTION_KEY:
        case F3_FUNCTION_KEY:
        case F4_FUNCTION_KEY:
            break;
        case F5_FUNCTION_KEY:
            if (G.currentMode == EDIT) {
                // enter debug mode
                resetBrainfuck(&G.B);
                modeSwitcher(DEBUG);
                globalRefreshScreen();
                
            }
            else if (G.currentMode != EDIT  && G.B.debugMode != EXECUTION_ENDED) {
                //continue
                G.B.debugMode = CONTINUE;
                while (G.B.debugMode == CONTINUE) {
                    processBrainFuck(&G.B);
                    /*
                    //atempt at a pause button without interrupt mechanism
                    char c = '\0';
                    read(STDIN_FILENO, &c, 1); //assuming read cannot crash the program in any way
                    if (c == CTRL_KEY('p')) {
                        G.B.debugMode = PAUSED;
                        c = '\0';
                    }
                    */
                }
            }
            break;
        case F6_FUNCTION_KEY:
            if (G.currentMode == EDIT) {
                // enter execute mode
                resetBrainfuck(&G.B);
                modeSwitcher(EXECUTE);
                globalRefreshScreen();
                G.B.debugMode = CONTINUE;
                while (G.B.debugMode == CONTINUE) {
                    processBrainFuck(&G.B);
                }
            }
            else if (G.currentMode == DEBUG /* && G.B.debugMode != EXECUTION_ENDED */) {
                // step into
                G.B.debugMode = STEP_BY_STEP;
                processBrainFuck(&G.B);
            }
            break;
        case F7_FUNCTION_KEY:
            {
                //step out
                if (G.currentMode == DEBUG  && G.B.debugMode != EXECUTION_ENDED) {
                    if (G.B.regenerateStack) {
                        regenerateBracketStack(G.B.instX, G.B.instY, &G.B);
                    }
                    bool inALoop = !coordStackIsEmpty(&G.B.bracketStack);
                    if (inALoop) {
                        int curStackSize = G.B.bracketStack.top;
                        G.B.debugMode = STEPPING_OUT;
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
                resetBrainfuck(&G.B);
                resetWindow(OUTPUT, &G.O);
                resetWindow(DATA_ARRAY, &G.dataArray);
                setStatusMessage("");
                globalRefreshScreen();
            }
            break;
        case F9_FUNCTION_KEY:
            // quit to editor
            activeWindowSwicher(TEXT_EDITOR);
            if (G.currentMode != EDIT) {
                modeSwitcher(EDIT);
                resetBrainfuck(&G.B);
                resetWindow(OUTPUT, &G.O);
                resetWindow(DATA_ARRAY, &G.dataArray);
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
                if (curChar == '\0' || curChar == ' ' || curChar == ')' || curChar == ']') {
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
                                moveCursorChecking(ARROW_RIGHT, &G.E);
                            }
                            else {
                                windowInsertChar(c, &G.E, true);
                            }
                            break;
                    }
                    moveCursorChecking(ARROW_LEFT, &G.E);
                }
                G.B.regenerateStack = true; 
            }
            break;
        }
        case ')':
        case '}':
        case ']': {
            if (G.activeWindow == TEXT_EDITOR) {
                if (G.E.cy < G.E.numRows) {
                    char curChar = G.E.row[G.E.cy].chars[G.E.cx];
                    if (curChar == c) {
                        moveCursorChecking(ARROW_RIGHT, &G.E);
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
                if (G.E.cy < G.E.numRows) {
                    char curChar = G.E.row[G.E.cy].chars[G.E.cx];
                    if (curChar == c) {
                        moveCursorChecking(ARROW_RIGHT, &G.E);
                    }
                    else if (curChar == '\0' || curChar == ' ' || curChar == ')' || curChar == ']') {
                        windowInsertChar(c, &G.E, true);
                        windowInsertChar(c, &G.E, true);
                        moveCursorChecking(ARROW_LEFT, &G.E);
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
                    //coordStackPush(G.E.cx, G.E.cy, &G.editorUndoStack);
                }
            }
            break;
    }
    curQuitTimes = QUIT_TIMES;
}

void moveCursorChecking(int key, window* this){
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

void moveCursorCheckingDatayArray(int key, window* this){ //this of this as virtual func of above
    if (G.currentMode == TEXT_EDITOR || this->type != DATA_ARRAY) return;

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

char* promptInput(char * prompt, int inputSizeLimit){
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
    }
}

//output
void windowScroll(window* this) {
    //first process tabs. Each time E.cx get change, we will get to here, and then calculate the correct E.rx to show
    this->rx = 0;
    if (this->cy < this->numRows) {
        this->rx = windowRowCxToRx(&this->row[this->cy], this->cx);
    }

    //now is actual scrolling
    if (this->cy < this->startRowOrCell) {
        this->startRowOrCell = this->cy;
    }
    if (this->cy >= this->startRowOrCell + this->windowRows) {
        this->startRowOrCell = this->cy - this->windowRows + 1;
    }

    if (this->rx < this->startCol) {
        this->startCol = this->rx;
    }
    if (this->rx >= this->startCol + this->windowCols) {
        this->startCol = this->rx - this->windowCols + 1;
    }
}

void dataArrayScroll(window* this) {
    if (G.currentMode == TEXT_EDITOR || this-> type != DATA_ARRAY) return;

    if (this->cy < this->startRowOrCell) {
        this->startRowOrCell = this->cy;
    }
    if (this->cy >= this->startRowOrCell + this->windowRows) {
        this->startRowOrCell = this->cy - this->windowRows + 1;
    }

    G.dataArray.rx = G.dataArray.cx * 4 + 1; 
}

void globalRefreshScreen(){
    windowScroll(&G.E);
    if (G.currentMode != TEXT_EDITOR) { 
        dataArrayScroll(&G.dataArray);
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
    
    //do smth diff when we allow for diff cursor
    int activeWindowX = G.activeWindowPtr->rx - G.activeWindowPtr->startCol + 1 + G.activeWindowPtr->startPosX;
    int activeWindowY = G.activeWindowPtr->cy - G.activeWindowPtr->startRowOrCell + 1 + G.activeWindowPtr->startPosY;
    basicMoveCursor(&ab, activeWindowX, activeWindowY);
    
    abAppend(&ab, "\x1b[?25h", 6);
    write(STDOUT_FILENO, ab.b, ab.len);
    abFree(&ab);

}

void drawEditor(struct abuf * ab) {
    abAppend(ab, "\x1b[H", 3);  //moves cursor position to 0,0
    for (int y = 0; y < G.E.windowRows; y++){
        int fileRow = G.E.startRowOrCell + y;
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
            if (len < 0) len = 0; //just display nothing
            if (len > G.E.windowCols) len = G.E.windowCols;
            //abAppend(ab, &E.row[fileRow].render[E.colOff], len);
            //basic syntax highlighting
            char* lineToPrint = &G.E.row[fileRow].render[G.E.startCol];
            int rxOfInst = windowRowCxToRx(&G.E.row[fileRow] , G.B.instX);
            for (int j = 0; j < len; j++) {
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
                if (G.currentMode == DEBUG && fileRow == G.B.instY && rxOfInst == G.E.startCol + j) {
                    abAppend(ab, "\x1b[7m", 4);
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
    basicMoveCursor(ab, G.E.windowCols + 1, 0);
    for (int y = 0; y < G.E.windowRows; y++) {
        abAppend(ab, "|\x1b[D\x1b[B", 7);
    }
    basicMoveCursor(ab, 0, G.E.windowRows + 1);
    for (int x = 0; x < G.fullScreenCols; x++) {
        abAppend(ab, "=", 1);
    }
}

void drawDataArray(struct abuf * ab){
    basicMoveCursor(ab, G.dataArray.startPosX, G.dataArray.startPosY);

    char buf[10];
    
    int cellToHighlight = G.B.arrayIndex;
    int index = G.dataArray.startRowOrCell * G.dataArray.numCells;

    for(int i = 0; i < G.dataArray.windowRows; i++) {
        for (int j = 0; j < G.dataArray.numCells; j++) {
            if (index == cellToHighlight) {
                abAppend(ab, "\x1b[7m", 4);
            }

            if (index < G.B.arraySize) {
                snprintf(buf, sizeof(buf),"%3d|", G.B.dataArray[index]);
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
        basicMoveCursor(ab, G.dataArray.startPosX, i + 2); //screen is actually 1-based. 0 is just default value, which just happens to move start of line/col
    }
}

 //I may be able to make all 3 window-drawing function one big functions, but there is no advantage
void drawOutput(struct abuf * ab) {
    basicMoveCursor(ab, G.O.startPosX, G.O.startPosY);
    for (int y = -1; y < G.O.windowRows - 1; y++){
        int outRow = G.O.startRowOrCell + y;
        if (y == -1) { //"Output:" in bold (use -1 so we don't mess up outRow)
            abAppend(ab, "\x1b[1;37m", 7);
            abAppend(ab, "Output:\x1b[0m", 11);
        }
        else {
            if (outRow < G.O.numRows) {
                //manually highlight cursor here if it's not the active window
                int len = G.O.row[outRow].rsize - G.O.startCol;
                if (len < 0) len = 0;
                if (len > G.O.windowCols) len = G.O.windowCols;
                //abAppend(ab, &G.O.row[outRow].render[G.O.startCol], len);

                // highlights where cursor should be
                // Problem is that highlight can only go on a place with character. So my best approximate is to hihglight just left of actual cursor
                char* lineToPrint = &G.O.row[outRow].render[G.O.startCol];
                G.O.rx = windowRowCxToRx(&G.O.row[outRow], G.O.cx);
                for (int j = 0; j < len; j++) {
                    if (G.activeWindow != OUTPUT && y == G.O.cy && G.E.startCol + j == G.O.rx) {
                        abAppend(ab, "\x1b[7m", 4);
                    }
                    abAppend(ab, &lineToPrint[j], 1);
                    abAppend(ab, "\x1b[0m", 4);
                }
                if (G.activeWindow != OUTPUT && y == G.O.cy && len == G.O.rx) {
                    abAppend(ab, "\x1b[7m \x1b[0m" , 9);
                }
            }
            else {
                if (G.activeWindow != OUTPUT && y == G.O.cy) {
                    abAppend(ab, "\x1b[7m~\x1b[0m" , 9);
                }
                else {
                    abAppend(ab, "~", 1);
                }
                
            }
        }
        abAppend(ab, "\x1b[K\r\n", 5);  //clear line right of cursor
    }
}

void drawStatusBar(struct abuf * ab) {
    basicMoveCursor(ab, 0, G.fullScreenRows - 1);
    
    abAppend(ab, "\x1b[7m", 4); //invert color, will also use for selection
    //file name
    char status[80], rightStatus[80];
    int len = snprintf(status, sizeof(status), "%.20s - %d lines %s", (G.fileName ? G.fileName : "No Name"), G.E.numRows, (G.E.dirty ? "(modified)" : "") );
    if (len > G.fullScreenCols) len = G.fullScreenCols;
    abAppend(ab, status, len);

    int rightLen = snprintf(rightStatus, sizeof(rightStatus), "%d/%d", G.E.cy + 1 , G.E.numRows);
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

void setStatusMessage(const char * fmt, ...) {
    //This is how you do function with varied amount of argument (variadic) like printf, 
    va_list ap;
    va_start(ap, fmt); //initalizes ap with start of variable list
    //nomrally you'd then use va_arg(va_list, <varl type>) to access next argument, but we just use vsnprintf to automatically parse fmt in the printf way
    vsnprintf(G.statusMsg, sizeof(G.statusMsg), fmt, ap);
    va_end(ap); //clean up for va_start
    G.statusMsg_time = time(NULL); //current time, in seconds since midnight Jan 1, 1970
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

void basicMoveCursor(struct abuf * ab, int x, int y) {
    char buf[32];
    snprintf(buf, sizeof(buf),"\x1b[%d;%dH", y, x);
    abAppend(ab, buf, strlen(buf));
}

//"member" functions
void globalInit() {
    resetBrainfuck(&G.B);
    resetWindow(OUTPUT, &G.O);
    resetWindow(TEXT_EDITOR, &G.E);
    resetWindow(DATA_ARRAY, &G.dataArray);

    G.currentMode = EDIT;
    updateWindowSizes();

    G.fileName = NULL;
    G.statusMsg[0] = '\0';
    G.statusMsg_time = 0;
    G.activeWindow = TEXT_EDITOR;
    G.activeWindowPtr = &G.E;

    undoStackInit(&(G.editorUndoStack));
}

void abAppend(struct abuf * ab, const char * s, int len) {
    char * new = (char *) realloc(ab->b, ab->len + len);
    if (new == NULL) return;

    memcpy(&new[ab->len], s, len);
    ab->b = new;
    ab->len += len;
}

void resetWindow(windowType givenType, window* this) {
    this->type = givenType;
    free(this->row);
    this->row = NULL;

    this->cx = 0;
    this->cy = 0;
    this->rx = 0;
    this->startRowOrCell = 0;
    this->startCol = 0;
    this->numRows = 0;
    this->dirty = 0;
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
            G.E.windowCols = (G.fullScreenCols / 2) - 2;
            G.E.windowRows = (G.fullScreenRows * 3) / 5 - 2;
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
        G.O.windowRows = G.fullScreenRows - G.E.windowRows - 1 - 2; //1 for border, 2 for status bar/message
    }
}

//Barinfuck (these functions don't really need this ptr, but I already implemented it)
void resetBrainfuck(brainFuckModule* this){
    this->arraySize = 30000;
    memset(this->dataArray, 0, 30000);  //turn this into dynamic later
    this->arrayIndex = 0;
    
    this->debugMode = PAUSED;

    this->instCounter = 0;

    this->instX = 0;
    this->instY = 0;

    coordStackInit(&(this->bracketStack));
    this->regenerateStack = false;

    this->errorMsg = NULL;
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

void processBrainFuck(brainFuckModule* this) {
    //validate inst
    if (this->instY >= G.E.numRows || this->debugMode == EXECUTION_ENDED) {
        this->debugMode = EXECUTION_ENDED;
        
        //put this in status bar (running vs finished and if error encountered in permanat status bar)
        if (this->errorMsg) {
            setStatusMessage("Execution finished. \x1b[7;31mError: %s\x1b[0m\x1b[7m Press F8 to restart, F9 to quit to editor", this->errorMsg);
        }
        else {
            if (coordStackIsEmpty(&this->bracketStack)) {
                setStatusMessage("Execution finished. Press F8 to restart, F9 to quit to editor");
            }
            else {
                int bracketX = coordStackTop(&this->bracketStack).x;
                int bracketY = coordStackTop(&this->bracketStack).y;
                setStatusMessage("Execution finished. \x1b[7;31mError: Opening bracket at {%d,%d} was not closed\x1b[0m\x1b[7m Press F8 to restart, F9 to quit to editor", bracketX, bracketY);
            }
        }
        return;
    }

    if (this->instX >= G.E.row[this->instY].size) {
        //This gets triggered on empty line. Can also happen when user delete stuff in run time
        instForward();
        return;
    }

    char curInst = G.E.row[this->instY].chars[this->instX];

    //validate data cell
    if (this->arrayIndex < 0 || this->arrayIndex >= this->arraySize) {
        this->debugMode = EXECUTION_ENDED;
        //preseumably there should be an error message set already
        return;
    }

    unsigned char* dataPtr = &(this->dataArray[this->arrayIndex]);

    switch(curInst){
        case '>':
            if (this->arrayIndex + 1 >= this->arraySize ) {
                brainfuckDie("Attemped to go out of array's upper bound.", this);
                //make this triger memory allocation later on
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
            //also check for change line and delete
            switch (*dataPtr) {
                case '\n': //ASCII 10
                //case '\r': //ASCII 13
                    windowInsertNewLine(&G.O, false);
                    globalRefreshScreen();
                    break;
                case 8: //backspace symbol
                    windowDelChar(&G.O, false);
                    break;
                case '\x1b':
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
                char* userInput = NULL;
                bool validInput = false;
                while (!validInput) {
                    userInput = promptInput("Enter value for selected cell (unsigned 8-bit alphanumeric only, ESC to cancel): %s", 3); 
                    if(userInput) {
                        if (userInput[0] > 31 && userInput[0] < 127) { //the returned buffer should be valid
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
                            //Something went wrong. promptInput shouldn't give something like this
                            validInput = false;
                            free(userInput);
                            userInput = NULL;
                        }
                    }
                    else {
                        //user canceled. So just go back to editor and pause without doing anything
                        this->debugMode = PAUSED;
                        return;
                    }
                }
                free(userInput);
                userInput = NULL;
                instForward();
            }
            break;
        case '[':
            if(*dataPtr){
                //add bracket loc to stack
                coordStackPush(this->instX, this->instY, &this->bracketStack);
                instForward();
            }
            else{
                //skip till matching closing bracket
                coordStack stackForSkippping; //need to free mem if dynamic
                coordStackInit(&stackForSkippping);
                
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
                        break;
                    }
                    char curInst = G.E.row[this->instY].chars[this->instX];

                    if (curInst == '[') {
                        coordStackPush(this->instX, this->instY, &stackForSkippping);
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
        default:
            instForward();
            break;
    }
    //only do this if need to scroll screen (horizontally and vertically)
    G.E.cy = this->instY;
    G.dataArray.cy = G.B.arrayIndex / G.dataArray.numCells;

    if (this->debugMode == STEP_BY_STEP) this->debugMode = PAUSED;
    ++(this->instCounter);
}

void regenerateBracketStack(int stopPointX, int stopPointY, brainFuckModule* this) {
    if (!this->regenerateStack) return;

    coordStackInit(&this->bracketStack);
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
            coordStackPush(this->instX, this->instY, &this->bracketStack);
        }
        else if (curInst == ']') {
            coordStackPop(&this->bracketStack);
        }
        instForward();
    }
    this->regenerateStack = false;
}

//coordinate stack
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
        brainfuckDie("Loop stack full.", &G.B);
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
        brainfuckDie("Tried to pop empty stack. Returned {0,0}.", &G.B);
        coordinate zeroCoord = {0,0};
        return zeroCoord;
    }
}

bool coordStackIsEmpty(coordStack* this) {
    return (this->top == -1);
}

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
    undoStruct returnStruct = {0,0,ADDITION,'\0'}; // just to have a default
    if (this->top > -1) {
        returnStruct = this->stackArray[this->top];
        this->top--;
    }
    return returnStruct;
}

bool undoStackIsEmpty(undoStack* this) {
    return (this->top == -1);
}