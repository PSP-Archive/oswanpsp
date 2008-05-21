
#include <pspkernel.h>
#include <pspctrl.h>
#include <stdio.h>
#include <string.h>
#include "mon.h"
#include "ws.h"
#include "pg.h"

void monPrintMem(unsigned int offset)
{
    int y;
    char buf[256];
    unsigned char* addr = RAM + offset;

    pgFillvram(0);
    mh_print(2, FONT_HEIGHT * 0, "ADDRESS: 00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F",RGB(255, 255, 255));
    for (y = 0; y < 20; y++)
    {
        sprintf(buf, "   %04X: %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X",
            offset, addr[0], addr[1], addr[2], addr[3], addr[4], addr[5], addr[6], addr[7],
            addr[8], addr[9], addr[10], addr[11], addr[12], addr[13], addr[14], addr[15]);
        mh_print(2, FONT_HEIGHT * (y+1), buf, RGB(255, 255, 255));
        addr += 16;
        offset += 16;
    }
    pgScreenFlipV();
}

void monMenu(void)
{
    SceCtrlData pad;
    unsigned int newButton, oldButton = 0;
    int offset = 0x1800;

    monPrintMem(offset);
    while(Run)
    {
        sceCtrlReadBufferPositive(&pad, 1);
        newButton = pad.Buttons;
        if (newButton & PSP_CTRL_CROSS)
        {
            pgFillvram(0);
            pgScreenFlipV();
            pgFillvram(0);
            return;
        }
        if ((newButton & PSP_CTRL_DOWN) && !(oldButton & PSP_CTRL_DOWN))
        {
            offset += 16;
            monPrintMem(offset);
        }
        if ((newButton & PSP_CTRL_UP) && !(oldButton & PSP_CTRL_UP))
        {
            offset -= 16;
            monPrintMem(offset);
        }
        if ((newButton & PSP_CTRL_RIGHT) && !(oldButton & PSP_CTRL_RIGHT))
        {
            offset += 16 * 20;
            monPrintMem(offset);
        }
        if ((newButton & PSP_CTRL_LEFT) && !(oldButton & PSP_CTRL_LEFT))
        {
            offset -= 16 * 20;
            monPrintMem(offset);
        }
        oldButton = newButton;
    }
}
