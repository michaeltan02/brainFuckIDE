# Brain Fuck IDE
A terminal-based brainfuck IDE made in C. 
Why C as opposed to C++? It's because I plan to port it to at least DE1-SoC (running ARM assembly), and hopefully game consoles such as NDS and the GameBoy!

-----------------------------------------------------------------------------------------------------------------------------------------------------------------------
Brainfuck Introduction
-----------------------------------------------------------------------------------------------------------------------------------------------------------------------


-----------------------------------------------------------------------------------------------------------------------------------------------------------------------
List of Features
-----------------------------------------------------------------------------------------------------------------------------------------------------------------------

To be considered an IDE, a program needs to have at least 3 things:

    1. Interpreter  ------------------------   COMPLETE
        * Supports original the 8 operators for brainfuck, ignores all other characters
        * Stops execution if user tries to go outside or memory array
        * Stores user output in a string that dynamically increase in size when needed (alos dumps output in a .txt file if memory cannot be allocated)
        > Plans to support shortcuts of letters and numbers
        > Plans to include options to output assembly/machine code
        
    2. Debugger  --------------------------   COMPLETE
        * Step-by-step execution (shows which step the user is on)
        * Can use ? to set breakpoints in the program
        * Can step out of loops
        * Has warning for when a loop executed >10000 times
        
    3. Text editor  -----------------------   IN PROGRESS
        * Currently, the user get one chance to type the program, which will be scanned by fget
        > Plans to add save/load of program on computer, and the ability to go back to the text editor after execution
        > Plans to add syntax highlighting
        
Some features specific to brainfuck:

    1. Display the memory array  ------------ COMPPLETE
    
    2. Text geneator  ----------------------- IN PROGRESS
    
    3. Inclusion of example programs  ------- IN PROGRESS
    
    4. Code compressor  --------------------- IN PROGRESS
    
    5. Ability to change data array type  --- IN PROGRESS



-----------------------------------------------------------------------------------------------------------------------------------------------------------------------
Brianfuck Implementation Choices
-----------------------------------------------------------------------------------------------------------------------------------------------------------------------
- Array is made of the traditional 30000 8-bit unsigned integer cells
- Choice of character set when outputing extended ASCII depend entirely on your operating system settings. On a computer with system locale set to English (United States), the windows terminal defauls to the Code page 437 character set. 
