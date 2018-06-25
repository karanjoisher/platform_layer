#include <sys/mman.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <stdio.h> 
#include <cstring>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include "utility.h"
#include "project_types.h"
#include "project_intrinsics.h"

typedef XImage LinuxOffscreenBuffer;

global_variable bool globalRunning; 
global_variable LinuxOffscreenBuffer globalOffscreenBuffer;
global_variable int globalMouseButtons[5];
global_variable int globalMouseX;
global_variable int globalMouseY;
global_variable uint8 globalKeys[32];

#if DEBUG_BUILD
void DebugKeyboardMapping(Display *display)
{
    int minKeyCode = 8;
    int maxKeyCode = 255;
    
    int keyCodesPerKeySym;
    KeySym *keyCodeToKeySyms = XGetKeyboardMapping(display, minKeyCode, maxKeyCode - minKeyCode + 1, &keyCodesPerKeySym);
    
    int indexIntoTable = 0;
    for(int i = minKeyCode; i <= maxKeyCode; i++)
    {
        fprintf(stdout, "KEYCODE %d: KEYSYMS-> ",i);
        for(int j = 0; j < keyCodesPerKeySym; j++)
        {
            int index = (i - minKeyCode) * keyCodesPerKeySym + j;
            if(i != 12)
            {
                KeySym a = keyCodeToKeySyms[index];
                char *str = XKeysymToString(a);
                if(str)
                {
                    fprintf(stdout, "%s, ", str);
                }
            }
        }
        fprintf(stdout, "\n");
    }
    //XFree(keyCodeToNumKeySyms);
}
#endif

// TODO(KARAN): Consider making this a macro
inline timespec LinuxNormalizeTimespec(timespec t)
{
    timespec result = t;
    if (result.tv_nsec >= 1000000000L)
    {
        result.tv_nsec -= 1000000000L;       
        result.tv_sec++;
    }
    return result;
}

inline timespec LinuxAddTimespec(timespec t1, timespec t2)
{
    timespec result = {};
    result.tv_nsec = t1.tv_nsec + t2.tv_nsec;
    result.tv_sec = t1.tv_sec + t2.tv_sec;
    result = LinuxNormalizeTimespec(result);
    return result;
}

inline timespec LinuxSubtractTimespec(timespec t1, timespec t2)
{
    timespec result = {};
    if(t1.tv_nsec < t2.tv_nsec)
    {
        result.tv_sec = t1.tv_sec - t2.tv_sec - 1;
        result.tv_nsec = t1.tv_nsec + 1000000000L - t2.tv_nsec; 
    }
    else
    {
        result.tv_sec = t1.tv_sec - t2.tv_sec;
        result.tv_nsec = t1.tv_nsec - t2.tv_nsec; 
    }
    result = LinuxNormalizeTimespec(result);
    return result;
}

inline int64 LinuxTimespecToNS(timespec t)
{
    int64 result;
    result = (t.tv_sec * 1000000000L) + t.tv_nsec;
    return result;
}

bool isKeyPressed(Display *display, char *keyboardState, KeySym keySym)
{
    bool result = false;
    KeyCode keyCode  = XKeysymToKeycode(display, keySym);
    
    int keyByte = keyCode/8;
    int keyBit = keyCode % 8;
    result = (keyboardState[keyByte] & (1 << keyBit)) != 0;
    return result;
}


