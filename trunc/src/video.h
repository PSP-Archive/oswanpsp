/*
* $Id$
*/

#ifndef PG_H_
#define PG_H_

#define FONT_HEIGHT			12
#define PIXELSIZE			1
#define LINESIZE			512
#define SCR_WIDTH			480
#define SCR_HEIGHT			272
#define BUF_WIDTH			512
#define	FRAMESIZE			(BUF_WIDTH * SCR_HEIGHT * 2)
#define SLICE_SIZE			64 // change this to experiment with different page-cache sizes
#define TEXTURE_FLAGS		(GU_TEXTURE_16BIT | GU_COLOR_5551 | GU_VERTEX_16BIT | GU_TRANSFORM_2D)
#define GU_FRAME_ADDR(frame)	(unsigned short *)((unsigned long)frame | 0x44000000)
#define RGB555(r,g,b)		((((b & 0xf8) << 7) | ((g & 0xf8) << 2) | ((r & 0xf8) >> 3))|0x8000)
#define RGB(r,g,b)			(((b) << 16) | ((g) << 8) | (r) | 0xFF000000)

typedef struct rect_t
{
	short left;
	short top;
	short right;
	short bottom;
} RECT;

enum colors {
	RED =	0xFF0000FF,
	GREEN =	0xFF00FF00,
	BLUE =	0xFFFF0000,
	WHITE =	0xFFFFFFFF,
	LITEGRAY = 0xFFBFBFBF,
	GRAY =  0xFF7F7F7F,
	DARKGRAY = 0xFF3F3F3F,		
	BLACK = 0xFF000000
};

void video_init(void);
void video_exit(void);
void video_wait_vsync(void);
void video_wait_vsync_n(unsigned long count);
void video_flip_screen(int vsync);
void video_clear_frame(void *frame);
void video_clear_screen(void);
void video_clear_rect(void *frame, RECT *rect);
void video_fill_frame(void *frame, unsigned long color);
void video_fill_rect(void *frame, unsigned long color, RECT *rect);
void video_copy_rect(void *src, void *dst, RECT *src_rect, RECT *dst_rect);
void mh_start(void);
void mh_print(int x,int y,const char *str,unsigned int color);
void mh_end(void);
void mh_print_num(int x, int y, int num, unsigned short color);

#endif
