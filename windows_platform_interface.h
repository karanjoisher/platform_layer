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

#include "api_function_spec.h"