
/*** includes ***/
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <termio.h>
#include <ctype.h>
#include <errno.h>
#include <sys/ioctl.h>

/*** Definitions ***/
#define CTRL_KEY(k) ((k) & 0x1f)    //interesitng, first time seeing function-like macro
struct termios orig_termio;

/*** functions ***/
//terminal
void enableRawMode(void);
void disableRawMode(void);
void die(const char * s);
char editorReadKey(void);

//input
void editorProcessKeypress(void);
void editorMoveCursor(char c);

int main(void)
{
	enableRawMode();

    write(STDOUT_FILENO, "\x1b[2J", 4);    //clear entire screen
    write(STDOUT_FILENO, "\x1b[H", 3);  //moves cursor position
    
    while (1){
        editorProcessKeypress();
    }

    return 0;
}

//terminal
void enableRawMode(){
    if (tcgetattr(STDIN_FILENO, &orig_termio) == -1){
        die("tcgetattr");
    }
    atexit(disableRawMode); //it's cool that this can be placed anywhere

    struct termios raw = orig_termio;
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
    if(tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termio) == -1){
        die("tcsetattr");   
    }
}

void die(const char *s){ //error handling
    write(STDOUT_FILENO, "\x1b[2J", 4); 
    write(STDOUT_FILENO, "\x1b[H", 3);
    perror(s);
    exit(1);
}

char editorReadKey(void){
    int nread;
    char c;
    while ((nread = read(STDIN_FILENO, &c, 1)) != 1){
        if(nread == -1 && errno != EAGAIN) die("read");
    }
    return c;
}

//input
void editorProcessKeypress(){
    char c = editorReadKey();
    switch (c) {
        case CTRL_KEY('q'):
            exit(0);
            break;
        case CTRL_KEY('c'):
            write(STDOUT_FILENO, "\x1b[2J", 4);    //clear entire screen
            write(STDOUT_FILENO, "\x1b[H", 3);  //moves cursor position
            break;
        case 27:
            write(STDOUT_FILENO, "\x1b[0;31mESC\r\n\x1b[0m", 16);
            break;
        default:
            if(iscntrl(c))
                printf ("%d\r\n", c);
            else
                printf ("%d(%c)\r\n", c, c);
            break;
    }
}
