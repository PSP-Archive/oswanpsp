/*
* $Id$

サウンドスレッドはe[mulator]のソースを利用しています
*/
#include <pspkernel.h>
#include <pspaudio.h>
//#include <pspaudio_kernel.h>
#include "apu.h"
#include "ws.h"
#include "./debug/debug.h"

// 64の倍数で
#define SND_BNKSIZE 512
#define SND_OVERLIMIT (10*SND_BNKSIZE)
#define SND_RNGSIZE (40*SND_BNKSIZE)
#define WAV_BUFFER 512
#define BUFSIZEN 0x10000
#define SND_L 0
#define SND_R 1
#define DIVIDE_FREQ 11025

static int SndWr = 0;
static int SndRd = 0;
static short sndbuffer[SND_RNGSIZE][2]; // Sound Ring Buffer
static int SndSleep = 0;
int PspAudioCh = -1;
int PspAudioThread = -1;
int WsWaveVol = 80;
unsigned long WaveMap;
int ChPlay[] = {0, 0, 0, 0};
int ChFreq[] = {0, 0, 0, 0};
int ChLVol[] = {0, 0, 0, 0};
int ChRVol[] = {0, 0, 0, 0};
int VoiceOn = 0;
SWEEP Swp;
NOISE Noise;
short WaveDataL[WAV_BUFFER + 4];
short WaveDataR[WAV_BUFFER + 4];
unsigned char PData[4][32];
unsigned char PDataN[8][BUFSIZEN];

int apuInit(void)
{
	int i, j;
	SndWr = SndRd = 0;

	for (i = 0; i < 4; i++) {
		for (j = 0; j < 32; j++) {
			PData[i][j] = 8;
		}
	}
	for (j = 0; j < WAV_BUFFER; j++) {
			WaveDataL[j] = 0x80;
			WaveDataR[j] = 0x80;
	}
	for (i = 0; i < 8; i++) {
		for (j = 0; j < BUFSIZEN; j++) {
			PDataN[i][j] = ((apuMrand(15 - i) & 1) ? 15 : 0);
		}
	}
//	sceAudioSetFrequency(PSP_AUDIO_FREQ_48K);
	PspAudioCh = sceAudioChReserve(PSP_AUDIO_NEXT_CHANNEL, SND_BNKSIZE, PSP_AUDIO_FORMAT_STEREO);
	if (PspAudioCh < 0) {
		return 1;
	}
	PspAudioThread = sceKernelCreateThread("audiothread", (SceKernelThreadEntry)&apuAudioThread, 0x12, 0x1000, 0, NULL);
	if(PspAudioThread < 0) {
		return 2;
	}
	sceKernelStartThread(PspAudioThread, 0, NULL);
	return 0;
}

void apuEnd(void)
{
	if (PspAudioCh >= 0)
	{
		sceAudioChRelease(PspAudioCh);
		PspAudioCh = -1;
	}
	if (PspAudioThread >= 0)
	{
		sceKernelDeleteThread (PspAudioThread);
		PspAudioThread = -1;
	}
//	sceAudioSetFrequency(PSP_AUDIO_FREQ_44K);
}

int apuAudioThread(int args, void *argp)
{
	short* src = NULL;
	int buflen;

	while(1)
	{
		if ((buflen = apuBufLen()) < SND_BNKSIZE)
		{
			apuSndSleep();
			continue;
		}
		src = apuBufGetLock(SND_BNKSIZE);
		if (src)
		{
			sceAudioOutputPannedBlocking(PspAudioCh, PSP_AUDIO_VOLUME_MAX, PSP_AUDIO_VOLUME_MAX, src);
			apuBufGetUnlock(src, SND_BNKSIZE);
			src = NULL;
		}
	}
	return 0;
}

void apuSndSleep(void)
{
	SndSleep = 1;
	sceKernelSleepThread();
	SndSleep = 0;
}

void apuSndWakeup(void)
{
	if (SndSleep)
	{
		sceKernelWakeupThread(PspAudioThread);
	}
}

short* apuBufGetLock(int size)
{
	if (apuBufLen() >= size) {
		return sndbuffer[SndRd];
	}
	return 0;
}

void apuBufGetUnlock(void* ptr,int size)
{
	if (ptr == (void*)&sndbuffer[SndRd]) {
		SndRd = (SndRd + size) % SND_RNGSIZE;
	}
}

int apuBufLen(void)
{
	int rd = SndRd;
	int wr = SndWr;
	if (wr >= rd) return wr - rd;;
	return SND_RNGSIZE - rd + wr;
}

void apuSoundProc16(short* waveL, short* waveR, int nSamples)
{
	int i;
	int swr = SndWr;

	for (i = 0; i < nSamples; i++) {
		sndbuffer[swr][SND_L] = waveL[i];
		sndbuffer[swr][SND_R] = waveR[i];
		swr = (swr + 1) % SND_RNGSIZE;
	}
	if (apuBufLen() < SND_OVERLIMIT) {
		SndWr = swr;
	}
	apuSndWakeup();
}

