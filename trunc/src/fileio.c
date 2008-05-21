
#include <pspkernel.h>
#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include "fileio.h"
#include "pg.h"
#include "ws.h"
#include "gpu.h"
#include "apu.h"
#include "debug/debug.h"
#include "./nec/nec.h"

char pg_workdir[MAX_PATH];
char CurDir[MAX_PATH]; // menu.c
char RomPath[MAX_PATH]; // menu.c
unsigned long CartSize;
unsigned char *ROMMap[0x100];
WSROMHEADER WsRomHeader;

void fileioSetModulePath(int args, char *argv[])
{
    int n;
    char pg_mypath[MAX_PATH];

    strncpy(pg_mypath,argv[0], sizeof(pg_mypath));
    pg_mypath[sizeof(pg_mypath)-1]=0;
    strcpy(pg_workdir,pg_mypath);
    for (n=strlen(pg_workdir); n>0 && pg_workdir[n-1]!='/'; --n) pg_workdir[n-1]=0;
    strcpy(CurDir, pg_workdir);
}

void fileioGetModulePath(char *fn, int nSize)
{
    strncpy(fn, pg_workdir, nSize);
}

void fileioGetStatePath(char *fn, int nSize, int slot)
{
    char* romName;
    char* romExt;
    char str[8];

    fileioGetModulePath(fn, nSize);
    strcat(fn, "STATE/");
    romName = fileioPathFindFileName(RomPath);
    strcat(fn, romName);
    if (!(romExt = strrchr(fn, '.')))
    {
        return;
    }
    *romExt = '\0';
    sprintf(str, ".%03d", slot);
    strcat(fn, str);
}

void fileioGetSavePath(char *fn, int nSize)
{
    char* romName;
    char* romExt;
    char str[8] = ".sav";

    fileioGetModulePath(fn, nSize);
    strcat(fn, "SAVE/");
    romName = fileioPathFindFileName(RomPath);
    strcat(fn, romName);
    if (!(romExt = strrchr(fn, '.')))
    {
        return;
    }
    *romExt = '\0';
    strcat(fn, str);
}

char *fileioPathFindFileName(const char *fn)
{
    char *pszSlash = NULL;
    int cbI = 0;

    while(fn[cbI]) {
        if ((((unsigned char)fn[cbI]>=0x80) && ((unsigned char)fn[cbI]<0xa0)) || ((unsigned char)fn[cbI]>=0xe0)) {
            if (fn[cbI+1]) {
                cbI+=2;
                continue;
            }
            break;
        }
        if (fn[cbI] == '/') {
            pszSlash = (char*)fn+cbI;
        }
        cbI++;
    }
    if (!pszSlash) {
        // âûã}èàíu
        pszSlash = strrchr(fn, '/');
    }
    if (pszSlash) pszSlash++;
    return pszSlash;
}

