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

#define MICHAEL_IDE_VER "0.6"
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
    F10_FUNCTION_KEY
};

enum window {
    TEXT_EDITOR = 1,
    DATA_ARRAY,
    OUTPUT
};

enum topMode {
    EDIT = 1,
    DEBUG,
    EXECUTE
};

//data
typedef struct erow {
    int size;
    char* chars;
    int rsize;
    char* render;
} erow;

typedef struct windowConfig {
    int startX, startY;
    int cx, cy;
    int rx;
    int numRows;
    int windowRows;
    int numCols;
    int rowOff, colOff;
    erow* row; //dynamic array
} windowConfig;

//window
void resetWindow(windowConfig* this);
void windowInsertRow(int at, char*s, size_t len, windowConfig* this);
void windowInsertChar(int c, windowConfig* this);

//coordinate
typedef struct coordinate {
    int x;
    int y;
} coordinate;

#define COORD_INIT {0, 0}

//stack of coordinate
typedef struct coordStack {
    int top;
    int size;
    coordinate stackArray[100];
}coordStack;

//stack
void coordStackInit(coordStack* this);
bool coordStackPush(int x, int y, coordStack* this);
void coordStackPop(coordStack* this);
coordinate coordStackTop(coordStack* this); //return {0,0} if stack empty
bool coordStackIsEmpty(coordStack* this);

enum debuggerMode {
    PAUSED = 0,
    STEP_BY_STEP,
    CONTINUE,
    STEPPING_OUT
};

//brainfuck interpter
typedef struct brainFuckConfig {
    unsigned char dataArray[30000];
    int arraySize;
    int arrayIndex;

    enum debuggerMode debugMode;

    int instCounter;
    bool executionEnded; //this can be a debuggerMode thing

    int instX, instY;
    
    coordStack bracketStack;

    char* errorMsg;
} brainFuckConfig;

//brainFuck
void resetBrainfuck(brainFuckConfig* this);
void processBrainFuck(brainFuckConfig* this);
void instForward(); //igore comments

//Highest level
struct environmentConfig {
    //editor stuff
    int cx, cy;
    int rx;
    int screenRows;
    int screenCols;
    int rowOff, colOff;
    int numRows;
    //put into winConfig editor

    int fullScreenRows;
    int fullScreenCols;

    windowConfig editor;
    windowConfig dataArray;

    erow* row; //dynamic array
    int dirty;
    char* fileName;
    char statusMsg [255]; //bottleneck for status message length
    time_t statusMsg_time;
    struct termios orig_termio;

    enum window activeWindow;
    enum topMode currentMode;
};

void initEditor(void);

//append buffer (dynamic string)
struct abuf {
    char* b;
    int len;
};

#define ABUF_INIT {NULL, 0}
void abAppend(struct abuf* ab, const char* s, int len);
void abFree(struct abuf* ab) {free(ab->b);}

/*** functions ***/
//terminal
void enableRawMode(void);
void disableRawMode(void);
void die(const char* s);
int editorReadKey(void);
int getWindowSize (int* rows, int* cols);
int getCursorPosition(int* rows, int* cols);

//row operation
void editorInsertRow(int at, char* s, size_t len);
void editorUpdateRow(erow* row);
int editorRowCxToRx(erow* row, int cx);
void editorRowInsertChar(erow* row, int at, int c);
void editorRowDelChar(erow* row, int at);
void editorFreeRow(erow* row);
void editorDelRow(int at);
void editorRowAppendString(erow* row, char* s, size_t len);

//editor operation
void editorInsertChar(int c);
void editorInsertNewLine();
void editorDelChar();

//file i/o
void editorOpen(char* fileName);
char* editorRowToString(int* bufLen);
void editorSave();

//input
void editorProcessKeypress(void);
void editorMoveCursor(int key);
char* editorPrompt(char* prompt, int inputSizeLimit); //our own scanf (-1 for no limit), need to free the returned buffer

//output
void editorScroll(void);
void editorRefreshScreen(void);
void editorDrawRows(struct abuf* ab);
void drawBorder(struct abuf* ab);
void drawDataArray(struct abuf* ab);
void drawOutput(struct abuf* ab);
void editorDrawStatusBar(struct abuf* ab);
void editorDrawMessageBar(struct abuf* ab);
void editorSetStatusMessage(const char* fmt, ...);
void generalMoveCursor(struct abuf* ab, int x, int y); //move cursor to arbitary position, no rule checking (name confusing with editorMoveCursor)

//modes and windows
void modeSwitcher(enum topMode nextMode);
void updateWindowSizes();

/*** global ***/
struct environmentConfig E;
brainFuckConfig B;
windowConfig O;

