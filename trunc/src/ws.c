/*
* $Id$
*/

#include <pspkernel.h>
#include <pspdisplay.h>
#include <pspctrl.h>
#include <stdio.h>
#include <malloc.h>
#include <time.h>
#include "pdata.h"
#include "fileio.h"
#include "video.h"
#include "ws.h"
#include "gpu.h"
#include "apu.h"
#include "./debug/debug.h"
#include "./nec/nec.h"

#define WS_CYCLES   256/8

static const unsigned char initialIoValue[256] = {
    0x00, 0x00, 0x9d, 0xbb, 0x00, 0x00, 0x00, 0x26, 0xfe, 0xde, 0xf9, 0xfb, 0xdb, 0xd7, 0x7f, 0xf5, // 0
    0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x9e, 0x9b, 0x00, 0x00, 0x00, 0x00, 0x99, 0xfd, 0xb7, 0xdf, // 1
    0x30, 0x57, 0x75, 0x76, 0x15, 0x73, 0x77, 0x77, 0x20, 0x75, 0x50, 0x36, 0x70, 0x67, 0x50, 0x77, // 2
    0x57, 0x54, 0x75, 0x77, 0x75, 0x17, 0x37, 0x73, 0x50, 0x57, 0x60, 0x77, 0x70, 0x77, 0x10, 0x73, // 3
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 4
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 5
    0x0a, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0f, 0x00, 0x00, 0x00, 0x00, // 6
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 7
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1f, 0x00, 0x00, // 8
    0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, // 9
    0x85, 0x00, 0x00, 0x00, 0x00, 0x00, 0x4f, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // A
    0x00, 0xdb, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x42, 0x00, 0x83, 0x00, // B
    0x2f, 0x00, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0xd1, 0xd1, 0xd1, 0xd1, 0xd1, 0xd1, 0xd1, 0xd1, // C
    0xd1, 0xd1, 0xd1, 0xd1, 0xd1, 0xd1, 0xd1, 0xd1, 0xd1, 0xd1, 0xd1, 0xd1, 0xd1, 0xd1, 0xd1, 0xd1, // D
    0xd1, 0xd1, 0xd1, 0xd1, 0xd1, 0xd1, 0xd1, 0xd1, 0xd1, 0xd1, 0xd1, 0xd1, 0xd1, 0xd1, 0xd1, 0xd1, // E
    0xd1, 0xd1, 0xd1, 0xd1, 0xd1, 0xd1, 0xd1, 0xd1, 0xd1, 0xd1, 0xd1, 0xd1, 0xd1, 0xd1, 0xd1, 0xd1  // F
};

#define RGBmono(m) ((m<<10)|(m<<5)|(m<<0)|0x8000)
static const unsigned short DefColor[]={
    RGBmono(31), RGBmono(29), RGBmono(27), RGBmono(25),
    RGBmono(23), RGBmono(21), RGBmono(19), RGBmono(17),
    RGBmono(15), RGBmono(13), RGBmono(11), RGBmono( 9),
    RGBmono( 7), RGBmono( 5), RGBmono( 3), RGBmono( 1)
};

int Run = 1;
unsigned char *Cart; // 本体データ mallocで確保してカートリッジ読み込み => ROM[2]-ROM[15]
unsigned char *StaticRAM; // セーブ用SRAM mallocで確保 => ROM[1]
unsigned char RAM[0x10000]; // 決まっているのであらかじめ確保 => ROM[0]
unsigned char *ROM[16]; // WS memory
unsigned char IO[0x100]; // IO port
unsigned short InternalEeprom[64]; // internal eeprom
EEPROM IProm, CProm;
unsigned long SramSize;
unsigned short PromSize;
unsigned short MonoColor[8];
unsigned short ColorPalette[16][16];
int hblankTimer, hblankTimerPreset;
int vblankTimer, vblankTimerPreset;
int RtcCount;
SceCtrlData Pad;
int Cursor;
int WsFrame = 0;
int WsSkip = 0;
int Fps = 0;
int Drop = 0;
int ScreenSize;
int Vsync;
unsigned char SRAM[0x10000]; // ダミーのSRAM(WonderWitchで使用)

RECT src_rect = {64,0,64+240,144};
RECT dst_rect = {120,64,120+240,64+144};
RECT large_rect = {20,0,20+453,SCR_HEIGHT};
RECT fps_rect = {0,0,32,32};

