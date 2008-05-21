/*
* $Id$
*/

void pgInit();
void pgWaitV();
void pgWaitVn(unsigned long count);
unsigned short* pgGetVramAddr(unsigned long x,unsigned long y);
void pgScreenFlip();
void pgFillvram(unsigned long color);
void mh_print(int x,int y,const char *str,int color);
void pgBitBlt(unsigned short *d);
void pgGuInit(void);

#define FONT_HEIGHT 9
#define SCR_WIDTH			480
#define SCR_HEIGHT			272
#define BUF_WIDTH			512
#define RGB(r,g,b) ((((b & 0xf8) << 7) | ((g & 0xf8) << 2) | ((r & 0xf8) >> 3))|0x8000)

