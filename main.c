#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <stddef.h>
#include <hp165x.h>

#define WIDTH 80
#define MEMORY_SIZE (800*1024)

char memory[MEMORY_SIZE];
FILE* myFile=NULL;

uint8_t openReadFile(const char* n) {
    char name[MAX_FILENAME_LENGTH+1];
    if (myFile != NULL)
        fclose(myFile);
    if (n == NULL) {
        if (quickPickFile(1, name)) 
            n = name;
        else
            return 0;
    }
    myFile = fopen(n, "r");
    if (myFile == NULL) {
        putText("\n[failed]\n");
        return 0;
    }
    putText("[loading ");
    putText(n);
    putText("]\n");
    return 1;
}

uint8_t openWriteFile(const char* n) {
    char inputFilename[MAX_FILENAME_LENGTH+1];
    
    if (n == NULL) {
        putText("Filename to save to? ");
        if (getText(inputFilename,sizeof(inputFilename)) < 0 || *inputFilename == 0)
            return 0;
        n = inputFilename;
    }
    if (myFile != NULL)
        fclose(myFile);
    myFile = fopen(n, "w");
    if (myFile == NULL) {
        putText("\n[failed]\n");
        return 0;
    }
    putText("\n[saving ");
    putText(n);
    putText("]\n");
    return 1;
}

void __attribute__((noinline)) drawCharacter(char c) {
    if (c == '\r')
        return;
    showTextCursor(0);
    if (c == '\b') {
        short x,y;
        x = getTextX();
        y = getTextY();
        if (x>0)
            x--;
        else {
            y--;
            x=WIDTH-1;
        }
        setTextXY(x,y);
        putChar(' ');
        setTextXY(x,y);
    }
    else {
        putChar(c);
    }
}

void __attribute__((noinline)) writeCharacterToFile(char c) {
    if (myFile != NULL) {
        if (c == 26) { 
            if (fclose(myFile)<0)
                putText("[error]\n");
            else
                putText("[saved]\n");
            
            myFile = NULL;
            return;
        }
        fputc(c, myFile);
        return;
    }
}

char __attribute__((noinline)) readCharacter(void) {

    if (!isTextCursorVisible())
        updateTextCursor(1);
    
    InputEvent_t e;
    if (getInputEvent(&e) && e.type == INPUT_KEY) {
        if (e.data.key.nativeKey == HP_KEY_STOP)
            reload();
        return e.data.key.character == '\n' ? '\r' : e.data.key.character;
    }
    else
        return 0;
}

char readCharacterFromFile(void) {
    if (myFile == NULL) {
        return 26;
    }

    int c = fgetc(myFile);
    if (c<0) {
        fclose(myFile);
        myFile = NULL;
        putText("[loaded]\n");
        return 26;
    }
    
    if (c == 0x0A)
        return 0x0D;
    else if (c == 0x0D)
        return 0;
    
    return c;
}

void __attribute__((noinline)) drawString(char* s) {
    updateTextCursor(0);
    putText(s);
    setTextCursorXY(getTextX(),getTextY());
    updateTextCursor(1);
}

extern uint8_t PRNlword; // relative to start of memory[]

int
main(int argc, char** argv) {
	(void)argc;
	(void)argv;
	
	initKeyboard(1);
	initScreen(392, WRITE_BLACK);
    
    *(uint32_t*)(memory+(uint32_t)&PRNlword) = getSeed32();
    
    asm("move.l #memory,%A0\n"
        "move.l #" _QUOTE(MEMORY_SIZE) ",%D0\n"
        "jmp LAB_COLD\n");
    return 0;
}