extern unsigned short *draw_frame;
extern unsigned short *work_frame;
extern unsigned short *tex_frame;
extern RECT full_rect;

/*-------------------------
  初期化
-------------------------*/
void wsInit(void)
{
    int ret;

    IProm.data = InternalEeprom;
    IProm.we = 0;
    CProm.data = NULL;
    CProm.we = 0;
	video_clear_frame(tex_frame);
    if ((ret = fileioOpenRom()))
    {
        if (ret == 4)
        {
            mh_print(0, 0, "ROMのサイズが一致しません",RGB(255, 255, 255));
        }
        else
        {
            mh_print(0, 0, "memory error",RGB(255, 255, 255));
        }
        video_copy_rect(tex_frame, draw_frame, &full_rect, &full_rect);
        video_wait_vsync_n(100);
        return;
    }
	video_clear_screen();
    fileioLoadIProm();
    fileioLoadData();
    memset(RAM, 0, 0x10000);
    memset(SRAM, 0xA0, 0x10000);
    ROM[0] = RAM;
    ROM[1] = SRAM;
    gpuInit();
    wsReset();
}

/*-------------------------
  パーソナルデータのプログラムをセット
-------------------------*/
void wsPdata(void)
{
    int i;

    RomPath[0] = '\0';
    IProm.data = InternalEeprom;
    IProm.we = 0;
    CProm.data = NULL;
    CProm.we = 0;
    Cart = (unsigned char*)malloc(0x20000);
    if (!Cart)
    {
        mh_print(0, 0, "memory error",RGB(255, 255, 255));
        video_copy_rect(tex_frame, draw_frame, &full_rect, &full_rect);
        video_wait_vsync_n(100);
        return;
    }
    memcpy(Cart + 0x1F080, pdata, size_pdata);
    SramSize = PromSize = 0;
    for (i = 0; i < 0xFF; i++)
    {
        ROMMap[i] = Cart;
    }
    ROMMap[0xFF] = Cart + 0x10000;
    Cursor = 0;
    fileioLoadIProm();
    memset(RAM, 0, 0x10000);
    ROM[0] = RAM;
    ROM[1] = SRAM;
    gpuInit();
    wsReset();
}

/*-------------------------
  終了
-------------------------*/
void wsExit(void)
{
    fileioSaveData();
    free(Cart);
    Cart = NULL;
    free(StaticRAM);
    StaticRAM = NULL;
    free(CProm.data);
    CProm.data = NULL;
}

/*-------------------------
  リセット
-------------------------*/
void wsReset(void)
{
    int i;

    memset(IO, 0, sizeof(IO));
    for (i = 0; i < 256; i++)
    {
        wsWritePort(i, initialIoValue[i]);
    }
    // CPU reset
    nec_reset(NULL);
    nec_set_reg(NEC_SP, 0x2000);
}

