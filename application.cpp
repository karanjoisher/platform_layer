#include "utility.h"
#include "project_types.h"

#if PLATFORM_WINDOWS
#include "windows_platform_interface.h"
#include "windows_implementation.cpp"
#endif

#if PLATFORM_LINUX
#include "linux_platform_interface.h"
#include "linux_implementation.cpp"
#endif


uint32 DebugController(PfWindow *window, PfRect *rect)
{
    uint32 color = 0xFF000000;
    
    
    if(PfGetMouseButtonState(window, 0) != 0)
    {
        color = color | 0xFFFF0000;
    }
    
    if(PfGetMouseButtonState(window, 1) != 0)
    {
        color = color | 0xFF00FF00;
    }
    
    if(PfGetMouseButtonState(window, 2) != 0)
    {
        color = color | 0xFF0000FF;
    }
    
    if(PfGetKeyState(window, 'W'))
    {
        rect->y  -= 2;
    }
    
    if(PfGetKeyState(window, 'A'))
    {
        rect->x  -= 2;
    }
    
    if(PfGetKeyState(window, 'S'))
    {
        rect->y  += 2;
    }
    
    if(PfGetKeyState(window, 'D'))
    {
        rect->x  += 2;
    }
    
    return color;
}

void DrawRectangle(PfOffscreenBuffer *offscreenBuffer, PfRect rect,  uint32 color)
{
    
    int32 minX = rect.x;
    int32 minY = rect.y;
    int32 maxX = minX + rect.width;
    int32 maxY = minY + rect.height;
    
    if(minX < 0)
    {
        minX = 0;
    }
    
    if(minY < 0)
    {
        minY = 0;
    }
    
    if(maxX > offscreenBuffer->width)
    {
        maxX = offscreenBuffer->width;
    }
    
    if(maxY > offscreenBuffer->height)
    {
        maxY = offscreenBuffer->height;
    }
    
    
    uint8 *row = (uint8*)offscreenBuffer->data + (minY * offscreenBuffer->pitch) + (minX * offscreenBuffer->bytesPerPixel);
    
    for(int32 y = minY; y < maxY; y++)
    {
        uint32 *pixel = (uint32*)row;
        for(int32 x = minX; x < maxX; x++)
        {
            *pixel = color;
            pixel++;
        }
        row = row + offscreenBuffer->pitch;
    }
}


void RenderGrid(PfOffscreenBuffer *offscreenBuffer)
{
    uint8 *row = (uint8 *)offscreenBuffer->data;
    uint32 redColor = 0xFFFF0000;
    uint32 blueColor = 0xFF0000FF;
    uint32 color = redColor;
    
    int32 rectWidth = 20;
    for(int32 y = 0; y < offscreenBuffer->height; y++)
    {
        uint32 *pixel = (uint32*)row;
        for(int32 x = 0; x < offscreenBuffer->width; x++)
        {
            *pixel = color;
            pixel++;
            if((x/rectWidth)%2 ==  (y/rectWidth)%2)
            {
                color = redColor;
            }
            else
            {
                color = blueColor;
            }
            
        }
        row += offscreenBuffer->pitch;
    }
}

