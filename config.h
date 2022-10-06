#pragma once
// config.h -- user tweakable parameters

// Text editor options
#define TAB_STOP 4
#define QUIT_TIMES 2 // how many times you need to press quit to quit without saving
#define AUTO_COMPLETE_BRACKET 1 // completes brackets and quotation marks. Works like VSCode
#define AUTO_INDENT 1
#define UNDO_STACK_SIZE 1000
#define CANCEL_SEARCH_RESTORE_CURSOR 1 // if disabled, ESC will behave same as enter when searching

// Brainfuck interpreter parameters
#define BREAK_POINT_REQUEST_THRESHOLD 4e+9 // brainfuck execution speed is ~5e+7 inst/s
#define BRAINFUCK_ARRAY_START_SIZE 30000
#define BRAINFUCK_ARRAY_INCREASE_INCREMENT 5000

// Window sizes (just tweak eidtor window size, the other two updates accordingly)
#define EDITOR_HEIGHT_IN_DEBUG_MODE 0.5 // fraction of full window height. must be <1
#define EDITOR_WIDTH_IN_DEBUG_MODE 0.5 // fraction of full window width. must be <1
