/*
* $Id$
*/

#ifndef APU_H_
#define APU_H_

typedef struct sweep {
    int on;
    int time;
    int step;
    int cnt;
} SWEEP;
typedef struct noise {
    int on;
    int pattern;
} NOISE;

extern int PspAudioThread;
extern unsigned long WaveMap;
extern int ChPlay[];
extern int ChFreq[];
extern int ChLVol[];
extern int ChRVol[];
extern int VoiceOn;
extern SWEEP Swp;
extern NOISE Noise;
extern unsigned char PData[4][32];

int apuInit(void);
void apuEnd(void);
int apuAudioThread(int args, void *argp);
void apuSndSleep(void);
void apuSndWakeup(void);
short* apuBufGetLock(int size);
void apuBufGetUnlock(void* ptr,int size);
int apuBufLen(void);
void apuSoundProc16(short* waveL,short* waveR,int nSamples);
unsigned int apuMrand(unsigned int Degree);
void apuSetPData(int addr, unsigned char val);
unsigned char apuVoice(void);
void apuSweep(void);
void apuNoise(void);
void apuWaveOut(short* buf);
void apuWaveSet(void);
void apuVoiceSet(void);

#endif
