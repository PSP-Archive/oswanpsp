/*
* $Id$
*/

#ifndef WS_H_
#define WS_H_

typedef struct stEEPROM
{
    unsigned short *data;
    int we;
} EEPROM;

extern int Run;
extern unsigned char *Cart;
extern unsigned char *StaticRAM;
extern unsigned char RAM[0x10000];
extern EEPROM IProm;
extern EEPROM CProm;
extern unsigned char *ROM[16];
extern unsigned char IO[0x100];
extern unsigned short InternalEeprom[64];
extern unsigned long SramSize;
extern unsigned short PromSize;
extern unsigned short MonoColor[8];
extern unsigned short ColorPalette[16][16];
extern int hblankTimer, hblankTimerPreset;
extern int vblankTimer, vblankTimerPreset;
extern int WsFrame;
extern int WsSkip;
extern int Fps;
extern int Drop;
extern int Cursor;
extern int ScreenSize;
extern int Vsync;

void wsInit(void);
void wsPdata(void);
void wsExit(void);
void wsReset(void);
void wsWriteMem(int,unsigned char);
void wsWritePort(unsigned char,unsigned char);
unsigned char wsReadPort(unsigned char);
void wsComEep(EEPROM*, unsigned short*, unsigned short*);
int wsExecuteFrame(int);
int wsExecute(void);

#endif
