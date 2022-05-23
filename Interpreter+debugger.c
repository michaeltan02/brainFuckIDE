#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#define printAmount                 10
#define strExpansionIncrement       2048    


void printDataArray(unsigned char * array, unsigned char * data_ptr, int amount);   
void specialPrint (unsigned char character);

int main(void)
{
    //the data array
    unsigned char data_array[30000] = {0}; 
    unsigned char *data_ptr = data_array;
    printf("Array initialized \n");
    printDataArray(data_array, data_ptr, printAmount);
    
    //extra variables
    bool firstOut = true;
    bool problemEncountered = false;
    bool debugMode = false, stepBystep = false, validInput = false, inALoop = false, steppingOut = false;
    char userInput =0; 
    int inst_ptr = 0;
    int loopCount = 0;

    unsigned char * outputStr = (unsigned char *) calloc (strExpansionIncrement+1, sizeof (unsigned char));
    int outputMaxCapacity = strExpansionIncrement; 
    int outputCurLength =0;

    //initialize the output dump folder
    FILE *out_file = fopen("Output.txt", "w");
    fprintf(out_file, "%s\n", "Output will be dumped here if more memory cannot be allocated"); 
    fclose(out_file);

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
                        if(outputCurLength >= outputMaxCapacity){
                            int newSize = outputMaxCapacity + strExpansionIncrement + 1;
                            unsigned char * temp = (unsigned char *) realloc (outputStr, newSize);
                            //unsigned char* temp = NULL;   //for testing the file version
                            if(temp == NULL){
                                printf ("Problem encountered -- Cannot allocate more memory to store output.\nThe following will be dumped to Output.txt\n%s\nOutput string reset.\n", outputStr);
                                out_file = fopen("Output.txt", "a");
                                if(out_file == NULL){
                                    printf("Problem encountered -- cannot open file.\n");
                                    problemEncountered = true;
                                }
                                fprintf(out_file, "%s\n\n", outputStr); 
                                fclose(out_file);
                                outputStr[0] = '\0';
                                strcat(outputStr, "...");
                                outputCurLength = 3;
                            }
                            else{
                                outputStr = temp;
                                outputMaxCapacity += strExpansionIncrement;
                            }
                            
                        }
                        strncat(outputStr, data_ptr,1);
                        outputCurLength++;
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
                loopCount = 0;
            }

            //only for debug mode
            if (stepBystep && !steppingOut){
                printf("Paused at \n%s", instruction);
                for (int i = 0; i < inst_ptr; i++)
                    printf(" ");
                printf("^ \n");
                printf("Current output: ");
                printf("%s\n", outputStr);
                printf("Current array: \n");
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
    free(outputStr);
	return 0;
}


void printDataArray(unsigned char * array, unsigned char * data_ptr, int amount){
    printf("Character version\n");
    for(int i=0; i<amount; i++){    //do some check before printing the character, just be a bit smater
        if(array[i]>31 && array[i] < 127){
            printf("  %c|", array[i]);
        }
        else{
            specialPrint(array[i]);
        }
    }
    printf("\nInteger version\n");
    for(int i=0; i<amount; i++){
            printf("%3d|", array[i]);
    }
    printf("\n");
    int locationInArray = (int) (data_ptr - array);
    for (int i = 0; i < locationInArray; i++)
        printf("    ");
    printf("  ^\n");
}



void specialPrint (unsigned char character){
    switch(character){
        case 0:
            printf("   ");
            break;
        case 1:
            printf("SOH");
            break;
        case 2:
            printf("STX");
            break;
        case 3:
            printf("ETX");
            break;
        case 4:
            printf("EOT");
            break;
        case 5:
            printf("ENQ");
            break;
        case 6:
            printf("ACK");
            break;
        case 7:
            printf("BEL");
            break;
        case 8:
            printf(" BS");
            break;
        case 9:
            printf(" HT");
            break;
        case 10:
            printf(" LF");
            break;
        case 11:
            printf(" VT");
            break;
        case 12:
            printf(" FF");
            break;
        case 13:
            printf(" CR");
            break;
        case 14:
            printf(" SO");
            break;
        case 15:
            printf(" SI");
            break;
        case 16:
            printf("DLE");
            break;
        case 17:
            printf("DC1");
            break;
        case 18:
            printf("DC2");
            break;
        case 19:
            printf("DC3");
            break;
        case 20:
            printf("DC4");
            break;
        case 21:
            printf("NAK");
            break;
        case 22:
            printf("SYN");
            break;
        case 23:
            printf("ETB");
            break;
        case 24:
            printf("CAN");
            break;
        case 25:
            printf(" EM");
            break;
        case 26:
            printf("SUB");
            break;
        case 27:
            printf("ESC");
            break;
        case 28:
            printf(" FS");
            break;
        case 29:
            printf(" GS");
            break;
        case 30:
            printf(" RS");
            break;
        case 31:
            printf(" US");
            break;
        case 127:
            printf("DEL");
            break;
        default:
            printf("   ");
            break;
    }
    printf("|");
}