int fileioOpenRom(void)
{
    SceUID fd;
    SceIoStat st;
    int i;

    memset(&st, 0, sizeof(st));
    sceIoGetstat(RomPath, &st);
    fd = sceIoOpen(RomPath, PSP_O_RDONLY, 0777);
    sceIoLseek32(fd, -10, SEEK_END);
    sceIoRead(fd, &WsRomHeader, sizeof(WsRomHeader));
    switch (WsRomHeader.romSize)
    {
        case 0: CartSize = 0x20000; break;
        case 1: CartSize = 0x40000; break;
        case 2: CartSize = 0x80000; break;
        case 3: CartSize = 0x100000; break;
        case 4: CartSize = 0x200000; break;
        case 5: CartSize = 0x300000; break;
        case 6: CartSize = 0x400000; break;
        case 7: CartSize = 0x600000; break;
        case 8: CartSize = 0x800000; break;
        case 9: CartSize = 0x1000000; break;
        default: CartSize = 0; break;
    }
    if (CartSize != st.st_size)
    {
        return 4;
    }
    SramSize = PromSize = 0;
    switch (WsRomHeader.saveSize)
    {
        case 1: SramSize = 0x02000; break;
        case 2: SramSize = 0x08000; break;
        case 3: SramSize = 0x20000; break;
        case 4: SramSize = 0x40000; break;
        case 5: SramSize = 0x80000; break;
        case 0x10: PromSize = 0x0080; break;
        case 0x20: PromSize = 0x0800; break;
        case 0x50: PromSize = 0x0400; break;
    }
    if (!(Cart = (unsigned char*)malloc(CartSize)))
    {
        return 1;
    }
    if (SramSize)
    {
        if (!(StaticRAM = (unsigned char*)malloc(SramSize)))
        {
            return 2;
        }
        ROM[0x01] = StaticRAM;
    }
    if (PromSize)
    {
        if (!(CProm.data = (unsigned short*)malloc(PromSize)))
        {
            return 3;
        }
    }
    sceIoLseek32(fd, 0-CartSize, SEEK_END);
    sceIoRead(fd, Cart, CartSize);
    sceIoClose(fd);
    for (i = 0; i < 0x100; i++)
    {
        if ((CartSize - (0x100 - i) * 0x10000) < 0)
        {
            ROMMap[i] = Cart;
        }
        else
        {
            ROMMap[i] = Cart + CartSize - (0x100 - i) * 0x10000;
        }
    }
    Cursor = WsRomHeader.isV & 1;
    return 0;
}

void fileioLoadIProm(void)
{
    SceUID fd;
    char buf[MAX_PATH];

    strcpy(buf, pg_workdir);
    strcat(buf, "eeprom.dat");
    if (!(fd = sceIoOpen(buf, PSP_O_RDONLY, 0777)))
    {
        return;
    }
    sceIoRead(fd, InternalEeprom, 128);
    sceIoClose(fd);
}

void fileioLoadData(void)
{
    SceUID fd;
    char buf[MAX_PATH];

    if (SramSize)
    {
        fileioGetSavePath(buf, MAX_PATH);
        if (!(fd = sceIoOpen(buf, PSP_O_RDONLY, 0777)))
        {
            return;
        }
        sceIoRead(fd, StaticRAM, SramSize);
        sceIoClose(fd);
    }
    else if (PromSize)
    {
        fileioGetSavePath(buf, MAX_PATH);
        if (!(fd = sceIoOpen(buf, PSP_O_RDONLY, 0777)))
        {
            return;
        }
        sceIoRead(fd, CProm.data, PromSize);
        sceIoClose(fd);
    }
}

void fileioSaveData(void)
{
    SceUID fd;
    char buf[MAX_PATH];

    strcpy(buf, pg_workdir);
    strcat(buf, "eeprom.dat");
    if (!(fd = sceIoOpen(buf, PSP_O_WRONLY | PSP_O_CREAT | PSP_O_TRUNC, 0777)))
    {
        return;
    }
    sceIoWrite(fd, IProm.data, 128);
    sceIoClose(fd);
    if (SramSize)
    {
        fileioGetSavePath(buf, MAX_PATH);
        if (!(fd = sceIoOpen(buf, PSP_O_WRONLY | PSP_O_CREAT | PSP_O_TRUNC, 0777)))
        {
            return;
        }
        sceIoWrite(fd, StaticRAM, SramSize);
        sceIoClose(fd);
    }
    else if (PromSize)
    {
        fileioGetSavePath(buf, MAX_PATH);
        if (!(fd = sceIoOpen(buf, PSP_O_WRONLY | PSP_O_CREAT | PSP_O_TRUNC, 0777)))
        {
            return;
        }
        sceIoWrite(fd, CProm.data, PromSize);
        sceIoClose(fd);
    }
    // WonderWitch Flash Save
    if (SramSize == 0x40000 && Cart)
    {
        if (!(fd = sceIoOpen(RomPath, PSP_O_WRONLY | PSP_O_CREAT | PSP_O_TRUNC, 0777)))
        {
            return;
        }
        sceIoWrite(fd, Cart, CartSize);
        sceIoClose(fd);
    }
}

