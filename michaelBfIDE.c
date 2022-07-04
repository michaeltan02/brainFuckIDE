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

#define MICHAEL_IDE_VER "0.02"
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

enum mode {
    EDIT = 1,
    DEBUG,
    EXECUTE
};

//data
typedef struct erow {
    int size;
    char * chars;
    int rsize;
    char * render;
} erow;

#define COORD_INIT {0, 0}

typedef struct windowConfig {
    int startX, startY;
    int cx, cy;
    int rx;
    int numRows;
    int numCols;
    int rowOff, colOff;
} windowConfig;

//config
struct editorConfig {
    int cx, cy;
    int rx;
    int screenRows;
    int screenCols;
    int rowOff, colOff;
    int numRows;

    int fullScreenRows;
    int fullScreenCols;

    windowConfig editor;
    windowConfig dataArray;
    windowConfig output;

    erow * row; //dynamic array
    int dirty;
    char * fileName;
    char statusMsg [255];    //bottleneck for status message length
    time_t statusMsg_time;
    struct termios orig_termio;

    enum window activeWindow;
    enum mode currentMode;
};

void initEditor(void);

//brainfuck interpter
struct brainFuckConfig {
    unsigned char dataArray[30000];
    bool debugMode, stepBystep, steppingOut;
    erow * output;
    int instX, instY;
};

void resetBrainfuck();

//append buffer (dynamic string)
struct abuf {
    char * b;
    int len;
};

#define ABUF_INIT {NULL, 0}
void abAppend(struct abuf * ab, const char * s, int len);
void abFree(struct abuf * ab) {free(ab->b);}

/*** functions ***/
//terminal
void enableRawMode(void);
void disableRawMode(void);
void die(const char * s);
int editorReadKey(void);
int getWindowSize (int * rows, int * cols);
int getCursorPosition(int * rows, int * cols);

//row operation
void editorInsertRow(int at, char *s, size_t len);
void editorUpdateRow(erow * row);
int editorRowCxToRx(erow * row, int cx);
void editorRowInsertChar(erow * row, int at, int c);
void editorRowDelChar(erow * row, int at);
void editorFreeRow(erow * row);
void editorDelRow(int at);
void editorRowAppendString(erow * row, char * s, size_t len);

//editor operation
void editorInsertChar(int c);
void editorInsertNewLine();
void editorDelChar();

//file i/o
void editorOpen(char * fileName);
char * editorRowToString(int * bufLen);
void editorSave();

//input
void editorProcessKeypress(void);
void editorMoveCursor(int key);
char * editorPrompt(char * prompt, int inputSizeLimit); //essentially our own scanf (-1 for no limit)

//output
void editorScroll(void);
void editorRefreshScreen(void);
void editorDrawRows(struct abuf * ab);
void drawBorder(struct abuf * ab);
void drawDataArray(struct abuf * ab);
void drawOutput(struct abuf * ab);
void editorDrawStatusBar(struct abuf * ab);
void editorDrawMessageBar(struct abuf * ab);
void editorSetStatusMessage(const char * fmt, ...);
void generalMoveCursor(struct abuf * ab, int x, int y);

//modes and windows
void modeSwitcher(enum mode nextMode);
void updateWindowSizes();

/*** global ***/
struct editorConfig E;
struct brainFuckConfig B;

