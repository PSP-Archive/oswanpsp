/*
* $Id$
*/

#include <pspkernel.h>
#include <pspctrl.h>
#include <psppower.h>
#include <stdio.h>
#include "fileio.h"
#include "video.h"
#include "menu.h"
#include "ws.h"
#include "gpu.h"
#include "apu.h"
#include "mon.h"
#include "./debug/debug.h"

PSP_MODULE_INFO("Oswan for PSP", PSP_MODULE_USER, 1, 1);
PSP_MAIN_THREAD_ATTR(PSP_THREAD_ATTR_USER);
PSP_HEAP_SIZE_MAX();

int PspFrame = 0;
extern SceCtrlData Pad;

// Exit callback
int ExitCallback(int Arg1, int Arg2, void *Common)
{
    Run = 0;
    return 0;
}

// Callback thread
int CallbackThread(SceSize Args, void *Argp)
{
    int CallbackId;

    CallbackId = sceKernelCreateCallback("Exit Callback", ExitCallback, NULL);
    sceKernelRegisterExitCallback(CallbackId);
    sceKernelSleepThreadCB();
    return 0;
}

// Sets up the callback thread and returns its thread id
int SetupCallbacks(void)
{
    int ThreadId = 0;

    ThreadId = sceKernelCreateThread("update_thread", CallbackThread, 0x11, 0xFA0, 0, 0);
    if (ThreadId >= 0)
    {
        sceKernelStartThread(ThreadId, 0, 0);
    }
    return ThreadId;
}

void VblankInt(void)
{
    if (++PspFrame >= 60)
    {
        Fps = WsFrame;
        Drop = WsSkip;
        WsFrame = PspFrame = WsSkip = 0;
    }
}

int main(int argc, char *argv[])
{
    int ret = 0;

    fileioSetModulePath(argc, argv);
    SetupCallbacks();
    sceKernelRegisterSubIntrHandler(PSP_VBLANK_INT, 1, VblankInt, NULL);
    sceKernelEnableSubIntr(PSP_VBLANK_INT, 1);
    sceCtrlSetSamplingCycle(0);
    sceCtrlSetSamplingMode(PSP_CTRL_MODE_ANALOG);
    video_init();
    apuInit();
    while (Run)
    {
		video_clear_screen();
        ret = menuList();
		video_clear_screen();
        switch (ret)
        {
        case 0: // Exit
            Run = 0;
            break;
        case 1: // ROM selected
            wsInit();
            break;
        case 2: // Personal data
            if (Cart)
            {
                wsExit();
            }
            wsPdata();
            break;
        }
        if (Cart)
        {
            while (Run)
            {
                wsExecute();
                sceCtrlReadBufferPositive(&Pad, 1);
				// L + SELECT : メニュー画面へ
                if ((Pad.Buttons & PSP_CTRL_LTRIGGER) && (Pad.Buttons & PSP_CTRL_SELECT))
                {
                    break;
                }
				// L + R : XキーとYキーの割り当てを変更(X=矢印,Y=アナログ)<=>(X=アナログ,Y=矢印)
                else if ((Pad.Buttons & PSP_CTRL_LTRIGGER) && (Pad.Buttons & PSP_CTRL_RTRIGGER))
                {
                    Cursor = (Cursor) ? 0 : 1;
                }
				// L + □ : メモリ表示（デバッグ用）
                else if ((Pad.Buttons & PSP_CTRL_LTRIGGER) && (Pad.Buttons & PSP_CTRL_SQUARE))
                {
                    monMenu();
                }
				// L + ○ : Pause
				else if ((Pad.Buttons & PSP_CTRL_LTRIGGER) && (Pad.Buttons & PSP_CTRL_CIRCLE))
                {
					// L+○ボタンが押されている間ループ
					while ((Pad.Buttons & PSP_CTRL_LTRIGGER) && (Pad.Buttons & PSP_CTRL_CIRCLE))
					{
						sceCtrlReadBufferPositive(&Pad, 1);
					}
					// L+○ボタンが押されるまでループ
					while (!((Pad.Buttons & PSP_CTRL_LTRIGGER) && (Pad.Buttons & PSP_CTRL_CIRCLE)))
					{
						sceCtrlReadBufferPositive(&Pad, 1);
					}
					// L+○ボタンが押されている間ループ
					while ((Pad.Buttons & PSP_CTRL_LTRIGGER) && (Pad.Buttons & PSP_CTRL_CIRCLE))
					{
						sceCtrlReadBufferPositive(&Pad, 1);
					}
                }
            }
        }
    }
	scePowerSetClockFrequency(222, 222, 111);
    if (Cart)
    {
        wsExit();
    }
    apuEnd();
	video_exit();
    sceKernelDisableSubIntr(PSP_VBLANK_INT, 1);
    sceKernelReleaseSubIntrHandler(PSP_VBLANK_INT, 1);
    sceKernelExitGame();
    return 0;
}
