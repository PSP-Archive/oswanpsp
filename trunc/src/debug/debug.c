#include <pspkernel.h>
#include "debug.h"
#include "../fileio.h"

// ‚¢‚Âƒnƒ“ƒO‚·‚é‚©‚í‚©‚ç‚È‚¢‚Ì‚Å–ˆ‰ño—Í
void OutputDebugString(void *buf, int nLen)
{
	int hFile = -1;
	char szPath[MAX_PATH];

	fileioGetModulePath(szPath, sizeof(szPath));
	strcat(szPath, "Debug.log");
	hFile = sceIoOpen(szPath, PSP_O_CREAT | PSP_O_APPEND | PSP_O_RDWR, 0777);
	if (hFile >= 0) {
		sceIoWrite(hFile, buf, nLen);
		sceIoClose(hFile);
	}
}
