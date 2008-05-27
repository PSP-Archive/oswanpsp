/*
* $Id$
*/

#include <pspkernel.h>
#include <stdio.h>
#include <string.h>
#include "video.h"
#include "ws.h"
#include "gpu.h"
#include "segment.h"
#include "./debug/debug.h"

unsigned short* BgMap;
unsigned short* FgMap;
unsigned long* SprMap;
unsigned long SprTable[128 * 4];
int SprCount;
unsigned char TileCache[1024*8*8]; // 8ƒhƒbƒg8—ñ
unsigned char HflippedTileCache[1024*8*8];
unsigned char MonoModifiedTile[1024]; //512 * 2bank(sorobangu)
unsigned char ColorModifiedTile[1024];
unsigned long TransparentTile[1024][8];
char LMask[256 + 8];
char WMaskIn[256];
char WMaskAll[256];

extern unsigned short *work_frame;

void gpuInit(void)
{
    memset(TileCache, 0, 1024*8*8);
    memset(HflippedTileCache, 0, 1024*8*8);
    memset(MonoModifiedTile, 1, 1024);
    memset(ColorModifiedTile, 1, 1024);
}

unsigned char* gpuMonoTileCache(unsigned short tileInfo, int row, int bank)
{
    unsigned short tileIndex;
    int line;

    tileIndex = tileInfo & 0x1FF;
    if (bank) // bank
    {
        tileIndex += 512;
    }
    if (MonoModifiedTile[tileIndex])
    {
        unsigned char* tileInCachePtr = TileCache + (tileIndex<<6);
        unsigned char* hflippedTileInCachePtr = HflippedTileCache + (tileIndex<<6) + 7;
        unsigned short* tileInRamPtr = (unsigned short*)(RAM + 0x2000 + (tileIndex<<4));
        unsigned short tileLine, tileTemp;

        for (line=0; line<8; line++)
        {
            tileLine = *tileInRamPtr++;
            tileTemp = tileLine & 0x8080; *hflippedTileInCachePtr-- = *tileInCachePtr++ = (tileTemp>>7)|(tileTemp>>14);
            tileTemp = tileLine & 0x4040; *hflippedTileInCachePtr-- = *tileInCachePtr++ = (tileTemp>>6)|(tileTemp>>13);
            tileTemp = tileLine & 0x2020; *hflippedTileInCachePtr-- = *tileInCachePtr++ = (tileTemp>>5)|(tileTemp>>12);
            tileTemp = tileLine & 0x1010; *hflippedTileInCachePtr-- = *tileInCachePtr++ = (tileTemp>>4)|(tileTemp>>11);
            tileTemp = tileLine & 0x0808; *hflippedTileInCachePtr-- = *tileInCachePtr++ = (tileTemp>>3)|(tileTemp>>10);
            tileTemp = tileLine & 0x0404; *hflippedTileInCachePtr-- = *tileInCachePtr++ = (tileTemp>>2)|(tileTemp>>9);
            tileTemp = tileLine & 0x0202; *hflippedTileInCachePtr-- = *tileInCachePtr++ = (tileTemp>>1)|(tileTemp>>8);
            tileTemp = tileLine & 0x0101; *hflippedTileInCachePtr-- = *tileInCachePtr++ = (tileTemp>>0)|(tileTemp>>7);
            hflippedTileInCachePtr += 16;
            TransparentTile[tileIndex][line] = tileLine;
        }
        MonoModifiedTile[tileIndex]=0;
    }
    if (tileInfo & 0x8000) // v flip
    {
        row = 7 - row;
    }
    if (TransparentTile[tileIndex][row] == 0) {
        return NULL;
    }
    if (tileInfo & 0x4000) // h flip
    {
        return(&HflippedTileCache[(tileIndex<<6)+(row<<3)]);
    }
    return(&TileCache[(tileIndex<<6)+(row<<3)]);
}

