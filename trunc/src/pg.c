// primitive graphics for Hello World PSP
//
// modified by ooba

#include <pspkernel.h>
#include <pspgu.h>
#include <pspdisplay.h>

#include "pg.h"

#include "font.h"
#include "fontNaga10.h"


//constants
#define     PIXELSIZE   1               //in short
#define     LINESIZE    512             //in short
#define     FRAMESIZE   0x44000         //in byte

//480*272 = 60*38
#define CMAX_X 60
#define CMAX_Y 38
#define CMAX2_X 30
#define CMAX2_Y 19
#define CMAX4_X 15
#define CMAX4_Y 9


//variables
//char *pg_vramtop=(char *)0x04000000;
#define pg_vramtop ((char *)0x04000000)
long pg_screenmode;
long pg_showframe;
long pg_drawframe;

unsigned int __attribute__((aligned(16))) gulist[512*512];
unsigned short __attribute__((aligned(16))) text_buf[512*512];


void pgWaitVn(unsigned long count)
{
    for (; count>0; --count) {
        sceDisplayWaitVblankStart();
    }
}


void pgWaitV()
{
    sceDisplayWaitVblankStart();
}


unsigned short* pgGetVramAddr(unsigned long x,unsigned long y)
{
    return text_buf + x + y*LINESIZE;
}


void pgPrint4(unsigned long x,unsigned long y,unsigned long color,const char *str)
{
    while (*str!=0 && x<CMAX4_X && y<CMAX4_Y) {
        pgPutChar(x*32,y*32,color,0,*str,1,0,4);
        str++;
        x++;
        if (x>=CMAX4_X) {
            x=0;
            y++;
        }
    }
}

void pgFillvram(unsigned long color)
{
    unsigned short *vptr0;       //pointer to vram
    unsigned long i;

    vptr0=pgGetVramAddr(0,0);
    for (i=0; i<FRAMESIZE/2; i++) {
        *vptr0=color;
        vptr0++;
    }
}

void pgBitBlt(unsigned long x,unsigned long y,unsigned long w,unsigned long h,unsigned long mag,const unsigned short *d)
{
    unsigned short *vptr0;       //pointer to vram
    unsigned short *vptr;        //pointer to vram
    unsigned long xx,yy,mx,my;
    const unsigned short *dd;

    vptr0=pgGetVramAddr(x,y);
    for (yy=0; yy<h; yy++) {
        for (my=0; my<mag; my++) {
            vptr=vptr0;
            dd=d;
            for (xx=0; xx<w; xx++) {
                for (mx=0; mx<mag; mx++) {
                    *vptr=*dd;
                    vptr+=PIXELSIZE*2;
                }
                dd++;
            }
            vptr0+=LINESIZE*2;
        }
        d+=w;
    }

}


void pgPutChar(unsigned long x,unsigned long y,unsigned long color,unsigned long bgcolor,unsigned char ch,char drawfg,char drawbg,char mag)
{
    unsigned short *vptr0;       //pointer to vram
    unsigned short *vptr;        //pointer to vram
    const unsigned char *cfont;     //pointer to font
    unsigned long cx,cy;
    unsigned long b;
    char mx,my;

//  if (ch>255) return;
    cfont=font+ch*8;
    vptr0=pgGetVramAddr(x,y);
    for (cy=0; cy<8; cy++) {
        for (my=0; my<mag; my++) {
            vptr=vptr0;
            b=0x80;
            for (cx=0; cx<8; cx++) {
                for (mx=0; mx<mag; mx++) {
                    if ((*cfont&b)!=0) {
                        if (drawfg) *vptr=color;
                    } else {
                        if (drawbg) *vptr=bgcolor;
                    }
                    vptr+=PIXELSIZE*2;
                }
                b=b>>1;
            }
            vptr0+=LINESIZE*2;
        }
        cfont++;
    }
}


void pgScreenFrame(long mode,long frame)
{
    pg_screenmode=mode;
    frame=(frame?1:0);
    pg_showframe=frame;
    if (mode==0) {
        //screen off
        pg_drawframe=frame;
        sceDisplaySetFrameBuf(0,0,0,1);
    } else if (mode==1) {
        //show/draw same
        pg_drawframe=frame;
        sceDisplaySetFrameBuf(pg_vramtop+(pg_showframe?FRAMESIZE:0),LINESIZE,PIXELSIZE,1);
    } else if (mode==2) {
        //show/draw different
        pg_drawframe=(frame?0:1);
        sceDisplaySetFrameBuf(pg_vramtop+(pg_showframe?FRAMESIZE:0),LINESIZE,PIXELSIZE,1);
    }
}

