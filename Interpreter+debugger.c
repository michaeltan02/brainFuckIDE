#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#define printAmount                 10
#define strExpansionIncrement       2048    


void printDataArray(unsigned char * array, unsigned char * data_ptr, int amount);   
void specialPrint (unsigned char character);
void cleanStdin (void);
void syntaxHighLightPrint (char* inst);    

void red() {printf("\033[0;31m");}
void redBold(){printf("\033[1;31m");}
void green() {printf("\033[0;32m");}
void yellow() {printf("\033[0;33m");}
void blue() {printf("\033[0;34m");}
void purple() {printf("\033[0;35m");}
void cyan() {printf("\033[0;36m");}
void white(){printf("\033[0;37m");}
void bold(){printf("\033[1;37m");}
void resetColour () {printf("\033[0m");}

int main(void)
{
    //the data array
    unsigned char data_array[30000] = {0}; 
    unsigned char *data_ptr = data_array;
    green();    printf("Array initialized \n");
    resetColour();
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
    yellow();    printf("Awaiting Instructions (? for breakpoints): \n");
    resetColour();
    if (fgets(instruction, 4096, stdin) != NULL) {  //It's only null if eof
        //remember that fgets actually include \n as a character when it creates the string  

        //process if the user want debug mode
        bold();    printf("Debug or Execute? (d/e) \n");
        resetColour();
        while(!validInput){
            //scanf(" %c", &userInput);    //holy shit C's scanf take in the \n. 
            userInput = getchar();
            //fflush(stdin);    //this actually isn't defined
            cleanStdin();
            if (userInput == 'd' || userInput == 'D'){
                debugMode = true;
                validInput = true;
                green();    printf("Entered Debug mode.\n");
            }
            else if (userInput == 'e' || userInput == 'E'){
                debugMode = false;
                validInput = true;
                green();    printf("Normal Execution.\n");
            } 
            else{
                red();  printf("Invalid input. Try again.\n");
            }  
            resetColour();
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
                        red();  printf ("Illegal access Denied -- Attempt to increment data pointer out of array.\n");
                        problemEncountered = true;
                    }
                    break;
                case '<':
                    if(data_ptr > data_array){
                        data_ptr--;    
                        inst_ptr++;
                    }
                    else{
                        red();  printf ("Illegal access Denied -- Attempt to decrement data pointer out of array.\n");
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
                                red();
                                printf ("Problem encountered -- Cannot allocate more memory to store output.\nThe following will be dumped to Output.txt\n%s\nOutput string reset.\n", outputStr);
                                out_file = fopen("Output.txt", "a");
                                if(out_file == NULL){
                                    printf("Problem encountered -- cannot open file.\n");
                                    problemEncountered = true;
                                }
                                resetColour();
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
                    bold();   printf("Entering input for below\n");
                    resetColour();
                    //printf("%s", instruction);   //shows user which , they are entering intput for
                    syntaxHighLightPrint(instruction);
                    for (int i = 0; i < inst_ptr; i++){
                        printf(" ");
                    }
                    printf("^\n");
                    printDataArray(data_array, data_ptr, printAmount);
                    yellow();   printf("Your input: ");
                    *data_ptr = getchar();
                    resetColour();  cleanStdin();
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
                    char nextChar = instruction[inst_ptr];
                    if (nextChar!= '+' && nextChar!= '-' && nextChar!= '>' && nextChar!= '<' &&
                        nextChar!= '.' && nextChar!= ',' && nextChar!= '[' && nextChar!= ']')
                        continue;
                    break;
                //consider adding something to skip over comments
            }

            if(problemEncountered)
                break;
            
            //protection against infinite loop
            if (loopCount > 10000){
                red();  printf ("Warning -- A loop has executed 10000 times.\n");
                resetColour();
                stepBystep = true;
                steppingOut = false;
                loopCount = 0;
            }

            //only for debug mode
            if (stepBystep && !steppingOut){
                green();   printf("Paused at \n");
                resetColour();
                //printf("%s", instruction);
                syntaxHighLightPrint(instruction);
                for (int i = 0; i < inst_ptr; i++)
                    printf(" ");
                printf("^ \n");
                bold(); printf("Current output: ");
                resetColour();
                printf("%s\n", outputStr);
                bold(); printf("Current array: \n");
                resetColour();
                printDataArray(data_array, data_ptr, printAmount);

                //check if the user want out of debug mode
				do{
                    bold();
                    if (!inALoop)
                        printf("Press Enter to Step into; c to Continue.\n");
                    else
                        printf("Press Enter to Step into; c to Continue; s to Step out.\n");

                    //consider adding a quit button
                    
                    resetColour();
                    validInput = true;  //assumption
                    userInput = getchar();
                    if (userInput == 'c' || userInput == 'C'){
                        stepBystep = false;
                        cleanStdin();
                    }
                    else if (userInput == '\n'){
                        stepBystep = true;
                    }
                    else if ((userInput == 's' || userInput == 'S') && inALoop){
                        steppingOut = true;
                        cleanStdin();
                    }
                    else{
                        validInput = false;
                        cleanStdin();
                        printf("Invalid input. Try again.\n");
                    }
                }while(!validInput);
            }
        }
        
        //final output
        if(!problemEncountered){
            green();    printf("\nCode finished sucessfully.\n"); 
        }
        else{
            red();  printf("\nProblem encountered. Execution failed at:\n");
            resetColour();
            //printf("%s", instruction);
            syntaxHighLightPrint(instruction);
            for (int i = 0; i < inst_ptr; i++)
                printf(" ");
            printf("^ \n");
        }
        if(debugMode){
            green();    printf("Final output: ");
            resetColour();
            printf("%s\n", outputStr);
        }
        bold(); printf("Final array:\n");
        resetColour();
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

void syntaxHighLightPrint (char* inst){
    for(int i =0; inst[i]!= '\0'; i++){
        switch(inst[i]){
            case '>':
            case '<':
                cyan();
                break;
            case '+':
            case '-':
                white();
                break;
            case '.':
            case ',':
                purple();
                break;
            case '[':
            case ']':
                yellow();
                break;
            case '?':
                red();
                break;
            default:
                blue();
                break;
        }
        putchar(inst[i]);
    }
    resetColour();
    //putchar('\n');    //assuming there is a \n at the end of string
}

void cleanStdin(void){
    char temp;
    do{
        temp = getchar();
    }while (temp != '\n' && temp != EOF);
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