void gpuRenderFGMono(void)
{
    int i, j;
    unsigned short *vptr;       // pointer to frame buffer
    unsigned char scrollX;
    unsigned char scrollY;      // scroll registerY + scanline
    unsigned char tileX, tileY; // coordinate in 32 * 32 map
    unsigned short *map;        // map address
    unsigned char *color;       // tile color
    unsigned char pal;
    char *layerMask;
    unsigned long *p;

    int windowMode = IO[0x00] & 0x30;
    p = (unsigned long*)(LMask + 8);
    //inside
    if (windowMode == 0x20) {
        if ((IO[0x02] < IO[0x09]) || (IO[0x02] > IO[0x0b])) {
            i = 224 / 4;
            while(i > 0) {
                p[--i] = 0;
            }
            return;
        }
        for (i = 0; i < IO[0x08]; i++) {
            LMask[i + 8]=0;
        }
        for (; i <= IO[0x0a]; i++) {
            LMask[i + 8]=1;
        }
        for (; i < 224; i++) {
            LMask[i + 8]=0;
        }
    }
    //outside
    else if (windowMode == 0x30) {
        if ((IO[0x02] >= IO[0x09]) && (IO[0x02] <= IO[0x0b])) {
            for (i = 0; i < IO[0x08]; i++) {
                LMask[i + 8]=1;
            }
            for (; i <= IO[0x0a]; i++) {
                LMask[i + 8]=0;
            }
            for (; i < 224; i++) {
                LMask[i + 8]=1;
            }
        }
        else {
            i = 224 / 4;
            while(i > 0) {
                p[--i] = 0x01010101;
            }
        }
    }
    else {
        i = 224 / 4;
        while(i > 0) {
            p[--i] = 0x01010101;
        }
    }
    // seek to the first tile
    scrollX = IO[0x12] >> 3;
    scrollY = (IO[0x13] + IO[0x02]) & 0xFF;
    map = FgMap + ((scrollY & 0xF8) << 2); // scrollY>>3 <<5
    tileX = IO[0x12] & 0x07;
    tileY = scrollY & 0x07;
    vptr = GU_FRAME_ADDR(work_frame) + (IO[0x02] << 9) + 64 - tileX;
    layerMask = LMask + 8 - tileX;
    for (j = -tileX; j < 224; j += 8) {
        color = gpuMonoTileCache(map[scrollX & 0x1F], tileY, map[scrollX & 0x1F] & 0x2000);
        pal = (map[scrollX & 0x1F] >> 9) &0x0F;
        if (pal & 0x04) {
            if (color) {
                if (*color && *layerMask) *vptr = ColorPalette[pal][*color];
                else *layerMask = 0;
                color++;layerMask++;vptr++;
                if (*color && *layerMask) *vptr = ColorPalette[pal][*color];
                else *layerMask = 0;
                color++;layerMask++;vptr++;
                if (*color && *layerMask) *vptr = ColorPalette[pal][*color];
                else *layerMask = 0;
                color++;layerMask++;vptr++;
                if (*color && *layerMask) *vptr = ColorPalette[pal][*color];
                else *layerMask = 0;
                color++;layerMask++;vptr++;
                if (*color && *layerMask) *vptr = ColorPalette[pal][*color];
                else *layerMask = 0;
                color++;layerMask++;vptr++;
                if (*color && *layerMask) *vptr = ColorPalette[pal][*color];
                else *layerMask = 0;
                color++;layerMask++;vptr++;
                if (*color && *layerMask) *vptr = ColorPalette[pal][*color];
                else *layerMask = 0;
                color++;layerMask++;vptr++;
                if (*color && *layerMask) *vptr = ColorPalette[pal][*color];
                else *layerMask = 0;
                color++;layerMask++;vptr++;
            }
            else {
                vptr += 8;
                *layerMask++ = 0;
                *layerMask++ = 0;
                *layerMask++ = 0;
                *layerMask++ = 0;
                *layerMask++ = 0;
                *layerMask++ = 0;
                *layerMask++ = 0;
                *layerMask++ = 0;
            }
        }
        else {
            if (color) {
                if (*layerMask) *vptr = ColorPalette[pal][*color];
                color++;layerMask++;vptr++;
                if (*layerMask) *vptr = ColorPalette[pal][*color];
                color++;layerMask++;vptr++;
                if (*layerMask) *vptr = ColorPalette[pal][*color];
                color++;layerMask++;vptr++;
                if (*layerMask) *vptr = ColorPalette[pal][*color];
                color++;layerMask++;vptr++;
                if (*layerMask) *vptr = ColorPalette[pal][*color];
                color++;layerMask++;vptr++;
                if (*layerMask) *vptr = ColorPalette[pal][*color];
                color++;layerMask++;vptr++;
                if (*layerMask) *vptr = ColorPalette[pal][*color];
                color++;layerMask++;vptr++;
                if (*layerMask) *vptr = ColorPalette[pal][*color];
                color++;layerMask++;vptr++;
            }
            else {
                if (*layerMask) *vptr = ColorPalette[pal][0]; layerMask++; vptr++;
                if (*layerMask) *vptr = ColorPalette[pal][0]; layerMask++; vptr++;
                if (*layerMask) *vptr = ColorPalette[pal][0]; layerMask++; vptr++;
                if (*layerMask) *vptr = ColorPalette[pal][0]; layerMask++; vptr++;
                if (*layerMask) *vptr = ColorPalette[pal][0]; layerMask++; vptr++;
                if (*layerMask) *vptr = ColorPalette[pal][0]; layerMask++; vptr++;
                if (*layerMask) *vptr = ColorPalette[pal][0]; layerMask++; vptr++;
                if (*layerMask) *vptr = ColorPalette[pal][0]; layerMask++; vptr++;
            }
        }
        scrollX++;
    }
}

