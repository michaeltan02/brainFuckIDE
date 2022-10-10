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
**General**
- Program starts in text editor mode. Ppress F5 or F6 to enter debug/execution mode (if or not to stop at breakpoints). Press F9 to go back to editor mode.
- In debug/execution mode, the terminal will be split into 3 windows (editor, data array, output). 
	- All three windows are independently-scrollable with arrow keys, home/end, page up/down
	- Switch active window with Ctrl + W. Only active window (where your blinking cursor is) will be scrolled
- Persistent status bar showing file names, line number, debugger mode, and cell number
- Temporary status messages
- The overall size of the program automatically updates when re-sizing terminal window. The relative size of the inner windows can be customized in config.h

**Text editor**
- Open existing file with ./michaelBfIDE <fiel name> (new file will be created if ran without file name argument)
- Save files with Ctrl + s
	- Will prmopt user for file name with a status message (Esc to cancel)
	- Quit confirmation for quitting without saving (custimizable)
	- Alt-s to save file as different name
	- Error-checking for invalid file name
	- As a fail-safe for failed saving, saving first write to a temp file and rename it only if writing was successful
- Navigate file with arrow keys, page up/down, home, end, ctrl + arrow keys (to skip words)


- Brainfuck syntax highlighting based on file type (accepts .b and .bf)
- Search with Ctrl + F (no case match by default, Alt + F for case match)
- Auto-indent (toggleable)
- Undo with Ctrl + Z (# of undo customizable)

**Interpreter/Debugger**
- Supports original the 8 operators for brainfuck
- # for single-line comments
- ? to set breakpoint


- Contine execution with F5
- Step-by-step execution with F6 (skips comment)
- Step out of loop with F7
- Restart with F8

- Checks loop validity before entering debug/execution mode (toggleable)
- Once in debug/execution mode, status bar now shows debugger mode and cell number
- Fully supports run-time code edit
- Auto request breakpoint when executed too long without break (amount customizable in config.h) 
- Error messages for run-time error

- If active window is editor, Ctrl + P to set next instruction to cursor location
- If active window is data array, Ctrl + C to set value of selected cell
- Ctrl + J to snap editor windows to next insqruction, data array window to current cell 


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
   