int main(int argc, char * argv[]) {
	enableRawMode();
    initEditor();
    //editorOpen("testFile.txt"); //for testing in VScode
    if (argc >= 2){ //command would be kilo fileName, kilo would be first argument
        editorOpen(argv[1]);
    }

    editorSetStatusMessage("HELP: Ctrl-S = Save | Ctrl-Q = quit");

    while (1){
        editorRefreshScreen();
        editorProcessKeypress();
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

void die(const char *s){ //error handling
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

int getWindowSize (int* rows, int * cols){
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

int getCursorPosition(int * rows, int * cols){
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
void editorInsertRow(int at, char *s, size_t len){
    if (at < 0 || at > E.numRows) return;
    //allocate memory for a new row
    erow * temp = realloc(E.row, sizeof(erow) * (E.numRows + 1));
    if (temp == NULL) die("editorInserRow");
    E.row = temp;
    memmove(&E.row[at + 1], &E.row[at], sizeof(erow) * (E.numRows - at));

    E.row[at].size = len;
    E.row[at].chars = (char *) malloc(len + 1);
    memcpy(E.row[at].chars, s, len);
    E.row[at].chars[len] = '\0'; //we do this manually cuz we stripped off the end of the line (\r\n)

    E.row[at].rsize = 0;
    E.row[at].render = NULL;
    editorUpdateRow(&E.row[at]);

    E.numRows++;
    E.dirty++;
}

void editorUpdateRow(erow * row){
    int j;
    int tabs = 0;
    int extraSpace = 0;
    //find number of tab
    for (j = 0; j < row->size; j++) {
        if (row->chars[j] == '\t') tabs++;
    }
    extraSpace += tabs * (TAB_STOP - 1);

    free (row->render);
    row->render = (char *) malloc(row->size + extraSpace + 1);

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

int editorRowCxToRx(erow * row, int cx){
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

void editorRowInsertChar(erow * row, int at, int c){
    if (at < 0 || at > row->size) at = row->size; //validate at just in case
    char * temp = realloc(row->chars, row->size + 2); //2 cuz the inserted char, and cuz size doesn't include null terminator
    if (temp == NULL) die("editorRowInsertChar");
    row->chars = temp;

    memmove(&row->chars[at + 1], &row->chars[at], row->size - at + 1); //+1 for null terminator
    row->size++;
    row->chars[at] = c;
    editorUpdateRow(row);
    E.dirty++;
}

void editorRowDelChar(erow * row, int at){
    if (at < 0 || at >= row->size) return;
    memmove(&row->chars[at], &row->chars[at + 1], row->size - at);
    row->size--;
    editorUpdateRow(row);
    E.dirty++;
}

void editorFreeRow(erow * row){
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

void editorRowAppendString(erow * row, char * s, size_t len){
    row->chars = (char *) realloc(row->chars, row->size + len + 1);
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

void editorInsertNewLine(){
    if (E.cx == 0) {
        editorInsertRow(E.cy, "", 0);
    }
    else {
        erow * row = &E.row[E.cy];
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
void editorOpen(char * fileName){
    free(E.fileName);
    E.fileName = strdup(fileName);

    FILE *fp = fopen(fileName, "r");
    if (!fp) die("fopen");
    char * line = NULL;
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

char * editorRowToString(int * bufLen){
    int totalLen = 0;
    int j;
    for (j = 0; j < E.numRows; j++) {
        totalLen += E.row[j].size + 1;  //+ 1 is for \n
    }
    if (bufLen != NULL) *bufLen = totalLen;

    char * buf = malloc(totalLen);
    char * p = buf;
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
    char * buf = editorRowToString(&len);

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
            editorInsertChar('1');
            break;
        case F2_FUNCTION_KEY:
            editorInsertChar('2');
            break;
        case F3_FUNCTION_KEY:
            editorInsertChar('3');
            break;
        case F4_FUNCTION_KEY:
            editorInsertChar('4');
            break;
        case F5_FUNCTION_KEY:
            if (E.currentMode == EDIT) {
                modeSwitcher(DEBUG);
            }
            break;
        case F6_FUNCTION_KEY:
            editorInsertChar('F');
            editorInsertChar('6');
            break;
        case F7_FUNCTION_KEY:
            editorInsertChar('F');
            editorInsertChar('7');
            break;
        case F8_FUNCTION_KEY:
            if (E.currentMode != EDIT) {
                modeSwitcher(EDIT);
            }
            break;
        case F9_FUNCTION_KEY:
            editorInsertChar('F');
            editorInsertChar('9');
            break;
        case F10_FUNCTION_KEY:
            editorInsertChar('F');
            editorInsertChar('1');
            editorInsertChar('0');
            break;

        case CTRL_KEY('l'): 
        case '\x1b':
            break;
        case '(':
        case '{':
        case '[':
        case '"':
        case 39:    // this char is '
            editorInsertChar(c);
            char curChar = E.row[E.cy].chars[E.cx];
            //if cx is at end of row, curChar would be '\0'
            if (curChar == '\0' || curChar == ' ' || curChar == ')' || curChar == ']') {  //[] is only really for braninfuck, but shouldn't really affect other language either
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
                        editorInsertChar(c);
                        break;
                }
                editorMoveCursor(ARROW_LEFT);
            }
            break;
        default:
            if ( (c > 31 && c < 127) || c == '\t') {
                editorInsertChar(c);
            }
            break;
    }
    curQuitTimes = QUIT_TIMES;
}

void editorMoveCursor(int key){
    erow *row = (E.cy >= E.numRows) ? NULL : & E.row[E.cy];

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

char * editorPrompt(char * prompt, int inputSizeLimit){
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
            abAppend(ab, &E.row[fileRow].render[E.colOff], len);
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
    
    for(int i = 0; i < 5; i++) {
        snprintf(buf, sizeof(buf),"%3d|", B.dataArray[i]);
        abAppend(ab, buf, strlen(buf));
    }
}

void drawOutput(struct abuf * ab){
    generalMoveCursor(ab, E.output.startX, E.output.startY);
    abAppend(ab, "Output: ", 8);

    /*
    for (int y = 0; y < E.output.numRows; y++){
        int outRow = E.output.rowOff + y;
        if (outRow < E.output.numRows) {
            int len = B.output[outRow].rsize - E.colOff;
            if (len < 0) len = 0; //just display nothing
            if (len > E.dataArray.numCols) len = E.dataArray.numCols;
            abAppend(ab, &B.output[outRow].render[E.colOff], len);
        }

        abAppend(ab, "\x1b[K", 3);  //clear line right of cursor
        abAppend(ab, "\r\n", 2);
    }
    */
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
}

void abAppend(struct abuf * ab, const char * s, int len){
    char * new = (char *) realloc(ab->b, ab->len + len);
    if (new == NULL) return;

    memcpy(&new[ab->len], s, len);
    ab->b = new;
    ab->len += len;
}

void resetBrainfuck(){
    //B.dataArray = {0};
    B.stepBystep = true;
    B.steppingOut = false;

}

//windows
void modeSwitcher(enum mode nextMode){
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
    E.output.startX = 0;
    E.output.startY = E.screenRows + 2;
}