#include "linux_platform_interface.h"

#include <stdio.h>
#include <malloc.h>
#include <time.h>
#include <string.h>

#include <X11/Xresource.h>
#include "pf_opengl.h"
#include <GL/glx.h>

#include "gl_error_handler.h"

global_variable XContext globalXlibContext;
global_variable Display*  globalDisplay;
global_variable Screen*  globalScreen;
global_variable int32  globalScreenNum;

global_variable Atom globalWmDeleteWindowAtom;
global_variable Atom globalWmState;
global_variable Atom globalWmStateFullscreen;
global_variable int32 globalKeyboard[256];
global_variable int32 globalMouseButtons[5];
global_variable GLXFBConfig globalFBConfig;
global_variable Visual*  globalVisual;
global_variable bool globalIsModernOpenGLContextSupported;

global_variable int32 globalGLMajorVersion = 3;
global_variable int32 globalGLMinorVersion = 3;
global_variable bool globalCoreProfile       = false;

typedef GLXContext (*GLXCreateContextAttribsFuncType)(Display*, GLXFBConfig, GLXContext, Bool, const int*);
global_variable GLXCreateContextAttribsFuncType glXCreateContextAttribsARB;


int32 GetMaxSamplesFBConfigIndexAndVisual(Display *display, GLXFBConfig* fbConfigs, int32 fbConfigsCount, Visual **visual)
{
    // NOTE(KARAN): Copy pasted the source from this site: https://www.khronos.org/opengl/wiki/Tutorial:_OpenGL_3.0_Context_Creation_(GLX)
    
    int32 bestFbc = -1, bestNumSamp = -1;
    
    int32 i;
    for (i = 0; i < fbConfigsCount; ++i)
    {
        XVisualInfo *vi = glXGetVisualFromFBConfig( display, fbConfigs[i] );
        if(vi)
        {
            int32 sampBuf, samples;
            glXGetFBConfigAttrib( display, fbConfigs[i], GLX_SAMPLE_BUFFERS, &sampBuf );
            glXGetFBConfigAttrib( display, fbConfigs[i], GLX_SAMPLES       , &samples  );
            
            if (bestFbc < 0 || sampBuf && samples > bestNumSamp ) bestFbc = i, bestNumSamp = samples, *visual = vi[0].visual;
        }
        XFree(vi);
    }
    
    return bestFbc;
}


bool IsExtensionSupported(const char *extList, const char *extension)
{
    // NOTE(KARAN): Copy pasted the source from this site: https://www.khronos.org/opengl/wiki/Tutorial:_OpenGL_3.0_Context_Creation_(GLX)
    const char *start;
    const char *where, *terminator;
    
    /* Extension names should not have spaces. */
    where = strchr(extension, ' ');
    if (where || *extension == '\0')
        return false;
    
    /* It takes a bit of care to be fool-proof about parsing the
       OpenGL extensions string. Don't be fooled by sub-strings,
       etc. */
    for (start=extList;;) {
        where = strstr(start, extension);
        
        if (!where)
            break;
        
        terminator = where + strlen(extension);
        
        if ( where == start || *(where - 1) == ' ' )
            if ( *terminator == ' ' || *terminator == '\0' )
            return true;
        
        start = terminator;
    }
    
    return false;
}


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
    
    
    /************** NOTE(KARAN): Get OpenGL Frame Buffer Configuration and Modern Context Creation Extension   ***************/
    
    int32 glxMajorVersion, glxMinorVersion;
    Bool querySuccess =  glXQueryVersion(globalDisplay, &glxMajorVersion, &glxMinorVersion);
    
    // NOTE(KARAN): Minimum GLX VERSION 1.3 is required for frame buffer configurations(FBConfig) 
    if(!(querySuccess == True && glxMajorVersion >= 1 && glxMinorVersion >= 3))
    {
        DEBUG_ERROR("Minimum GLX version required is 1.3. Detected version: %d.%d", glxMajorVersion, glxMinorVersion);
    }
    
    // NOTE(KARAN) : GLX_PBUFFERBIT can also be specified for the GLX_DRAWABLE_TYPE  property.
    int32 desiredFBAttribs[] = 
    {
        GLX_X_RENDERABLE , True                           ,
        GLX_DRAWABLE_TYPE, GLX_WINDOW_BIT | GLX_PIXMAP_BIT,
        GLX_RENDER_TYPE  , GLX_RGBA_BIT                   ,
        GLX_X_VISUAL_TYPE, GLX_TRUE_COLOR                 ,
        GLX_RED_SIZE     , 8                              ,
        GLX_GREEN_SIZE   , 8                              ,
        GLX_BLUE_SIZE    , 8                              ,
        GLX_ALPHA_SIZE   , 8                              ,
        GLX_DEPTH_SIZE   , 24                             ,
        GLX_STENCIL_SIZE , 8                              ,
        GLX_DOUBLEBUFFER , True                           ,
        None
    };
    
    globalScreenNum = XDefaultScreen(globalDisplay);
    int32 screenNum = globalScreenNum;
    
    int32 possibleFBConfigsCount;
    GLXFBConfig* possibleFBConfigs = glXChooseFBConfig(globalDisplay, screenNum, desiredFBAttribs, &possibleFBConfigsCount);
    
    if(!possibleFBConfigs)
    {
        DEBUG_ERROR("No FBConfig satisfying our settings were found");
    }
    
    int32 choosenFBConfigIndex = GetMaxSamplesFBConfigIndexAndVisual(globalDisplay, possibleFBConfigs, possibleFBConfigsCount, &globalVisual);
    
    globalFBConfig = possibleFBConfigs[choosenFBConfigIndex];
    
    XFree(possibleFBConfigs);
    
    const char *glxExtensionsString = glXQueryExtensionsString(globalDisplay, screenNum);
    glXCreateContextAttribsARB = (GLXCreateContextAttribsFuncType)
        glXGetProcAddressARB( (const GLubyte *) "glXCreateContextAttribsARB" );
    
    globalIsModernOpenGLContextSupported = IsExtensionSupported(glxExtensionsString, "GLX_ARB_create_context") && glXCreateContextAttribsARB;
    
    
    /*********** Loading Modern OpenGL functions *******/
    GrabOpenGLFuncPointers();
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
        
        glXMakeCurrent(window->display, window->windowHandle, window->glxContext);
        GL_CALL(glViewport(0, 0, width, height));
        
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
    window->screen = globalScreen;
    window->screenNum = globalScreenNum;
    
    if(window->display == 0)
    {
        fprintf(stderr, "ERROR:Cannot connect to XLib. LINE: %d, FUNCTION: %s, FILE:%s\n", __LINE__, __func__, __FILE__);
    }
    