/*-------------------------
  メモリ書き込み
  0x00000 : 内蔵RAM
  0x10000 : Flash RAM
  0x20000 : ROM
-------------------------*/
#define	FLASH_CMD_ADDR1			0x10AAA
#define	FLASH_CMD_ADDR2			0x10555
#define	FLASH_CMD_DATA1			0xAA
#define	FLASH_CMD_DATA2			0x55
#define	FLASH_CMD_RESET			0xF0
#define	FLASH_CMD_ERASE			0x80
#define	FLASH_CMD_ERASE_CHIP	0x10
#define	FLASH_CMD_ERASE_SECT	0x30
#define	FLASH_CMD_CONTINUE_SET	0x20
#define	FLASH_CMD_CONTINUE_RES1	0x90
#define	FLASH_CMD_CONTINUE_RES2	0xF0
#define	FLASH_CMD_CONTINUE_RES3	0x00
#define	FLASH_CMD_WRITE			0xA0
void wsWriteMem(int addr, unsigned char val)
{
    unsigned short color, pal;
    unsigned char r, g, b;
	static int flash_command1    = 0;
	static int flash_command2    = 0;
	static int flash_write_set   = 0;
	static int flash_write_one   = 0;
	static int flash_write_reset = 0;
	static int write_enable      = 0;

    if (addr < 0x10000)
    {
        if ((addr >= 0x2000) && (addr < 0x6000))
        {
            if (RAM[addr] != val)
            {
                MonoModifiedTile[(addr - 0x2000) >> 4] = 1;
            }
        }
        if ((addr >= 0x4000) && (addr < 0xC000))
        {
            if (RAM[addr] != val)
            {
                ColorModifiedTile[(addr - 0x4000) >> 5] = 1;
            }
        }
        RAM[addr] = val;
        // palette ram
        if (addr >= 0xFE00)
        {
            // RGB444 format
            color = *(unsigned short*)(RAM + (addr & 0xFFFE));
            r   = (color >> 8) & 0x0F;
            g   = (color >> 4) & 0x0F;
            b   =  color       & 0x0F;
            pal = (b << 11) | (g << 6) | (r << 1) | 0x8000;
            ColorPalette[(addr & 0x1E0) >> 5][(addr & 0x1E) >> 1] = pal;
        }
        if (!((addr - WaveMap) & 0xFFC0))
        {
            apuSetPData(addr & 0x3F, val);
        }
    }
    else if (StaticRAM && addr < 0x20000)
    {
		// WonderWitch
		// FLASH ROM command sequence
		if (flash_command2) {
			if (addr == FLASH_CMD_ADDR1) {
				switch (val) {
				case FLASH_CMD_CONTINUE_SET:
					flash_write_set   = 1;
					flash_write_reset = 0;
					break;
				case FLASH_CMD_WRITE:
					flash_write_one = 1;
					break;
				case FLASH_CMD_RESET:
					break;
				case FLASH_CMD_ERASE:
					break;
				case FLASH_CMD_ERASE_CHIP:
					break;
				case FLASH_CMD_ERASE_SECT:
					break;
				default:
					break;
				}
			}
			flash_command2 = 0;
		}
		else if (flash_command1) {
			if (addr == FLASH_CMD_ADDR2 && val == FLASH_CMD_DATA2) {
				flash_command2 = 1;
			}
			flash_command1 = 0;
		}
		else if (addr == FLASH_CMD_ADDR1 && val == FLASH_CMD_DATA1) {
			flash_command1 = 1;
		}
		if (IO[0xC1] < 8) {
			// normal sram
            ROM[1][addr & 0xFFFF] = val;
		}
		else if (IO[0xC1] >= 8 && IO[0xC1] < 15) {
			// FLASH ROM use SRAM bank(port 0xC1:8-14)(0xC1:15 0xF0000-0xFFFFF are write protected)
			if (write_enable || flash_write_one) {
				ROM[IO[0xC1]][addr & 0xFFFF] = val;
				write_enable    = 0;
				flash_write_one = 0;
			}
			else if (flash_write_set) {
				switch (val) {
				case FLASH_CMD_WRITE:
					write_enable      = 1;
					flash_write_reset = 0;
					break;
				case FLASH_CMD_CONTINUE_RES1:
					flash_write_reset = 1;
					break;
				case FLASH_CMD_CONTINUE_RES2:
				case FLASH_CMD_CONTINUE_RES3:
					if (flash_write_reset) {
						flash_write_set   = 0;
						flash_write_reset = 0;
					}
					break;
				default:
					flash_write_reset = 0;
				}
			}
		}
    }
}

