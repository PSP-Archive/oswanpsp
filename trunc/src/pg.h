/*
* $Id$
*/

#ifndef PG_H_
#define PG_H_

#define FONT_HEIGHT			9
#define PIXELSIZE			1
#define LINESIZE			512
#define SCR_WIDTH			480
#define SCR_HEIGHT			272
#define BUF_WIDTH			512
#define	FRAMESIZE			(BUF_WIDTH * SCR_HEIGHT * 2)
#define SLICE_SIZE			64 // change this to experiment with different page-cache sizes
#define TEXTURE_FLAGS		(GU_TEXTURE_16BIT | GU_COLOR_5551 | GU_VERTEX_16BIT | GU_TRANSFORM_2D)
#define GU_FRAME_ADDR(frame)		(unsigned short *)((unsigned long)frame | 0x44000000)
#define RGB(r,g,b) ((((b & 0xf8) << 7) | ((g & 0xf8) << 2) | ((r & 0xf8) >> 3))|0x8000)

typedef struct rect_t
{
	short left;
	short top;
	short right;
	short bottom;
} RECT;

void pgWaitV();
void pgWaitVn(unsigned long count);
unsigned short* pgGetVramAddr(unsigned long x,unsigned long y);
void pgFillvram(unsigned long color);
void mh_print(int x,int y,const char *str,int color);
void video_copy_rect(void *src, void *dst, RECT *src_rect, RECT *dst_rect);
void video_flip_screen(int vsync);
void pgGuInit(void);

#endif