void fileioStateSave(int slot)
{
    char file[MAX_PATH];
    SceUID fd;
    char* I;
    int Ilen;

    fileioGetStatePath(file, MAX_PATH, slot);
    if (!(fd = sceIoOpen(file, PSP_O_WRONLY | PSP_O_CREAT | PSP_O_TRUNC, 0777)))
    {
        return;
    }
    I = nec_getRegPtr(&Ilen);
    sceIoWrite(fd, I, Ilen);
    sceIoWrite(fd, RAM, sizeof(RAM));
    sceIoWrite(fd, IO, sizeof(IO));
    if (SramSize) sceIoWrite(fd, StaticRAM, SramSize);
    if (PromSize) sceIoWrite(fd, CProm.data, PromSize);
    sceIoWrite(fd, MonoColor, sizeof(MonoColor));
    sceIoWrite(fd, ColorPalette, sizeof(ColorPalette));
    sceIoWrite(fd, &hblankTimer, sizeof(hblankTimer));
    sceIoWrite(fd, &hblankTimerPreset, sizeof(hblankTimerPreset));
    sceIoWrite(fd, &vblankTimer, sizeof(vblankTimer));
    sceIoWrite(fd, &vblankTimerPreset, sizeof(vblankTimerPreset));
    sceIoClose(fd);
}

void fileioStateLoad(int slot)
{
    char file[MAX_PATH];
    SceUID fd;
    char* I;
    int Ilen;
    int i;

    fileioGetStatePath(file, MAX_PATH, slot);
    if (!(fd = sceIoOpen(file, PSP_O_RDONLY, 0777)))
    {
        return;
    }
    gpuInit();
    I = nec_getRegPtr(&Ilen);
    sceIoRead(fd, I, Ilen);
    sceIoRead(fd, RAM, sizeof(RAM));
    sceIoRead(fd, IO, sizeof(IO));
    if (SramSize) sceIoRead(fd, StaticRAM, SramSize);
    if (PromSize) sceIoRead(fd, CProm.data, PromSize);
    sceIoRead(fd, MonoColor, sizeof(MonoColor));
    sceIoRead(fd, ColorPalette, sizeof(ColorPalette));
    sceIoRead(fd, &hblankTimer, sizeof(hblankTimer));
    sceIoRead(fd, &hblankTimerPreset, sizeof(hblankTimerPreset));
    sceIoRead(fd, &vblankTimer, sizeof(vblankTimer));
    sceIoRead(fd, &vblankTimerPreset, sizeof(vblankTimerPreset));
    sceIoClose(fd);
    for (i = 0x80; i <= 0x90; i++)
    {
        wsWritePort(i, IO[i]);
    }
    wsWritePort(0x04, IO[0x04]); // SprMap
    wsWritePort(0x07, IO[0x07]); // BG FG Map
    wsWritePort(0xC1, IO[0xC1]);
    wsWritePort(0xC2, IO[0xC2]);
    wsWritePort(0xC3, IO[0xC3]);
    i = (IO[0xC0] << 4) & 0xF0;
    ROM[0x04] = ROMMap[0x04 | i];
    ROM[0x05] = ROMMap[0x05 | i];
    ROM[0x06] = ROMMap[0x06 | i];
    ROM[0x07] = ROMMap[0x07 | i];
    ROM[0x08] = ROMMap[0x08 | i];
    ROM[0x09] = ROMMap[0x09 | i];
    ROM[0x0A] = ROMMap[0x0A | i];
    ROM[0x0B] = ROMMap[0x0B | i];
    ROM[0x0C] = ROMMap[0x0C | i];
    ROM[0x0D] = ROMMap[0x0D | i];
    ROM[0x0E] = ROMMap[0x0E | i];
    ROM[0x0F] = ROMMap[0x0F | i];
}


