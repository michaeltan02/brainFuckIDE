# Brain Fuck IDE
<pre>
  __  __ _      _                _ _       ____            _        __            _      _____ _____  ______ 
 |  \/  (_)    | |              | ( )     |  _ \          (_)      / _|          | |    |_   _|  __ \|  ____|
 | \  / |_  ___| |__   __ _  ___| |/ ___  | |_) |_ __ __ _ _ _ __ | |_ _   _  ___| | __   | | | |  | | |__   
 | |\/| | |/ __| '_ \ / _` |/ _ \ | / __| |  _ <| '__/ _` | | '_ \|  _| | | |/ __| |/ /   | | | |  | |  __|  
 | |  | | | (__| | | | (_| |  __/ | \__ \ | |_) | | | (_| | | | | | | | |_| | (__|   <   _| |_| |__| | |____ 
 |_|  |_|_|\___|_| |_|\__,_|\___|_| |___/ |____/|_|  \__,_|_|_| |_|_|  \__,_|\___|_|\_\ |_____|_____/|______| (v1.0)
</pre>                                                                                                        
                                                                                                         

A terminal-based brainfuck IDE made in C, for Unix terminal

-----------------------------------------------------------------------------------------------------------------------------------------------------------------------
Introduction
-----------------------------------------------------------------------------------------------------------------------------------------------------------------------
This IDE is a combination of a basic but fully functional text editor, a brainfuck interpreter, and a brianfuck debugger.\
The program will start with the editor window maximized, and split into **three windows (editor, data array, output)** upon hitting F5 (debug) or F6 (execute). 

The focus of the program is on **visualization** of brainfuck execution. With each instruction executed, you will see exactly how the instruction pointer and data array updates. 
Due to this focus, the program *does not perform optimization*. Instead, I decided to play to the strength of using an interpreter and designed the IDE to fully **support code editing during run-time**. 

Below you will find a basic user guide, screenshots, the full list of features, list of hotkeys, brainfuck implementation decisions, and desgin decision/challenges. 

P.S. If you have never heard of the esoteric langauge brainfuck, here is a quick video explaination: [Brainf**k in 100 Seconds](https://www.youtube.com/watch?v=hdHjjBS4cs8)

-----------------------------------------------------------------------------------------------------------------------------------------------------------
Screenshots & Basic Guide
-----------------------------------------------------------------------------------------------------------------------------------------------------------

-----------------------------------------------------------------------------------------------------------------------------------------------------------------------
Full List of Features
-----------------------------------------------------------------------------------------------------------------------------------------------------------------------
To be considered an IDE, a program needs to have at least 3 things integrated (* for complete, > for in progress):

    1. Interpreter  ------------------------   COMPLETE
        * Supports original the 8 operators for brainfuck, ignores all other characters
        * Stops execution if user tries to go outside or memory array
        * Display output in its own section, using a custom dynamic struct
        * Displays changes to the memory array
        * Highlights current instruction and data cell
		* NOTE: Since this is an interpreter, I decided to go the extra mile and made sure to allow one to edit the code during run time.
        
    2. Debugger  --------------------------   COMPLETE
  		* Enter debugger by pressing F5      
		* Step-by-step execution with F6
            *Automatically skip comments during execution
        * Can use ? to set breakpoints. Continue to break point with F5
        * Step out of loop with F7
		* Restart with F8
		* Quit to editor with F9
        * Has warning for when >10000 times executed without a break. This is to compensate for the fact that there is no pause button
        > Plan to add error checking for incomplete loop and stepping outside data array (will be called automatically before entering debug mode, can also be called via hotkey)
        
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
        > Syntax highlighting based on file TYPE
        > Searching
        > Auto-indent
        > Save-as for existing files
        > Line number
        
    Integration  --------------------------   IN-PROGRESS
        * User can now press F5 to enter debug mode. In debug mode, the terminal will be split into 3 windows (instructions, data array, output)
        * When in debug mode. Press F5 to continue to breakpoint, F6 to step-into, F8 to quit to 
        > Plan to let user Allow user to switch which window they want to scroll
        > Plan to allowe the user to switch between displaying data array as numbers and as characters
    
    BONUS: raw input viewer ---------------   COMPLETE
        * I also included a helper program I made for this project, rawInputViewer
        * As its name suggest, it lets you see the raw ASCII code and escape sequences your terminal recieves. Useful if you want to add your own special keys to the IDE. 
        * Ctrl + Q to quit, Ctrl + C to clear the screen

-----------------------------------------------------------------------------------------------------------------------------------------------------------------------
Brianfuck Implementation Choices
-----------------------------------------------------------------------------------------------------------------------------------------------------------------------
- Cell size is 8-bit unsigned
- Array is dynamic, with starting length and increase increment custimizable in config.h
- ',' input operator accepts numbers (range form 0 - 255) or ASCII characters. Each ',' take ONE byte, and cull the rest of the input
- '.' output operator only prints cells ranging 20 - 127 (i.e., ASCII, excluding control char). Additionally, 8 (or 127) for backspace, 9 for tab, 10 for new line

-----------------------------------------------------------------------------------------------------------------------------------------------------------
Config.h
-----------------------------------------------------------------------------------------------------------------------------------------------------------
This file includes some parameters that can be tweaked (text editor settings, brainfuck settings, window sizes).
It *IS* a header file though, so you do need to be re-compile the program before the changes take effect.
For non-programmers, that just means you need to enter **make** into your terminal before changes take effect.
 
-----------------------------------------------------------------------------------------------------------------------------------------------------------------------
Special thanks
-----------------------------------------------------------------------------------------------------------------------------------------------------------------------
- The text editor is based on [antirez's kilo](http://antirez.com/news/108), with some additional features and structural changes
- I learnt about kilo and UNIX I/O throught this [wonderful guide](https://viewsourcecode.org/snaptoken/kilo/index.html)
- The logo was generated using http://patorjk.com/software/taag/#p=display&f=Big&t=Michael'%20Brainfuck%20IDE
- Some useful tools and resources:
    - A list of escape sequences: https://vt100.net/docs/vt100-ug/chapter3.html#ED
    - The ASCII table I used as reference: https://www.rapidtables.com/code/text/ascii-table.html
    - I learnt about stack implementation here: https://www.digitalocean.com/community/tutorials/stack-in-c
   