void gpuRenderScanLineMono(void)
{
    int i, j;
    unsigned short *vptr;       // pointer to frame buffer
    unsigned short baseColor, tmpColor;
    unsigned char scrollX;
    unsigned char scrollY;      // scroll registerY + scanline
    unsigned char tileX, tileY; // coordinate in 32 * 32 map
    unsigned short *map;        // map address
    unsigned char *color;       // tile color
    unsigned char pal;
    char *layerMask;
    char *windowMask;
    unsigned long *p;

    if (IO[0x60] & 0x80) {
        baseColor = ColorPalette[(IO[0x01] & 0xF0) >> 4][IO[0x01] & 0x0F];
    }
    else {
        baseColor = MonoColor[IO[0x01] & 0x07];
    }
// BG layer
    if (IO[0x00] & 0x01) {
        scrollX = IO[0x10] >> 3;
        scrollY = (IO[0x11] + IO[0x02]) & 0xFF;
        map = BgMap + ((scrollY & 0xF8) << 2); // scrollY>>3 <<5
        tileX = IO[0x10] & 0x07;
        tileY = scrollY & 0x07;
        vptr = GU_FRAME_ADDR(work_frame) + (IO[0x02] << 9) + 64 - tileX;
        for (j = -tileX; j < 224; j+=8) {
            color = gpuMonoTileCache(map[scrollX & 0x1F], tileY, map[scrollX & 0x1F] & 0x2000);
            pal = (map[scrollX & 0x1F] >> 9) & 0x0F;
            tmpColor = ColorPalette[pal][0];
            if (pal & 0x04) {
                ColorPalette[pal][0] = baseColor;
            }
            if (color) {
                *vptr++ = ColorPalette[pal][*color++];
                *vptr++ = ColorPalette[pal][*color++];
                *vptr++ = ColorPalette[pal][*color++];
                *vptr++ = ColorPalette[pal][*color++];
                *vptr++ = ColorPalette[pal][*color++];
                *vptr++ = ColorPalette[pal][*color++];
                *vptr++ = ColorPalette[pal][*color++];
                *vptr++ = ColorPalette[pal][*color++];
            }
            else {
                *vptr++ = ColorPalette[pal][0];
                *vptr++ = ColorPalette[pal][0];
                *vptr++ = ColorPalette[pal][0];
                *vptr++ = ColorPalette[pal][0];
                *vptr++ = ColorPalette[pal][0];
                *vptr++ = ColorPalette[pal][0];
                *vptr++ = ColorPalette[pal][0];
                *vptr++ = ColorPalette[pal][0];
            }
            ColorPalette[pal][0] = tmpColor;
            scrollX++;
        }
    }
    else {
        vptr = GU_FRAME_ADDR(work_frame) + (IO[0x02] << 9) + 64;
        for (i = 0; i < 224; i++) {
            *vptr++ = baseColor;
        }
    }
// FG layer
    if (IO[0x00] & 0x02) {
        gpuRenderFGMono();
    }
    else {
        p = (unsigned long*)(LMask + 8);
        i = 224 / 4;
        while(i > 0) {
            p[--i] = 0;
        }
    }
// Sprites
    if (IO[0x00] & 0x04) {
        // seek to last sprite
        unsigned long* sprBase = SprTable + SprCount;
        if(IO[0x00] & 0x08) {
            if ((IO[0x02] >= IO[0x0D]) && (IO[0x02] <= IO[0x0F])) {
                for(j = 0; j < IO[0x0C]; j++) {
                    WMaskIn[j] = 0;
                }
                for(; j <= IO[0x0E]; j++) {
                    WMaskIn[j] = 1;
                }
                for(; j < 224; j++) {
                    WMaskIn[j] = 0;
                }
            }
            else {
                p = (unsigned long*)WMaskIn;
                i = 224 / 4;
                while(i > 0) {
                    p[--i] = 0;
                }
            }
        }
        else {
            p = (unsigned long*)WMaskAll;
            i = 224 / 4;
            while(i > 0) {
                p[--i] = 0x01010101;
            }
        }
        for (i = SprCount; i > 0; i--) {
            sprBase--;
            unsigned long spr = *sprBase;
            short x= (spr & 0xFF000000) >> 24;
            short y= (spr & 0x00FF0000) >> 16;
            unsigned short tileInfo = spr & 0xFFFF;
            pal = ((spr & 0x0E00) >> 9) + 8;
            int layer = spr & 0x2000;
            int clipside = (spr & 0x1000) >> 12;
            if (y > 248) y -= 256;
            if ((IO[0x02] < y) || (IO[0x02] > (y+7))) {
                continue;
            }
            if (x > 248) x -= 256;
            if ((x < -7) || (x > 223)) {
                continue;
            }
            color = gpuMonoTileCache(tileInfo, (IO[0x02] - y)&0x07, 0);
            int nbPixels=8;
            if (x < 0) {
                if (color) {
                    color -= x;
                }
                nbPixels += x;
                x=0;
            }
            if (x + nbPixels > 224) {
                nbPixels = (224 - x);
            }
            vptr = GU_FRAME_ADDR(work_frame) + (IO[0x02] << 9) + 64 + x;
            layerMask = LMask + 8 + x;
            if(IO[0x00] & 0x08) {
                windowMask = WMaskIn + x;
            } else {
                windowMask = WMaskAll + x;
                clipside = 0;
            }
            if (color) {
                if (pal & 0x04) {
                    while (nbPixels--) {
                        if (*color && (clipside ^ *windowMask) && (layer||!(*layerMask))) {
                            *vptr = ColorPalette[pal][*color];
                        }
                        color++; vptr++; layerMask++; windowMask++;
                    }
                }
                else {
                    while (nbPixels--) {
                        if((clipside ^ *windowMask) && (layer || !(*layerMask))){
                            *vptr = ColorPalette[pal][*color];
                        }
                        color++; vptr++; layerMask++; windowMask++;
                    }
                }
            }
            else {
                if (!(pal & 0x04)) {
                    while (nbPixels--) {
                        if((clipside ^ *windowMask) && (layer || !(*layerMask))){
                            *vptr = ColorPalette[pal][0];
                        }
                        vptr++; layerMask++; windowMask++;
                    }
                }
            }
        }
    }
}

