#ifndef MENU_H_
#define MENU_H_

typedef struct filelist {
    char name[256];
    int type;
} FILELIST;

int menuList(void);
void menuPrintList(int);
int menuRomList(unsigned int);
int menuGetFileList(void);
void menuPrintRomList(int, int);
void menuStateList(unsigned int, int);
void menuStateSavePrintList(int);
void menuConfig(unsigned int, int);

#endif