/*-------------------------
  IOポート書き込み
-------------------------*/
void wsWritePort(unsigned char port,unsigned char val)
{
    int i, j, k, n;

    switch(port)
    {
        case 0x04:
                    SprMap = (unsigned long*)(RAM + ((val & 0x3F) << 9));
                    break;
        case 0x07:
                    BgMap = (unsigned short*)(RAM + ((val & 0x0F) << 11));
                    FgMap = (unsigned short*)(RAM + ((val & 0xF0) << 7));
                    break;
        case 0x15:
                    if (val & 2) Cursor = 1;
					else if (val & 4) Cursor = 0;
                    break;
        case 0x1C:
        case 0x1D:
        case 0x1E:
        case 0x1F:
                    if (!(IO[0x60] & 0x80))
                    {
                        i = (port - 0x1C) << 1;
                        MonoColor[i] = DefColor[val & 0x0F];
                        MonoColor[i+1] = DefColor[(val & 0xF0)>>4];
                        for(k = 0x20; k < 0x40; k++)
                        {
                            i = (k & 0x1E) >> 1;
                            j = 0;
                            if(k & 0x01) j = 2;
                            ColorPalette[i][j] = MonoColor[IO[k] & 0x07];
                            ColorPalette[i][j + 1] = MonoColor[(IO[k]>>4) & 0x07];
                        }
                    }
                    break;
        case 0x20:
        case 0x21:
        case 0x22:
        case 0x23:
        case 0x24:
        case 0x25:
        case 0x26:
        case 0x27:
        case 0x28:
        case 0x29:
        case 0x2A:
        case 0x2B:
        case 0x2C:
        case 0x2D:
        case 0x2E:
        case 0x2F:
        case 0x30:
        case 0x31:
        case 0x32:
        case 0x33:
        case 0x34:
        case 0x35:
        case 0x36:
        case 0x37:
        case 0x38:
        case 0x39:
        case 0x3A:
        case 0x3B:
        case 0x3C:
        case 0x3D:
        case 0x3E:
        case 0x3F:
                    if (!(IO[0x60] & 0x80))
                    {
                        i = (port & 0x1E) >> 1;
                        j = 0;
                        if (port & 0x01) j = 2;
                        ColorPalette[i][j] = MonoColor[val & 0x07];
                        ColorPalette[i][j + 1] = MonoColor[(val >> 4) &0x07];
                    }
                    break;
        case 0x42:
                    val &= 0x0F;
                    break;
        case 0x43:
                    val = 0;
                    break;
        case 0x48:
                    if(val & 0x80)
                    {
                        i = *(unsigned long*)(IO + 0x40);
                        j = *(unsigned short*)(IO + 0x44);
                        n = *(unsigned short*)(IO + 0x46);
                        for(k=0; k < n; k++)
                        {
                            wsWriteMem(j, ROM[i >> 16][i & 0xFFFF]);
                            i++;
                            j++;
                        }
                        *(unsigned long*)(IO + 0x40) = i;
                        *(unsigned short*)(IO + 0x44) = j;
                        *(unsigned short*)(IO + 0x46) = 0;
                        val &= 0x7F;
                    }
                    break;
        case 0x60:
                    if (IO[0x60] != val)
                    {
                        memset(MonoModifiedTile, 1, 1024);
                        memset(ColorModifiedTile, 1, 1024);
                    }
                    break;
        case 0x80:
        case 0x81:
                    IO[port] = val;
                    ChFreq[0] = *(unsigned short*)(IO + 0x80);
                    return;
        case 0x82:
        case 0x83:
                    IO[port] = val;
                    ChFreq[1] = *(unsigned short*)(IO + 0x82);
                    return;
        case 0x84:
        case 0x85:
                    IO[port] = val;
                    ChFreq[2] = *(unsigned short*)(IO + 0x84);
                    return;
        case 0x86:
        case 0x87:
                    IO[port] = val;
                    ChFreq[3] = *(unsigned short*)(IO + 0x86);
                    return;
        case 0x88:
                    ChLVol[0] = (val >> 4) & 0x0F;
                    ChRVol[0] = val & 0x0F;
                    break;
        case 0x89:
                    ChLVol[1] = (val >> 4) & 0x0F;
                    ChRVol[1] = val & 0x0F;
                    break;
        case 0x8A:
                    ChLVol[2] = (val >> 4) & 0x0F;
                    ChRVol[2] = val & 0x0F;
                    break;
        case 0x8B:
                    ChLVol[3] = (val >> 4) & 0x0F;
                    ChRVol[3] = val & 0x0F;
                    break;
        case 0x8C:
                    Swp.step = (signed char)val;
                    break;
        case 0x8D:
                    Swp.time = (val + 1) << 5;
                    break;
        case 0x8E:
                    Noise.pattern = val & 0x07;
                    break;
        case 0x8F:
                    WaveMap = val << 6;
                    for (i = 0; i < 64; i++)
                    {
                        apuSetPData(WaveMap + i, RAM[WaveMap + i]);
                    }
                    break;
        case 0x90:
                    ChPlay[0] = val & 0x01;
                    ChPlay[1] = val & 0x02;
                    ChPlay[2] = val & 0x04;
                    ChPlay[3] = val & 0x08;
                    VoiceOn   = val & 0x20;
                    Swp.on    = val & 0x40;
                    Noise.on  = val & 0x80;
                    break;
        case 0xA0:
                    val = 0x02;
                    break;
        case 0xA2:
                    if (val & 0x01)
                    {
                        hblankTimer = hblankTimerPreset;
                    }
                    else
                    {
                        hblankTimer = 0;
                    }
                    if (val & 0x04)
                    {
                        vblankTimer = vblankTimerPreset;
                    }
                    else
                    {
                        vblankTimer = 0;
                    }
                    break;
        case 0xA4:
        case 0xA5:
                    IO[port] = val;
                    hblankTimer = hblankTimerPreset = *(unsigned short*)(IO + 0xA4);
                    return;
        case 0xA6:
        case 0xA7:
                    IO[port] = val;
                    vblankTimer = vblankTimerPreset = *(unsigned short*)(IO + 0xA6);
                    return;
        case 0xB5:
                    IO[0xB5] = val & 0xF0;
                    // 0x40 : Buttons
                    if (IO[0xB5] & 0x40) IO[0xB5] |= ((Pad.Buttons >> 11) & 0x0C) | ((Pad.Buttons >> 2) & 2);
					// 0x20 : X cursor
					// 0x10 : Y cursor
                    if (Cursor) {
                        if (IO[0xB5] & 0x20) IO[0xB5] |= ((Pad.Lx>0xC0)<<1) | (Pad.Lx<0x40)<<3 | (Pad.Ly>0xC0)<<2 | (Pad.Ly<0x40);
                        if (IO[0xB5] & 0x10) IO[0xB5] |= (Pad.Buttons >> 4) & 0x0F;
                    }
                    else {
                        if (IO[0xB5] & 0x20) IO[0xB5] |= (Pad.Buttons >> 4) & 0x0F; 
                        if (IO[0xB5] & 0x10) IO[0xB5] |= ((Pad.Lx>0xC0)<<1) | (Pad.Lx<0x40)<<3 | (Pad.Ly>0xC0)<<2 | (Pad.Ly<0x40);
                    }
                    return;
        case 0xB6:
                    IO[0xB6] &= ~val;
                    return;
        case 0xBE:
                    wsComEep(&IProm, (unsigned short*)(IO+0xBC), (unsigned short*)(IO+0xBA));
                    val >>= 4;
                    break;
        case 0xC0:
                    if(nec_get_reg(NEC_CS) >= 0x4000) // prefetch 処理（コナン2）
                    {
                        nec_execute(2);
                    }
                    j = (val << 4) & 0xF0;
                    ROM[0x04] = ROMMap[0x04 | j];
                    ROM[0x05] = ROMMap[0x05 | j];
                    ROM[0x06] = ROMMap[0x06 | j];
                    ROM[0x07] = ROMMap[0x07 | j];
                    ROM[0x08] = ROMMap[0x08 | j];
                    ROM[0x09] = ROMMap[0x09 | j];
                    ROM[0x0A] = ROMMap[0x0A | j];
                    ROM[0x0B] = ROMMap[0x0B | j];
                    ROM[0x0C] = ROMMap[0x0C | j];
                    ROM[0x0D] = ROMMap[0x0D | j];
                    ROM[0x0E] = ROMMap[0x0E | j];
                    ROM[0x0F] = ROMMap[0x0F | j];
                    break;
        case 0xC1:
                    if (!SramSize)
                    {
                        break;
                    }
                    val &= 0x0F;
                    if (val <= (SramSize >> 16))
                    {
                        ROM[0x01] = StaticRAM + val * 0x10000;
                    }
                    else
                    {
                        ROM[0x01] = SRAM;
                    }
                    break;
        case 0xC2:
                    ROM[0x02] = ROMMap[val];
                    break;
        case 0xC3: // GUNPEYは0を書き込むときがある
                    if (val == 0)
                    {
                        break;
                    }
                    ROM[0x03] = ROMMap[val];
                    break;
        case 0xC8:
                    wsComEep(&CProm, (unsigned short*)(IO+0xC6), (unsigned short*)(IO+0xC4));
                    if(val & 0x10)
                    {
                        val >>= 4;
                    }
                    if(val& 0x20)
                    {
                        val >>= 4;
                    }
                    if(val & 0x40)
                    {
                        val >>= 5;
                    }
                    break;
        case 0xCA:
                    if (val == 0x15)
                    {
                        RtcCount = 0;
                    }
                    break;
        default:
                    break;
    }
    IO[port] = val;
//    DEBUGVALUE2("ioWrite", port, val);
}

