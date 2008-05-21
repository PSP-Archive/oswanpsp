/*
* $Id$
*/

#include <pspkernel.h>
#include <pspctrl.h>
#include <stdio.h>
#include <string.h>
#include "fileio.h"
#include "video.h"
#include "ws.h"
#include "menu.h"
#include "debug/debug.h"

#define MAX_LINE 28
#define MENU_SIZE 7

FILELIST FileList[256];
int FileCount;
const char *Title = "Oswan for PSP";
const char *Menu[MENU_SIZE] = {
    "ROM 選択",
    "パーソ\ナルデータ",
    "ステートセーブ",
    "ステートロード",
    NULL,
    NULL,
    "終了"
};
const char *Screen[2] = {
    "画像サイズ 小",
    "画像サイズ 大",
};
const char *VsyncMenu[2] = {
    "Vsync OFF",
    "Vsync ON",
};

extern unsigned short *draw_frame;
extern unsigned short *tex_frame;
extern RECT full_rect;

int menuList(void)
{
    int selectItem = 0;
    int ret;
    SceCtrlData pad;
    unsigned int newButton, oldButton = 0;

    Menu[4] = Screen[ScreenSize];
    Menu[5] = VsyncMenu[Vsync];
    menuPrintList(selectItem);
    while (Run)
    {
        sceCtrlReadBufferPositive(&pad, 1);
        newButton = pad.Buttons;
        if ((newButton & PSP_CTRL_CIRCLE) && !(oldButton & PSP_CTRL_CIRCLE))
        {
            video_clear_frame(tex_frame);
            switch (selectItem)
            {
            case 0:
                ret = menuRomList(newButton);
                if (ret == 1)
                {
                    return 1;
                }
                break;
            case 1:
                return 2;
                break;
            case 2:
                menuStateList(newButton, 1);
                break;
            case 3:
                menuStateList(newButton, 0);
                break;
            case 4:
                menuConfig(newButton, 0);
                break;
            case 5:
                menuConfig(newButton, 1);
                break;
            case 6:
                return 0;
            }
            menuPrintList(selectItem);
        }
        if ((newButton & PSP_CTRL_DOWN) && !(oldButton & PSP_CTRL_DOWN))
        {
            selectItem++;
            if (selectItem >= MENU_SIZE) selectItem = 0;
            menuPrintList(selectItem);
        }
        if ((newButton & PSP_CTRL_UP) && !(oldButton & PSP_CTRL_UP))
        {
            selectItem--;
            if (selectItem < 0) selectItem = MENU_SIZE - 1;
            menuPrintList(selectItem);
        }
        if ((newButton & PSP_CTRL_START) && !(oldButton & PSP_CTRL_START))
        {
            while ((pad.Buttons & PSP_CTRL_START))
            {
                sceCtrlReadBufferPositive(&pad, 1);
            }
            return 3;
        }
        oldButton = newButton;
        video_copy_rect(tex_frame, draw_frame, &full_rect, &full_rect);
		video_flip_screen(1);
    }
    return 0;
}

void menuPrintList(int selectItem)
{
    int i;
    video_clear_frame(tex_frame);
    mh_print(0, 0, Title,RGB(255, 255, 255));
    for (i = 0; i < MENU_SIZE; i++)
    {
        if (i == selectItem)
        {
            mh_print(2, FONT_HEIGHT * (i + 2), "*",RGB(255, 0, 0));
            mh_print(10, FONT_HEIGHT * (i + 2), Menu[i],RGB(255, 0, 0));
        }
        else
        {
            mh_print(10, FONT_HEIGHT * (i + 2), Menu[i],RGB(255, 255, 255));
        }
    }
}

void menuBsortList(void)
{
    int i, j;
    FILELIST tmp;

    for (i = 0; i < FileCount - 1; i++)
    {
        for (j = 0; j < FileCount - i - 1; j++)
        {
            if (FileList[j].type < FileList[j+1].type)
            {
                continue;
            }
            else if (FileList[j].type > FileList[j+1].type)
            {
                tmp = FileList[j];
                FileList[j] = FileList[j+1];
                FileList[j+1] = tmp;
            }
            else if (strcmp(FileList[j].name, FileList[j+1].name) > 0)
            {
                tmp = FileList[j];
                FileList[j] = FileList[j+1];
                FileList[j+1] = tmp;
            }
        }
    }
}