unsigned char* gpuColorTileCache(unsigned short tileInfo, int row, int bank)
{
    unsigned short tileIndex;
    int line;

    tileIndex = tileInfo & 0x1FF;
    if (bank) // bank
    {
        tileIndex += 512;
    }
    if (ColorModifiedTile[tileIndex])
    {
        unsigned char* tileInCachePtr = TileCache + (tileIndex<<6);
        unsigned char* hflippedTileInCachePtr = HflippedTileCache + (tileIndex<<6) + 7;
        unsigned long* tileInRamPtr = (unsigned long*)(RAM + 0x4000 + (tileIndex<<5));
        unsigned long tileLine, tileTemp;

        if (IO[0x60] & 0x20) // pack
        {
            for (line=0;line<8;line++)
            {
                tileLine=*tileInRamPtr++;
                *hflippedTileInCachePtr-- = *tileInCachePtr++ = (tileLine>>4) & 0x0f;
                *hflippedTileInCachePtr-- = *tileInCachePtr++ = (tileLine>>0) & 0x0f;
                *hflippedTileInCachePtr-- = *tileInCachePtr++ = (tileLine>>12) & 0x0f;
                *hflippedTileInCachePtr-- = *tileInCachePtr++ = (tileLine>>8) & 0x0f;
                *hflippedTileInCachePtr-- = *tileInCachePtr++ = (tileLine>>20) & 0x0f;
                *hflippedTileInCachePtr-- = *tileInCachePtr++ = (tileLine>>16) & 0x0f;
                *hflippedTileInCachePtr-- = *tileInCachePtr++ = (tileLine>>28) & 0x0f;
                *hflippedTileInCachePtr-- = *tileInCachePtr++ = (tileLine>>24) & 0x0f;
                hflippedTileInCachePtr += 16;
                TransparentTile[tileIndex][line] = tileLine;
            }
        }
        else // layer
        {
            for (line=0; line<8; line++)
            {
                tileLine = *tileInRamPtr++;
                tileTemp = tileLine & 0x80808080; *hflippedTileInCachePtr-- = *tileInCachePtr++ = (tileTemp>>7)|(tileTemp>>14)|(tileTemp>>21)|(tileTemp>>28);
                tileTemp = tileLine & 0x40404040; *hflippedTileInCachePtr-- = *tileInCachePtr++ = (tileTemp>>6)|(tileTemp>>13)|(tileTemp>>20)|(tileTemp>>27);
                tileTemp = tileLine & 0x20202020; *hflippedTileInCachePtr-- = *tileInCachePtr++ = (tileTemp>>5)|(tileTemp>>12)|(tileTemp>>19)|(tileTemp>>26);
                tileTemp = tileLine & 0x10101010; *hflippedTileInCachePtr-- = *tileInCachePtr++ = (tileTemp>>4)|(tileTemp>>11)|(tileTemp>>18)|(tileTemp>>25);
                tileTemp = tileLine & 0x08080808; *hflippedTileInCachePtr-- = *tileInCachePtr++ = (tileTemp>>3)|(tileTemp>>10)|(tileTemp>>17)|(tileTemp>>24);
                tileTemp = tileLine & 0x04040404; *hflippedTileInCachePtr-- = *tileInCachePtr++ = (tileTemp>>2)|(tileTemp>> 9)|(tileTemp>>16)|(tileTemp>>23);
                tileTemp = tileLine & 0x02020202; *hflippedTileInCachePtr-- = *tileInCachePtr++ = (tileTemp>>1)|(tileTemp>> 8)|(tileTemp>>15)|(tileTemp>>22);
                tileTemp = tileLine & 0x01010101; *hflippedTileInCachePtr-- = *tileInCachePtr++ = (tileTemp>>0)|(tileTemp>> 7)|(tileTemp>>14)|(tileTemp>>21);
                hflippedTileInCachePtr += 16;
                TransparentTile[tileIndex][line] = tileLine;
            }
        }
        ColorModifiedTile[tileIndex]=0;
    }
    if (tileInfo & 0x8000) // v flip
    {
        row = 7 - row;
    }
    if (TransparentTile[tileIndex][row] == 0) {
        return NULL;
    }
    if (tileInfo & 0x4000) // h flip
    {
        return(&HflippedTileCache[(tileIndex<<6)+(row<<3)]);
    }
    return(&TileCache[(tileIndex<<6)+(row<<3)]);
}

