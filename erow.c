#include "erow.h"

void rowInsertChar(erow* this, int at, int c) {
    if (at < 0 || at > this->size) at = this->size; // validate at just in case
    char* temp = realloc(this->chars, this->size + 2); // 2 cuz the inserted char, and cuz size doesn't include null terminator
    assert(temp); // die is better though
    // if (temp == NULL) die("rowInsertChar");
    this->chars = temp;

    memmove(&this->chars[at + 1], &this->chars[at], this->size - at + 1); // +1 for null terminator
    this->size++;
    this->chars[at] = c;
    rowUpdate(this);
}

void rowDelChar(erow* this, int at) {
    if (at < 0 || at >= this->size) return;
    memmove(&this->chars[at], &this->chars[at + 1], this->size - at);
    this->size--;
    rowUpdate(this);
}

void rowAppendString(erow * this, char* s, size_t len) {
    this->chars = (char*) realloc(this->chars, this->size + len + 1);
    memcpy(&this->chars[this->size], s, len);
    this->size += len;
    this->chars[this->size] = '\0';
    rowUpdate(this);
}

void rowUpdate(erow* this) {
    int j;
    int tabs = 0;
    int extraSpace = 0;
    // find number of tab
    for (j = 0; j < this->size; j++) {
        if (this->chars[j] == '\t') tabs++;
    }
    extraSpace += tabs * (TAB_STOP - 1);

    free (this->render);
    this->render = (char*) malloc(this->size + extraSpace + 1);

    int idx = 0;
    for (j = 0; j < this->size; j++) {
        if (this->chars[j] == '\t') {
            this->render[idx++] = ' ';
            while (idx % TAB_STOP != 0) {
                this->render[idx++] = ' ';
            }
        }
        else {
            this->render[idx] = this->chars[j];
            idx++;
        }
    }
    this->render[idx] = '\0';
    this->rsize = idx;
}

void rowFree(erow* this) {
    free (this->chars);
    free (this->render);
}