int main(int argc, char* argv[]) {
	enableRawMode();
    initEditor();
    //editorOpen("bfHelloWorld.bf"); //for testing in VScode
    if (argc >= 2){ //command would be kilo fileName, kilo would be first argument
        editorOpen(argv[1]);
    }

    editorSetStatusMessage("HELP: Ctrl-S = Save | Ctrl-Q = quit");

    while (1){
        editorRefreshScreen();
        editorProcessKeypress();

        if (E.currentMode == DEBUG) {
            if (B.debugMode == STEP_BY_STEP) {
                    processBrainFuck(&B);
                }
            else if (B.debugMode == CONTINUE) {
                while (B.debugMode == CONTINUE) {
                    processBrainFuck(&B);
                }
            }
            else if (B.debugMode == STEPPING_OUT) {
                //current implementation will stop at first closing bracket
                int curStackSize = B.bracketStack.top;
                while (B.debugMode == STEPPING_OUT && B.bracketStack.top != curStackSize - 1) {
                    processBrainFuck(&B);
                }
                B.debugMode = PAUSED;
            }
        }
    }

    return 0;
}

//terminal
void enableRawMode(){
    if (tcgetattr(STDIN_FILENO, &E.orig_termio) == -1){
        die("tcgetattr");
    }
    atexit(disableRawMode); //it's cool that this can be placed anywhere

    struct termios raw = E.orig_termio;
    raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    raw.c_oflag &= ~(OPOST);
    raw.c_cflag |= (CS8);
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);

    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 1;    //adds a timeout to read (in 0.1s)

    if(tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1){   //TCSAFLUCH defines how the change is applied
        die("tcsetattr"); 
    }
}

void disableRawMode(void){
    if(tcsetattr(STDIN_FILENO, TCSAFLUSH, &E.orig_termio) == -1){
        die("tcsetattr");   
    }
}

void die(const char* s){ //error handling
    write(STDOUT_FILENO, "\x1b[2J", 4);
    write(STDOUT_FILENO, "\x1b[H", 3);
    perror(s);
    write(STDOUT_FILENO, "\r", 1);
    exit(1);
}