void gpuRenderFGColor(void)
{
// FG layer
    int i, j;
    unsigned short *vptr;       // pointer to frame buffer
    unsigned char scrollX;
    unsigned char scrollY;      // scroll registerY + scanline
    unsigned char tileX, tileY; // coordinate in 32 * 32 map
    unsigned short *map;        // map address
    unsigned char *color;       // tile color
    unsigned char pal;
    char *layerMask;
    unsigned long *p;

    int windowMode = IO[0x00] & 0x30;
    p = (unsigned long*)(LMask + 8);
    //inside
    if (windowMode == 0x20) {
        if ((IO[0x02] < IO[0x09]) || (IO[0x02] > IO[0x0B])) {
            i = 224 / 4;
            while(i > 0) {
                p[--i] = 0;
            }
            return;
        }
        for (i = 0; i < IO[0x08]; i++) {
            LMask[i + 8] = 0;
        }
        for (; i <= IO[0x0A]; i++) {
            LMask[i + 8] = 1;
        }
        for (; i < 224; i++) {
            LMask[i + 8] = 0;
        }
    }
    //outside
    else if (windowMode == 0x30) {
        if ((IO[0x02] >= IO[0x09]) && (IO[0x02] <= IO[0x0B])) {
            for (i = 0; i < IO[0x08]; i++) {
                LMask[i + 8] = 1;
            }
            for (; i <= IO[0x0A]; i++) {
                LMask[i + 8] = 0;
            }
            for (; i < 224; i++) {
                LMask[i + 8] = 1;
            }
        }
        else {
            i = 224 / 4;
            while(i > 0) {
                p[--i] = 0x01010101;
            }
        }
    }
    else {
        i = 224 / 4;
        while(i > 0) {
            p[--i] = 0x01010101;
        }
    }
    // seek to the first tile
    scrollX = IO[0x12] >> 3;
    scrollY = (IO[0x13] + IO[0x02]) & 0xFF;
    map = FgMap + ((scrollY & 0xF8) << 2); // scrollY>>3 <<5
    tileX = IO[0x12] & 0x07;
    tileY = scrollY & 0x07;
    vptr = GU_FRAME_ADDR(work_frame) + (IO[0x02] << 9) + 64 - tileX;
    layerMask = LMask + 8 - tileX;
    for (j = -tileX; j < 224; j += 8) {
        color = gpuColorTileCache(map[scrollX & 0x1F], tileY, map[scrollX & 0x1F] & 0x2000);
        if (color) {
            pal = (map[scrollX & 0x1F] >> 9) &0x0F;
            if (*color && *layerMask) *vptr = ColorPalette[pal][*color];
            else *layerMask = 0;
            color++;layerMask++;vptr++;
            if (*color && *layerMask) *vptr = ColorPalette[pal][*color];
            else *layerMask = 0;
            color++;layerMask++;vptr++;
            if (*color && *layerMask) *vptr = ColorPalette[pal][*color];
            else *layerMask = 0;
            color++;layerMask++;vptr++;
            if (*color && *layerMask) *vptr = ColorPalette[pal][*color];
            else *layerMask = 0;
            color++;layerMask++;vptr++;
            if (*color && *layerMask) *vptr = ColorPalette[pal][*color];
            else *layerMask = 0;
            color++;layerMask++;vptr++;
            if (*color && *layerMask) *vptr = ColorPalette[pal][*color];
            else *layerMask = 0;
            color++;layerMask++;vptr++;
            if (*color && *layerMask) *vptr = ColorPalette[pal][*color];
            else *layerMask = 0;
            color++;layerMask++;vptr++;
            if (*color && *layerMask) *vptr = ColorPalette[pal][*color];
            else *layerMask = 0;
            color++;layerMask++;vptr++;
        }
        else {
            vptr += 8;
            *layerMask++ = 0;
            *layerMask++ = 0;
            *layerMask++ = 0;
            *layerMask++ = 0;
            *layerMask++ = 0;
            *layerMask++ = 0;
            *layerMask++ = 0;
            *layerMask++ = 0;
        }
        scrollX++;
    }
}

