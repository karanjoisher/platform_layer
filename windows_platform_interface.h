#pragma once
#include <windows.h>
#include "project_types.h"

typedef int64 PfTimestamp;

struct WinOffscreenBuffer
{
    BITMAPINFO info;
    void *data;
    int width;
    int height;
    int pitch;
    int bytesPerPixel = 4;
};

struct PfOffscreenBuffer
{
    void *data;
    int width;
    int height;
    int pitch;
    int bytesPerPixel = 4;
};

struct PfRect
{
    int32 x;
    int32 y; 
    int32 width;
    int32 height;
};

struct PfWindow
{
    HWND windowHandle; //Window = unsigned long 
    WinOffscreenBuffer offscreenBuffer;
    WINDOWPLACEMENT prevWindowPlacement = {sizeof(prevWindowPlacement)};
    bool shouldClose;
    bool hasKeyboardFocus;
};

void PfInitialize();
void PfCreateWindow(PfWindow *window, char *title, int32 xPos, int32 yPos, int32 width, int32 height);
void PfResizeWindow(PfWindow *window, int32 width, int32 height);
PfRect PfGetClientRect(PfWindow *window);
PfRect PfGetWindowRect(PfWindow *window);
void PfGetOffscreenBuffer(PfWindow *window, PfOffscreenBuffer *offscreenBuffer);
void PfBlitToScreen(PfWindow *window);
void PfToggleFullscreen(PfWindow *window);
int32 PfGetKeyState(int32 vkCode);
int32 PfGetKeyState(PfWindow *window, int32 vkCode);
int32 PfGetMouseButtonState(PfWindow *window, int32 index);
int32 PfGetMouseButtonState(int32 index);
bool PfGetMouseCoordinates(PfWindow *window, int32 *x, int32 *y);
PfTimestamp PfGetTimestamp();
real32 PfGetSeconds(PfTimestamp startTime, PfTimestamp endTime);
uint64 PfRdtsc();
void PfSetWindowTitle(PfWindow *window, char *title);