struct Vertex
{
	unsigned short u, v;
	unsigned short color;
	short x, y, z;
};

void pgScreenFlip()
{
    /*
    pg_showframe=(pg_showframe?0:1);
    pg_drawframe=(pg_drawframe?0:1);
    sceDisplaySetFrameBuf(pg_vramtop+(pg_showframe?FRAMESIZE:0),LINESIZE,PIXELSIZE,0);
    */
	struct Vertex* vertices = (struct Vertex*)sceGuGetMemory(2 * sizeof(struct Vertex));

	vertices[0].u = 0; vertices[0].v = 0;
	vertices[0].color = 0;
	vertices[0].x = 0; vertices[0].y = 0; vertices[0].z = 0;

	vertices[1].u = SCR_WIDTH; vertices[1].v = SCR_HEIGHT;
	vertices[1].color = 0;
	vertices[1].x = SCR_WIDTH; vertices[1].y = SCR_HEIGHT; vertices[1].z = 0;

    sceGuStart(GU_DIRECT,gulist);
    sceGuTexMode(GU_PSM_5551,0,0,0); // 16-bit RGBA
    sceGuTexImage(0,512,512,512,text_buf); // setup texture as a 512x512 texture, even though the buffer is only 512x272 (480 visible)
    sceGuTexFunc(GU_TFX_REPLACE,GU_TCC_RGBA); // don't get influenced by any vertex colors
    sceGuTexFilter(GU_NEAREST,GU_NEAREST); // point-filtered sampling
	sceGuDrawArray(GU_SPRITES,GU_TEXTURE_16BIT|GU_COLOR_5551|GU_VERTEX_16BIT|GU_TRANSFORM_2D,2,0,vertices);
    sceGuFinish();
    sceGuSync(0,0);
	sceGuSwapBuffers();
}


void pgScreenFlipV()
{
    pgWaitV();
    pgScreenFlip();
}


// by kwn
void Draw_Char_Hankaku(int x,int y,const unsigned char c,int col) {
    unsigned short *vr;
    unsigned char  *fnt;
    unsigned char  pt;
    unsigned char ch;
    int x1,y1;

    ch = c;

    // mapping
    if (ch<0x20)
        ch = 0;
    else if (ch<0x80)
        ch -= 0x20;
    else if (ch<0xa0)
        ch = 0;
    else
        ch -= 0x40;

    fnt = (unsigned char *)&hankaku_font10[ch*10];

    // draw
    vr = pgGetVramAddr(x,y);
    for(y1=0;y1<10;y1++) {
        pt = *fnt++;
        for(x1=0;x1<5;x1++) {
            if (pt & 1)
                *vr = col;
            else
                *vr = 0x8000;
            vr++;
            pt = pt >> 1;
        }
        vr += LINESIZE-5;
    }
}

// by kwn
void Draw_Char_Zenkaku(int x,int y,const unsigned char u,unsigned char d,int col) {
    // ELISA100.FNTに存在しない文字
    const unsigned short font404[] = {
        0xA2AF, 11,
        0xA2C2, 8,
        0xA2D1, 11,
        0xA2EB, 7,
        0xA2FA, 4,
        0xA3A1, 15,
        0xA3BA, 7,
        0xA3DB, 6,
        0xA3FB, 4,
        0xA4F4, 11,
        0xA5F7, 8,
        0xA6B9, 8,
        0xA6D9, 38,
        0xA7C2, 15,
        0xA7F2, 13,
        0xA8C1, 720,
        0xCFD4, 43,
        0xF4A5, 1030,
        0,0
    };
    unsigned short *vr;
    unsigned short *fnt;
    unsigned short pt;
    int x1,y1;

    unsigned long n;
    unsigned short code;
    int j;

    // SJISコードの生成
    code = u;
    code = (code<<8) + d;

    // SJISからEUCに変換
    if(code >= 0xE000) code-=0x4000;
    code = ((((code>>8)&0xFF)-0x81)<<9) + (code&0x00FF);
    if((code & 0x00FF) >= 0x80) code--;
    if((code & 0x00FF) >= 0x9E) code+=0x62;
    else code-=0x40;
    code += 0x2121 + 0x8080;

    // EUCから恵梨沙フォントの番号を生成
    n = (((code>>8)&0xFF)-0xA1)*(0xFF-0xA1)
        + (code&0xFF)-0xA1;
    j=0;
    while(font404[j]) {
        if(code >= font404[j]) {
            if(code <= font404[j]+font404[j+1]-1) {
                n = -1;
                break;
            } else {
                n-=font404[j+1];
            }
        }
        j+=2;
    }
    fnt = (unsigned short *)&zenkaku_font10[n*10];

    // draw
    vr = pgGetVramAddr(x,y);
    for(y1=0;y1<10;y1++) {
        pt = *fnt++;
        for(x1=0;x1<10;x1++) {
            if (pt & 1)
                *vr = col;
            else
                *vr = 0x8000;
            vr++;
            pt = pt >> 1;
        }
        vr += LINESIZE-10;
    }
}

