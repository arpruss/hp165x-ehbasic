#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <stddef.h>
#include <hp165x.h>

#define WIDTH 80

char memory[200*1024];

FILE* myFile=NULL;

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

char writeBuffer[256];
short writeBufferPos=0;

void __attribute__((noinline)) writeCharacterToFile(char c) {
    if (myFile != NULL) {
        if (c == 26) { 
            fclose(myFile);
            myFile = NULL;
            return;
        }
        fputc(c, myFile);
        return;
    }
    if (c == '\r') {
        writeBuffer[writeBufferPos] = 0;
        if (writeBuffer[0] == '$') {
            if (myFile != NULL)
                fclose(myFile);
            if (!strncmp(writeBuffer+1,"WRITE ",6)) {
                myFile = fopen(writeBuffer+1+6,"w");
            }
            else if (!strncmp(writeBuffer+1,"READ ",5)) {
                char *space = strchr(writeBuffer+1+5,' ');
                if (space != NULL) {
                    *space = 0;
                }
                myFile = fopen(writeBuffer+1+5,"r");
            }
        }
        writeBufferPos = 0;
    }
    else {
        if (writeBufferPos + 1 < sizeof(writeBuffer))
            writeBuffer[writeBufferPos++] = c;
    }
}

char __attribute__((noinline)) readCharacter(void) {

    if (!isTextCursorVisible())
        updateTextCursor(1);
    
    InputEvent_t e;
    if (getInputEvent(&e) && e.type == INPUT_KEY) {
        if (e.data.key.nativeKey == KEY_STOP)
            reload();
        return e.data.key.character;
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

main(int argc, char** argv) {
	(void)argc;
	(void)argv;
	
	initKeyboard(1);
	initScreen(392, WRITE_BLACK);
    drawString("ehbasic starting...\n");
    
    asm("move.l #memory,%A0\n"
        "move.l #(200*1024),%D0\n"
        "jmp LAB_COLD\n");
}