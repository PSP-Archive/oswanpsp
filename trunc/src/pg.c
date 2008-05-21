/*
* $Id$
*/

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

unsigned int __attribute__((aligned(16))) gulist[512*512];
unsigned short __attribute__((aligned(16))) text_buf[512*512];

struct Vertex
{
	unsigned short u, v;
	unsigned short color;
	short x, y, z;
};

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

#define SLICE (32)
void pgBitBlt(unsigned short *d)
{
	int start, end, width;
	int sx = 0, sy = 0, sw = 240, sh = 144;
	int dx = 0, dy = 0;

    sceGuStart(GU_DIRECT,gulist);
    sceGuTexMode(GU_PSM_5551,0,0,0);
    sceGuTexImage(0,sw,sh,sw*2,d);
    sceGuTexFunc(GU_TFX_REPLACE,GU_TCC_RGBA);
    sceGuTexFilter(GU_NEAREST,GU_NEAREST);
	for (start = sx, end = sx+sw; start < end; start += SLICE, dx += SLICE)
	{
		struct Vertex* vertices = (struct Vertex*)sceGuGetMemory(2 * sizeof(struct Vertex));
		width = (start + SLICE) < end ? SLICE : end-start;

		vertices[0].u = start; vertices[0].v = sy;
		vertices[0].color = 0;
		vertices[0].x = dx; vertices[0].y = dy; vertices[0].z = 0;

		vertices[1].u = start + width; vertices[1].v = sy + sh;
		vertices[1].color = 0;
		vertices[1].x = dx + width; vertices[1].y = dy + sh; vertices[1].z = 0;

		sceGuDrawArray(GU_SPRITES,GU_TEXTURE_16BIT|GU_COLOR_5551|GU_VERTEX_16BIT|GU_TRANSFORM_2D,2,0,vertices);
	}
    sceGuFinish();
    sceGuSync(0,0);
}

void pgScreenFlip()
{
	int start, end, width;
	int dx = 0;

    sceGuStart(GU_DIRECT,gulist);
    sceGuTexMode(GU_PSM_5551,0,0,0); // 16-bit RGBA
    sceGuTexImage(0,512,512,512,text_buf); // setup texture as a 512x512 texture, even though the buffer is only 512x272 (480 visible)
    sceGuTexFunc(GU_TFX_REPLACE,GU_TCC_RGBA); // don't get influenced by any vertex colors
    sceGuTexFilter(GU_NEAREST,GU_NEAREST); // point-filtered sampling
	for (start = 0, end = SCR_WIDTH; start < end; start += SLICE, dx += SLICE)
	{
		struct Vertex* vertices = (struct Vertex*)sceGuGetMemory(2 * sizeof(struct Vertex));
		width = (start + SLICE) < end ? SLICE : end-start;

		vertices[0].u = start; vertices[0].v = 0;
		vertices[0].color = 0;
		vertices[0].x = dx; vertices[0].y = 0; vertices[0].z = 0;

		vertices[1].u = start + width; vertices[1].v = SCR_HEIGHT;
		vertices[1].color = 0;
		vertices[1].x = dx + width; vertices[1].y = SCR_HEIGHT; vertices[1].z = 0;

		sceGuDrawArray(GU_SPRITES,GU_TEXTURE_16BIT|GU_COLOR_5551|GU_VERTEX_16BIT|GU_TRANSFORM_2D,2,0,vertices);
	}
    sceGuFinish();
    sceGuSync(0,0);
}


void pgScreenFlipV()
{
    pgWaitV();
    pgScreenFlip();
}

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