/*-------------------------
  IOポート読み込み
-------------------------*/
unsigned char wsReadPort(unsigned char port)
{
    if (port == 0xCA)
    {
        return IO[0xCA] | 0x80;
    }
    else if (port == 0xCB)
    {
        if (IO[0xCA] == 0x15)    // get time command
        {
            unsigned char year, mon, mday, wday, hour, min, sec, j;
            struct tm *newtime;
            time_t long_time;
            sceKernelLibcTime(&long_time);
            long_time += 9 * 60 * 60; // GMT + 9
            newtime = localtime(&long_time);

            #define  BCD(value) ((value/10)<<4)|(value%10)
            switch(RtcCount)
            {
            case 0:
                RtcCount++;
                year = newtime->tm_year;
                year %= 100;
                return BCD(year);
            case 1:
                RtcCount++;
                mon = newtime->tm_mon;
                mon++;
                return BCD(mon);
            case 2:
                RtcCount++;
                mday = newtime->tm_mday;
                return BCD(mday);
            case 3:
                RtcCount++;
                wday = newtime->tm_wday;
                return BCD(wday);
            case 4:
                RtcCount++;
                hour = newtime->tm_hour;
                j = BCD(hour);
                if(hour>11)
                    j |= 0x80;
                return j;
            case 5:
                RtcCount++;
                min = newtime->tm_min;
                return BCD(min);
            case 6:
                RtcCount=0;
                sec = newtime->tm_sec;
                return BCD(sec);
            }
            return 0;
        }
        else
        {
            // set ack
            return (IO[0xCB] | 0x80);
        }
    }
    return IO[port];
}

