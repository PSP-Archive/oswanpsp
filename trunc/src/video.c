/*
* $Id$
*
* NJ氏のソースを使用させていただいてます
*/

#include <pspkernel.h>
#include <pspgu.h>
#include <pspdisplay.h>
#include "video.h"
#include "font.h"
#include "intraFont.h"

unsigned int __attribute__((aligned(16))) gulist[512*512];
unsigned short *show_frame;
unsigned short *draw_frame;
unsigned short *work_frame;
unsigned short *tex_frame;
RECT full_rect = { 0, 0, SCR_WIDTH, SCR_HEIGHT };
intraFont* jpn0;

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

/*--------------------------------------------------------
	ビデオ処理初期化
--------------------------------------------------------*/

void video_init(void)
{
    // Init intraFont library
    intraFontInit();
    jpn0 = intraFontLoad("flash0:/font/jpn0.pgf", INTRAFONT_STRING_SJIS | INTRAFONT_CACHE_ALL);

	draw_frame = (void *)(FRAMESIZE * 0);
	show_frame = (void *)(FRAMESIZE * 1);
	work_frame = (void *)(FRAMESIZE * 2);
	tex_frame  = (void *)(FRAMESIZE * 3);

	sceGuDisplay(GU_FALSE);
	sceGuInit();

	sceGuStart(GU_DIRECT, gulist);
	sceGuDrawBuffer(GU_PSM_4444, draw_frame, BUF_WIDTH);
	sceGuDispBuffer(SCR_WIDTH, SCR_HEIGHT, show_frame, BUF_WIDTH);
	sceGuOffset(2048 - (SCR_WIDTH / 2), 2048 - (SCR_HEIGHT / 2));
	sceGuViewport(2048, 2048, SCR_WIDTH, SCR_HEIGHT);

	sceGuEnable(GU_SCISSOR_TEST);
	sceGuScissor(0, 0, SCR_WIDTH, SCR_HEIGHT);

	sceGuDisable(GU_ALPHA_TEST);
	sceGuAlphaFunc(GU_LEQUAL, 0, 0x01);

	sceGuEnable(GU_BLEND);
	sceGuBlendFunc(GU_ADD, GU_SRC_ALPHA, GU_ONE_MINUS_SRC_ALPHA, 0, 0);

	sceGuDisable(GU_DEPTH_TEST);
	sceGuDepthRange(65535, 0);
	sceGuDepthFunc(GU_GEQUAL);
	sceGuDepthMask(GU_TRUE);

	sceGuEnable(GU_TEXTURE_2D);
	sceGuTexMode(GU_PSM_4444, 0, 0, GU_FALSE);
	sceGuTexScale(1.0f / BUF_WIDTH, 1.0f / BUF_WIDTH);
	sceGuTexOffset(0, 0);
	sceGuTexFunc(GU_TFX_REPLACE, GU_TCC_RGBA);

	sceGuClutMode(GU_PSM_4444, 0, 0xff, 0);

	sceGuSetDither(&dither_matrix);
	sceGuDisable(GU_DITHER);

	sceGuClearDepth(0);
	sceGuClearColor(0);

	sceGuFinish();
	sceGuSync(0, GU_SYNC_FINISH);

	sceDisplayWaitVblankStart();
	sceGuDisplay(GU_TRUE);
}

/*--------------------------------------------------------
	ビデオ処理終了(共通)
--------------------------------------------------------*/

void video_exit(void)
{
    intraFontUnload(jpn0);
	sceGuDisplay(GU_FALSE);
	sceGuTerm();
}

/*--------------------------------------------------------
	VSYNCを待つ
--------------------------------------------------------*/

void video_wait_vsync(void)
{
	sceDisplayWaitVblankStart();
}

void video_wait_vsync_n(unsigned long count)
{
    for (; count > 0; --count) {
        sceDisplayWaitVblankStart();
    }
}

/*--------------------------------------------------------
	スクリーンをフリップ
--------------------------------------------------------*/

void video_flip_screen(int vsync)
{
	if (vsync) sceDisplayWaitVblankStart();
	show_frame = draw_frame;
	draw_frame = sceGuSwapBuffers();
}

