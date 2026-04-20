#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <stddef.h>
#include <hp165x.h>

#define WIDTH 80
#define MEMORY_SIZE (400*1024)

#define MAX_PICK_FILES 144

char memory[MEMORY_SIZE];
char writeBuffer[32];
unsigned short writeBufferPos=0;
FILE* myFile=NULL;

static uint16_t pickFileList[MAX_PICK_FILES];
static uint16_t numPickFiles;

#define WRITE_OVERLAY_GRAY       0b110000000001
#define WRITE_OVERLAY_FOREGROUND 0b101000000001
#define WRITE_OVERLAY_BACKGROUND 0b100000000001
#define WRITE_OVERLAY_ERASE 	 0b111000000001

#define WIN_X1 6
#define WIN_X2 (WIDTH-WIN_X1)
#define WIN_Y1 3
#define WIN_Y2 (28-WIN_Y1)

static uint16_t savedX;
static uint16_t savedY;
static uint16_t savedFore;
static uint16_t savedBack;

static char* pickFileNamer(unsigned short i) {
	DirEntry_t d;
	static char name[MAX_FILENAME_LENGTH+1];
	
	if (-1 != getDirEntry(pickFileList[i], &d)) {
		strcpy(name, d.name);
		return name;
	}
	else {
		return "";
	}
}

static int compareFiles(const void* p1, const void* p2) {
    DirEntry_t d1;
    DirEntry_t d2;
    d1.name[0] = 0;
    d2.name[0] = 0;
    getDirEntry(*(uint16_t*)p1, &d1);
    getDirEntry(*(uint16_t*)p2, &d2);
    return strcasecmp(d1.name, d2.name);
}

static unsigned short pickFileLoader(void) {
	DirEntry_t d;
	int i = 0;
	numPickFiles = 0;
	while (-1 != getDirEntry(i, &d) && numPickFiles < MAX_PICK_FILES) {
        if (d.type == 1) 
            pickFileList[numPickFiles++] = i;
		i++;
	}
    
    if (numPickFiles > 0) {
        qsort(pickFileList, numPickFiles, sizeof(uint16_t), compareFiles);
    }
    
	return numPickFiles;
}

void hp_set_window(void) {
	savedFore = getTextForeground();
	savedBack = getTextBackground();
	savedX = getTextX();
	savedY = getTextY();
	uint16_t h = getFontHeight();
	*SCREEN_MEMORY_CONTROL = WRITE_OVERLAY_GRAY;
	frameRectangle(WIN_X1*FONT_WIDTH,WIN_Y1*h,WIN_X2*FONT_WIDTH,WIN_Y2*h,8);
	*SCREEN_MEMORY_CONTROL = WRITE_OVERLAY_BACKGROUND;
	fillRectangle(WIN_X1*FONT_WIDTH,WIN_Y1*h,WIN_X2*FONT_WIDTH,WIN_Y2*h);
	setTextWindow(WIN_X1,WIN_Y1,WIN_X2,WIN_Y2);
	setTextColors(WRITE_OVERLAY_FOREGROUND,WRITE_OVERLAY_BACKGROUND);
	setTextXY(0,0);
}

void hp_clear_window() {
	uint16_t h = getFontHeight();
	*SCREEN_MEMORY_CONTROL = WRITE_OVERLAY_ERASE;
	fillRectangle(WIN_X1*FONT_WIDTH-8,WIN_Y1*h-8,WIN_X2*FONT_WIDTH+8,WIN_Y2*h+8);
	setTextWindow(0,0,0,0);
	setTextColors(savedFore,savedBack);
	setTextXY(savedX,savedY);
}

uint8_t openReadFile(void) {
    hp_set_window();
	short i = hpChooser(1, 1, WIN_X2-WIN_X1-2, WIN_Y2-WIN_Y1-2, 2, 10, pickFileLoader, pickFileNamer, CHOOSER_DISK_BASED);
    hp_clear_window();
    if (i < 0)
        return 0;
    if (myFile != NULL)
        fclose(myFile);
    myFile = fopen(pickFileNamer(i), "r");
    if (myFile == NULL)
        return 0;
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
            fclose(myFile);
            myFile = NULL;
            putText("[loaded]\n");
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
                if (writeBuffer[1+5]) 
                    myFile = fopen(writeBuffer+1+5,"r");
                else 
                    openReadFile();
            }
        }
        writeBufferPos = 0;
    }
    else {
        if (writeBufferPos + 1u < sizeof(writeBuffer))
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

int
main(int argc, char** argv) {
	(void)argc;
	(void)argv;
	
	initKeyboard(1);
	initScreen(392, WRITE_BLACK);
    drawString("ehbasic starting...\n");
    
    asm("move.l #memory,%A0\n"
        "move.l #" _QUOTE(MEMORY_SIZE) ",%D0\n"
        "jmp LAB_COLD\n");
    return 0;
}