/*-------------------------
  EEPROM通信処理
-------------------------*/
void wsComEep(EEPROM* eeprom, unsigned short* cmd, unsigned short* data)
{
    int i, j, op, addr;
    const int tblmask[16][5]=
    {
        {0x0000, 0, 0x0000, 0, 0x0000}, // dummy
        {0x0000, 0, 0x0000, 0, 0x0000},
        {0x0000, 0, 0x0000, 0, 0x0000},
        {0x0000, 0, 0x0000, 0, 0x0000},
        {0x000C, 2, 0x0003, 0, 0x0003},
        {0x0018, 3, 0x0006, 1, 0x0007},
        {0x0030, 4, 0x000C, 2, 0x000F},
        {0x0060, 5, 0x0018, 3, 0x001F},
        {0x00C0, 6, 0x0030, 4, 0x003F}, // 1Kbits
        {0x0180, 7, 0x0060, 5, 0x007F},
        {0x0300, 8, 0x00C0, 6, 0x00FF},
        {0x0600, 9, 0x0180, 7, 0x01FF},
        {0x0C00, 10, 0x0300, 8, 0x03FF}, // 16Kbits
        {0x1800, 11, 0x0600, 9, 0x07FF},
        {0x3000, 12, 0x0C00, 10, 0x0FFF},
        {0x6000, 13, 0x1800, 11, 0x1FFF},
    };
    if(eeprom->data == NULL)
    {
        return;
    }
    for(i = 15, j = 0x8000; i >= 0; i--, j >>= 1)
    {
        if(*cmd & j)
        {
            break;
        }
    }
    op = (*cmd & tblmask[i][0]) >> tblmask[i][1];
    switch(op)
    {
        case 0:
            addr = (*cmd & tblmask[i][2]) >> tblmask[i][3];
            switch(addr)
            {
                case 0:
                    eeprom->we = 0;
                    break;
                case 1:
                    for(j = tblmask[i][4]; j >= 0; j--)
                    {
                        eeprom->data[j] = *data;
                    }
                    break;
                case 2:
                    if(eeprom->we)
                    {
                        memset(eeprom->data, 0xFF, sizeof(eeprom->data)*2);
                    }
                    break;
                case 3:
                    eeprom->we = 1;
                    break;
            }
            *data = 0;
            break;
        case 1:
            if(eeprom->we)
            {
                addr = *cmd & tblmask[i][4];
                eeprom->data[addr] = *data;
            }
            *data = 0;
            break;
        case 2:
            addr = *cmd & tblmask[i][4];
            *data = eeprom->data[addr];
            break;
        case 3:
            if(eeprom->we)
            {
                addr = *cmd & tblmask[i][4];
                eeprom->data[addr] = 0xFFFF;
            }
            *data = 0;
            break;
        default: break;
    }
}

/*-------------------------
  サウンドに同期
  実機では12KHzで75.5fps
  PSPでは44.1KHzなので4分して11.025KHzで再生
  フレームレートも 75.5 * 11.025 / 12= 69.4が標準速度
-------------------------*/
int wsFrameSkipSound(void)
{
    static int lastSkip = 0;

    while(apuBufLen()>(512*6)) {
        sceKernelDelayThread(1000);
    }
    if(apuBufLen() <= 512) {
        if(lastSkip > 3) {
            lastSkip = 0;
            return 1;
        }
        lastSkip++;
        return 0;
    }
    lastSkip = 0;
    return 1;
}