/*--------------------------------------------------------
	VRAMのアドレスを取得
--------------------------------------------------------*/

unsigned short* video_frame_addr(void *frame, int x, int y)
{
		return GU_FRAME_ADDR(frame) + x + (y << 9);
}

/*--------------------------------------------------------
	指定したフレームをクリア
--------------------------------------------------------*/

void video_clear_frame(void *frame)
{
	sceGuStart(GU_DIRECT, gulist);
	sceGuDrawBufferList(GU_PSM_4444, frame, BUF_WIDTH);
	sceGuScissor(0, 0, BUF_WIDTH, SCR_HEIGHT);
	sceGuClearColor(0);
	sceGuClear(GU_COLOR_BUFFER_BIT);
	sceGuFinish();
	sceGuSync(0, GU_SYNC_FINISH);
}


/*--------------------------------------------------------
	描画/表示フレームをクリア
--------------------------------------------------------*/

void video_clear_screen(void)
{
	video_clear_frame(show_frame);
	video_clear_frame(draw_frame);
}


/*--------------------------------------------------------
	指定した矩形範囲をクリア
--------------------------------------------------------*/

void video_clear_rect(void *frame, RECT *rect)
{
	sceGuStart(GU_DIRECT, gulist);
	sceGuDrawBufferList(GU_PSM_4444, frame, BUF_WIDTH);
	sceGuScissor(rect->left, rect->top, rect->right, rect->bottom);
	sceGuClearColor(0);
	sceGuClear(GU_COLOR_BUFFER_BIT);
	sceGuFinish();
	sceGuSync(0, GU_SYNC_FINISH);
}


/*--------------------------------------------------------
	指定したフレームを塗りつぶし
--------------------------------------------------------*/

void video_fill_frame(void *frame, unsigned long color)
{
	sceGuStart(GU_DIRECT, gulist);
	sceGuDrawBufferList(GU_PSM_4444, frame, BUF_WIDTH);
	sceGuScissor(0, 0, SCR_WIDTH, SCR_HEIGHT);
	sceGuClearColor(color);
	sceGuClear(GU_COLOR_BUFFER_BIT);
	sceGuFinish();
	sceGuSync(0, GU_SYNC_FINISH);
}


/*--------------------------------------------------------
	指定した矩形範囲を塗りつぶし
--------------------------------------------------------*/