int menuRomList(unsigned int oldButton)
{
    int isSelected = 0;
    int startPos = 0;
    int selectFile = 0;
    SceCtrlData pad;
    unsigned int newButton;
    char* index;

    FileCount = menuGetFileList();
    if (FileCount < 0)
    {
        return 0;
    }
    while(Run && !isSelected)
    {
        menuPrintRomList(startPos, selectFile);
        sceCtrlReadBufferPositive(&pad, 1);
        newButton = pad.Buttons;
        if ((newButton & PSP_CTRL_CROSS) && !(oldButton & PSP_CTRL_CROSS))
        {
            return 0;
        }
        if ((newButton & PSP_CTRL_CIRCLE) && !(oldButton & PSP_CTRL_CIRCLE))
        {
            if(FileList[startPos + selectFile].type == 0) {
                if(strcmp("..", FileList[startPos + selectFile].name) == 0) {
                    index = strrchr(CurDir, '/');
                    if (index != NULL) *index = '\0';
                    index = strrchr(CurDir, '/');
                    if (index != NULL) *(index+1) = '\0';
                }
                else
                {
                    strcat(CurDir, FileList[startPos + selectFile].name);
                    strcat(CurDir, "/");
                }
                startPos = selectFile = 0;
                FileCount = menuGetFileList();
                if (FileCount < 0)
                {
                    return 0;
                }
            }
            else if(FileList[startPos + selectFile].type == 1)
            {
                if (Cart)
                {
                    wsExit();
                }
                video_clear_frame(tex_frame);
                mh_print(50, 50, "Loading...", RGB(255, 255, 255));
                mh_print(70, 60, FileList[startPos + selectFile].name, RGB(255, 255, 255));
                video_copy_rect(tex_frame, draw_frame, &full_rect, &full_rect);
				video_flip_screen(1);
                strcpy(RomPath, CurDir);
                strcat(RomPath, FileList[startPos + selectFile].name);
                return 1;
            }
        }
        if ((newButton & PSP_CTRL_DOWN) && !(oldButton & PSP_CTRL_DOWN))
        {
            selectFile++;
        }
        if ((newButton & PSP_CTRL_UP) && !(oldButton & PSP_CTRL_UP))
        {
            selectFile--;
        }
        if ((newButton & PSP_CTRL_RIGHT) && !(oldButton & PSP_CTRL_RIGHT))
        {
            selectFile += 5;
            if (startPos + selectFile >= FileCount)
            {
                selectFile = FileCount - startPos;
            }
            if (selectFile > MAX_LINE - 1)
            {
                startPos += selectFile - MAX_LINE;
            }
        }
        if ((newButton & PSP_CTRL_LEFT) && !(oldButton & PSP_CTRL_LEFT))
        {
            selectFile -= 5;
            if (selectFile < 0)
            {
                startPos += selectFile + 1;
            }
        }
        oldButton = newButton;
        if (selectFile < 0)
        {
            selectFile = 0;
            startPos--;
            if (startPos < 0)
            {
                startPos = 0;
            }
        }
        if (FileCount <= MAX_LINE)
        {
            if (selectFile >= FileCount)
            {
                selectFile = FileCount - 1;
            }
        }
        else
        {
            if (selectFile > MAX_LINE - 1)
            {
                selectFile = MAX_LINE - 1;
                startPos++;
                if ((startPos + MAX_LINE) > FileCount)
                {
                    startPos = FileCount - MAX_LINE;
                }
            }
        }
        video_copy_rect(tex_frame, draw_frame, &full_rect, &full_rect);
		video_flip_screen(1);
    }
    return 0;
}

int menuGetFileList(void)
{
    int dfd;
    SceIoDirent dir;

    memset(&dir, 0, sizeof(dir)); // 初期化しないとreadに失敗する
    FileCount = 0;
    dfd = sceIoDopen(CurDir);
    if(dfd >= 0)
    {
        while (sceIoDread(dfd, &dir) > 0)
        {
            if (stricmp(dir.d_name, "EBOOT.PBP") == 0 ||
                stricmp(dir.d_name, "eeprom.dat") == 0 ||
                stricmp(dir.d_name, "SAVE") == 0 ||
                stricmp(dir.d_name, "STATE") == 0 ||
                stricmp(dir.d_name, ".") == 0)
            {
                continue;
            }
            if (dir.d_stat.st_attr & FIO_SO_IFDIR)
            {
                FileList[FileCount].type = 0;
            }
            else
            {
                FileList[FileCount].type = 1;
            }
            strcpy(FileList[FileCount].name, dir.d_name);
            FileCount++;
            if (FileCount > 255)
            {
                break;
            }
        }
        sceIoDclose(dfd);
        menuBsortList();
        return FileCount;
    }
    return -1;
}

