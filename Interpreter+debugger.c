#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#define printAmount     10
#define maxOutput       2048    


void printDataArray(char * array, char * data_ptr, int amount);   
int main(void)
{
    //the data array
    char data_array[30000] = {0}; 
    char *data_ptr = data_array;
    printf("Array initialized \n");
    printDataArray(data_array, data_ptr, printAmount);
    
    //extra variables
    bool firstOut = true;
    bool problemEncountered = false;
    bool debugMode = false, stepBystep = false, validInput = false, inALoop = false, steppingOut = false;
    char userInput =0; 
    int inst_ptr = 0;
    char outputStr[maxOutput] = {0};
    int outputLength =0;
    bool tempOutputStrSoln = false;
    int loopCount = 0;

    //get commands from user
    char instruction [4096];    //this should be a good enough buffer size
    printf("Awaiting Instructions (? for breakpoints): \n");
    if (fgets(instruction, 4096, stdin) != NULL) {  //It's only null if eof
        //remember that fgets actually include \n as a character when it creates the string  

        //process if the user want debug mode
        printf("Debug or Execute? (d/e) \n");
        while(!validInput){
            //scanf(" %c", &userInput);    //holy shit C's scanf take in the \n. 
            userInput = getchar();
            fflush(stdin);
            if (userInput == 'd' || userInput == 'D'){
                debugMode = true;
                validInput = true;
                printf("Entered Debug mode.\n");
            }
            else if (userInput == 'e' || userInput == 'E'){
                debugMode = false;
                validInput = true;
                printf("Normal Execution.\n");
            } 
            else{
                printf("Invalid input. Try again.\n");
            }  
        }
        
        //Brainfuck interpreter
        while (instruction[inst_ptr] != '\0' && instruction[inst_ptr] != '\n'){
            switch(instruction[inst_ptr]){
                case '>':
                    if(data_ptr < (data_array+30000-1)){
                        data_ptr++;    
                        inst_ptr++;
                    }
                    else{
                        printf ("Illegal access Denied -- Attempt to increment data pointer out of array.\n");
                        problemEncountered = true;
                    }
                    break;
                case '<':
                    if(data_ptr > data_array){
                        data_ptr--;    
                        inst_ptr++;
                    }
                    else{
                        printf ("Illegal access Denied -- Attempt to decrement data pointer out of array.\n");
                        problemEncountered = true;
                    }
                    break;
                case '+':
                    ++*data_ptr;
                    inst_ptr++;
                    break;
                case '-':
                    --*data_ptr;
                    inst_ptr++;
                    break;
                case '.':
                    if(!debugMode){ 
                        if(firstOut){   
                            printf("Output: ");
                            firstOut = false;
                        }
                        printf("%c", *data_ptr);
                    }
                    else{
                        if(outputLength < (maxOutput - 1)){
                            strncat(outputStr, data_ptr,1);
                            //printf("Output: %s\n", outputStr);
                            outputLength++;
                        }
                        else{   //look into a way to do safe string concatenation instead
                            outputStr[0] = '\0';    //this is clever
                            outputLength =0;
                            tempOutputStrSoln = true;
                        }
                    }
                    inst_ptr++;
                    break;
                case ',':
                    printf("Entering input for below\n%s", instruction);   //shows user which , they are entering intput for
                    for (int i = 0; i < inst_ptr; i++){
                        printf(" ");
                    }
                    printf("^\n");
                    printDataArray(data_array, data_ptr, printAmount);
                    printf("Your input: ");
                    *data_ptr = getchar();
                    fflush(stdin);
                    inst_ptr++;
                    break;
                case '[':
                    if(*data_ptr){
                        inst_ptr++;
						inALoop = true;
                    }
                    else{
                        while(instruction[inst_ptr] != ']') inst_ptr++;
                    }
                    break;
                case ']':
                    if(*data_ptr){
                        while(instruction[inst_ptr] != '[') inst_ptr--;
                        if(debugMode) loopCount++;
                    }
                    else{
						inALoop = false;
						steppingOut = false;
                        loopCount = 0;
                        inst_ptr++;
                    }
                    break;
                case '?':
                    if(debugMode){
                        stepBystep = true;
                    }
                    inst_ptr++;
                    break;
                default:
                    inst_ptr++;
                    break;
            }

            if(problemEncountered)
                break;
            
            //protection against infinite loop
            if (loopCount > 10000){
                printf ("Warning -- A loop has executed 10000 times.\n");
                stepBystep = true;
                steppingOut = false;
            }

            //only for debug mode
            if (stepBystep && !steppingOut){
                printf("Paused at \n%s", instruction);
                for (int i = 0; i < inst_ptr; i++)
                    printf(" ");
                printf("^ \n");
                if(tempOutputStrSoln){
                    printf("Max output length reached. Oldder output will be hidden\nCurrent output: ...");
                }
                else{
                    printf("Current output: ");
                }
                printf("%s\n", outputStr);
                printf("Current array \n");
                printDataArray(data_array, data_ptr, printAmount);

                //check if the user want out of debug mode
				do{
                    if (!inALoop)
                        printf("Press Enter to Step into; press c to Continue.\n");
                    else
                        printf("Press Enter to Step into; press c to Continue; press s to Step out.\n");

                    validInput = true;  //assumption
                    userInput = getchar();
                    fflush(stdin);
                    if (userInput == 'c' || userInput == 'C')
                        stepBystep = false;
                    else if (userInput == '\n')
                        stepBystep = true;
                    else if ((userInput == 's' || userInput == 'S') && inALoop)
                        steppingOut = true;
                    else{
                        validInput = false;
                        printf("Invalid input. Try again.\n");
                    }
                }while(!validInput);
            }
        }
        
        //final output
        if(!problemEncountered){
            printf("\nCode finished sucessfully.\n"); 
        }
        else{
            printf("\nProblem encountered. Execution failed at:\n%s", instruction);
                for (int i = 0; i < inst_ptr; i++)
                    printf(" ");
                printf("^ \n");
        }
        if(debugMode){
            printf("Final output: %s\n", outputStr);
        }
        printf("Final array:\n");
        printDataArray(data_array, data_ptr, printAmount);
    }

	printf("Press any key to quit.");
	getchar();
	return 0;
}


void printDataArray(char * array, char * data_ptr, int amount){
    printf("Character version:\n");
    for(int i=0; i<amount; i++){    //do some check before printing the character, just be a bit smater
		printf("  ");
		printf("%c|", array[i]);
    }
    printf("\nInteger version:\n");
    for(int i=0; i<amount; i++){
            printf("%3d|", (int) array[i]);
    }
    printf("\n");
    int locationInArray = (int) (data_ptr - array);
    for (int i = 0; i < locationInArray; i++)
        printf("    ");
    printf("  ^\n");
}

