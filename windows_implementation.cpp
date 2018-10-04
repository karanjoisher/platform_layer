#include <stdio.h>
#include <intrin.h>  
#include "utility.h"
#include "project_types.h"
#include "windows_platform_interface.h"

global_variable WNDCLASS globalWindowClass;
global_variable int32 globalKeyboard[256];
global_variable int32 globalMouseButtons[5];
global_variable int64 globalQueryPerformanceHZ;

LRESULT CALLBACK WinWindowCallback(HWND windowHandle, UINT message, WPARAM wParam, LPARAM lParam)
{
    /* NOTE(KARAN): Windows sends us two types of messages: Queued and non queued.
    * Non queued messages are sent to this function directly.
    * Queued messages are consumed via Peek Message
    */
    LRESULT result = 0;
    
    PfWindow *window = (PfWindow*)GetPropA(windowHandle, "PfWindow");
    
    switch(message)
    {
        case WM_NCDESTROY:
        {
            RemovePropA(windowHandle, "PfWindow");
            result = DefWindowProc(windowHandle, message, wParam, lParam); 
        }break;
        case WM_SETFOCUS:
        {
            if(window) window->hasKeyboardFocus = true;
        }break;
        case WM_KILLFOCUS:
        {
            if(window) window->hasKeyboardFocus = false;
        }break;
        
        case WM_KEYUP:
        case WM_KEYDOWN:
        case WM_SYSKEYDOWN:
        case WM_SYSKEYUP:
        {
            // TODO(KARAN): Separate keyboard updating from this callback
            WPARAM vkCode = wParam;
            
            int32 repeatCount = lParam & 0x07FFF;
            int32 scanCode = (lParam >> 16) & 0x07F;
            int32 isExtendedKey = (lParam >> 23) & 0x01;
            int32 reserved = (lParam >> 25) & 0x07;
            int32 isAltDown = (lParam >> 29) & 0x01;
            
            bool wasKeyDown = ((lParam >> 30) & 0x01) != 0;
            bool isKeyDown = ((lParam >> 31) & 0x01) == 0;
            
            globalKeyboard[vkCode] = isKeyDown;
            
            if(vkCode == VK_LBUTTON)
            {
                globalMouseButtons[0] = isKeyDown;
            }
            
            if(vkCode == VK_MBUTTON)
            {
                globalMouseButtons[1] = isKeyDown;
            }
            
            if(vkCode == VK_RBUTTON)
            {
                globalMouseButtons[2] = isKeyDown;
            }
            
            if(vkCode == VK_XBUTTON1)
            {
                globalMouseButtons[3] = isKeyDown;
            }
            
            if(vkCode == VK_XBUTTON2)
            {
                globalMouseButtons[4] = isKeyDown;
            }
            
        }break;
        case WM_SIZE:
        {
            int32 newWidth = lParam & 0x0FFFF;
            int32 newHeight = (lParam >> 16) & 0x0FFFF;
            if(window) PfResizeWindow(window, newWidth, newHeight);
        }break;
        case WM_CLOSE:
        {
            if(window) window->shouldClose = true;
            DestroyWindow(windowHandle);
        }break;
        case WM_DESTROY:
        {
            if(window) window->shouldClose = true;
            result = DefWindowProc(windowHandle, message, wParam, lParam); 
        }break;
        default:
        {
            result = DefWindowProc(windowHandle, message, wParam, lParam); 
        }break;
    }
    
    return result;
}


void PfInitialize()
{
    LARGE_INTEGER queryPerformanceHZResult;
    
    BOOL result = QueryPerformanceFrequency(&queryPerformanceHZResult);
    ASSERT(result != 0);
    
    globalQueryPerformanceHZ  = queryPerformanceHZResult.QuadPart;
    
    globalWindowClass.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    globalWindowClass.lpfnWndProc = WinWindowCallback;
    globalWindowClass.hInstance = GetModuleHandle(0);globalWindowClass.lpszClassName = "windowsPlatformLayer";
    
    if(RegisterClassA(&globalWindowClass) == 0)
    {
        fprintf(stderr,"ERROR: Could not register window class. LINE: %d, FUNCTION:%s, FILE:%s\n", __LINE__, __func__, __FILE__);
    }
}


