/*
* $Id$
*/

#include <pspkernel.h>
#include <pspgu.h>
#include <pspdisplay.h>
#include "pg.h"
#include "font.h"
#include "fontNaga10.h"

unsigned int __attribute__((aligned(16))) gulist[512*512];
unsigned short *show_frame;
unsigned short *draw_frame;
unsigned short *work_frame;
unsigned short *tex_frame;
RECT full_rect = { 0, 0, SCR_WIDTH, SCR_HEIGHT };

static const ScePspIMatrix4 dither_matrix =
{
	// Bayer dither
	{  0,  8,  2, 10 },
	{ 12,  4, 14,  6 },
	{  3, 11,  1,  9 },
	{ 15,  7, 13,  5 }
};

struct Vertex
{
	unsigned short u, v;
	unsigned short color;
	short x, y, z;
};

void pgGuInit(void)
{
	draw_frame = (void *)(FRAMESIZE * 0);
	show_frame = (void *)(FRAMESIZE * 1);
	work_frame = (void *)(FRAMESIZE * 2);
	tex_frame  = (void *)(FRAMESIZE * 3);

	sceGuDisplay(GU_FALSE);
	sceGuInit();

	sceGuStart(GU_DIRECT, gulist);
	sceGuDrawBuffer(GU_PSM_5551, draw_frame, BUF_WIDTH);
	sceGuDispBuffer(SCR_WIDTH, SCR_HEIGHT, show_frame, BUF_WIDTH);
	sceGuOffset(2048 - (SCR_WIDTH / 2), 2048 - (SCR_HEIGHT / 2));
	sceGuViewport(2048, 2048, SCR_WIDTH, SCR_HEIGHT);

	sceGuEnable(GU_SCISSOR_TEST);
	sceGuScissor(0, 0, SCR_WIDTH, SCR_HEIGHT);

	sceGuDisable(GU_ALPHA_TEST);
	sceGuAlphaFunc(GU_LEQUAL, 0, 0x01);

	sceGuDisable(GU_BLEND);
	sceGuBlendFunc(GU_ADD, GU_SRC_ALPHA, GU_ONE_MINUS_SRC_ALPHA, 0, 0);

	sceGuDisable(GU_DEPTH_TEST);
	sceGuDepthRange(65535, 0);
	sceGuDepthFunc(GU_GEQUAL);
	sceGuDepthMask(GU_TRUE);

	sceGuEnable(GU_TEXTURE_2D);
	sceGuTexMode(GU_PSM_5551, 0, 0, GU_FALSE);
	sceGuTexScale(1.0f / BUF_WIDTH, 1.0f / BUF_WIDTH);
	sceGuTexOffset(0, 0);
	sceGuTexFunc(GU_TFX_REPLACE, GU_TCC_RGBA);

	sceGuClutMode(GU_PSM_5551, 0, 0xff, 0);

	sceGuSetDither(&dither_matrix);
	sceGuDisable(GU_DITHER);

	sceGuClearDepth(0);
	sceGuClearColor(0);

	sceGuFinish();
	sceGuSync(0, GU_SYNC_FINISH);

	sceDisplayWaitVblankStart();
	sceGuDisplay(GU_TRUE);
}

void pgWaitVn(unsigned long count)
{
    for (; count > 0; --count) {
        sceDisplayWaitVblankStart();
    }
}

void pgWaitV()
{
    sceDisplayWaitVblankStart();
}

unsigned short* pgGetVramAddr(unsigned long x,unsigned long y)
{
    return GU_FRAME_ADDR(tex_frame) + x + y*LINESIZE;
}

