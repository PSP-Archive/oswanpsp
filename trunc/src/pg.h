// primitive graphics for Hello World PSP

typedef struct Vertex16 {
    unsigned short u, v;
	short x, y, z;
} Vertex16;

void pgInit();
void pgWaitV();
void pgWaitVn(unsigned long count);
unsigned short* pgGetVramAddr(unsigned long x,unsigned long y);
void pgScreenFrame(long mode,long frame);
void pgScreenFlip();
void pgScreenFlipV();
void pgPrint(unsigned long x,unsigned long y,unsigned long color,const char *str);
void pgPrint2(unsigned long x,unsigned long y,unsigned long color,const char *str);
void pgPrint4(unsigned long x,unsigned long y,unsigned long color,const char *str);
void pgFillvram(unsigned long color);
void pgBitBlt(unsigned long x,unsigned long y,unsigned long w,unsigned long h,unsigned long mag,const unsigned short *d);
void pgPutChar(unsigned long x,unsigned long y,unsigned long color,unsigned long bgcolor,unsigned char ch,char drawfg,char drawbg,char mag);
void mh_print(int x,int y,const char *str,int color);
void pgBitBltN1h(unsigned short *d);
void pgGuInit(void);
void blt_hard(unsigned short* pBuf,int x,int y,int w,int h,int rot,int bw,int bh,int mode);

#define FONT_HEIGHT 9
#define SCR_WIDTH			480
#define SCR_HEIGHT			272
#define BUF_WIDTH			512
#define RGB(r,g,b) ((((b & 0xf8) << 7) | ((g & 0xf8) << 2) | ((r & 0xf8) >> 3))|0x8000)