void PfResizeWindow(PfWindow *window, int32 width, int32 height)
{
    if(window->offscreenBuffer.data)
    {
        VirtualFree(window->offscreenBuffer.data, 0,MEM_RELEASE);
        window->offscreenBuffer.data = 0;
    }
    
    window->offscreenBuffer.width = width;
    window->offscreenBuffer.height = height;
    window->offscreenBuffer.bytesPerPixel = 4;
    window->offscreenBuffer.pitch = window->offscreenBuffer.width  * window->offscreenBuffer.bytesPerPixel;
    
    int bitmapMemorySize = window->offscreenBuffer.bytesPerPixel * width  * height;
    
    ASSERT(bitmapMemorySize >= 0);
    
    (window->offscreenBuffer.info).bmiHeader.biSize = sizeof((window->offscreenBuffer.info).bmiHeader);
    (window->offscreenBuffer.info).bmiHeader.biWidth = width;
    
    // NOTE(KARAN): Negative height to set origin at top left corner
    (window->offscreenBuffer.info).bmiHeader.biHeight = -height;
    (window->offscreenBuffer.info).bmiHeader.biPlanes = 1;
    (window->offscreenBuffer.info).bmiHeader.biBitCount = 32;
    
    if(bitmapMemorySize > 0)
    {
        window->offscreenBuffer.data = VirtualAlloc(0, bitmapMemorySize, MEM_COMMIT | MEM_RESERVE,PAGE_READWRITE);
        
        ASSERT(window->offscreenBuffer.data);
    }
    
}

void PfCreateWindow(PfWindow *windowPtr, char *title, int32 xPos, int32 yPos, int32 width, int32 height)
{
    PfWindow window = {};
    DWORD windowStyle = WS_OVERLAPPEDWINDOW | WS_VISIBLE;
    
    RECT windowRect = {};
    windowRect.left = 0;
    windowRect.top = 0;
    windowRect.right = width;
    windowRect.bottom = height;
    
    BOOL windowRectResult = AdjustWindowRect(&windowRect, windowStyle, 0);
    
    ASSERT(windowRectResult != 0);
    
    windowRect.right = windowRect.right - windowRect.left;
    windowRect.bottom = windowRect.bottom - windowRect.top;
    
    window.windowHandle = CreateWindowEx(0,globalWindowClass.lpszClassName, title, windowStyle, xPos, yPos, windowRect.right, windowRect.bottom, 0, 0, globalWindowClass.hInstance,0);
    
    if(window.windowHandle == 0)
    {
        DEBUG_LOG(stderr,"ERROR: Could not create window. LINE: %d, FUNCTION:%s, FILE:%s\n", __LINE__, __func__, __FILE__);
    }
    ASSERT(window.windowHandle);
    BOOL propertySetResult = SetPropA(window.windowHandle, "PfWindow", windowPtr);
    HWND prevFocusWindowHandle = SetFocus(window.windowHandle);
    ASSERT(prevFocusWindowHandle != 0);
    
    // NOTE(KARAN): If the window already had keyboard focus, WM_SETFOCUS message isn't sent to the WINDOWPROC. Hence this hack.
    if(prevFocusWindowHandle == window.windowHandle)
    {
        window.hasKeyboardFocus = true;
    }
    
    ASSERT(propertySetResult != 0);
    
    PfResizeWindow(&window, width, height);
    *windowPtr = window;
}

PfRect PfGetClientRect(PfWindow *window)
{
    PfRect result = {};
    
    RECT rect;
    GetClientRect(window->windowHandle, &rect);
    
    result.x = rect.left;
    result.y = rect.top;
    result.width = rect.right;
    result.height = rect.bottom;
    
    return result;
}


PfRect PfGetWindowRect(PfWindow *window)
{
    PfRect result = {};
    
    RECT rect;
    GetWindowRect(window->windowHandle, &rect);
    
    result.x = rect.left;
    result.y = rect.top;
    result.width = rect.right;
    result.height = rect.bottom;
    
    return result;
}