int main()
{
    PfInitialize();
    
    int x = 0;
    int y = 0;
    int width = 256;
    int height = 256;
    
    PfWindow window[2] = {};
    
    PfCreateWindow(&window[0], (char*)"WINDOW 0", x, y, width, height);
    
    PfCreateWindow(&window[1], (char*)"WINDOW 1", 256, 0, width, height);
    
    PfRect rect1 = {0, 0, 10, 10};
    PfRect rect2 = {0, 0, 10, 10};
    
    PfTimestamp start = PfGetTimestamp();
    uint64 startCycles = PfRdtsc();
    while(!window[0].shouldClose || !window[1].shouldClose)
    {
#if PLATFORM_WINDOWS
        
        // NOTE(KARAN):  Message Pump
        globalMouseButtons[0] = GetKeyState(VK_LBUTTON) >> 15;
        globalMouseButtons[1] = GetKeyState(VK_MBUTTON) >> 15;
        globalMouseButtons[2] = GetKeyState(VK_RBUTTON) >> 15;
        globalMouseButtons[3] = GetKeyState(VK_XBUTTON1) >> 15;
        globalMouseButtons[4] = GetKeyState(VK_XBUTTON2) >> 15;
        
        for(int32 i = 0; i < ARRAY_COUNT(window); i++)
        {
            MSG message = {};
            while(PeekMessage(&message, window[i].windowHandle, 0, 0, PM_REMOVE) > 0)
            {
                //DEBUG_LOG(stdout, "FIELDING MESSAGE: %d\n", message.message);
                switch(message.message)
                {
                    case WM_QUIT:
                    {
                        window[i].shouldClose = true;
                    }break;
                    default:
                    {
                        TranslateMessage(&message);
                        DispatchMessage(&message);
                    }break;
                }
            }
        }
#elif PLATFORM_LINUX
        // NOTE(KARAN):  Message Pump
        
        Display *display = globalDisplay;
        XEvent event;
        int32 queueLength;
        while((queueLength = XPending(display)) > 0)
        {
            XNextEvent(display, &event);
            PfWindow *eventWindow;
            int32 findContextResult = XFindContext(display, event.xany.window, globalXlibContext, (XPointer*)&eventWindow);
            ASSERT(findContextResult == 0);
            
            switch(event.type)
            {
                case EnterNotify:
                case LeaveNotify:
                {
                    eventWindow->isWindowUnderMouse = (event.type == EnterNotify);
                }break;
                case FocusIn:
                case FocusOut:
                {
                    eventWindow->hasKeyboardFocus = (event.type == FocusIn);
                }break;
                case ButtonPress:
                case ButtonRelease:
                {
                    XButtonEvent buttonEvent = event.xbutton;
                    int32 state = event.type == ButtonPress ? 1 : 0;
                    int32 index = buttonEvent.button - 1;
                    
                    globalMouseButtons[index] = state;
                }break;
                case KeyPress:
                case KeyRelease:
                {
                    XKeyEvent keyEvent = event.xkey;
                    globalKeyboard[keyEvent.keycode] = (event.type == KeyPress) ? 1 : 0;
                    
                }break;
                case ConfigureNotify:
                {
                    PfRect rect = PfGetClientRect(eventWindow);
                    
                    XConfigureEvent configureEvent = event.xconfigure;
                    if(eventWindow->offscreenBuffer.width != configureEvent.width || eventWindow->offscreenBuffer.height != configureEvent.height)
                    {
                        PfResizeWindow(eventWindow, configureEvent.width, configureEvent.height);
                        
                    }
                    
                }break;
                case DestroyNotify:
                {
                    
                }break;
                case ClientMessage:
                {
                    if(event.xclient.data.l[0] == globalWmDeleteWindowAtom) 
                    {
                        eventWindow->shouldClose = true;
                        XDestroyWindow(display, eventWindow->windowHandle);
                        break;
                    }
                }
                default:
                {
                    
                }break;
            }
        }
#endif
        
#if PLATFORM_WINDOWS
#define ALT_KEY VK_MENU
#define ENTER_KEY VK_RETURN
#elif PLATFORM_LINUX
#define ALT_KEY XK_Alt_L
#define ENTER_KEY XK_Return
#endif
        
        static bool toggled = false;
        
        if(PfGetKeyState(ALT_KEY) == 0 || PfGetKeyState(ENTER_KEY) == 0)
        {
            toggled = false;
        }
        
        if(!toggled && PfGetKeyState(&window[0], ALT_KEY) && PfGetKeyState(&window[0], ENTER_KEY) != 0)
        {
            toggled = true;
            PfToggleFullscreen(&window[0]);
        }
        
        
        if(!toggled && PfGetKeyState(&window[1], ALT_KEY) && PfGetKeyState(&window[1], ENTER_KEY) != 0)
        {
            toggled = true;
            PfToggleFullscreen(&window[1]);
        }
        
        uint32 color1 = DebugController(&window[0], &rect1);
        uint32 color2 = DebugController(&window[1], &rect2);
        
        PfOffscreenBuffer offscreenBuffer1 = {};
        PfOffscreenBuffer offscreenBuffer2 = {};
        
        PfGetOffscreenBuffer(&window[0], &offscreenBuffer1);
        PfGetOffscreenBuffer(&window[1], &offscreenBuffer2);
        
        RenderGrid(&offscreenBuffer1);
        
        DrawRectangle(&offscreenBuffer1, rect1, color1);
        
        DrawRectangle(&offscreenBuffer2, PfRect{0, 0, offscreenBuffer2.width, offscreenBuffer2.height}, 0xFFFFFFFF);
        DrawRectangle(&offscreenBuffer2, rect2, color2);
        
        if(!window[0].shouldClose) PfBlitToScreen(&window[0]);
        if(!window[1].shouldClose) PfBlitToScreen(&window[1]);
        
#if PLATFORM_LINUX
        timespec nanoSecondsPerFrameTimespec = {0, 33000000};
        
        timespec sleepTimeEnd = AddTimespec(start, nanoSecondsPerFrameTimespec);
        while(clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &sleepTimeEnd,0) != 0)
        {
        }
#endif
        PfTimestamp end = PfGetTimestamp();
        uint64 endCycles = PfRdtsc();
        
        real32 secondsPerFrame = PfGetSeconds(start, end);
        real32 cyclesPerFrame = real32(endCycles - startCycles);
        real32 framesPerSecond = 1.0f/secondsPerFrame;
        
        startCycles = endCycles;
        start = end;
        
        
        char temp[256];
        
        int32 xMouse = -2;
        int32 yMouse = -2;
        bool inside;
        
        if(!window[0].shouldClose) inside = PfGetMouseCoordinates(&window[0], &xMouse, &yMouse);
        sprintf(temp, "%.2fms %.2FPS %.3fMHz Inside: %d X: %d Y: %d", secondsPerFrame * 1000.0f, 1.0f/secondsPerFrame, cyclesPerFrame/(secondsPerFrame * 1000.0f * 1000.0f), inside, xMouse, yMouse);
        
        if(!window[0].shouldClose) PfSetWindowTitle(&window[0], temp);
        
        if(!window[1].shouldClose) inside = PfGetMouseCoordinates(&window[1], &xMouse, &yMouse);
        sprintf(temp, "%.2fms %.2FPS %.3fMHz Inside: %d X: %d Y: %d", secondsPerFrame * 1000.0f, 1.0f/secondsPerFrame, cyclesPerFrame/(secondsPerFrame * 1000.0f * 1000.0f), inside, xMouse, yMouse);
        
        if(!window[1].shouldClose) PfSetWindowTitle(&window[1], temp);
    }
    
#if PLATFORM_LINUX
    XCloseDisplay(globalDisplay);
#endif
    return 0;
}
