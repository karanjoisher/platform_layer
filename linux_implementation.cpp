#include "linux_platform_interface.h"

#include <stdio.h>
#include <malloc.h>
#include <time.h>

#include <X11/Xresource.h>

global_variable XContext globalXlibContext;
global_variable Display*  globalDisplay;
global_variable Atom globalWmDeleteWindowAtom;
global_variable Atom globalWmState;
global_variable Atom globalWmStateFullscreen;
global_variable int32 globalKeyboard[256];
global_variable int32 globalMouseButtons[5];

void PfInitialize()
{
    globalXlibContext = XUniqueContext();
    
    /* NOTE(KARAN): XOpenDisplay opens a connection between the XServer and the window. 
       *  Currently I have chosen to have only one connection for all windows as XPending and XNextEvent can pull all events
*  for all the created windows using just one display.
       *  If I open a connection for each window, I would have to iterate over each connection to poll all the events.
       */
    globalDisplay = XOpenDisplay(":0");
    globalWmDeleteWindowAtom = XInternAtom(globalDisplay, "WM_DELETE_WINDOW", False);
    
    globalWmState = XInternAtom(globalDisplay, "_NET_WM_STATE", true);
    globalWmStateFullscreen = XInternAtom(globalDisplay, "_NET_WM_STATE_FULLSCREEN", true);
    
}

void PfResizeWindow(PfWindow *window, int32 width, int32 height)
{
    if(window->offscreenBuffer.data)
    {
        free(window->offscreenBuffer.data);
        window->offscreenBuffer.data = 0;
        //bufferSize = offscreenBuffer->width * offscreenBuffer->height * bytesPerPixel;
        //int unmapResult = munmap((void*)offscreenBuffer->data, bufferSize);
        //ASSERT(unmapResult == 0);
    }
    
    int32 bytesPerPixel = (window->offscreenBuffer.bits_per_pixel/8);
    window->offscreenBuffer.width = width;
    window->offscreenBuffer.height = height;
    window->offscreenBuffer.bytes_per_line = window->offscreenBuffer.width * bytesPerPixel;
    
    int32 bufferSize = width * height * bytesPerPixel;
    ASSERT(bufferSize >= 0);
    //void *tempOffscreenMemory = mmap(0, bufferSize, PROT_READ|PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    //ASSERT(tempOffscreenMemory != MAP_FAILED);
    
    if(bufferSize > 0)
    {
        window->offscreenBuffer.data = (char*)malloc(bufferSize);//tempOffscreenMemory;
        ASSERT(window->offscreenBuffer.data);
    }
}