void video_fill_rect(void *frame, unsigned long color, RECT *rect)
{
	sceGuStart(GU_DIRECT, gulist);
	sceGuDrawBufferList(GU_PSM_4444, frame, BUF_WIDTH);
	sceGuScissor(rect->left, rect->top, rect->right, rect->bottom);
	sceGuClearColor(color);
	sceGuClear(GU_COLOR_BUFFER_BIT);
	sceGuFinish();
	sceGuSync(0, GU_SYNC_FINISH);
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

	sceGuDrawBufferList(GU_PSM_4444, dst, BUF_WIDTH);
	sceGuScissor(dst_rect->left, dst_rect->top, dst_rect->right, dst_rect->bottom);
	sceGuDisable(GU_ALPHA_TEST);

	sceGuTexMode(GU_PSM_4444, 0, 0, GU_FALSE);
	sceGuTexImage(0, BUF_WIDTH, BUF_WIDTH, BUF_WIDTH, GU_FRAME_ADDR(src));
	sceGuTexFunc(GU_TFX_REPLACE, GU_TCC_RGBA);
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

/*--------------------------------------------------------
	矩形範囲を左右反転してコピー
--------------------------------------------------------*/

void video_copy_rect_flip(void *src, void *dst, RECT *src_rect, RECT *dst_rect)
{
	short j, sw, dw, sh, dh;
	struct Vertex *vertices;

	sw = src_rect->right - src_rect->left;
	dw = dst_rect->right - dst_rect->left;
	sh = src_rect->bottom - src_rect->top;
	dh = dst_rect->bottom - dst_rect->top;

	sceGuStart(GU_DIRECT, gulist);

	sceGuDrawBufferList(GU_PSM_4444, dst, BUF_WIDTH);
	sceGuScissor(dst_rect->left, dst_rect->top, dst_rect->right, dst_rect->bottom);
	sceGuDisable(GU_ALPHA_TEST);

	sceGuTexMode(GU_PSM_4444, 0, 0, GU_FALSE);
	sceGuTexImage(0, BUF_WIDTH, BUF_WIDTH, BUF_WIDTH, GU_FRAME_ADDR(src));
	sceGuTexFunc(GU_TFX_REPLACE, GU_TCC_RGBA);
	if (sw == dw && sh == dh)
		sceGuTexFilter(GU_NEAREST, GU_NEAREST);
	else
		sceGuTexFilter(GU_LINEAR, GU_LINEAR);

	for (j = 0; (j + SLICE_SIZE) < sw; j = j + SLICE_SIZE)
	{
    	vertices = (struct Vertex *)sceGuGetMemory(2 * sizeof(struct Vertex));

		vertices[0].u = src_rect->left + j;
		vertices[0].v = src_rect->top;
		vertices[0].x = dst_rect->right - j * dw / sw;
		vertices[0].y = dst_rect->bottom;

		vertices[1].u = src_rect->left + j + SLICE_SIZE;
		vertices[1].v = src_rect->bottom;
		vertices[1].x = dst_rect->right - (j + SLICE_SIZE) * dw / sw;
		vertices[1].y = dst_rect->top;

		sceGuDrawArray(GU_SPRITES, TEXTURE_FLAGS, 2, NULL, vertices);
	}

	if (j < sw)
	{
		vertices = (struct Vertex *)sceGuGetMemory(2 * sizeof(struct Vertex));

		vertices[0].u = src_rect->left + j;
		vertices[0].v = src_rect->top;
		vertices[0].x = dst_rect->right - j * dw / sw;
		vertices[0].y = dst_rect->bottom;

		vertices[1].u = src_rect->right;
		vertices[1].v = src_rect->bottom;
		vertices[1].x = dst_rect->left;
		vertices[1].y = dst_rect->top;

		sceGuDrawArray(GU_SPRITES, TEXTURE_FLAGS, 2, NULL, vertices);
	}

	sceGuFinish();
	sceGuSync(0, GU_SYNC_FINISH);
}


/*--------------------------------------------------------
	矩形範囲を270度回転してコピー
--------------------------------------------------------*/

void video_copy_rect_rotate(void *src, void *dst, RECT *src_rect, RECT *dst_rect)
{
	short j, sw, dw, sh, dh;
	struct Vertex *vertices;

	sw = src_rect->right - src_rect->left;
	dw = dst_rect->right - dst_rect->left;
	sh = src_rect->bottom - src_rect->top;
	dh = dst_rect->bottom - dst_rect->top;

	sceGuStart(GU_DIRECT, gulist);

	sceGuDrawBufferList(GU_PSM_4444, dst, BUF_WIDTH);
	sceGuScissor(dst_rect->left, dst_rect->top, dst_rect->right, dst_rect->bottom);
	sceGuDisable(GU_ALPHA_TEST);
	sceGuDisable(GU_BLEND);
	sceGuDisable(GU_DEPTH_TEST);

	sceGuTexMode(GU_PSM_4444, 0, 0, GU_FALSE);
	sceGuTexImage(0, BUF_WIDTH, BUF_WIDTH, BUF_WIDTH, GU_FRAME_ADDR(src));
	sceGuTexFunc(GU_TFX_REPLACE, GU_TCC_RGBA);
	if (sw == dh && sh == dw)
		sceGuTexFilter(GU_NEAREST, GU_NEAREST);
	else
		sceGuTexFilter(GU_LINEAR, GU_LINEAR);

	vertices = (struct Vertex *)sceGuGetMemory(2 * sizeof(struct Vertex));

	for (j = 0; (j + SLICE_SIZE) < sw; j = j + SLICE_SIZE)
	{
		vertices = (struct Vertex *)sceGuGetMemory(2 * sizeof(struct Vertex));

		vertices[0].u = src_rect->right - j;
		vertices[0].v = src_rect->bottom;
		vertices[0].x = dst_rect->right;
		vertices[0].y = dst_rect->top - j * dh / sw;

		vertices[1].u = src_rect->right - j + SLICE_SIZE;
		vertices[1].v = src_rect->top;
		vertices[1].x = dst_rect->right;
		vertices[1].y = dst_rect->bottom - (j + SLICE_SIZE) * dh / sw;

		sceGuDrawArray(GU_SPRITES, TEXTURE_FLAGS, 2, NULL, vertices);
	}

	if (j < sw)
	{
		vertices = (struct Vertex *)sceGuGetMemory(2 * sizeof(struct Vertex));

		vertices[0].u = src_rect->right + j;
		vertices[0].v = src_rect->bottom;
		vertices[0].x = dst_rect->right;
		vertices[0].y = dst_rect->top - j * dh / sw;

		vertices[1].u = src_rect->left;
		vertices[1].v = src_rect->top;
		vertices[1].x = dst_rect->left;
		vertices[1].y = dst_rect->bottom;

		sceGuDrawArray(GU_SPRITES, TEXTURE_FLAGS, 2, NULL, vertices);
	}

	sceGuFinish();
	sceGuSync(0, GU_SYNC_FINISH);
}


/*--------------------------------------------------------
	テクスチャを矩形範囲を指定して描画
--------------------------------------------------------*/

void video_draw_texture(unsigned long src_fmt, unsigned long dst_fmt, void *src, void *dst, RECT *src_rect, RECT *dst_rect)
{
	int j, sw, dw, sh, dh;
	struct Vertex *vertices;

	sw = src_rect->right - src_rect->left;
	dw = dst_rect->right - dst_rect->left;
	sh = src_rect->bottom - src_rect->top;
	dh = dst_rect->bottom - dst_rect->top;

	sceGuStart(GU_DIRECT, gulist);

	sceGuDrawBufferList(dst_fmt, dst, BUF_WIDTH);
	sceGuScissor(dst_rect->left, dst_rect->top, dst_rect->right, dst_rect->bottom);

	sceGuTexMode(src_fmt, 0, 0, GU_FALSE);
	sceGuTexImage(0, BUF_WIDTH, BUF_WIDTH, BUF_WIDTH, GU_FRAME_ADDR(src));
	sceGuTexFunc(GU_TFX_REPLACE, GU_TCC_RGBA);
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

/*--------------------------------------------------------
	文字列表示
--------------------------------------------------------*/

void mh_start(void)
{
	sceGuStart(GU_DIRECT, gulist);
	sceGuDrawBufferList(GU_PSM_4444, draw_frame, BUF_WIDTH);
}

void mh_print(int x,int y,const char *str,unsigned int color)
{
    intraFontSetStyle(jpn0, 0.6f, color, 0, 0);
	intraFontPrint(jpn0, x, y + 12, str);
}

void mh_end(void)
{
	sceGuFinish();
	sceGuSync(0, GU_SYNC_FINISH);
}

void mh_print_num(int x, int y, int num, unsigned short color)
{
	int i, j, n, val[4];
	unsigned char ch, *ft;
	unsigned short* vr;
	unsigned short *vp;

	if (num < 10) 
	{
		n = 2;
		val[0] = num;
		val[1] = 0;
	}
	else if (num < 100)
	{
		n = 2;
		val[0] = num % 10;
		val[1] = num / 10;
	}
	else if (num < 1000)
	{
		n = 3;
		val[0] = num % 10;
		num /= 10;
		val[1] = num % 10;
		val[2] = num / 10;
	}
	else if (num < 10000)
	{
		n = 4;
		val[0] = num % 10;
		num /= 10;
		val[1] = num % 10;
		num /= 10;
		val[2] = num % 10;
		val[3] = num / 10;
	}
	else return;
	while (n--)
	{
		ft = (unsigned char*)font + ((val[n]+'0') << 3);
		vr = video_frame_addr(tex_frame, x, y);
		for (i = 0; i < 8; i++)
		{
			vp = vr;
			ch = 0x80;
			for (j = 0; j < 8; j++)
			{
				if (*ft & ch)
				{
					*vp++ = 0xFFFF;
				}
				else
				{
					*vp++ = 0x8000;
				}
				ch >>= 1;
			}
			ft++;
			vr += BUF_WIDTH;
		}
		x += 8;
	}
}
