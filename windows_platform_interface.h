#pragma once

#include <windows.h>
#include "project_types.h"
//#include <GL/gl.h>

#if defined(PF_TIME)
typedef int64 PfTimestamp;
#endif

#if defined(PF_WINDOW_AND_INPUT)

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
    HDC deviceContext;
    HGLRC glContext;
    bool shouldClose;
    bool fullscreen;
    bool hasKeyboardFocus;
    
    // HACK(KARAN): This is temporarily added to support 
    // offscreenbuffer rendering via opengl modern contexts.
    uint32 offscreenBufferTextureId;
    int32 vao;
    uint32 programId;
};
#endif

#include "api_interface.h"