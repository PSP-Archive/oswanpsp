/*
* $Id$
*/

#ifndef FILEIO_H_
#define FILEIO_H_

#define MAX_PATH 256

#define SEEK_SET    0
#define SEEK_CUR    1
#define SEEK_END    2

typedef struct wsRomHeaderStruct
{
    char developperId;
    char colorSuport;
    char cartId;
    char unknown;
    char romSize;
    char saveSize;
    char isV;
    char RTC;
    short checkSum;

} __attribute__ ((packed)) WSROMHEADER, *PWSROMHEADER;

extern char CurDir[MAX_PATH];
extern char RomPath[MAX_PATH];
extern unsigned char *ROMMap[0x100];
extern unsigned long CartSize;

void fileioSetModulePath(int argc, char *argv[]);
void fileioGetModulePath(char *fn, int nSize);
void fileioGetStatePath(char *fn, int nSize, int slot);
void fileioGetSavePath(char *fn, int nSize);
char *fileioPathFindFileName(const char *fn);
int fileioOpenRom(void);
void fileioLoadIProm(void);
void fileioLoadData(void);
void fileioSaveData(void);
void fileioStateSave(int slot);
void fileioStateLoad(int slot);

#endif