void PfCreateWindow(PfWindow *window, char *title, int32 xPos, int32 yPos, int32 width, int32 height)
{
    PfWindow clear = {};
    *window = clear;
    
    /* NOTE(KARAN): XOpenDisplay opens a connection between the XServer and the window. 
       *  Currently I have chosen to have only one connection for all windows as XPending and XNextEvent can pull all events
*  for all the created windows using just one display.
       *  If I open a connection for each window, I would have to iterate over each connection to poll all the events.
       */
    //window->display = XOpenDisplay(":0");
    window->display = globalDisplay;
    
    if(window->display == 0)
    {
        fprintf(stderr, "ERROR:Cannot connect to XLib. LINE: %d, FUNCTION: %s, FILE:%s\n", __LINE__, __func__, __FILE__);
    }
    
#if SLOW_BUILD
    XSynchronize(window->display, true);
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
    XInitImage(&window->offscreenBuffer);
    window->offscreenBuffer.width = width;
    window->offscreenBuffer.height = height;		       /* size of image */
    window->offscreenBuffer.xoffset = 0;			    /* number of pixels offset in X direction */
    window->offscreenBuffer.format = ZPixmap;		   /* XYBitmap, XYPixmap, ZPixmap */
    window->offscreenBuffer.byte_order = LSBFirst;	  /* data byte order, LSBFirst, MSBFirst */
    window->offscreenBuffer.bitmap_unit = 32;		   /* quant. of scanline 8, 16, 32 */
    window->offscreenBuffer.bitmap_bit_order = LSBFirst;/* LSBFirst, MSBFirst */
    window->offscreenBuffer.bitmap_pad = 32;			/* 8, 16, 32 either XY or ZPixmap */
    window->offscreenBuffer.depth = 32;			     /* depth of image */
    window->offscreenBuffer.bits_per_pixel = 32;		/* bits per pixel (ZPixmap) */
    window->offscreenBuffer.red_mask = 0x00FF0000;	  /* bits in z arrangement */
    window->offscreenBuffer.green_mask = 0x0000FF00;
    window->offscreenBuffer.blue_mask = 0x000000FF;
    
    PfResizeWindow(window, width, height);
    
    int32 screenNum = XDefaultScreen(window->display);
    
    XVisualInfo visualInfo = {};
    visualInfo.screen = screenNum;
    visualInfo.depth = 32;
    visualInfo.c_class = TrueColor;
    visualInfo.bits_per_rgb = 8;
    long visualInfoMask = VisualScreenMask | VisualDepthMask | VisualClassMask | VisualBitsPerRGBMask;
    int32 possibleVisualsCount;
    XVisualInfo *possibleVisuals = XGetVisualInfo(window->display, visualInfoMask, &visualInfo, &possibleVisualsCount);
    Visual *chosenVisual = 0;
    if(possibleVisuals)
    {
        chosenVisual = possibleVisuals[0].visual;
        XFree((void*)possibleVisuals);
    }
    else
    {
        fprintf(stderr,"ERROR: Could not get a visual of our choice. LINE: %d, FUNCTION:%s, FILE:%s\n", __LINE__, __func__, __FILE__);
    }
    ASSERT(chosenVisual != 0);
    
    Window rootWindow = XDefaultRootWindow(window->display);
    Screen *screen = XScreenOfDisplay(window->display, screenNum);
    window->screen = screen;
    
    XSetWindowAttributes windowAttributes = {};
    
    /* NOTE(KARAN): For some reason border_pixel needs to be initialized.
     Maybe initializing border_pixel overrides the need of border_pixmap
     
Border_pixmap default is CopyFromParent
     In our case, parent and child dont have same depth, so that would cause an error
     
     But maybe defining the border_pixel gets rid of that call and hence everything seems to work ... ??? */
    windowAttributes.border_pixel = WhitePixel(window->display, screenNum);
    
    // NOTE(KARAN):If you set the colormap to CopyFromParent, the parent window's colormap is copied and used by its child. However, the child window must have the same visual type as the parent, or a BadMatch error results. 
    windowAttributes.colormap = XCreateColormap(window->display, rootWindow, chosenVisual, AllocNone);		
    
    windowAttributes.event_mask = StructureNotifyMask | EnterWindowMask | LeaveWindowMask | FocusChangeMask | KeyPressMask | KeyReleaseMask | ButtonPressMask | ButtonReleaseMask | ButtonMotionMask | PointerMotionMask; 
    
    unsigned long windowAttributesMask = CWBorderPixel | CWColormap | CWEventMask;
    
    if(chosenVisual)
    {
        window->windowHandle =XCreateWindow(window->display, rootWindow, xPos, yPos, width, height, 0, 32,InputOutput, chosenVisual, windowAttributesMask, &windowAttributes);
    }
    
    XStoreName(window->display, window->windowHandle, title);
    
    // TODO(KARAN): Should GC be global or per window?
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
    
    window->graphicsContext = XCreateGC(window->display, window->windowHandle, gcAttributesMask, &gcAttributes);
    
    Status status = XSetWMProtocols(globalDisplay, window->windowHandle, &globalWmDeleteWindowAtom, 1);
    ASSERT(status != 0);
    
    int32 saveContextResult = XSaveContext(window->display, window->windowHandle, globalXlibContext, (XPointer)window);
    ASSERT(saveContextResult == 0);
    
    XMapRaised(window->display, window->windowHandle);
}

PfRect PfGetClientRect(PfWindow *window)
{
    PfRect result = {};
    Window rootWindow;
    uint32 borderWidth;
    uint32 pixelDepth;
    Status successIndicator = XGetGeometry(window->display, window->windowHandle, &rootWindow, 
                                           &result.x, &result.y,
                                           (uint32*)&result.width, (uint32*)&result.height,
                                           &borderWidth, &pixelDepth);
    ASSERT(successIndicator != 0); 
    
    return result;
}

void PfGetOffscreenBuffer(PfWindow *window, PfOffscreenBuffer *offscreenBuffer)
{
    offscreenBuffer->data = window->offscreenBuffer.data;
    offscreenBuffer->width = window->offscreenBuffer.width;
    offscreenBuffer->height = window->offscreenBuffer.height;
    offscreenBuffer->bytesPerPixel = window->offscreenBuffer.bits_per_pixel/8;
    offscreenBuffer->pitch = window->offscreenBuffer.bytes_per_line;
}

void PfBlitToScreen(PfWindow *window)
{
    XPutImage(window->display, window->windowHandle, window->graphicsContext, &window->offscreenBuffer, 0, 0, 0, 0, window->offscreenBuffer.width, window->offscreenBuffer.height);
}