void LinuxResizeOffscreenBuffer(LinuxOffscreenBuffer *offscreenBuffer, int width, int height)
{
    int bytesPerPixel = (offscreenBuffer->bits_per_pixel/8);
    int bufferSize; 
    if(offscreenBuffer->data)
    {
        bufferSize = offscreenBuffer->width * offscreenBuffer->height * bytesPerPixel;
        int unmapResult = munmap((void*)offscreenBuffer->data, bufferSize);
        ASSERT(unmapResult == 0);
    }
    offscreenBuffer->width = width;
    offscreenBuffer->height = height;
    offscreenBuffer->bytes_per_line = offscreenBuffer->width * bytesPerPixel;
    bufferSize = offscreenBuffer->width * offscreenBuffer->height * bytesPerPixel;
    void *tempOffscreenMemory = mmap(0, bufferSize, PROT_READ|PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    ASSERT(tempOffscreenMemory != MAP_FAILED);
    offscreenBuffer->data = (uint8*)tempOffscreenMemory;
    
}

void ClearScreen(uint8 *offscreenMemory, int width, int height, uint32 pitch, uint32 color)
{
    uint8 *row = offscreenMemory;
    for(int y = 0; y < height; y++)
    {
        uint32 *pixel = (uint32*)row;
        for(int x = 0; x < width; x++)
        {
            *pixel = color;
            pixel++;
        }
        row += pitch;
    }
}

void DrawRectangle(uint8 *offscreenMemory, int width, int height, uint32 pitch,real32 rectMinX, real32 rectMinY, real32 rectMaxX, real32 rectMaxY, uint32 color)
{
    
    int32 minX = roundReal32ToInt32(rectMinX);
    int32 minY = roundReal32ToInt32(rectMinY);
    int32 maxX = roundReal32ToInt32(rectMaxX);
    int32 maxY = roundReal32ToInt32(rectMaxY);
    
    if(minX < 0)
    {
        minX = 0;
    }
    
    if(minY < 0)
    {
        minY = 0;
    }
    
    if(maxX > width)
    {
        maxX = width;
    }
    
    if(maxY > height)
    {
        maxY = height;
    }
    
    int  bytesPerPixel = 4;
    uint8 *row = offscreenMemory + (minY * pitch) + (minX * bytesPerPixel); 
    for(int y = minY; y < maxY; y++)
    {
        uint32 *pixel = (uint32*)row;
        for(int x = minX; x < maxX; x++)
        {
            *pixel = color;
            pixel++;
        }
        row = row + pitch;
    }
}

void Render(uint8 *offscreenMemory, int width, int height, uint32 pitch)
{
    uint8 *row = offscreenMemory;
    uint32 redColor = 0xFFFF0000;
    uint32 blueColor = 0xFF0000FF;
    uint32 color = redColor;
    int rectWidth = 20;
    for(int y = 0; y < height; y++)
    {
        uint32 *pixel = (uint32*)row;
        for(int x = 0; x < width; x++)
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
        row += pitch;
    }
}

int main()
{
    Display *display = XOpenDisplay(":0");
    Window window, rootWindow;
    int screen_num;
    Screen *screen;
    int32 framesPerSecond = 30;
    real32 secondsPerFrame = 1/(real32)framesPerSecond;
    uint64 nanoSecondsPerFrame = (uint64)(secondsPerFrame * 1000.0f * 1000.0f * 1000.0f); 
    if(display)
    {
#if SLOW_BUILD
        XSynchronize(display, true);
#endif
        /* NOTE(KARAN): 
Memory format = 
Address 0:BB
        Address 1:GG
 Address 2:RR
 Address 3:AA
 
 uint32 format = 
 MSB ----> LSB
0xAA RR GG BB
*/
        XInitImage(&globalOffscreenBuffer);
        globalOffscreenBuffer.width = 960;
        globalOffscreenBuffer.height = 340;		/* size of image */
        globalOffscreenBuffer.xoffset = 0;			/* number of pixels offset in X direction */
        globalOffscreenBuffer.format = ZPixmap;			/* XYBitmap, XYPixmap, ZPixmap */
        globalOffscreenBuffer.byte_order = LSBFirst;			/* data byte order, LSBFirst, MSBFirst */
        globalOffscreenBuffer.bitmap_unit = 32;		/* quant. of scanline 8, 16, 32 */
        globalOffscreenBuffer.bitmap_bit_order = LSBFirst;		/* LSBFirst, MSBFirst */
        globalOffscreenBuffer.bitmap_pad = 32;			/* 8, 16, 32 either XY or ZPixmap */
        globalOffscreenBuffer.depth = 32;			/* depth of image */
        globalOffscreenBuffer.bits_per_pixel = 32;		/* bits per pixel (ZPixmap) */
        globalOffscreenBuffer.red_mask = 0x00FF0000;		/* bits in z arrangement */
        globalOffscreenBuffer.green_mask = 0x0000FF00;
        globalOffscreenBuffer.blue_mask = 0x000000FF;
        LinuxResizeOffscreenBuffer(&globalOffscreenBuffer, 960, 340);
        
        screen_num = XDefaultScreen(display);
        
        XVisualInfo visualInfo = {};
        visualInfo.screen = screen_num;
        visualInfo.depth = 32;
        visualInfo.c_class = TrueColor;
        visualInfo.bits_per_rgb = 8;
        long visualInfoMask = VisualScreenMask | VisualDepthMask | VisualClassMask | VisualBitsPerRGBMask;
        int possibleVisualsCount;
        XVisualInfo *possibleVisuals = XGetVisualInfo(display, visualInfoMask, &visualInfo, &possibleVisualsCount);
        
        Visual *chosenVisual = 0;
        if(possibleVisuals)
        {
            chosenVisual = possibleVisuals[0].visual;
            XFree((void*)possibleVisuals);
        }
        else
        {
            // TODO(KARAN): Diagnostic 
            fprintf(stderr,"Could not get a visual of our choice\n");
        }
        
        rootWindow = XDefaultRootWindow(display);
        screen = XScreenOfDisplay(display, screen_num);
        
        XSetWindowAttributes windowAttributes = {};
        
        /* NOTE(KARAN): For some reason border_pixel needs to be initialized.
         Maybe initializing border_pixel overrides the need of border_pixmap
         Border_pixmap default is CopyFromParent
         In our case, parent and child dont have same depth, so that would cause an error
         But maybe defining the border_pixel gets rid of that call and hence everything seems to work ... ??? */
        windowAttributes.border_pixel = WhitePixel(display, screen_num);
        
        // NOTE(KARAN):If you set the colormap to CopyFromParent, the parent window's colormap is copied and used by its child. However, the child window must have the same visual type as the parent, or a BadMatch error results. 
        windowAttributes.colormap = XCreateColormap(display, rootWindow, chosenVisual, AllocNone);		
        unsigned long windowAttributesMask = CWBorderPixel | CWColormap;
        
        ASSERT(chosenVisual != 0);
        if(chosenVisual)
        {
            window = XCreateWindow(display, rootWindow, 0, 0, globalOffscreenBuffer.width, globalOffscreenBuffer.height, 0, 32, 
                                   InputOutput, chosenVisual, windowAttributesMask, &windowAttributes);
        }
        XStoreName(display, window, "Window");
        
        
        XGCValues gcAttributes = {};
        unsigned long gcAttributesMask =GCFunction | GCPlaneMask |GCForeground |GCBackground | GCLineWidth |GCLineStyle | GCCapStyle | GCJoinStyle		| GCFillStyle | GCFillRule | GCTileStipXOrigin  | GCTileStipYOrigin |  GCSubwindowMode | GCGraphicsExposures | GCClipXOrigin | GCClipYOrigin | GCClipMask | GCDashOffset| GCDashList| GCArcMode /* GCTile | GCStipple | GCFont| */;
        gcAttributes.function = GXcopy;			/* logical operation */
        gcAttributes.plane_mask = AllPlanes;	/* plane mask */
        gcAttributes.foreground = 0xFFFF0000;	/* foreground pixel */
        gcAttributes.background = 0xFF000000;	/* background pixel */
        gcAttributes.line_width = 5;			/* line width (in pixels) */
        gcAttributes.line_style = LineSolid;			/* LineSolid, LineOnOffDash, LineDoubleDash */
        gcAttributes.cap_style = CapButt;			/* CapNotLast, CapButt, CapRound, CapProjecting */
        gcAttributes.join_style = JoinMiter;			/* JoinMiter, JoinRound, JoinBevel */
        gcAttributes.fill_style = FillSolid;			/* FillSolid, FillTiled, FillStippled FillOpaqueStippled*/
        gcAttributes.fill_rule = EvenOddRule;			/* EvenOddRule, WindingRule */
        gcAttributes.arc_mode = ArcPieSlice;			/* ArcChord, ArcPieSlice */
        gcAttributes.ts_x_origin = 0;		/* offset for tile or stipple operations */
        gcAttributes.ts_y_origin = 0;
        gcAttributes.subwindow_mode = ClipByChildren;		/* ClipByChildren, IncludeInferiors */
        gcAttributes.graphics_exposures = True;	/* boolean, should exposures be generated */
        gcAttributes.clip_x_origin = 0;		/* origin for clipping */
        gcAttributes.clip_y_origin = 0;
        gcAttributes.clip_mask = None;		/* bitmap clipping; other calls for rects */
        gcAttributes.dash_offset = 0;		/* patterned/dashed line information */
        gcAttributes.dashes = 4;
        GC graphicsContext = XCreateGC(display, window, gcAttributesMask, &gcAttributes);
        
        XMapRaised(display, window);
        
        /* TODO(KARAN): Many more events can be defined here : https://tronche.com/gui/x/xlib/events/mask.html
        Check which ones do you need to service.*/
        // TODO(KARAN): FocusChangeMask,EnterWindowMask, LeaveWindowMask to check whether window is active or not 
        long eventsWanted = StructureNotifyMask;
#if 0
        eventsWanted =eventsWanted|KeyPressMask|KeyReleaseMask|eventsWanted|ButtonPressMask|ButtonReleaseMask; 
#endif
        XSelectInput(display, window, eventsWanted);
        
        XEvent event;
        globalRunning = true;
        
        clockid_t clockId;
        /*
  NOTE(KARAN): I tried using CLOCK_PROCESS_CPUTIME_ID and CLOCK_MONOTONIC_RAW.
        CLOCK_PROCESS_CPUTIME_ID gives problems while calling clock_nanosleep. Possibly due to the fact that process isn't bound to one CPU.
        
        TODO(KARAN): Try using sched_setaffinity() to bound this process to one CPU and then check whether clock_nanosleep works with CLOCK_PROCESS_CPUTIME_ID, if it does then my above guess is correct. 
        
 CLOCK_MONOTONIC_RAW isn't supported for clock_nanosleep
*/
        clockid_t fallBackClockIds[] = {CLOCK_MONOTONIC, CLOCK_REALTIME};
        for(int i = 0; i < ARRAY_COUNT(fallBackClockIds); i++)
        {
            timespec ignore;
            int result = clock_gettime(fallBackClockIds[i], &ignore);
            if(result == 0)
            {
                clockId = fallBackClockIds[i];
                break;
            }
        }
        
        timespec startTime;
        clock_gettime(clockId, &startTime);
        unsigned long long startCycles = rdtsc();
        while(globalRunning)
        {
            Window rootWindowOfEventWindow;
            Window childWindowOfEventWindowContainingMouse;
            int rootWindowMouseX, rootWindowMouseY;
            unsigned int  buttonMask;
            
            // NOTE(KARAN): XQueryPointer returns false if the event window and root window are not on the same screen. The coordinates obtained are invalid in such case. 
            int xQueryPointerResult = XQueryPointer(display, window, &rootWindowOfEventWindow ,&childWindowOfEventWindowContainingMouse, &rootWindowMouseX, &rootWindowMouseY,
                                                    &globalMouseX, &globalMouseY, &buttonMask);
            ASSERT(xQueryPointerResult);
            globalMouseButtons[0] = buttonMask & Button1Mask;
            globalMouseButtons[1] = buttonMask & Button2Mask;
            globalMouseButtons[2] = buttonMask & Button3Mask;
            globalMouseButtons[3] = buttonMask & Button4Mask;
            globalMouseButtons[4] = buttonMask & Button5Mask;
            
            
            XQueryKeymap(display, globalKeys);
            
            /* NOTE(KARAN): Maybe we want to use XCheckMaskEvent to support multiple windows
 or just cycle through all windows using XCheckWindowEvent */
            if(isKeyPressed(display, globalKeys, XK_Escape))
            {
                XDestroyWindow(display, window);
                XCloseDisplay(display);
                return 0;
            }
            
            
            while(XCheckWindowEvent(display, window, eventsWanted, &event))
            {
                if(event.type == ConfigureNotify)
                {
                    XConfigureEvent configureEvent = event.xconfigure;
                    if(globalOffscreenBuffer.width != configureEvent.width || globalOffscreenBuffer.height != configureEvent.height)
                    {
                        
                        LinuxResizeOffscreenBuffer(&globalOffscreenBuffer, configureEvent.width, configureEvent.height);
                    }
                }
                
#if 0
                // NOTE(KARAN): Event driven approach for mouse events
                if(event.type == ButtonPress || event.type == ButtonRelease)
                {
                    XButtonEvent buttonEvent = event.xbutton;
                    int state = event.type == ButtonPress ? 1 : 0;
                    int index = buttonEvent.button - 1;
                    globalMouseButtons[index] = state;
                    // NOTE(KARAN): Mouse X and Mouse Y will be invalid if the event window and root window are not on the same screen
                    ASSERT(buttonEvent.same_screen);
                    globalMouseX = buttonEvent.x;
                    globalMouseY = buttonEvent.y;
                }
#endif
            }
            
            Render((uint8*)globalOffscreenBuffer.data, globalOffscreenBuffer.width, globalOffscreenBuffer.height, globalOffscreenBuffer.bytes_per_line);
            
            // NOTE(KARAN): Mouse Testing 
            uint32 color = 0xFF000000;
            if(globalMouseButtons[0])
            {
                color = color | 0xFFFF0000;
            }
            if(globalMouseButtons[1])
            {
                color = color | 0xFF00FF00;
            }
            if(globalMouseButtons[2])
            {
                color = color | 0xFF0000FF;
            }
            
            real32 rectWidth = 20;
            real32 rectHeight = 20;
            DrawRectangle((uint8*)globalOffscreenBuffer.data, globalOffscreenBuffer.width, globalOffscreenBuffer.height , globalOffscreenBuffer.bytes_per_line, (real32)globalMouseX, (real32)globalMouseY, (real32)globalMouseX + rectWidth, (real32)globalMouseY + rectHeight, color);
            
            // NOTE(KARAN): Keyboard Test
            
            local_persist uint32 rect2X = globalOffscreenBuffer.width/2;
            local_persist uint32 rect2Y = globalOffscreenBuffer.height/2;
            
            int rect2Width = 40;
            int rect2Height = 40;
            int speed = 2;
            if(isKeyPressed(display, globalKeys, XK_Control_L))
            {
                rect2Width /= 2;
            }
            if(isKeyPressed(display, globalKeys, XK_W))
            {
                rect2Y -= speed;
            }
            if(isKeyPressed(display, globalKeys, XK_A))
            {
                rect2X -= speed;
            }
            if(isKeyPressed(display, globalKeys, XK_S))
            {
                rect2Y += speed;
            }
            if(isKeyPressed(display, globalKeys, XK_D))
            {
                rect2X += speed;
            }
            
            DrawRectangle((uint8*)globalOffscreenBuffer.data, globalOffscreenBuffer.width, globalOffscreenBuffer.height , globalOffscreenBuffer.bytes_per_line, (real32)rect2X, (real32)rect2Y, (real32)rect2X + rect2Width, (real32)rect2Y + rect2Height, 0xFFFFFFFF);
            
            XPutImage(display, window, graphicsContext, &globalOffscreenBuffer, 0, 0, 0, 0, globalOffscreenBuffer.width, globalOffscreenBuffer.height);
            
            
            timespec nanoSecondsPerFrameTimespec = {0, nanoSecondsPerFrame};
            timespec sleepTimeEnd = LinuxAddTimespec(startTime, nanoSecondsPerFrameTimespec);
            while(clock_nanosleep(clockId, TIMER_ABSTIME, &sleepTimeEnd,0) != 0)
            {
            }
            
            timespec endTime;
            clock_gettime(clockId, &endTime);
            unsigned long long endCycles = rdtsc();
            int64 frameTime = LinuxTimespecToNS(LinuxSubtractTimespec(endTime, startTime));
            unsigned long long frameCycles = endCycles - startCycles; 
            real64 frameTimeMS = (real64)frameTime / 1000000.0;
            real64 frameMegaCycles = (real64)frameCycles/(1000.0 * 1000.0);
            real64 megaCyclesPerSecond = (frameMegaCycles * (1.0/frameTimeMS)) * 1000.0;
            
            char title[1024];
            snprintf(title,1024, "FrameTime:%.2lfms/f | FrameCycles:%.2lfMC/f | CyclesPerSecond:%.2lfMC/s", frameTimeMS, (real64)frameMegaCycles, megaCyclesPerSecond);
            XStoreName(display, window, title);
            
            startTime = endTime;
            startCycles = endCycles;
        }
    }
    else
    {
        fprintf(stderr, "ERROR: Cannot connect to XLib\n");
    }
    
    return 0;
}