#if SLOW_BUILD
    XSynchronize(window->display, true);
#endif
    
    int32 screenNum = window->screenNum;
    Screen *screen = window->screen;
    
    /* NOTE(KARAN): Creation of offscreen buffer
    * Memory format =  Address 0:BB
    *                  Address 1:GG
    *                  Address 2:RR
    *                  Address 3:AA
    * uint32 format =  MSB ----> LSB
    *                  0xAA RR GG BB
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
    window->offscreenBuffer.depth = 24;			     /* depth of image */
    window->offscreenBuffer.bits_per_pixel = 32;		/* bits per pixel (ZPixmap) */
    window->offscreenBuffer.red_mask = 0x00FF0000;	  /* bits in z arrangement */
    window->offscreenBuffer.green_mask = 0x0000FF00;
    window->offscreenBuffer.blue_mask = 0x000000FF;
    
    PfResizeWindow(window, width, height);
    
    
    Visual *chosenVisual = 0;
    /*
    XVisualInfo visualInfo = {};
    visualInfo.screen = screenNum;
    visualInfo.depth = 24;
    visualInfo.c_class = TrueColor;
    visualInfo.bits_per_rgb = 8;
    long visualInfoMask = VisualScreenMask | VisualDepthMask | VisualClassMask | VisualBitsPerRGBMask;
    int32 possibleVisualsCount;
    
    XVisualInfo *possibleVisuals = glXGetVisualFromFBConfig(window->display, choosenFBConfig );//XGetVisualInfo(window->display, visualInfoMask, &visualInfo, &possibleVisualsCount);
    if(possibleVisuals)
    {
        chosenVisual = possibleVisuals[0].visual;
        XFree((void*)possibleVisuals);
    }
    else
    {
        fprintf(stderr,"ERROR: Could not get a visual of our choice. LINE: %d, FUNCTION:%s, FILE:%s\n", __LINE__, __func__, __FILE__);
    }*/
    
    chosenVisual = globalVisual;
    ASSERT(chosenVisual != 0);
    
    Window rootWindow = XDefaultRootWindow(window->display);
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
    
    if(globalVisual)
    {
        window->windowHandle =XCreateWindow(window->display, rootWindow, xPos, yPos, width, height, 0, 24,InputOutput, chosenVisual, windowAttributesMask, &windowAttributes);
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
    
    window->glxContext = 0;
    /*
NOTE(KARAN): GLX_ARB_create_context is an extension in GLX which enables the user to create modern OpenGL  contexts.
 * If this extension exists, we create a modern OpenGL context, else we defer to the default context creation.*/
    
    if(globalIsModernOpenGLContextSupported)
    {
        /* NOTE(KARAN): Deatils about context creation 
* https://www.khronos.org/registry/OpenGL/extensions/ARB/GLX_ARB_create_context.txt
*
    * GLX_CONTEXT_MAJOR_VERSION_ARB           
         * GLX_CONTEXT_MINOR_VERSION_ARB           
        * GLX_CONTEXT_FLAGS_ARB          Possible values: GLX_CONTEXT_DEBUG_BIT_ARB, 
*                                                 GLX_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB
        * GLX_CONTEXT_PROFILE_MASK_ARB   Possible values: GLX_CONTEXT_CORE_PROFILE_BIT_ARB,  
*                                                 GLX_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB
        */
        int32 profile = GLX_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB;
        if(globalCoreProfile)
        {
            profile = GLX_CONTEXT_CORE_PROFILE_BIT_ARB;
        }
        
        int32 desiredContextAttribs[] =
        {
            GLX_CONTEXT_MAJOR_VERSION_ARB  , globalGLMajorVersion,
            GLX_CONTEXT_MINOR_VERSION_ARB  , globalGLMinorVersion,
            GLX_CONTEXT_FLAGS_ARB        , 
#if DEBUG_BUILD
            GLX_CONTEXT_DEBUG_BIT_ARB
#else
                0
#endif
                ,
            GLX_CONTEXT_PROFILE_MASK_ARB   , globalCoreProfile,
            None
        };
        
        window->glxContext = glXCreateContextAttribsARB(window->display, globalFBConfig, 0,
                                                        True, desiredContextAttribs);
    }
    else
    {
        window->glxContext = glXCreateNewContext(window->display, globalFBConfig, GLX_RGBA_TYPE, 0, True);
    }
    
    glXMakeCurrent(window->display, window->windowHandle, window->glxContext);
    
    DEBUG_LOG(stdout, "OpenGL version: %s\n\n", glGetString(GL_VERSION));
    glViewport(0, 0, width, height);
    
    GL_CALL(glGenTextures(1, &window->offscreenBufferTextureId));
    GL_CALL(glBindTexture(GL_TEXTURE_2D, window->offscreenBufferTextureId));
    
    GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST));
    GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST));
    GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER));
    GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER));
    GLfloat debugColor[] = {1.0f, 0.0f, 1.0f, 1.0f};
    GL_CALL(glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, debugColor));
    
    GL_CALL(glEnable(GL_BLEND));
    GL_CALL(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));
    
    GL_CALL(glBindTexture(GL_TEXTURE_2D, 0));
    
    real32 xMax = 1.0f;
    // TODO(KARAN): Flip XImage so that yMax can be set to 1.0f
    real32 yMax = -1.0f;
    
    real32 vertices[] = 
    {
        // positions          // texture coords
        xMax,  yMax, 0.0f,   1.0f, 1.0f, // top right
        xMax, -yMax, 0.0f,   1.0f, 0.0f, // bottom right
        -xMax, -yMax, 0.0f,   0.0f, 0.0f, // bottom left
        -xMax,  yMax, 0.0f,   0.0f, 1.0f  // top left 
    };
    uint32 indices[] = 
    {
        0, 1, 3, // first triangle
        1, 2, 3  // second triangle
    };
    
    uint32 vbo, vao, ebo;
    GL_CALL(glGenVertexArrays(1, &vao));
    GL_CALL(glGenBuffers(1, &vbo));
    GL_CALL(glGenBuffers(1, &ebo));
    
    window->vao = vao;
    
    glBindVertexArray(vao);
    
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
    
    // position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    // texture coord attribute
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    
    char *vertexShaderSource = "#version 330 core\nlayout (location = 0) in vec3 aPos;layout (location = 1) in vec2 aTexCoord;out vec3 ourColor; out vec2 TexCoord; void main() { gl_Position = vec4(aPos, 1.0); ourColor = vec3(1.0f, 1.0f, 1.0f); TexCoord = vec2(aTexCoord.x, aTexCoord.y);}";
    char *fragmentShaderSource = "#version 330 core\nout vec4 FragColor; in vec3 ourColor; in vec2 TexCoord; uniform sampler2D texture1; void main(){FragColor = texture(texture1, TexCoord);}";
    
    uint32 vertexShader;
    GL_CALL(vertexShader = glCreateShader(GL_VERTEX_SHADER));
    GL_CALL(glShaderSource(vertexShader, 1, &vertexShaderSource, 0));
    GL_CALL(glCompileShader(vertexShader));
    
    int32 success;
    char infoLog[512];
    GL_CALL(glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success));
    if (!success)
    {
        GL_CALL(glGetShaderInfoLog(vertexShader, 512, NULL, infoLog));
        DEBUG_ERROR("%s", infoLog);
    }
    
    
    uint32 fragmentShader;
    GL_CALL(fragmentShader = glCreateShader(GL_FRAGMENT_SHADER));
    GL_CALL(glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL));
    GL_CALL(glCompileShader(fragmentShader));
    
    GL_CALL(glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success));
    if (!success)
    {
        GL_CALL(glGetShaderInfoLog(vertexShader, 512, NULL, infoLog));
        DEBUG_ERROR("%s", infoLog);
    }
    
    uint32 shaderProgram;
    GL_CALL(shaderProgram = glCreateProgram());
    window->programId = shaderProgram;
    
    GL_CALL(glAttachShader(shaderProgram, vertexShader));
    GL_CALL(glAttachShader(shaderProgram, fragmentShader));
    GL_CALL(glLinkProgram(shaderProgram));
    
    GL_CALL(glDeleteShader(vertexShader));
    GL_CALL(glDeleteShader(fragmentShader));  
    
    glBindVertexArray(0);
    
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