unsigned int apuMrand(unsigned int Degree)
{
#define BIT(n) (1<<n)
	typedef struct {
		unsigned int N;
		int InputBit;
		int Mask;
	} POLYNOMIAL;

	static POLYNOMIAL TblMask[]=
	{
		{ 2,BIT(2) ,BIT(0)|BIT(1)},
		{ 3,BIT(3) ,BIT(0)|BIT(1)},
		{ 4,BIT(4) ,BIT(0)|BIT(1)},
		{ 5,BIT(5) ,BIT(0)|BIT(2)},
		{ 6,BIT(6) ,BIT(0)|BIT(1)},
		{ 7,BIT(7) ,BIT(0)|BIT(1)},
		{ 8,BIT(8) ,BIT(0)|BIT(2)|BIT(3)|BIT(4)},
		{ 9,BIT(9) ,BIT(0)|BIT(4)},
		{10,BIT(10),BIT(0)|BIT(3)},
		{11,BIT(11),BIT(0)|BIT(2)},
		{12,BIT(12),BIT(0)|BIT(1)|BIT(4)|BIT(6)},
		{13,BIT(13),BIT(0)|BIT(1)|BIT(3)|BIT(4)},
		{14,BIT(14),BIT(0)|BIT(1)|BIT(4)|BIT(5)},
		{15,BIT(15),BIT(0)|BIT(1)},
		{ 0,	  0,	  0},
	};

	static POLYNOMIAL *pTbl = TblMask;
	static int ShiftReg = BIT(2)-1;
	int XorReg = 0;
	int Masked;

	if(pTbl->N != Degree)
	{
		pTbl = TblMask;
		while(pTbl->N)
		{
			if(pTbl->N == Degree)
			{
				break;
			}
			pTbl++;
		}
		if(!pTbl->N)
		{
			pTbl--;
		}

		ShiftReg &= pTbl->InputBit-1;
		if(!ShiftReg)
		{
			ShiftReg = pTbl->InputBit-1;
		}
	}

	Masked = ShiftReg & pTbl->Mask;
	while(Masked)
	{
		XorReg ^= Masked & 0x01;
		Masked >>= 1;
	}

	if(XorReg)
	{
		ShiftReg |= pTbl->InputBit;
	}
	else
	{
		ShiftReg &= ~pTbl->InputBit;
	}
	ShiftReg >>= 1;

	return ShiftReg;
}

void apuSetPData(int addr, unsigned char val)
{
	int i, j;

	i = (addr & 0x30) >> 4;
	j = (addr & 0x0F) << 1;
	PData[i][j]=(unsigned char)(val & 0x0F);
	PData[i][j + 1]=(unsigned char)((val & 0xF0)>>4);
}

unsigned char apuVoice(void)
{
	int i, j, k;

	if ((IO[0x52] & 0x88) == 0x80) { // DMA start
		j = *(unsigned short*)(IO + 0x4A) | (IO[0x4C] << 16); // start address
		i = *(unsigned short*)(IO + 0x4E); // size
		k = (((IO[0x52] & 0x03) == 0x03) ? 2 : 1);
		IO[0x89] = ROM[j >> 16][j & 0xFFFF];
		i -= k;
		j += k;
    //    i--;
    //    j++;
		if (i <= 0) {
			i = 0;
			IO[0x52] &= 0x7F; // DMA end
		}
		*(unsigned short*)(IO + 0x4A) = j;
		*(unsigned short*)(IO + 0x4C) = j >> 16;
		*(unsigned short*)(IO + 0x4E) = i;
	}
	return (VoiceOn ? IO[0x89] : 0x80);
}

void apuSweep(void)
{
	if ((Swp.step) && Swp.on) { // sweep on
		if (Swp.cnt < 0) {
			Swp.cnt = Swp.time;
			ChFreq[2] += Swp.step;
			ChFreq[2] &= 0x7ff;
		}
		Swp.cnt--;
	}
}

void apuWaveSet(void)
{
	static int point[] = {0, 0, 0, 0};
	static int preindex[] = {0, 0, 0, 0};
	int    channel, index;
	short  value, lVol[4], rVol[4];
	short  LL, RR, vVol;
	static int wPos = 0;
//ChPlay[0] = 1;
//ChPlay[1] = 0;
//ChPlay[2] = 0;
//ChPlay[3] = 0;
	apuSweep();
	for (channel = 0; channel < 4; channel++) {
        lVol[channel] = 0;
        rVol[channel] = 0;
		if (ChPlay[channel]) {
		    if (channel == 1 && VoiceOn) {
		        continue;
		    }
			else if (channel == 3 && Noise.on){
				index = (3072000 / DIVIDE_FREQ) * point[3] / (2048 - ChFreq[3]);
				if ((index %= BUFSIZEN) == 0 && preindex[3]) {
					point[3] = 0;
				}
				value = (short)PDataN[Noise.pattern][index] - 8;
			}
			else {
				index = (3072000 / DIVIDE_FREQ) * point[channel] / (2048 - ChFreq[channel]);
				if ((index %= 32) == 0 && preindex[channel]) {
					point[channel] = 0;
				}
				value = (short)PData[channel][index] - 8;
			}
			preindex[channel] = index;
			point[channel]++;
			lVol[channel] = value * ChLVol[channel]; // -8*15=-120, 7*15=105
			rVol[channel] = value * ChRVol[channel];
		}
	}
	vVol = ((short)apuVoice() - 0x80);
	// mix 16bits wave -32768 ～ +32767 32768/120 = 273
	LL = (lVol[0] + lVol[1] + lVol[2] + lVol[3] + vVol) * WsWaveVol;
	RR = (rVol[0] + rVol[1] + rVol[2] + rVol[3] + vVol) * WsWaveVol;

	WaveDataL[wPos] = LL;
	WaveDataR[wPos++] = RR;
	WaveDataL[wPos] = LL;
	WaveDataR[wPos++] = RR;
	WaveDataL[wPos] = LL;
	WaveDataR[wPos++] = RR;
	WaveDataL[wPos] = LL;
	WaveDataR[wPos++] = RR;
	if (wPos >= WAV_BUFFER) {
		apuSoundProc16(WaveDataL, WaveDataR, WAV_BUFFER);
		wPos = 0;
	}
}