void PfGetOffscreenBuffer(PfWindow *window, PfOffscreenBuffer *offscreenBuffer)
{
    offscreenBuffer->width = window->offscreenBuffer.width;
    offscreenBuffer->height = window->offscreenBuffer.height;
    offscreenBuffer->pitch = window->offscreenBuffer.pitch;
    offscreenBuffer->bytesPerPixel = window->offscreenBuffer.bytesPerPixel;
    offscreenBuffer->data = window->offscreenBuffer.data;
}

void PfBlitToScreen(PfWindow *window)
{
    HDC deviceContextHandle = GetDC(window->windowHandle);
    StretchDIBits(deviceContextHandle,
                  0, 0,window->offscreenBuffer.width, window->offscreenBuffer.height,
                  0,0,window->offscreenBuffer.width, window->offscreenBuffer.height,
                  window->offscreenBuffer.data, &(window->offscreenBuffer.info),DIB_RGB_COLORS,SRCCOPY);
}

void PfToggleFullscreen(PfWindow *window)
{
    DWORD windowStyle = GetWindowLong(window->windowHandle, GWL_STYLE);
    if(windowStyle & WS_OVERLAPPEDWINDOW)
    {
        MONITORINFO monitorInfo = {sizeof(monitorInfo)};
        if(GetWindowPlacement(window->windowHandle, &(window->prevWindowPlacement)) &&
           GetMonitorInfo(MonitorFromWindow(window->windowHandle, MONITOR_DEFAULTTOPRIMARY), &monitorInfo))
        {
            SetWindowLong(window->windowHandle, GWL_STYLE, windowStyle & ~WS_OVERLAPPEDWINDOW);
            SetWindowPos(window->windowHandle, HWND_TOP,
                         monitorInfo.rcMonitor.left, monitorInfo.rcMonitor.top,
                         monitorInfo.rcMonitor.right - monitorInfo.rcMonitor.left,
                         monitorInfo.rcMonitor.bottom - monitorInfo.rcMonitor.top,
                         SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
        }
    }
    else
    {
        SetWindowLong(window->windowHandle, GWL_STYLE, windowStyle | WS_OVERLAPPEDWINDOW);
        SetWindowPlacement(window->windowHandle, &(window->prevWindowPlacement));
        SetWindowPos(window->windowHandle, 0, 0, 0, 0, 0,
                     SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER |
                     SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
    }
}


int32 PfGetKeyState(int32 vkCode)
{
    int32 result;
    result = globalKeyboard[vkCode];
    return result;
}

int32 PfGetKeyState(PfWindow *window, int32 vkCode)
{
    int32 result = 0;
    if(window->hasKeyboardFocus)//windowHandle == globalKeyboardFocusWindowHandle)
    {
        result = globalKeyboard[vkCode];
    }
    return result;
}

int32 PfGetMouseButtonState(PfWindow *window, int32 index)
{
    
    int32 result = 0;
    if(window->hasKeyboardFocus)// == globalKeyboardFocusWindowHandle)
    {
        result = globalMouseButtons[index];
    }
    return result;
}


int32 PfGetMouseButtonState(int32 index)
{
    int32 result;
    result = globalMouseButtons[index];
    return result;
}

bool PfGetMouseCoordinates(PfWindow *window, int32 *x, int32 *y)
{
    bool result = true;
    
    POINT mousePos = {};
    GetCursorPos(&mousePos);
    ScreenToClient(window->windowHandle, &mousePos);
    
    *x = mousePos.x;
    *y = mousePos.y;
    
    if(mousePos.x < 0 || mousePos.x > window->offscreenBuffer.width)
    {
        result = false;
    }
    
    if(mousePos.y < 0 || mousePos.y > window->offscreenBuffer.height)
    {
        result = false;
    }
    
    return result;
}


PfTimestamp PfGetTimestamp()
{
    int64 result;
    LARGE_INTEGER temp;
    QueryPerformanceCounter(&temp);
    result = temp.QuadPart;
    return result;
}

real32 PfGetSeconds(int64 startTime, int64 endTime)
{
    real32 result;
    real32 a = (real32)(endTime - startTime);
    real32 b = (real32)(globalQueryPerformanceHZ);
    result = a/b;
    return result;
}

uint64 PfRdtsc()
{
    uint64 result;
    result = __rdtsc();  
    return result;
}

void PfSetWindowTitle(PfWindow *window, char *title)
{
    SetWindowTextA(window->windowHandle, title);
}