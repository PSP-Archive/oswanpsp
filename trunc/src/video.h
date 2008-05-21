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
#define GU_FRAME_ADDR(frame)	(unsigned short *)((unsigned long)frame | 0x44000000)
#define RGB(r,g,b)			((((b & 0xf8) << 7) | ((g & 0xf8) << 2) | ((r & 0xf8) >> 3))|0x8000)

typedef struct rect_t
{
	short left;
	short top;
	short right;
	short bottom;
} RECT;

void pgGuInit(void);
void video_wait_vsync(void);
void video_wait_vsync_n(unsigned long count);
void video_flip_screen(int vsync);
void video_clear_frame(void *frame);
void video_clear_screen(void);
void video_clear_rect(void *frame, RECT *rect);
void video_fill_frame(void *frame, unsigned long color);
void video_fill_rect(void *frame, unsigned long color, RECT *rect);
void video_copy_rect(void *src, void *dst, RECT *src_rect, RECT *dst_rect);
void mh_print(int x,int y,const char *str,int color);

#endif
