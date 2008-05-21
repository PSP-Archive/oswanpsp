/*
* $Id$
*/

#ifndef GPU_H_
#define GPU_H_

extern unsigned short* BgMap;
extern unsigned short* FgMap;
extern unsigned long* SprMap;
extern unsigned long SprTable[];
extern int SprCount;
extern unsigned char MonoModifiedTile[];
extern unsigned char ColorModifiedTile[];
extern unsigned short FrameBuffer[];

void gpuInit(void);
void gpuRenderScanLineMono(void);
void gpuRenderScanLineColor(void);
void gpuSetSegment(void);

#endif