void PfglRenderWindow(PfWindow *window)
{
    GL_CALL(glUseProgram(window->programId));
    
    //GL_CALL(glEnable(GL_TEXTURE_2D));
    GL_CALL(glActiveTexture(GL_TEXTURE0));
    GL_CALL(glBindTexture(GL_TEXTURE_2D, window->offscreenBufferTextureId));
    GL_CALL(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, window[0].offscreenBuffer.width, window[0].offscreenBuffer.height, 0,
                         GL_BGRA, GL_UNSIGNED_BYTE, window[0].offscreenBuffer.data));
    
    // render container
    GL_CALL(glBindVertexArray(window->vao));
    GL_CALL(glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0));
    GL_CALL(glUseProgram(0));
    GL_CALL(glBindVertexArray(0));
#if 0
    glEnable(GL_TEXTURE_2D);
    
    GL_CALL(glBindTexture(GL_TEXTURE_2D, window->offscreenBufferTextureId));
    GL_CALL(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, window[0].offscreenBuffer.width, window[0].offscreenBuffer.height, 0,
                         GL_BGRA, GL_UNSIGNED_BYTE, window[0].offscreenBuffer.data));
    
    
    real32 xMax = 1.0f;
    // TODO(KARAN): Flip XImage so that yMax can be set to 1.0f
    real32 yMax = -1.0f;
    
    GL_CALL(glBegin(GL_TRIANGLES));
    
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    
    // Lower triangle
    GL_CALL(glTexCoord2f(0.0f, 0.0f));
    GL_CALL(glVertex2f(-xMax, -yMax));
    
    GL_CALL(glTexCoord2f(1.0f, 0.0f));
    GL_CALL(glVertex2f(xMax, -yMax));
    
    GL_CALL(glTexCoord2f(1.0f, 1.0f));
    GL_CALL(glVertex2f(xMax, yMax));
    
    // Upper triangle
    GL_CALL(glTexCoord2f(0.0f, 0.0f));
    GL_CALL(glVertex2f(-xMax, -yMax));
    
    GL_CALL(glTexCoord2f(1.0f, 1.0f));
    GL_CALL(glVertex2f(xMax, yMax));
    
    GL_CALL(glTexCoord2f(0.0f, 1.0f));
    GL_CALL(glVertex2f(-xMax, yMax));
    
    glEnd();
    
    GL_CALL(glBindTexture(GL_TEXTURE_2D, 0));
    glDisable(GL_TEXTURE_2D);
#endif
}

void PfglMakeCurrent(PfWindow *window)
{
    if(window)
    {
        glXMakeCurrent(window->display, window->windowHandle, window->glxContext);
    }
    // TODO(KARAN): Not sure how to unbind context
    
}


void PfGLConfig(int32 glMajorVersion, int32 glMinorVersion, bool coreProfile)
{
    globalGLMajorVersion = glMajorVersion;
    globalGLMinorVersion = glMinorVersion;
    globalCoreProfile = coreProfile;
}

void PfglSwapBuffers(PfWindow *window)
{
    glXSwapBuffers(window->display, window->windowHandle);
}