void menuPrintRomList(int startPos, int selectFile)
{
    int i;
    int color;
    char str[MAX_PATH];

    video_clear_frame(tex_frame);
    mh_print(0, 0, Title,RGB(255, 255, 255));
    mh_print(0, FONT_HEIGHT, CurDir,RGB(0, 0, 255));
    mh_print(2, (selectFile + 2) * FONT_HEIGHT, "*",RGB(255, 0, 0));
    for (i = startPos; i < startPos + MAX_LINE; i++)
    {
        if (i >= FileCount)
        {
            break;
        }
        if (FileList[i].type == 0)
        {
            if ((i - startPos) == selectFile)
            {
                color = RGB(255, 0, 0);
            }
            else
            {
                color = RGB(0, 255, 0);
            }
            sprintf(str, "<%s>", FileList[i].name);
            mh_print(10, (i - startPos + 2) * FONT_HEIGHT, str, color);
        }
        else if (FileList[i].type == 1)
        {
            if ((i - startPos) == selectFile)
            {
                color = RGB(255, 0, 0);
            }
            else
            {
                color = RGB(255, 255, 255);
            }
            mh_print(10, (i - startPos + 2) * FONT_HEIGHT, FileList[i].name, color);
        }
        else
        {
            mh_print(10, (i - startPos + 2) * FONT_HEIGHT, FileList[i].name,RGB(100, 100, 100));
        }
    }
}

void menuStateList(unsigned int oldButton, int save)
{
    int selectItem = 0;
    SceCtrlData pad;
    unsigned int newButton;

    if (RomPath[0] == '\0')
    {
        return;
    }
    menuStateSavePrintList(selectItem);
    while (Run)
    {
        sceCtrlReadBufferPositive(&pad, 1);
        newButton = pad.Buttons;
        if ((newButton & PSP_CTRL_CROSS) && !(oldButton & PSP_CTRL_CROSS))
        {
            return;
        }
        if ((newButton & PSP_CTRL_CIRCLE) && !(oldButton & PSP_CTRL_CIRCLE))
        {
            if (save)
            {
                fileioStateSave(selectItem);
            }
            else
            {
                fileioStateLoad(selectItem);
            }
            menuStateSavePrintList(selectItem);
        }
        if ((newButton & PSP_CTRL_DOWN) && !(oldButton & PSP_CTRL_DOWN))
        {
            selectItem++;
            if (selectItem > 9) selectItem = 0;
            menuStateSavePrintList(selectItem);
        }
        if ((newButton & PSP_CTRL_UP) && !(oldButton & PSP_CTRL_UP))
        {
            selectItem--;
            if (selectItem < 0) selectItem = 9;
            menuStateSavePrintList(selectItem);
        }
        oldButton = newButton;
        video_copy_rect(tex_frame, draw_frame, &full_rect, &full_rect);
		video_flip_screen(1);
    }
    return;
}

void menuStateSavePrintList(int index)
{
    int i, ret;
    int color;
    SceIoStat st;
    char file[MAX_PATH];
    char list[32];
    char date[20];

    video_clear_frame(tex_frame);
    mh_print(0, 0, Title,RGB(255, 255, 255));
    mh_print(0, FONT_HEIGHT, "STATE SLOTS", RGB(0, 0, 255));
    for (i = 0; i < 10; i++)
    {
        fileioGetStatePath(file, MAX_PATH, i);
        memset(&st, 0, sizeof(st));
        ret = sceIoGetstat(file, &st);
        if (ret >= 0)
        {
            sprintf(date, "%04d/%02d/%02d %02d:%02d:%02d",
            st.st_mtime.year, st.st_mtime.month, st.st_mtime.day, st.st_mtime.hour, st.st_mtime.minute, st.st_mtime.second);
            color = RGB(255, 255, 255);
        }
        else
        {
            date[0] = '\0';
            color = RGB(128, 128, 128);
        }
        sprintf(list, "SLOT%03d %s", i, date);
        if (i == index)
        {
            mh_print(2, FONT_HEIGHT * (i + 2), "*",RGB(255, 0, 0));
            mh_print(10, FONT_HEIGHT * (i + 2), list, RGB(255, 0, 0));
        }
        else {
            mh_print(10, FONT_HEIGHT * (i + 2), list, color);
        }
    }
}

void menuConfig(unsigned int oldButton, int mode)
{
    switch (mode)
    {
    case 0:
        ScreenSize = ScreenSize ? 0 : 1;
        Menu[4] = Screen[ScreenSize];
        break;
    case 1:
        Vsync = Vsync ? 0 : 1;
        Menu[5] = VsyncMenu[Vsync];
        break;
    }
}