/*-------------------------
  1フレーム(159 scanlines)実行
-------------------------*/
int wsExecuteFrame(int render)
{
    int i;
    int frameFin = 0;
    unsigned long *sprRamBase, *sprBuffer;
    int irqLoop, irqAck;
    static int wsCycles = WS_CYCLES;
    static unsigned long vblankCount = 0;

    while (!frameFin) {
        if(IO[0x02] == 140) {
            sprRamBase = SprMap + IO[0x05];
            SprCount = IO[0x06];
            if (SprCount > 0x80) {
                SprCount = 0x80;
            }
            sprBuffer = (unsigned long*)SprTable;
            for (i = 0; i < SprCount; i++) {
                *sprBuffer++ = *sprRamBase++;
            }
        }
        if(render && (IO[0x02] < 144)) {
            if (IO[0x60] & 0x40)
            {
                gpuRenderScanLineColor();
            }
            else
            {
                gpuRenderScanLineMono();
            }
        }
        // HBlank Timer Interrupt
        if (hblankTimer == 1) {
            IO[0xb6] |= 0x80;
        }
        // Update HBlank count
        (*(unsigned short*)(IO+0xA8))++;
        // HBLANK count down
        if (hblankTimer && (IO[0xa2] & 0x01)) {
            hblankTimer--;
            if (!hblankTimer && (IO[0xa2] & 0x02))
                hblankTimer = hblankTimerPreset;
            *(unsigned short*)(IO+0xA4) = hblankTimer;
        }
        if (IO[0x02] == 144) {
            frameFin = 1;
            // Update VBlank count
            vblankCount++;
            // 0xAAは4バイト境界に合わない
            //*(unsigned long*)(IO+0xAA) = vblankCount;
            *(unsigned short*)(IO+0xAA) = vblankCount & 0xFFFF;
            *(unsigned short*)(IO+0xAC) = vblankCount >> 16;
            // VBlank Begin Interrupt
            IO[0xb6] |= 0x40;
            // VBlank Timer Interrupt
            if (vblankTimer == 1)
                IO[0xb6] |= 0x20;
            // VBLANK count down
            if (vblankTimer && (IO[0xa2] & 0x04)) {
                vblankTimer--;
                if (!vblankTimer && (IO[0xa2] & 0x08))
                    vblankTimer = vblankTimerPreset;
                *(unsigned short*)(IO+0xA6) = vblankTimer;
            }
        }
        // Scanline Match Interrupt
        if ((IO[0x02] == IO[0x03])) {
            IO[0xb6] |= 0x10;
        }
        for(irqLoop = 0; irqLoop < 8; irqLoop++) {
            int execCycles = nec_execute(wsCycles);
            wsCycles += WS_CYCLES - execCycles;
            if(IO[0xb6]) {
                irqAck = 0x80;
                for(i = 7; i >= 4; i--) {
                    if ((IO[0xb6] & irqAck) && (IO[0xb2] & irqAck)) {
                        nec_int((IO[0xb0] + i) << 2);
                        break;
                    }
                    irqAck >>= 1;
                }
            }
        }
        if(++IO[0x02] > 158) {
            IO[0x02] = 0;
        }
        apuWaveSet();
    }
    return frameFin;
}

/*-------------------------
  エミュ実行ループ
-------------------------*/
int wsExecute(void)
{
    int i;
    int render;

    for (i = 4; i--;)
    {
        render = wsFrameSkipSound();
        //render = 1;
        wsExecuteFrame(render);
        WsFrame++;
        if (render)
        {
            mh_print_num(0, 0, Fps, RGB555(255,255,255));
            mh_print_num(0, 10, Fps - Drop, RGB555(255,255,255));
			video_copy_rect(tex_frame, draw_frame, &fps_rect, &fps_rect);
            gpuSetSegment();
			if (ScreenSize)
			{
				video_copy_rect(work_frame, draw_frame, &src_rect, &large_rect);
			}
			else
			{
				video_copy_rect(work_frame, draw_frame, &src_rect, &dst_rect);
			}
			video_flip_screen(Vsync);
        }
        else
        {
            WsSkip++;
        }
    }
    return 0;
}