int editorReadKey(void){
    int nread;
    char c;
    while ((nread = read(STDIN_FILENO, &c, 1)) != 1){
        if(nread == -1 && errno != EAGAIN) die("read");
    }
    if (c == '\x1b') {
        char seq[5];
        if (read(STDIN_FILENO, &seq[0], 1) != 1) return '\x1b';
        if (read(STDIN_FILENO, &seq[1], 1) != 1) return '\x1b';

        if (seq[0] == '['){
            if (seq[1] >= '0' && seq[1] <= '9') {
                if (read(STDIN_FILENO, &seq[2], 1) != 1) return '\x1b';
                if (seq[2] == '~'){
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

int getWindowSize (int* rows, int* cols){
    struct winsize ws;
    if(ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0){
        if (write(STDOUT_FILENO, "\x1b[999C\x1b[999B", 12) != 12) return -1;  //try move cursor a ton
        return getCursorPosition(rows, cols);  
    }
    else {
        *rows = ws.ws_row;
        *cols = ws.ws_col;
        return 0;
    }
}

int getCursorPosition(int* rows, int* cols){
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
void editorInsertRow(int at, char* s, size_t len){
    if (at < 0 || at > E.numRows) return;
    // allocate memory for a new row
    erow* temp = realloc(E.row, sizeof(erow) * (E.numRows + 1));
    if (temp == NULL) die("editorInserRow");
    E.row = temp;
    memmove(&E.row[at + 1], &E.row[at], sizeof(erow) * (E.numRows - at));

    E.row[at].size = len;
    E.row[at].chars = (char*) malloc(len + 1);
    memcpy(E.row[at].chars, s, len);
    E.row[at].chars[len] = '\0'; //we do this manually cuz we stripped off the end of the line (\r\n)

    E.row[at].rsize = 0;
    E.row[at].render = NULL;
    editorUpdateRow(&E.row[at]);

    E.numRows++;
    E.dirty++;
}

void windowInsertRow(int at, char* s, size_t len, windowConfig* this) {
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
    //editorUpdateRow(&E.row[at]);

    this->numRows++;
}

void editorUpdateRow(erow* row){
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

int editorRowCxToRx(erow* row, int cx){
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

void editorRowInsertChar(erow* row, int at, int c){
    if (at < 0 || at > row->size) at = row->size; //validate at just in case
    char* temp = realloc(row->chars, row->size + 2); //2 cuz the inserted char, and cuz size doesn't include null terminator
    if (temp == NULL) die("editorRowInsertChar");
    row->chars = temp;

    memmove(&row->chars[at + 1], &row->chars[at], row->size - at + 1); //+1 for null terminator
    row->size++;
    row->chars[at] = c;
    editorUpdateRow(row);
    E.dirty++;
}

void editorRowDelChar(erow* row, int at){
    if (at < 0 || at >= row->size) return;
    memmove(&row->chars[at], &row->chars[at + 1], row->size - at);
    row->size--;
    editorUpdateRow(row);
    E.dirty++;
}

void editorFreeRow(erow* row){
    free (row->chars);
    free (row->render);
}

void editorDelRow(int at){
    if (at < 0 || at >= E.numRows) return;
    editorFreeRow(&E.row[at]);
    memmove(&E.row[at], &E.row[at + 1], sizeof(erow) * (E.numRows - at - 1));
    E.numRows--;
    E.dirty++;
}

void editorRowAppendString(erow * row, char* s, size_t len){
    row->chars = (char*) realloc(row->chars, row->size + len + 1);
    memcpy(&row->chars[row->size], s, len);
    row->size += len;
    row->chars[row->size] = '\0';
    editorUpdateRow(row);
    E.dirty++;
}

//editor operation
void editorInsertChar(int c){
    if (E.cy == E.numRows) {
        editorInsertRow(E.numRows,"", 0);
    }
    editorRowInsertChar(&E.row[E.cy], E.cx, c);
    E.cx++;
}

void windowInsertChar(int c, windowConfig* this) {
    if (this->cy == this->numRows) {
        windowInsertRow(this->numRows,"", 0, this);
    }
    editorRowInsertChar(&this->row[this->cy], this->cx, c);
    this->cx++;
}

void editorInsertNewLine(){
    if (E.cx == 0) {
        editorInsertRow(E.cy, "", 0);
    }
    else {
        erow* row = &E.row[E.cy];
        editorInsertRow(E.cy + 1, &row->chars[E.cx], row->size - E.cx);
        row = &E.row[E.cy]; //in case realloc change address of things
        row->size = E.cx;
        row->chars[row->size] = '\0';
        editorUpdateRow(row);
    }
    E.cy++;
    E.cx = 0;
}

void editorDelChar(){
    if (E.cy == E.numRows) return;
    if (E.cx == 0 && E.cy == 0) return;
    if (E.cx > 0) {
        editorRowDelChar(&E.row[E.cy], E.cx - 1);
        E.cx--;
    }
    else {
        E.cx = E.row[E.cy - 1].size;
        editorRowAppendString(&E.row[E.cy - 1], E.row[E.cy].chars, E.row[E.cy].size);
        editorDelRow(E.cy);
        E.cy--;
    }
}

//file i/o
void editorOpen(char* fileName){
    free(E.fileName);
    E.fileName = strdup(fileName);

    FILE *fp = fopen(fileName, "r");
    if (!fp) die("fopen");
    char* line = NULL;
    size_t lineCap = 0; //cuz getline can be more lineCap than needed
    ssize_t lineLen; 
    
    while ((lineLen = getline(&line, &lineCap, fp)) != -1) {
        while (lineLen > 0 && (line[lineLen-1] == '\n' || line[lineLen-1] == '\r') ){ //strip off change line
            lineLen--;
        }
        editorInsertRow(E.numRows, line, lineLen);
    }

    free(line);
    fclose(fp);
    E.dirty = 0;
}

char* editorRowToString(int* bufLen){
    int totalLen = 0;
    int j;
    for (j = 0; j < E.numRows; j++) {
        totalLen += E.row[j].size + 1;  //+ 1 is for \n
    }
    if (bufLen != NULL)*bufLen = totalLen;

    char* buf = malloc(totalLen);
    char* p = buf;
    for (j = 0; j < E.numRows; j++) {
        memcpy(p, E.row[j].chars, E.row[j].size);
        p += E.row[j].size;
        *p = '\n';
        p++;
    }
    return buf;
}

void editorSave(){
    if (E.fileName == NULL) {
        E.fileName = editorPrompt("Save as (ESC to cancel): %s", 255);
        if (E.fileName == NULL) {
            editorSetStatusMessage("Save aborted");
            return;
        }
    }

    int len;
    char* buf = editorRowToString(&len);

    int fd = open(E.fileName, O_RDWR | O_CREAT, 0644);  //RDWR for read & write, CREAT means create file if not exist, 0644 set permission
    if (fd != -1) {
        if (ftruncate(fd, len) != -1) {
        //ftruncate changes file sizze of len. Doing this instead of reseting entire file means we lose less stuff if write() fail. Better soln would be to use a buffer file 
            if (write(fd, buf, len) == len) {
                close(fd);
                free(buf);
                E.dirty = 0;
                editorSetStatusMessage("%d bytes written to disk", len);
                return;
            }
        }
        close(fd);
    }

    free(buf);
    editorSetStatusMessage("Can't save! I/O errror: %s", strerror(errno));
}

//input
void editorProcessKeypress(){
    static int curQuitTimes = QUIT_TIMES;   //preserve value even after going out of scope. Don't get re-initialized
    
    int c = editorReadKey();

    //putting it here just before an input is processed. Hopefully nothing break
    updateWindowSizes();

    switch (c) {
        case '\r':
            editorInsertNewLine();
            break;
        case CTRL_KEY('q'):
            if (E.dirty && curQuitTimes > 0) {
                editorSetStatusMessage("Warning!!! File has unsaved changes. Press Ctrl-Q %d more times to quit.", curQuitTimes);
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
        case PAGE_UP:
        case PAGE_DOWN:
        {
            if (c == PAGE_UP) {
                E.cy = E.rowOff;
            }
            else if (c == PAGE_DOWN) {
                E.cy = E.rowOff + E.screenRows - 1;
                if (E.cy > E.numRows) E.cy = E.numRows;
            }

            for (int times = E.screenRows - 1; times > 0; times--) 
                editorMoveCursor( (c == PAGE_UP) ? ARROW_UP : ARROW_DOWN);
        }
        case HOME_KEY:
            E.cx = 0;
            break;
        case END_KEY:
            if (E.cy < E.numRows)
                E.cx = E.row[E.cy].size;
            break;
        case BACKSPACE:
        case CTRL_KEY('h'):
        case DEL_KEY:
            if (c == DEL_KEY) editorMoveCursor(ARROW_RIGHT);
            editorDelChar();
            break;
        case ARROW_UP:
        case ARROW_LEFT:
        case ARROW_DOWN:
        case ARROW_RIGHT:
            editorMoveCursor(c);
            break;
        case CTRL_UP:
        case CTRL_LEFT:
            while (1){
                editorMoveCursor(ARROW_LEFT);
                char curChar = E.row[E.cy].chars[E.cx -1];  //there are so many edge cases and stuff to check, like "", &&
                if (curChar == ' ' || curChar == '(' ||
                    curChar == ')' || curChar == '[' ||
                    curChar == ']' || curChar == '.' ||
                    curChar == '#' || curChar == '"' ||
                    curChar == '\t' ||
                    E.row[E.cy].chars[E.cx] == '"'||
                    E.cx == 0) break;
            }
            break;
        case CTRL_DOWN:
        case CTRL_RIGHT:
            while (1){
                editorMoveCursor(ARROW_RIGHT);
                char curChar = E.row[E.cy].chars[E.cx];
                if (curChar == ' ' || curChar == '(' ||
                    curChar == ')' || curChar == '[' ||
                    curChar == ']' || curChar == '.' ||
                    curChar == '&' || curChar == ',' ||
                    curChar == ':' || curChar == '"' ||
                    curChar == '\t' ||
                    (E.row[E.cy].chars[E.cx-1] == '/' && E.row[E.cy].chars[E.cx-2] == '/')||
                    E.row[E.cy].chars[E.cx-1] == '"'||
                    E.cx == E.row[E.cy].size ) break;
            }
            break;
        case F1_FUNCTION_KEY:
            break;
        case F2_FUNCTION_KEY:
            break;
        case F3_FUNCTION_KEY:
            break;
        case F4_FUNCTION_KEY:
            break;
        case F5_FUNCTION_KEY:
            if (E.currentMode == EDIT) {
                resetBrainfuck(&B);
                modeSwitcher(DEBUG);
                editorRefreshScreen();
                
            }
            else if (E.currentMode != EDIT) {
                //continue
                B.debugMode = CONTINUE;
            }
            break;
        case F6_FUNCTION_KEY:
            //step into
            if (E.currentMode == DEBUG) {
                B.debugMode = STEP_BY_STEP;
            }
            break;
        case F7_FUNCTION_KEY: {
                //step out
                if (E.currentMode == DEBUG && !B.executionEnded) {
                    bool inALoop = !coordStackIsEmpty(&B.bracketStack);
                    if (inALoop) {
                        B.debugMode = STEPPING_OUT;
                    }
                    else {
                        editorSetStatusMessage("Not in loop");
                    }
                }
            }
            break;
        case F8_FUNCTION_KEY:
            //restart
            if (E.currentMode != EDIT) {
                E.cx = 0;
                E.cy = 0;
                resetBrainfuck(&B);
                resetWindow(&O);
                editorRefreshScreen();
            }
            break;
        case F9_FUNCTION_KEY:
            //stop
            if (E.currentMode != EDIT) {
                modeSwitcher(EDIT);
                resetBrainfuck(&B);
                resetWindow(&O);
            }
            break;
        case F10_FUNCTION_KEY:
            break;
        case CTRL_KEY('l'): 
        case '\x1b':
            break;
        case '(':
        case '{':
        case '[': {
            editorInsertChar(c);
            char curChar = E.row[E.cy].chars[E.cx];
            //if cx is at end of row, curChar would be '\0'
            if (curChar == '\0' || curChar == ' ' || curChar == ')' || curChar == ']') {
                switch (c) {
                    case '(':
                        editorInsertChar(')');
                        break;
                    case '{':
                        editorInsertChar('}');
                        break;
                     case '[':
                        editorInsertChar(']');
                        break;
                    case '"':
                    case 39:
                        if (curChar == c) {
                            editorMoveCursor(ARROW_RIGHT);
                        }
                        else {
                            editorInsertChar(c);
                        }
                        break;
                }
                editorMoveCursor(ARROW_LEFT);
            }
            break;
        }
        case ')':
        case '}':
        case ']': {
            if (E.cy < E.numRows) {
                char curChar = E.row[E.cy].chars[E.cx];
                if (curChar == c) {
                    editorMoveCursor(ARROW_RIGHT);
                }
                else {
                    editorInsertChar(c);
                }
            }
            else {
                editorInsertChar(c);
            }
            break;
        }
        case '"':
        case 39: {    // this char is '
            if (E.cy < E.numRows) {
                char curChar = E.row[E.cy].chars[E.cx];
                if (curChar == c) {
                    editorMoveCursor(ARROW_RIGHT);
                }
                else if (curChar == '\0' || curChar == ' ' || curChar == ')' || curChar == ']') {
                    editorInsertChar(c);
                    editorInsertChar(c);
                    editorMoveCursor(ARROW_LEFT);
                }
                else {
                    editorInsertChar(c);
                }
            }
            else {
                editorInsertChar(c);
            }
            break;
        }
        default:
            if ( (c > 31 && c < 127) || c == '\t') {
                editorInsertChar(c);
            }
            break;
    }
    curQuitTimes = QUIT_TIMES;
}

void editorMoveCursor(int key){
    erow* row = (E.cy >= E.numRows) ? NULL : & E.row[E.cy];

    switch (key) {
        case ARROW_UP:
            if (E.cy > 0) E.cy--;
            break;
        case ARROW_LEFT:
            if (E.cx != 0) {
                E.cx--;
            } 
            else if (E.cy > 0) {
                E.cy--;
                E.cx = E.row[E.cy].size;
            }
            break;
        case ARROW_DOWN:
            if (E.cy < E.numRows) E.cy++;
            break;
        case ARROW_RIGHT:
            if (row && E.cx < row->size ) {
                E.cx++;
            }
            else if (E.cy < E.numRows && E.cx == row->size) {
                E.cy++;
                E.cx = 0;
            }
            break;
    }
    //correct cursor if needed
    row = (E.cy >= E.numRows) ? NULL : & E.row[E.cy];
    int rowLen = row ? row->size : 0;
    if (E.cx > rowLen) {
        E.cx = rowLen;
    }
}

char* editorPrompt(char * prompt, int inputSizeLimit){
    size_t bufSize = 128;
    char * buf = malloc(bufSize);   //our dynamic input buffer

    size_t bufLen = 0;
    buf[0] = '\0';

    while (1) {
        editorSetStatusMessage(prompt, buf);
        editorRefreshScreen();

        int c = editorReadKey();
        if (c == DEL_KEY || c == CTRL_KEY('h') || c == BACKSPACE) { //presed backspace
            if (bufLen != 0) {
                bufLen--;
                buf[bufLen] = '\0';
            }
        }
        else if (c == '\x1b') {  //pressed esc
            editorSetStatusMessage("");
            free(buf);
            return NULL;
        }
        else if (c == '\r') {   //pressed enter
            if (bufLen != 0) {
                editorSetStatusMessage("");
                if (inputSizeLimit != -1 && bufLen > inputSizeLimit) {
                    editorSetStatusMessage("Input size limit (%d characters) reached. Please delete some characters", inputSizeLimit);
                    editorRefreshScreen();
                    editorReadKey();
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
void editorScroll(){
    //first process tabs. Each time E.cx get change, we will get to here, and then calculate the correct E.rx to show
    E.rx = 0;
    if (E.cy < E.numRows) {
        E.rx = editorRowCxToRx(&E.row[E.cy], E.cx);
    }

    //now is actual scrolling
    if (E.cy < E.rowOff) {
        E.rowOff = E.cy;
    }
    if (E.cy >= E.rowOff + E.screenRows) {
        E.rowOff = E.cy - E.screenRows + 1;
    }

    if (E.rx < E.colOff) {
        E.colOff = E.rx;
    }
    if (E.rx >= E.colOff + E.screenCols) {
        E.colOff = E.rx - E.screenCols + 1;
    }
}

void editorRefreshScreen(){
    editorScroll();

    struct abuf ab = ABUF_INIT;

    abAppend(&ab, "\x1b[?25l", 6); //hide cursor
    //let's have each function move the cursor position independantly
    
    editorDrawRows(&ab);

    if (E.currentMode == DEBUG) {
        drawBorder(&ab);
        drawDataArray(&ab);
        drawOutput(&ab);
    }

    editorDrawStatusBar(&ab);
    editorDrawMessageBar(&ab);
    
    generalMoveCursor(&ab, E.rx - E.colOff + 1, E.cy - E.rowOff + 1);
    
    abAppend(&ab, "\x1b[?25h", 6);
    write(STDOUT_FILENO, ab.b, ab.len);
    abFree(&ab);

}

void editorDrawRows(struct abuf * ab) {
    abAppend(ab, "\x1b[H", 3);  //moves cursor position to 0,0
    for (int y = 0; y < E.screenRows; y++){
        int fileRow = E.rowOff + y;
        if (fileRow >= E.numRows) {
            if (E.numRows == 0 && y == E.screenRows / 3) {  //welcome message
                char welcome[80];
                int welcomelen = snprintf(welcome, sizeof(welcome), "Michael's Brainfuck IDE -- version %s", MICHAEL_IDE_VER);

                int padding = (E.screenCols - welcomelen) / 2;
                if (padding != 0) {
                    abAppend(ab, "~", 1);
                    padding--;
                }
                for (int i = padding; i > 0; i--) abAppend(ab, " ", 1);

                if (welcomelen > E.screenCols) welcomelen = E.screenCols;
                abAppend(ab, welcome, welcomelen);
            }
            else {
                abAppend(ab, "~", 1);
            }
        }
        else {
            int len = E.row[fileRow].rsize - E.colOff;
            if (len < 0) len = 0; //just display nothing
            if (len > E.screenCols) len = E.screenCols;
            //abAppend(ab, &E.row[fileRow].render[E.colOff], len);
            //basic syntax highlighting
            char* lineToPrint = &E.row[fileRow].render[E.colOff];
            int rxOfInst = editorRowCxToRx(&E.row[fileRow] , B.instX);
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
                if (E.currentMode == DEBUG && y == B.instY && rxOfInst == E.colOff + j) {
                    abAppend(ab, "\x1b[7m", 4);
                }
                abAppend(ab, &lineToPrint[j], 1);
                abAppend(ab, "\x1b[0m", 4);
            }
        }

        abAppend(ab, "\x1b[K", 3);  //clear line rihgt of cursor
        abAppend(ab, "\r\n", 2);
    }
}

void drawBorder(struct abuf * ab){
    generalMoveCursor(ab, E.screenCols + 1, 0);
    for (int y = 0; y < E.screenRows; y++) {
        abAppend(ab, "|\x1b[D\x1b[B", 7);
    }
    generalMoveCursor(ab, 0, E.screenRows + 1);
    for (int x = 0; x < E.fullScreenCols; x++) {
        abAppend(ab, "=", 1);
    }
}

void drawDataArray(struct abuf * ab){
    generalMoveCursor(ab, E.dataArray.startX, E.dataArray.startY);

    char buf[10];
    
    int locationInArray = B.arrayIndex;
    for(int i = 0; i < 16; i++) {
        if (i == locationInArray) {
            abAppend(ab, "\x1b[7m", 4);
        }
        snprintf(buf, sizeof(buf),"%3d|", B.dataArray[i]);
        abAppend(ab, buf, strlen(buf));
        if (i == locationInArray) {
            abAppend(ab, "\x1b[m", 3);
        }
    }

    printf("  ^\n");
    /*here is the idea:
    alllocate array (make it dynamic to claim Turing completeness, but static for starter) and do brainfuck operation with 1D array
    fix number of cell displayed in 1 row to 16.
    when printing, treat it as 2D array using pointer arithmetic. That way we can do the whole rowOffset thing
    example: https://www.geeksforgeeks.org/dynamically-allocate-2d-array-c/
    I also want to number the rows, but save that for later
    */

}

void drawOutput(struct abuf * ab){
    generalMoveCursor(ab, O.startX, O.startY);
    for (int y = -1; y < O.windowRows; y++){
        int outRow = O.rowOff + y;
        if (y == -1) {
            abAppend(ab, "\x1b[1;37m", 7);
            abAppend(ab, "Output:\x1b[0m", 11);
        }
        else {
            if (outRow < O.numRows) {
                int len = O.row[outRow].rsize - O.colOff;
                if (len < 0) len = 0;
                if (len > O.numCols) len = O.numCols;
                abAppend(ab, &O.row[outRow].render[E.colOff], len);
            }
            else {
                abAppend(ab, "~", 1);
            }
        }
        abAppend(ab, "\x1b[K\r\n", 5);  //clear line right of cursor
    }
}

void editorDrawStatusBar(struct abuf * ab){
    generalMoveCursor(ab, 0, E.fullScreenRows - 1);
    
    abAppend(ab, "\x1b[7m", 4); //invert color, will also use for selection
    //file name
    char status[80], rightStatus[80];
    int len = snprintf(status, sizeof(status), "%.20s - %d lines %s", (E.fileName ? E.fileName : "No Name"), E.numRows, (E.dirty ? "(modified)" : "") );
    if (len > E.fullScreenCols) len = E.fullScreenCols;
    abAppend(ab, status, len);

    int rightLen = snprintf(rightStatus, sizeof(rightStatus), "%d/%d", E.cy+1 , E.numRows);
    //fill rest of screen with spaces for the white bar effect
    while (len < E.fullScreenCols) {
        if (E.fullScreenCols - len == rightLen) {
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

void editorSetStatusMessage(const char * fmt, ...){
    //This is how you do function with varied amount of argument (variadic) like printf, 
    va_list ap;
    va_start(ap, fmt); //initalizes ap with start of variable list
    //nomrally you'd then use va_arg(va_list, <varl type>) to access next argument, but we just use vsnprintf to automatically parse fmt in the printf way
    vsnprintf(E.statusMsg, sizeof(E.statusMsg), fmt, ap);
    va_end(ap); //clean up for va_start
    E.statusMsg_time = time(NULL); //current time, in seconds since midnight Jan 1, 1970
}

void editorDrawMessageBar(struct abuf * ab){
    abAppend(ab, "\x1b[7m", 4);

    abAppend(ab, "\x1b[K", 3);
    int msgLen = strlen(E.statusMsg);
    if (msgLen > E.fullScreenCols) msgLen = E.fullScreenCols;
    time_t curTime = time(NULL) - E.statusMsg_time;
    if ( ( (time(NULL) - E.statusMsg_time) < 5 ) && msgLen) {
        abAppend(ab, E.statusMsg, msgLen);
    }
    abAppend(ab, "\x1b[m", 3);
}

void generalMoveCursor(struct abuf * ab, int x, int y) {
    char buf[32];
    snprintf(buf, sizeof(buf),"\x1b[%d;%dH", y, x);
    abAppend(ab, buf, strlen(buf));
}

//"member" functions
void initEditor(){
    E.currentMode = EDIT;
    updateWindowSizes();

    E.cx = 0;
    E.cy = 0;
    E.rx = 0;
    E.rowOff = 0;
    E.colOff = 0;
    E.numRows = 0;
    E.row = NULL;

    E.dirty = 0;
    E.fileName = NULL;
    E.statusMsg[0] = '\0';
    E.statusMsg_time = 0;
    E.activeWindow = TEXT_EDITOR;

    resetBrainfuck(&B);
    resetWindow(&O);
}

void abAppend(struct abuf * ab, const char * s, int len){
    char * new = (char *) realloc(ab->b, ab->len + len);
    if (new == NULL) return;

    memcpy(&new[ab->len], s, len);
    ab->b = new;
    ab->len += len;
}

void resetWindow(windowConfig* this) {
    free(this->row);
    this->row = NULL;

    this->cx = 0;
    this->cy = 0;
    this->rx = 0;
    this->rowOff = 0;
    this->colOff = 0;
    this->numRows = 0;
}

//windows
void modeSwitcher(enum topMode nextMode){
    E.currentMode = nextMode;
    write(STDOUT_FILENO, "\x1b[2J", 4);    //clear entire screen
    updateWindowSizes();
    editorRefreshScreen();
}

void updateWindowSizes(){
    if (getWindowSize(&E.fullScreenRows, &E.fullScreenCols) == -1) {
        die("getWindowSize");
    }
    //consider clearing screen if screen size changed. Like, windows that isn't written doesn't get refreshed
    switch (E.currentMode) {
        case EDIT:
            E.screenCols = E.fullScreenCols;
            E.screenRows = E.fullScreenRows - 2;
            break;
        case DEBUG:
            E.screenRows = (E.fullScreenRows * 3) / 5 - 2;
            E.screenCols = (E.fullScreenCols / 2) - 2;
            break;
    }

    E.dataArray.startX = E.screenCols + 2;
    E.dataArray.startY = 0;
    O.startX = 0;
    O.startY = E.screenRows + 2;
    O.numCols = E.screenCols;
    O.windowRows = E.fullScreenRows - E.screenRows - 1 - 2; //1 for border, 2 for status bar/message
}

//Barinfuck
void resetBrainfuck(brainFuckConfig* this){
    this->arraySize = 30000;
    memset(this->dataArray, 0, 30000);  //turn this into dynamic later
    this->arrayIndex = 0;
    
    this->debugMode = PAUSED;

    this->instCounter = 0;

    this->executionEnded = false;

    this->instX = 0;
    this->instY = 0;

    coordStackInit(&(this->bracketStack));

    this->errorMsg = NULL;
}

void instForward() { //igores comments
    char nextChar = '\0';
    while (nextChar != '+' && nextChar != '-' && nextChar != '>' && nextChar != '<' &&
            nextChar != '.' && nextChar != ',' && nextChar != '[' && nextChar != ']' && nextChar != '?') {
        erow *row = (B.instY >= E.numRows) ? NULL : & E.row[B.instY];
        //there is a - 1 because we don't need it to go 1 space outside the current line
        if (row && B.instX < row->size - 1 ) {
            B.instX++;
        }
        else if (B.instY < E.numRows && B.instX >= row->size - 1) {
            B.instY++;
            B.instX = 0;
        }

        erow *nextRow = (B.instY >= E.numRows) ? NULL : & E.row[B.instY];
        if (nextRow == NULL) return;
        if (B.instX < nextRow->size) {
            nextChar = nextRow->chars[B.instX];
        }
        else {
            nextChar = '\0';
        }
    }
}

void processBrainFuck(brainFuckConfig* this) {
    //validate inst
    if (this->instY >= E.numRows || this->executionEnded) {
        this->executionEnded = true;
        this->debugMode = PAUSED;
        
        if (this->errorMsg) {
            editorSetStatusMessage("Execution finished. %s Press F9 to go back to editor", this->errorMsg);
        }
        else {
            editorSetStatusMessage("Execution finished. Press F9 to go back to editor");
        }
        //Defintely need to give an error message, too. Put it in the brainfuck struct
        return;
    }

    if (this->instX >= E.row[this->instY].size) {
        //This gets triggered on empty line. Can also happen when user delete stuff in run time
        instForward();
        return;
    }

    char curInst = E.row[this->instY].chars[this->instX];

    //validate data cell
    if (this->arrayIndex < 0 || this->arrayIndex >= this->arraySize) {
        this->executionEnded = true;
        //preseumably there should be an error message set already
        return;
    }

    unsigned char* dataPtr = &(this->dataArray[this->arrayIndex]);

    switch(curInst){
        case '>':
            if (this->arrayIndex + 1 >= this->arraySize ) {
                this->executionEnded = true;
                editorSetStatusMessage("\x1b[7;31mError: Attemped to go out of array's upper bound.\x1b[0m\x1b[7m");
                this->errorMsg = "\x1b[7;31mError: Attemped to go out of array's upper bound.\x1b[0m\x1b[7m";
                editorRefreshScreen();
                //make this triger memory allocation later on
                return;
            }
            else {
                this->arrayIndex++;
                instForward();
            }
            this->instCounter++;
            break;
        case '<':
            if (this->arrayIndex - 1 < 0) {
                this->executionEnded = true;
                editorSetStatusMessage("\x1b[7;31mError: Attemped to go out of array's lower bound.\x1b[0m\x1b[7m");
                this->errorMsg = "\x1b[7;31mError: Attemped to go out of array's lower bound.\x1b[0m\x1b[7m";
                editorRefreshScreen();
                //we should be able to do a check for this before running the code
                return;
            }
            else {
                this->arrayIndex--;
                instForward();
                this->instCounter++;
            }
            this->instCounter++;
            break;
        case '+':
            (*dataPtr)++;
            instForward();
            this->instCounter++;
            break;
        case '-':
            (*dataPtr)--;
            instForward();
            this->instCounter++;
            break;
        case '.':
            //also check for change line and delete
            if (*dataPtr > 31 && *dataPtr < 127) {
                windowInsertChar(*dataPtr, &O);
            }
            instForward();
            this->instCounter++;
            break;
        case ',': {
            char* userInput = NULL;
            bool validInput = false;
            while (userInput == NULL || !validInput) {
                userInput = editorPrompt("Enter value for selected cell (unsigned 8-bit alphanumeric only): %s", 3); 
                if(userInput) {
                    if (userInput[0] > 31 && userInput[0] < 127) { //the returned buffer should be valid
                        if (userInput[0] >= 48 && userInput[0] <= 57) {
                            //We only consider it a number if first char is number
                            int potentialNum = atoi(userInput);
                            if(potentialNum == 0 && (userInput[0] != '0')) {
                                validInput = false;
                                free(userInput);
                                userInput = NULL;
                                editorSetStatusMessage("Invalid input");
                                editorRefreshScreen();
                                editorReadKey();
                            }
                            else if (potentialNum > 255) {
                                validInput = false;
                                free(userInput);
                                userInput = NULL;
                                editorSetStatusMessage("Invalid input -- Number exceeded 8-bit limit");
                                editorRefreshScreen();
                                editorReadKey();
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
                        //we don't want to let user cancel this input
                        validInput = false;
                        free(userInput);
                        userInput = NULL;
                    }
                }
            }
            free(userInput);
            userInput = NULL;

            instForward();
            this->instCounter++;
            break;
        }
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
                    if (this->instY >= E.numRows) {
                        skipping = false;
                        this->executionEnded = true;
                        editorSetStatusMessage("\x1b[7;31mError: Missing closing bracket.\x1b[0m\x1b[7m");
                        this->errorMsg = "\x1b[7;31mError: Missing closing bracket.\x1b[0m\x1b[7m";
                        break;
                    }
                    if (this->instX >= E.row[this->instY].size) {
                        instForward();
                        break;
                    }
                    char curInst = E.row[this->instY].chars[this->instX];

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
                //go back to last open bracket
                if (!coordStackIsEmpty(&this->bracketStack)) {
                    this->instX = coordStackTop(&this->bracketStack).x;
                    this->instY = coordStackTop(&this->bracketStack).y;
                    instForward(); //don't execute the open bracket again
                }
                else {
                    this->executionEnded = true;
                    editorSetStatusMessage("\x1b[7;31mError: Cannot find opening bracket.\x1b[0m\x1b[7m");
                    this->errorMsg = "\x1b[7;31mError: Cannot find opening bracket.\x1b[0m\x1b[7m";
                    editorRefreshScreen();
                }
                //to-do: regenerate stack if user edited code during run time 
            }
            else{
                //exit loop
                coordStackPop(&this->bracketStack);
                instForward();
            }
            break;
        case '?':
            this->debugMode = PAUSED;
            instForward();
            this->instCounter = 0;
            break;
        default:
            instForward();
            break;
    }
    E.cy = this->instY; //this is just to make sure we scroll to the right place
    if (this->debugMode == STEP_BY_STEP) this->debugMode = PAUSED;
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
        coordinate zeroCoord = {0,0};
        editorSetStatusMessage("Error: tried to pop empty stack. Returned {0,0}");
        editorRefreshScreen;
        editorReadKey();
        return zeroCoord;
    }
}

bool coordStackIsEmpty(coordStack* this) {
    return (this->top == -1);
}