#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>




void printDataArray(char * array, char * data_ptr, int amount);   
int main(void)
{
    //the data array
    char data_array[30000] = {0}; 
    char *data_ptr = data_array;
    printf("Array initialized \n");
    printDataArray(data_array, data_ptr, 10);

    //get commands from user
    char instruction [4096];    //this should be a good enough buffer size
    printf("Awaiting Instructions (? for breakpoints): \n");
    bool firstOut = true;
    if (fgets(instruction, 4096, stdin) != NULL) {  //It's only null if eof
        //remember that fgets actually include \n as a character when it creates the string
        
        char *data_ptr = data_array;    
        int inst_ptr = 0;

        //process if the user want debug mode
        printf("Debug or Execute? (d/e) \n");
        bool debugMode = false, stepBystep = false, validInput = false, inALoop = false, steppingOut = false;
        char userInput =0; 
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
        
        //interpret instructions
        while (instruction[inst_ptr] != '\0' && instruction[inst_ptr] != '\n'){
            switch(instruction[inst_ptr]){
                case '>':
                    data_ptr++;
                    inst_ptr++;
                    break;
                case '<':
                    
                    data_ptr--;     //create a system so user cannot go out of bound
                    inst_ptr++;
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
                    if(firstOut){   //for now, store output in a string so it shows up properly in debug mode. After text editor, 
                                    //upgrade it to store output in a file (cuz C doesn't have automtiac memory safe string concatenation)
                        printf("Output: ");
                        firstOut = false;
                    }
                    printf("%c", *data_ptr);
                    inst_ptr++;
                    break;
                case ',':
                    printf("Entering input here: \n%s", instruction);   //shows user which , they are entering intput for
                    for (int i = 0; i < inst_ptr; i++)
                        printf(" ");
                    printf("^\n");
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
                    }
                    else{
						inALoop = false;
						steppingOut = false;
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
            if (stepBystep && !steppingOut){
                printf("Paused at \n%s", instruction);
                for (int i = 0; i < inst_ptr; i++)
                    printf(" ");
                printf("^ \nCurrent array \n");
                printDataArray(data_array, data_ptr, 10);

                //for if the user want out of debug mode
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
        
        printf("\nCode finished. Array updated. \n");
        printDataArray(data_array, data_ptr, 10);
    }
	printf("Press any key to quit.");
	getchar();
	return 0;
}


void printDataArray(char * array, char * data_ptr, int amount){
    printf("Character version:\n");
    for(int i=0; i<amount; i++){
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

