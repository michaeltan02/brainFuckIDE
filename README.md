# Brain Fuck IDE
A terminal-based brainfuck IDE made in C, for Unix terminal (requires you to install Bash on Windows)

-----------------------------------------------------------------------------------------------------------------------------------------------------------------------
Brainfuck Introduction
-----------------------------------------------------------------------------------------------------------------------------------------------------------------------


-----------------------------------------------------------------------------------------------------------------------------------------------------------------------
List of Features
-----------------------------------------------------------------------------------------------------------------------------------------------------------------------

To be considered an IDE, a program needs to have at least 3 things integrated (* for complete, > for in progress):

    1. Interpreter  ------------------------   COMPLETE
        * Supports original the 8 operators for brainfuck, ignores all other characters
        * Stops execution if user tries to go outside or memory array
        * Display output in its own section, using a custom dynamic struct
        * Displays changes to the memory array
        
    2. Debugger  --------------------------   COMPLETE
        * Step-by-step execution (shows which step the user is on)
            *Automatically skip comments during execution
        * Can use ? to set breakpoints in the program and continue exeuction till breakpoints are reached
        * Can step out of loops
        * Has warning for when a loop executed >10000 times
        
    3. Text editor  -----------------------   BASE COMPLTE, MORE QOL FEATURES PLANNED
        * View/Edit text (includnig proper tab rendering)
        * Navigate file with arrow keys, page up/down, home, end, ctrl + arrow keys (to skip words)
        * Support vertical + horizontal scrolling
        * Persistent status bar and temporary status messages
        * Open file with ./michaelBfIDE <fiel name> (new file will be created if ran without file name argument)
        * Save (Ctrl + S) files. Prompts the user to enter a name if saving a new file, complete with error checking for file name size >255
        * Warning for quitting (Ctrl + Q) without saving
        * Auto-complete brackets and quotation marks
        * Supports window-reiszing
        > Syntac highlighting based on file TYPE
        > Searching
        > VIM-sytle navigation
        > Auto-indent
        > Selction, copy, cut, paste, undo
        
    Integration  -----------------------   IN-PROGRESS
        * User can now press F5 to enter debug mode (F8 to quit). In debug mode, the terminal will be split into 3 windows (instructions, data array, output)
        > Currently in the middle of porting interpter and debugger logic
        > Allow user to switch between windows
        
-----------------------------------------------------------------------------------------------------------------------------------------------------------------------
Brianfuck Implementation Choices
-----------------------------------------------------------------------------------------------------------------------------------------------------------------------
- Array is currently made of the traditional 30000 8-bit unsigned integer cells, with plans to make the array dynamic
- Choice of character set when outputing extended ASCII depend entirely on your operating system settings. On a computer with system locale set to English (United States), the windows terminal defauls to the Code page 437 character set. 