// by kwn
void mh_print(int x,int y,const char *str,int color) {
    unsigned char ch = 0,bef = 0;

    while(*str != 0) {
        ch = (unsigned char)*str++;
        if (bef!=0) {
            Draw_Char_Zenkaku(x,y,bef,ch,color);
            x+=10;
            bef=0;
        } else {
            if (((ch>=0x80) && (ch<0xa0)) || (ch>=0xe0)) {
                bef = ch;
            } else {
                Draw_Char_Hankaku(x,y,ch,color);
                x+=5;
            }
        }
    }
}

//ちょい早いx1 - LCK
#define VRAM1 (unsigned long*)(0x04000000+128*2+64*512*2+0x40000000)
#define VRAM2 (unsigned long*)(0x04000000+128*2+64*512*2+0x40000000+0x44000)
void pgBitBltN1h(unsigned short *d)
{
    unsigned long *v0;      //pointer to vram
    unsigned long yy, xx;
    unsigned long *s;

    v0 = (pg_drawframe) ? VRAM2 : VRAM1;
    s = (unsigned long*)d;
    for (yy=0; yy<144; yy++) {
        for (xx = 0; xx < 120; xx++) {
            *v0++ = *s++;
        }
        v0 += (LINESIZE/2-120);
    }
}

/******************************************************************************/


void pgGuInit(void)
{
	sceGuDisplay(GU_FALSE);
	sceGuInit();

	sceGuStart(GU_DIRECT, gulist);
    sceGuDrawBuffer(GU_PSM_5551,(void*)0,BUF_WIDTH);
    sceGuDispBuffer(SCR_WIDTH,SCR_HEIGHT,(void*)0x44000,BUF_WIDTH);
    sceGuDepthBuffer((void*)0x88000,BUF_WIDTH);
	sceGuOffset(2048 - (SCR_WIDTH / 2), 2048 - (SCR_HEIGHT / 2));
	sceGuViewport(2048, 2048, SCR_WIDTH, SCR_HEIGHT);

	sceGuEnable(GU_SCISSOR_TEST);
	sceGuScissor(0, 0, SCR_WIDTH, SCR_HEIGHT);

	sceGuDisable(GU_ALPHA_TEST);
	sceGuDisable(GU_BLEND);
	sceGuDisable(GU_DEPTH_TEST);

	sceGuEnable(GU_TEXTURE_2D);
	sceGuTexMode(GU_PSM_5551, 0, 0, GU_FALSE);
	sceGuTexScale(1.0f / BUF_WIDTH, 1.0f / BUF_WIDTH);
	sceGuTexOffset(0, 0);
	sceGuTexFunc(GU_TFX_REPLACE, GU_TCC_RGBA);

	sceGuClutMode(GU_PSM_5551, 0, 0xff, 0);

	sceGuClearDepth(0);
	sceGuClearColor(0);

	sceGuFinish();
	sceGuSync(0, GU_SYNC_FINISH);

	sceDisplayWaitVblankStart();
	sceGuDisplay(GU_TRUE);
}

void blt_hard(unsigned short* pBuf,int x,int y,int w,int h,int rot,int bw,int bh,int mode)
{

}