void gpuRenderScanLineColor(void)
{
    int i, j;
    unsigned short *vptr;       // pointer to frame buffer
    unsigned short baseColor, tmpColor;
    unsigned char scrollX;
    unsigned char scrollY;      // scroll registerY + scanline
    unsigned char tileX, tileY; // coordinate in 32 * 32 map
    unsigned short *map;        // map address
    unsigned char *color;       // tile color
    unsigned char pal;
    char *layerMask;
    char *windowMask;
    unsigned long *p;

    baseColor = ColorPalette[(IO[0x01] & 0xF0) >> 4][IO[0x01] & 0x0F];
// BG layer
    if (IO[0x00] & 0x01) {
        scrollX = IO[0x10] >> 3;
        scrollY = (IO[0x11] + IO[0x02]) & 0xFF;
        map = BgMap + ((scrollY & 0xF8) << 2); // scrollY>>3 * 32
        tileX = IO[0x10] & 0x07;
        tileY = scrollY & 0x07;
        vptr = GU_FRAME_ADDR(work_frame) + (IO[0x02] << 9) + 64 - tileX;
        for (j = -tileX; j < 224; j += 8) {
            color = gpuColorTileCache(map[scrollX & 0x1F], tileY, map[scrollX & 0x1F] & 0x2000);
            if (color) {
                pal = (map[scrollX & 0x1F] >> 9) &0x0F;
                tmpColor = ColorPalette[pal][0];
                ColorPalette[pal][0] = baseColor;
                *vptr++ = ColorPalette[pal][*color++];
                *vptr++ = ColorPalette[pal][*color++];
                *vptr++ = ColorPalette[pal][*color++];
                *vptr++ = ColorPalette[pal][*color++];
                *vptr++ = ColorPalette[pal][*color++];
                *vptr++ = ColorPalette[pal][*color++];
                *vptr++ = ColorPalette[pal][*color++];
                *vptr++ = ColorPalette[pal][*color++];
                ColorPalette[pal][0] = tmpColor;
            }
            else {
                *vptr++ = baseColor;
                *vptr++ = baseColor;
                *vptr++ = baseColor;
                *vptr++ = baseColor;
                *vptr++ = baseColor;
                *vptr++ = baseColor;
                *vptr++ = baseColor;
                *vptr++ = baseColor;
            }
            scrollX++;
        }
    }
    else {
        vptr = GU_FRAME_ADDR(work_frame) + (IO[0x02] << 9) + 64;
        for (i = 0; i < 224; i++) {
            *vptr++ = baseColor;
        }
    }
    if (IO[0x00] & 0x02) {
        gpuRenderFGColor();
    }
    else {
        p = (unsigned long*)(LMask + 8);
        i = 224 / 4;
        while(i > 0) {
            p[--i] = 0;
        }
    }
// Sprites
    if (IO[0x00] & 0x04) {
        // seek to last sprite
        unsigned long* sprBase = SprTable + SprCount;
        if(IO[0x00] & 0x08) {
            if ((IO[0x02] >= IO[0x0D]) && (IO[0x02] <= IO[0x0F])) {
                for(j = 0; j < IO[0x0C]; j++) {
                    WMaskIn[j] = 0;
                }
                for(; j <= IO[0x0E]; j++) {
                    WMaskIn[j] = 1;
                }
                for(; j < 224; j++) {
                    WMaskIn[j] = 0;
                }
            }
            else {
                p = (unsigned long*)WMaskIn;
                i = 224 / 4;
                while(i > 0) {
                    p[--i] = 0;
                }
            }
        }
        else {
            p = (unsigned long*)WMaskAll;
            i = 224 / 4;
            while(i > 0) {
                p[--i] = 0x01010101;
            }
        }
        for (i = SprCount; i > 0; i--) {
            sprBase--;
            unsigned long spr = *sprBase;
            short x= (spr & 0xFF000000) >> 24;
            short y= (spr & 0x00FF0000) >> 16;
            unsigned short tileInfo = spr & 0xFFFF;
            pal = ((spr & 0xE00) >> 9) + 8;
            int layer = spr & 0x2000;
            int clipside = (spr & 0x1000) >> 12;
            if (y > 248) y -= 256;
            if ((IO[0x02] < y) || (IO[0x02] > (y+7))) {
                continue;
            }
            if (x > 248) x -= 256;
            if ((x < -7) || (x > 223)) {
                continue;
            }
            color = gpuColorTileCache(tileInfo, (IO[0x02] - y)&0x07, 0);
            int nbPixels=8;
            if (x < 0) {
                if (color) {
                    color -= x;
                }
                nbPixels += x;
                x=0;
            }
            if (x + nbPixels > 224) {
                nbPixels = (224 - x);
            }
            vptr = GU_FRAME_ADDR(work_frame) + (IO[0x02] << 9) + 64 + x;
            layerMask = LMask + 8 + x;
            if(IO[0x00] & 0x08) {
                windowMask = WMaskIn + x;
            } else {
                windowMask = WMaskAll + x;
                clipside = 0;
            }
            if (color) {
                while (nbPixels--) {
                    if (*color && (clipside ^ *windowMask) && (layer || !(*layerMask))) {
                        *vptr = ColorPalette[pal][*color];
                    }
                    color++; vptr++; layerMask++; windowMask++;
                }
            }
        }
    }
}