void pgFillvram(unsigned long color)
{
    unsigned short *vptr0;       //pointer to vram
    unsigned long i;

    vptr0 = pgGetVramAddr(0,0);
    for (i = 0; i < FRAMESIZE; i++) {
        *vptr0 = color;
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
    if (ch < 0x20)
        ch = 0;
    else if (ch < 0x80)
        ch -= 0x20;
    else if (ch < 0xa0)
        ch = 0;
    else
        ch -= 0x40;

    fnt = (unsigned char *)&hankaku_font10[ch*10];

    // draw
    vr = pgGetVramAddr(x,y);
    for(y1 = 0; y1 < 10; y1++) {
        pt = *fnt++;
        for(x1 = 0; x1 < 5; x1++) {
            if (pt & 1)
                *vr = col;
            else
                *vr = 0x8000;
            vr++;
            pt = pt >> 1;
        }
        vr += LINESIZE - 5;
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
    if(code >= 0xE000) code -= 0x4000;
    code = ((((code >> 8) & 0xFF) - 0x81) << 9) + (code&0x00FF);
    if((code & 0x00FF) >= 0x80) code--;
    if((code & 0x00FF) >= 0x9E) code+=0x62;
    else code -= 0x40;
    code += 0x2121 + 0x8080;

    // EUCから恵梨沙フォントの番号を生成
    n = (((code >> 8) & 0xFF) - 0xA1) * (0xFF - 0xA1)
        + (code & 0xFF) - 0xA1;
    j=0;
    while(font404[j]) {
        if(code >= font404[j]) {
            if(code <= font404[j] + font404[j + 1] - 1) {
                n = -1;
                break;
            } else {
                n -= font404[j + 1];
            }
        }
        j += 2;
    }
    fnt = (unsigned short *)&zenkaku_font10[n * 10];

    // draw
    vr = pgGetVramAddr(x,y);
    for(y1 = 0; y1 < 10; y1++) {
        pt = *fnt++;
        for(x1 = 0; x1 < 10; x1++) {
            if (pt & 1)
                *vr = col;
            else
                *vr = 0x8000;
            vr++;
            pt = pt >> 1;
        }
        vr += LINESIZE - 10;
    }
}

// by kwn
void mh_print(int x,int y,const char *str,int color) {
    unsigned char ch = 0,bef = 0;

    while(*str != 0) {
        ch = (unsigned char)*str++;
        if (bef != 0) {
            Draw_Char_Zenkaku(x, y, bef, ch, color);
            x += 10;
            bef = 0;
        } else {
            if (((ch >= 0x80) && (ch < 0xa0)) || (ch >= 0xe0)) {
                bef = ch;
            } else {
                Draw_Char_Hankaku(x, y, ch, color);
                x += 5;
            }
        }
    }
}

/*--------------------------------------------------------
	矩形範囲をコピー
--------------------------------------------------------*/

void video_copy_rect(void *src, void *dst, RECT *src_rect, RECT *dst_rect)
{
	int j, sw, dw, sh, dh;
	struct Vertex *vertices;

	sw = src_rect->right - src_rect->left;
	dw = dst_rect->right - dst_rect->left;
	sh = src_rect->bottom - src_rect->top;
	dh = dst_rect->bottom - dst_rect->top;

	sceGuStart(GU_DIRECT, gulist);

	sceGuDrawBufferList(GU_PSM_5551, dst, BUF_WIDTH);
	sceGuScissor(dst_rect->left, dst_rect->top, dst_rect->right, dst_rect->bottom);
	sceGuDisable(GU_ALPHA_TEST);

	sceGuTexMode(GU_PSM_5551, 0, 0, GU_FALSE);
	sceGuTexImage(0, BUF_WIDTH, BUF_WIDTH, BUF_WIDTH, GU_FRAME_ADDR(src));
	if (sw == dw && sh == dh)
		sceGuTexFilter(GU_NEAREST, GU_NEAREST);
	else
		sceGuTexFilter(GU_LINEAR, GU_LINEAR);

	for (j = 0; (j + SLICE_SIZE) < sw; j = j + SLICE_SIZE)
	{
		vertices = (struct Vertex *)sceGuGetMemory(2 * sizeof(struct Vertex));

		vertices[0].u = src_rect->left + j;
		vertices[0].v = src_rect->top;
		vertices[0].x = dst_rect->left + j * dw / sw;
		vertices[0].y = dst_rect->top;

		vertices[1].u = src_rect->left + j + SLICE_SIZE;
		vertices[1].v = src_rect->bottom;
		vertices[1].x = dst_rect->left + (j + SLICE_SIZE) * dw / sw;
		vertices[1].y = dst_rect->bottom;

		sceGuDrawArray(GU_SPRITES, TEXTURE_FLAGS, 2, NULL, vertices);
	}

	if (j < sw)
	{
		vertices = (struct Vertex *)sceGuGetMemory(2 * sizeof(struct Vertex));

		vertices[0].u = src_rect->left + j;
		vertices[0].v = src_rect->top;
		vertices[0].x = dst_rect->left + j * dw / sw;
		vertices[0].y = dst_rect->top;

		vertices[1].u = src_rect->right;
		vertices[1].v = src_rect->bottom;
		vertices[1].x = dst_rect->right;
		vertices[1].y = dst_rect->bottom;

		sceGuDrawArray(GU_SPRITES, TEXTURE_FLAGS, 2, NULL, vertices);
	}

	sceGuFinish();
	sceGuSync(0, GU_SYNC_FINISH);
}

void video_flip_screen(int vsync)
{
	if (vsync) sceDisplayWaitVblankStart();
	show_frame = draw_frame;
	draw_frame = sceGuSwapBuffers();
}
