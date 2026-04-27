#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <stddef.h>
#include <hp165x.h>

#define WIDTH 80
#define MEMORY_SIZE (800*1024)

#define MAX_PICK_FILES 144

char memory[MEMORY_SIZE];
FILE* myFile=NULL;

static uint16_t pickFileList[MAX_PICK_FILES];
static uint16_t numPickFiles;

#define WRITE_OVERLAY_GRAY       0b110000000001
#define WRITE_OVERLAY_FOREGROUND 0b101000000001
#define WRITE_OVERLAY_BACKGROUND 0b100000000001
#define WRITE_OVERLAY_ERASE 	 0b111000000001

#define WIN_X1 10
#define WIN_X2 (WIDTH-WIN_X1)
#define WIN_Y1 3
#define WIN_Y2 (28-WIN_Y1)

static uint16_t savedX;
static uint16_t savedY;
static uint16_t savedFore;
static uint16_t savedBack;

static char* pickFileNamer(unsigned short i) {
	ROMDirEntry_t* dp = getROMDirEntry(i);
    if (dp == NULL)
        return "";
	static char name[MAX_FILENAME_LENGTH+1];
    unpadFilename(name, dp->name);
    return name;
}

static int compareFiles(const void* p1, const void* p2) {
    char* n1 = (char*)getROMDirEntry(*(uint16_t*)p1)->name;
    char* n2 = (char*)getROMDirEntry(*(uint16_t*)p2)->name;
    return strncasecmp(n1, n2, MAX_FILENAME_LENGTH);
}

static unsigned short pickFileLoader(void) {
    if (refreshDir()<0)
        return 0;
    
	ROMDirEntry_t* dp;
	int i = 0;
	numPickFiles = 0;
	while (NULL != (dp=getROMDirEntry(i)) && numPickFiles < MAX_PICK_FILES) {
        if (dp->type == 1) {
            pickFileList[numPickFiles++] = i;
        }
		i++;
	}
    delayTicks(500);
    
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

uint8_t openReadFile(const char* n) {
    if (myFile != NULL)
        fclose(myFile);
    if (n == NULL) {
        hp_set_window();
        short i = hpChooser(1, 1, WIN_X2-WIN_X1-2, WIN_Y2-WIN_Y1-2, 2, 10, pickFileLoader, pickFileNamer, CHOOSER_DISK_BASED);
        hp_clear_window();
        if (i < 0)
            return 0;
        n = pickFileNamer(i);
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