void gpuSetSegment(void)
{
    unsigned short *vptr, *seg;
    int x, y;

    vptr = GU_FRAME_ADDR(work_frame) + 64 + 224;
    for (y = 0; y < 144;)
    {
        if (y == 0 && (IO[0x15] & 0x20)) seg = circle3;
        else if (y == 12 && (IO[0x15] & 0x10)) seg = circle2;
        else if (y == 24 && (IO[0x15] & 8)) seg = circle1;
        else if (y == 36 && (IO[0x15] & 4)) seg = horizontal;
        else if (y == 50 && (IO[0x15] & 2)) seg = vertical;
        else if (y == 120 && (IO[0x15] & 1)) seg = sleep;
        else seg = NULL;
        if (seg)
        {
            for (x = 0; x < 12; x++)
            {
                *vptr++ = *seg++;
                *vptr++ = *seg++;
                *vptr++ = *seg++;
                *vptr++ = *seg++;
                *vptr++ = *seg++;
                *vptr++ = *seg++;
                *vptr++ = *seg++;
                *vptr++ = *seg++;
                *vptr++ = *seg++;
                *vptr++ = *seg++;
                *vptr++ = *seg++;
                *vptr++ = *seg++;
                *vptr++ = *seg++;
                *vptr++ = *seg++;
                *vptr++ = *seg++;
                *vptr++ = *seg++;
                vptr += (512-16);
                y++;
            }
        }
        else
        {
            *vptr++ = 0x8000;
            *vptr++ = 0x8000;
            *vptr++ = 0x8000;
            *vptr++ = 0x8000;
            *vptr++ = 0x8000;
            *vptr++ = 0x8000;
            *vptr++ = 0x8000;
            *vptr++ = 0x8000;
            *vptr++ = 0x8000;
            *vptr++ = 0x8000;
            *vptr++ = 0x8000;
            *vptr++ = 0x8000;
            *vptr++ = 0x8000;
            *vptr++ = 0x8000;
            *vptr++ = 0x8000;
            *vptr++ = 0x8000;
            vptr += (512-16);
            y++;
        }
    }
}