void PfToggleFullscreen(PfWindow *window)
{
    /* NOTE(KARAN): 
    
This spec helped in implementation of this function.
_NET_WM_STATE property has to be changed for modifying the window's fullscreen status.

https://standards.freedesktop.org/wm-spec/wm-spec-1.3.html#idm140130317598336

    */
    
    XEvent event = {};
    event.type = ClientMessage;
    event.xclient.window = window->windowHandle;
    event.xclient.format = 32;
    event.xclient.message_type = globalWmState;
    
    /* NOTE(KARAN): 
    _NET_WM_STATE_REMOVE        0     remove/unset property 
    _NET_WM_STATE_ADD           1     add/set property 
        _NET_WM_STATE_TOGGLE        2     toggle property  
*/
    event.xclient.data.l[0] = 2; // _NET_WM_STATE_TOGGLE
    event.xclient.data.l[1] = globalWmStateFullscreen;
    event.xclient.data.l[2] = 0; // No second property
    event.xclient.data.l[3] = 1; // Normal window
    
    Status result = XSendEvent(window->display,
                               DefaultRootWindow(window->display),
                               False,
                               SubstructureNotifyMask | SubstructureRedirectMask,
                               &event);
    ASSERT(result != 0);
}

int32 PfGetKeyState(int32 vkCode)
{
    int32 result;
    KeyCode keyCode = XKeysymToKeycode(globalDisplay, vkCode);
    result = globalKeyboard[keyCode];
    return result;
}

int32 PfGetKeyState(PfWindow *window, int32 vkCode)
{
    int32 result = 0;
    if(window->hasKeyboardFocus)
    {
        KeyCode keyCode = XKeysymToKeycode(window->display, vkCode);
        result = globalKeyboard[keyCode];
    }
    return result;
}



int32 PfGetMouseButtonState(PfWindow *window, int32 index)
{
    
    int32 result = 0;
    if(window->hasKeyboardFocus)
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
    Window rootWindowOfEventWindow;
    Window childWindowOfEventWindowContainingMouse;
    int32 rootWindowMouseX, rootWindowMouseY;
    uint32  buttonMask;
    
    int32 xQueryPointerResult = XQueryPointer(window->display, window->windowHandle, &rootWindowOfEventWindow ,&childWindowOfEventWindowContainingMouse, &rootWindowMouseX, &rootWindowMouseY,
                                              x, y, &buttonMask);
    ASSERT(xQueryPointerResult);
    
    
    if(*x < 0 || *x > window->offscreenBuffer.width)
    {
        result = false;
    }
    else if(*y < 0 || *y > window->offscreenBuffer.height)
    {
        result = false;
    }
    else
    {
        result = window->isWindowUnderMouse;
    }
    
    return result;
}


PfTimestamp PfGetTimestamp()
{
    PfTimestamp result = {};
    clock_gettime(CLOCK_MONOTONIC, &result);
    return result;
}

inline void HandleNanoSecondOverflow(PfTimestamp *timestamp)
{
    if (timestamp->tv_nsec >= 1000000000L)
    {
        timestamp->tv_nsec -= 1000000000L;       
        timestamp->tv_sec++;
    }
}

real32 PfGetSeconds(PfTimestamp startTime, PfTimestamp endTime)
{
    timespec result = {};
    if(endTime.tv_nsec < startTime.tv_nsec)
    {
        result.tv_sec = (endTime.tv_sec - 1) - startTime.tv_sec; // NOTE(KARAN): Borrowing from seconds part 
        result.tv_nsec = endTime.tv_nsec + 1000000000L - endTime.tv_nsec; 
    }
    else
    {
        result.tv_sec = endTime.tv_sec - startTime.tv_sec;
        result.tv_nsec = endTime.tv_nsec - startTime.tv_nsec; 
    }
    
    HandleNanoSecondOverflow(&result);
    
    real32 nanoSecondsToSeconds = ((real32)result.tv_nsec)/1000000000.0f;
    real32 seconds = ((real32)result.tv_sec) + nanoSecondsToSeconds;
    
    return seconds;
}

inline timespec AddTimespec(timespec t1, timespec t2)
{
    timespec result = {};
    result.tv_nsec = t1.tv_nsec + t2.tv_nsec;
    result.tv_sec = t1.tv_sec + t2.tv_sec;
    HandleNanoSecondOverflow(&result);
    return result;
}



#if defined(__i386__)

__inline__ uint64  PfRdtsc(void)
{
    uint64 x;
    __asm__ volatile (".byte 0x0f, 0x31" : "=A" (x));
    return x;
}

#elif defined(__x86_64__)

__inline__ uint64 PfRdtsc(void)
{
    unsigned hi, lo;
    __asm__ __volatile__ ("rdtsc" : "=a"(lo), "=d"(hi));
    return ( (uint64)lo)|( ((uint64)hi)<<32 );
}
#endif


void PfSetWindowTitle(PfWindow *window, char *title)
{
    XStoreName(window->display, window->windowHandle, title);
}