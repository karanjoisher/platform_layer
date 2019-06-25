#include "linux_platform_interface.h"

#include <stdio.h>
#include <errno.h>
#include <sys/stat.h> 
#include <fcntl.h>
#include <unistd.h>
#include <malloc.h>
#include <time.h>
#include <string.h>
#include <sys/mman.h>
#include <X11/Xresource.h>
#include <X11/XKBlib.h>
#include "pf_opengl.h"
#include <GL/glx.h>

#if defined(PF_WINDOW_AND_INPUT)

global_variable XContext globalXlibContext;
global_variable Display*  globalDisplay;
global_variable Screen*  globalScreen;
global_variable int32  globalScreenNum;

global_variable Atom globalWmDeleteWindowAtom;
global_variable Atom globalWmState;
global_variable Atom globalWmStateFullscreen;
global_variable int32 globalKeyboard[2][256];
global_variable int32 globalMouseButtons[5];
global_variable GLXFBConfig globalFBConfig;
global_variable Visual*  globalVisual;
global_variable bool globalIsModernOpenGLContextSupported;
global_variable bool globalXkbSupported;
global_variable int globalX11KeyCodesToPositionCodeMap[256];
global_variable int globalX11KeyCodesToPfVkCodeMap[256];

global_variable int32 globalGLMajorVersion = 3;
global_variable int32 globalGLMinorVersion = 3;
global_variable bool globalCoreProfile       = false;

typedef GLXContext (*GLXCreateContextAttribsFuncType)(Display*, GLXFBConfig, GLXContext, Bool, const int*);
typedef int (*GLXSwapIntervalMESAFuncType)(unsigned int interval);
global_variable GLXCreateContextAttribsFuncType glXCreateContextAttribsARB;
// TODO(KARAN): Need to check whether extensions are supported before using GetProcAddress, There are 3 types of SwapIntervals, need to query which one is available.
global_variable GLXSwapIntervalMESAFuncType glXSwapInterval;

void DebugLinuxPrintKeyboardMapping(Display *display)
{
#if DEBUG_BUILD
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
                    fprintf(stdout, "%lu: %s, ", a, str);
                }
            }
        }
        fprintf(stdout, "\n");
    }
    XFree(keyCodeToKeySyms);
#endif
}

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
#endif


void PfInitialize()
{
#if defined(PF_WINDOW_AND_INPUT)
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
    // TODO(KARAN): Need to check whether extensions are supported before using GetProcAddress, There are 3 types of SwapIntervals, need to query which one is available.
    
    int32 choosenFBConfigIndex = GetMaxSamplesFBConfigIndexAndVisual(globalDisplay, possibleFBConfigs, possibleFBConfigsCount, &globalVisual);
    
    globalFBConfig = possibleFBConfigs[choosenFBConfigIndex];
    
    XFree(possibleFBConfigs);
    
    const char *glxExtensionsString = glXQueryExtensionsString(globalDisplay, screenNum);
    glXCreateContextAttribsARB = (GLXCreateContextAttribsFuncType)glXGetProcAddressARB( (const GLubyte *) "glXCreateContextAttribsARB" );
    
    glXSwapInterval = (GLXSwapIntervalMESAFuncType)(glXGetProcAddress( (const GLubyte *) "glXSwapIntervalMESA" ));
    
    globalIsModernOpenGLContextSupported = IsExtensionSupported(glxExtensionsString, "GLX_ARB_create_context") && glXCreateContextAttribsARB;
    
    
    /*********** Loading Modern OpenGL functions *******/
    GrabOpenGLFuncPointers();
    
    
    /*** XKB entension initialization ***/
    
    int32 major = XkbMajorVersion;
    int32 minor = XkbMinorVersion;
    
    globalXkbSupported = XkbLibraryVersion(&major, &minor);
    
    if(globalXkbSupported)
    {
        int32 majorExtension;
        int32 eventExtension;
        int32 errorExtension;
        globalXkbSupported = XkbQueryExtension
            (globalDisplay, &majorExtension, &eventExtension, &errorExtension, &major, &minor);
    }
    
    
    if(globalXkbSupported)
    {
        DEBUG_LOG("Xkb supported. Version: %d.%d\n", major, minor);
    }
    else
    {
        DEBUG_LOG("Xkb not supported\n");
    }
    
#if 1
    /*** Scan code to PfCodes mapping ***/
    // If xkb is supported
    // 1. Get the keyboard mappings
    // 2. Map the keycodes to PfCodes using their keynames
    bool x11KeyCodesToPositionCodeMapSet = false;
    if(globalXkbSupported)
    {
        char keyPositionName[XkbKeyNameLength + 1] = {};
        XkbDescPtr kbDesc = XkbGetMap(globalDisplay, 0, XkbUseCoreKbd);
        ASSERT(kbDesc, "Failed to get Xkb keyboard description");
        
        Status success = XkbGetNames(globalDisplay, XkbKeyNamesMask, kbDesc);
        ASSERT(success == Success, "Failed to get Xkb keyboard  scancode=>identifier mapping");
        
        for(int32 scanCode = kbDesc->min_key_code; scanCode <= kbDesc->max_key_code; scanCode++)
        {
            PfKeyCode pfKeyCode = PF_NULL;
            
            Copy(keyPositionName, kbDesc->names->keys[scanCode].name, XkbKeyNameLength);
            if(AreStringsSame(keyPositionName, "ESC")) pfKeyCode = PF_ESC;
            else if(AreStringsSame(keyPositionName, "FK01")) pfKeyCode = PF_F1;
            else if(AreStringsSame(keyPositionName, "FK02")) pfKeyCode = PF_F2;
            else if(AreStringsSame(keyPositionName, "FK03")) pfKeyCode = PF_F3;
            else if(AreStringsSame(keyPositionName, "FK04")) pfKeyCode = PF_F4;
            else if(AreStringsSame(keyPositionName, "FK05")) pfKeyCode = PF_F5;
            else if(AreStringsSame(keyPositionName, "FK06")) pfKeyCode = PF_F6;
            else if(AreStringsSame(keyPositionName, "FK07")) pfKeyCode = PF_F7;
            else if(AreStringsSame(keyPositionName, "FK08")) pfKeyCode = PF_F8;
            else if(AreStringsSame(keyPositionName, "FK09")) pfKeyCode = PF_F9;
            else if(AreStringsSame(keyPositionName, "FK10")) pfKeyCode = PF_F10;
            else if(AreStringsSame(keyPositionName, "FK11")) pfKeyCode = PF_F11;
            else if(AreStringsSame(keyPositionName, "FK12")) pfKeyCode = PF_F12;
            else if(AreStringsSame(keyPositionName, "PRSC")) pfKeyCode = PF_PRINT_SCREEN;
            else if(AreStringsSame(keyPositionName, "SCLK")) pfKeyCode = PF_SCROLL_LOCK;
            else if(AreStringsSame(keyPositionName, "PAUS")) pfKeyCode = PF_PAUSE;
            else if(AreStringsSame(keyPositionName, "PAUS")) pfKeyCode = PF_PAUSE;
            
            else if(AreStringsSame(keyPositionName, "TLDE")) pfKeyCode = PF_TILDE;
            else if(AreStringsSame(keyPositionName, "AE01")) pfKeyCode = PF_1;
            else if(AreStringsSame(keyPositionName, "AE02")) pfKeyCode = PF_2;
            else if(AreStringsSame(keyPositionName, "AE03")) pfKeyCode = PF_3;
            else if(AreStringsSame(keyPositionName, "AE04")) pfKeyCode = PF_4;
            else if(AreStringsSame(keyPositionName, "AE05")) pfKeyCode = PF_5;
            else if(AreStringsSame(keyPositionName, "AE06")) pfKeyCode = PF_6;
            else if(AreStringsSame(keyPositionName, "AE07")) pfKeyCode = PF_7;
            else if(AreStringsSame(keyPositionName, "AE08")) pfKeyCode = PF_8;
            else if(AreStringsSame(keyPositionName, "AE09")) pfKeyCode = PF_9;
            else if(AreStringsSame(keyPositionName, "AE10")) pfKeyCode = PF_0;
            else if(AreStringsSame(keyPositionName, "AE11")) pfKeyCode = PF_MINUS;
            else if(AreStringsSame(keyPositionName, "AE12")) pfKeyCode = PF_EQUALS;
            else if(AreStringsSame(keyPositionName, "BKSP")) pfKeyCode = PF_BACKSPACE;
            
            else if(AreStringsSame(keyPositionName, "TAB")) pfKeyCode = PF_TAB;
            else if(AreStringsSame(keyPositionName, "AD01")) pfKeyCode = PF_Q;
            else if(AreStringsSame(keyPositionName, "AD02")) pfKeyCode = PF_W;
            else if(AreStringsSame(keyPositionName, "AD03")) pfKeyCode = PF_E;
            else if(AreStringsSame(keyPositionName, "AD04")) pfKeyCode = PF_R;
            else if(AreStringsSame(keyPositionName, "AD05")) pfKeyCode = PF_T;
            else if(AreStringsSame(keyPositionName, "AD06")) pfKeyCode = PF_Y;
            else if(AreStringsSame(keyPositionName, "AD07")) pfKeyCode = PF_U;
            else if(AreStringsSame(keyPositionName, "AD08")) pfKeyCode = PF_I;
            else if(AreStringsSame(keyPositionName, "AD09")) pfKeyCode = PF_O;
            else if(AreStringsSame(keyPositionName, "AD10")) pfKeyCode = PF_P;
            else if(AreStringsSame(keyPositionName, "AD11")) pfKeyCode = PF_OPEN_SQUARE_BRACKET;
            else if(AreStringsSame(keyPositionName, "AD12")) pfKeyCode = PF_CLOSE_SQUARE_BRACKET;
            else if(AreStringsSame(keyPositionName, "BKSL")) pfKeyCode = PF_BACKSLASH;
            
            else if(AreStringsSame(keyPositionName, "CAPS")) pfKeyCode = PF_CAPS_LOCK;
            else if(AreStringsSame(keyPositionName, "AC01")) pfKeyCode = PF_A;
            else if(AreStringsSame(keyPositionName, "AC02")) pfKeyCode = PF_S;
            else if(AreStringsSame(keyPositionName, "AC03")) pfKeyCode = PF_D;
            else if(AreStringsSame(keyPositionName, "AC04")) pfKeyCode = PF_F;
            else if(AreStringsSame(keyPositionName, "AC05")) pfKeyCode = PF_G;
            else if(AreStringsSame(keyPositionName, "AC06")) pfKeyCode = PF_H;
            else if(AreStringsSame(keyPositionName, "AC07")) pfKeyCode = PF_J;
            else if(AreStringsSame(keyPositionName, "AC08")) pfKeyCode = PF_K;
            else if(AreStringsSame(keyPositionName, "AC09")) pfKeyCode = PF_L;
            else if(AreStringsSame(keyPositionName, "AC10")) pfKeyCode = PF_SEMICOLON;
            else if(AreStringsSame(keyPositionName, "AC11")) pfKeyCode = PF_APOSTROPHE;
            else if(AreStringsSame(keyPositionName, "RTRN")) pfKeyCode = PF_ENTER;
            
            else if(AreStringsSame(keyPositionName, "LFSH")) pfKeyCode = PF_LEFT_SHIFT;
            else if(AreStringsSame(keyPositionName, "AB01")) pfKeyCode = PF_Z;
            else if(AreStringsSame(keyPositionName, "AB02")) pfKeyCode = PF_X;
            else if(AreStringsSame(keyPositionName, "AB03")) pfKeyCode = PF_C;
            else if(AreStringsSame(keyPositionName, "AB04")) pfKeyCode = PF_V;
            else if(AreStringsSame(keyPositionName, "AB05")) pfKeyCode = PF_B;
            else if(AreStringsSame(keyPositionName, "AB06")) pfKeyCode = PF_N;
            else if(AreStringsSame(keyPositionName, "AB07")) pfKeyCode = PF_M;
            else if(AreStringsSame(keyPositionName, "AB08")) pfKeyCode = PF_COMMA;
            else if(AreStringsSame(keyPositionName, "AB09")) pfKeyCode = PF_PERIOD;
            else if(AreStringsSame(keyPositionName, "AB10")) pfKeyCode = PF_FORWARD_SLASH;
            else if(AreStringsSame(keyPositionName, "RTSH")) pfKeyCode = PF_RIGHT_SHIFT;
            
            else if(AreStringsSame(keyPositionName, "LCTL")) pfKeyCode = PF_LEFT_CTRL;
            else if(AreStringsSame(keyPositionName, "LWIN")) pfKeyCode = PF_LEFT_WIN;
            else if(AreStringsSame(keyPositionName, "LALT")) pfKeyCode = PF_LEFT_ALT;
            else if(AreStringsSame(keyPositionName, "SPCE")) pfKeyCode = PF_SPACEBAR;
            else if(AreStringsSame(keyPositionName, "RALT")) pfKeyCode = PF_RIGHT_ALT;
            else if(AreStringsSame(keyPositionName, "RWIN")) pfKeyCode = PF_RIGHT_WIN;
            else if(AreStringsSame(keyPositionName, "MENU")) pfKeyCode = PF_MENU;
            else if(AreStringsSame(keyPositionName, "RCTL")) pfKeyCode = PF_RIGHT_CTRL;
            
            else if(AreStringsSame(keyPositionName, "INS")) pfKeyCode = PF_INSERT;
            else if(AreStringsSame(keyPositionName, "HOME")) pfKeyCode = PF_HOME;
            else if(AreStringsSame(keyPositionName, "PGUP")) pfKeyCode = PF_PAGE_UP;
            else if(AreStringsSame(keyPositionName, "DELE")) pfKeyCode = PF_DELETE;
            else if(AreStringsSame(keyPositionName, "END")) pfKeyCode = PF_END;
            else if(AreStringsSame(keyPositionName, "PGDN")) pfKeyCode = PF_PAGE_DOWN;
            
            else if(AreStringsSame(keyPositionName, "NMLK")) pfKeyCode = PF_NUM_LOCK;
            else if(AreStringsSame(keyPositionName, "KPDV")) pfKeyCode = PF_NUMPAD_DIVIDE;
            else if(AreStringsSame(keyPositionName, "KPMU")) pfKeyCode = PF_NUMPAD_MULTIPLY;
            else if(AreStringsSame(keyPositionName, "KPSU")) pfKeyCode = PF_NUMPAD_MINUS;
            else if(AreStringsSame(keyPositionName, "KP7")) pfKeyCode = PF_NUMPAD_7;
            else if(AreStringsSame(keyPositionName, "KP8")) pfKeyCode = PF_NUMPAD_8;
            else if(AreStringsSame(keyPositionName, "KP9")) pfKeyCode = PF_NUMPAD_9;
            else if(AreStringsSame(keyPositionName, "KPAD")) pfKeyCode = PF_NUMPAD_PLUS;
            else if(AreStringsSame(keyPositionName, "KP4")) pfKeyCode = PF_NUMPAD_4;
            else if(AreStringsSame(keyPositionName, "KP5")) pfKeyCode = PF_NUMPAD_5;
            else if(AreStringsSame(keyPositionName, "KP6")) pfKeyCode = PF_NUMPAD_6;
            else if(AreStringsSame(keyPositionName, "KP1")) pfKeyCode = PF_NUMPAD_1;
            else if(AreStringsSame(keyPositionName, "KP2")) pfKeyCode = PF_NUMPAD_2;
            else if(AreStringsSame(keyPositionName, "KP3")) pfKeyCode = PF_NUMPAD_3;
            else if(AreStringsSame(keyPositionName, "KP0")) pfKeyCode = PF_NUMPAD_0;
            else if(AreStringsSame(keyPositionName, "KPDL")) pfKeyCode = PF_NUMPAD_PERIOD;
            else if(AreStringsSame(keyPositionName, "KPEN")) pfKeyCode = PF_NUMPAD_ENTER;
            
            
            globalX11KeyCodesToPositionCodeMap[scanCode] = pfKeyCode;
        }
        XkbFreeNames(kbDesc, XkbKeyNamesMask, True);
        XkbFreeKeyboard(kbDesc, 0, True);
        x11KeyCodesToPositionCodeMapSet = true;
    }
    
    int minKeyCode = 8;
    int maxKeyCode = 255;
    
    int keySymsPerKeyCode;
    KeySym *keyCodeToKeySyms = XGetKeyboardMapping(globalDisplay, minKeyCode, maxKeyCode - minKeyCode + 1, &keySymsPerKeyCode);
    
    int indexIntoTable = 0;
    for(int i = minKeyCode; i <= maxKeyCode; i++)
    {
        PfKeyCode pfKeyName = PF_NULL;
        int keySymIndex = 0;
        //  index =  row number      * row width         + column number; 
        int index = (i - minKeyCode) * keySymsPerKeyCode + keySymIndex;
        
        KeySym keySym = keyCodeToKeySyms[index];
        switch(keySym)
        {
            case XK_Escape:         pfKeyName = PF_EQUALS;break;
            case XK_F1:             pfKeyName = PF_F1;break;
            case XK_F2:             pfKeyName = PF_F2;break;
            case XK_F3:             pfKeyName = PF_F3;break;
            case XK_F4:             pfKeyName = PF_F4;break;
            case XK_F5:             pfKeyName = PF_F5;break;
            case XK_F6:             pfKeyName = PF_F6;break;
            case XK_F7:             pfKeyName = PF_F7;break;
            case XK_F8:             pfKeyName = PF_F8;break;
            case XK_F9:             pfKeyName = PF_F9;break;
            case XK_F10:            pfKeyName = PF_F10;break;
            case XK_F11:            pfKeyName = PF_F11;break;
            case XK_F12:            pfKeyName = PF_F12;break;
            case XK_Print:          pfKeyName = PF_PRINT_SCREEN;break;
            case XK_Scroll_Lock:    pfKeyName = PF_SCROLL_LOCK;break;
            case XK_Pause:          pfKeyName = PF_PAUSE;break;
            
            case XK_grave:          pfKeyName = PF_TILDE;break;
            case XK_1:              pfKeyName = PF_1;break;
            case XK_2:              pfKeyName = PF_2;break;
            case XK_3:              pfKeyName = PF_3;break;
            case XK_4:              pfKeyName = PF_4;break;
            case XK_5:              pfKeyName = PF_5;break;
            case XK_6:              pfKeyName = PF_6;break;
            case XK_7:              pfKeyName = PF_7;break;
            case XK_8:              pfKeyName = PF_8;break;
            case XK_9:              pfKeyName = PF_9;break;
            case XK_0:              pfKeyName = PF_0;break;
            case XK_minus:          pfKeyName = PF_MINUS;break;
            case XK_equal:          pfKeyName = PF_EQUALS;break;
            case XK_BackSpace:      pfKeyName = PF_BACKSPACE;break;
            
            case XK_Tab:            pfKeyName = PF_TAB;break;
            case XK_q:              pfKeyName = PF_Q;break;
            case XK_w:              pfKeyName = PF_W;break;
            case XK_e:              pfKeyName = PF_E;break;
            case XK_r:              pfKeyName = PF_R;break;
            case XK_t:              pfKeyName = PF_T;break;
            case XK_y:              pfKeyName = PF_Y;break;
            case XK_u:              pfKeyName = PF_U;break;
            case XK_i:              pfKeyName = PF_I;break;
            case XK_o:              pfKeyName = PF_O;break;
            case XK_p:              pfKeyName = PF_P;break;
            case XK_bracketleft:    pfKeyName = PF_OPEN_SQUARE_BRACKET;break;
            case XK_bracketright:   pfKeyName = PF_CLOSE_SQUARE_BRACKET;break;
            case XK_backslash:      pfKeyName = PF_BACKSLASH;break;
            
            case XK_Caps_Lock:      pfKeyName = PF_CAPS_LOCK;break;
            case XK_a:              pfKeyName = PF_A;break;
            case XK_s:              pfKeyName = PF_S;break;
            case XK_d:              pfKeyName = PF_D;break;
            case XK_f:              pfKeyName = PF_F;break;
            case XK_g:              pfKeyName = PF_G;break;
            case XK_h:              pfKeyName = PF_H;break;
            case XK_j:              pfKeyName = PF_J;break;
            case XK_k:              pfKeyName = PF_K;break;
            case XK_l:              pfKeyName = PF_L;break;
            case XK_semicolon:      pfKeyName = PF_SEMICOLON;break;
            case XK_apostrophe:     pfKeyName = PF_APOSTROPHE;break;
            case XK_Return:         pfKeyName = PF_ENTER;break;
            
            case XK_Shift_L:        pfKeyName = PF_LEFT_SHIFT;break;
            case XK_z:              pfKeyName = PF_Z;break;
            case XK_x:              pfKeyName = PF_X;break;
            case XK_c:              pfKeyName = PF_C;break;
            case XK_v:              pfKeyName = PF_V;break;
            case XK_b:              pfKeyName = PF_B;break;
            case XK_n:              pfKeyName = PF_N;break;
            case XK_m:              pfKeyName = PF_M;break;
            case XK_comma:          pfKeyName = PF_COMMA;break;
            case XK_period:         pfKeyName = PF_PERIOD;break;
            case XK_slash:          pfKeyName = PF_FORWARD_SLASH;break;
            case XK_Shift_R:        pfKeyName = PF_RIGHT_SHIFT;break;
            
            case XK_Control_L:      pfKeyName = PF_LEFT_CTRL;break;
            case XK_Super_L:        pfKeyName = PF_LEFT_WIN;break;
            case XK_Meta_L:
            case XK_Alt_L:          pfKeyName = PF_LEFT_ALT;break;
            case XK_space:          pfKeyName = PF_SPACEBAR;break;
            case XK_Mode_switch: 
            case XK_ISO_Level3_Shift:
            case XK_Meta_R:
            case XK_Alt_R:          pfKeyName = PF_RIGHT_ALT;break;
            case XK_Super_R:        pfKeyName = PF_RIGHT_WIN;break;
            case XK_Menu:           pfKeyName = PF_MENU;break;
            case XK_Control_R:      pfKeyName = PF_RIGHT_CTRL;break;
            
            case XK_Insert:         pfKeyName = PF_INSERT;break;
            case XK_Home:           pfKeyName = PF_HOME;break;
            case XK_Page_Up:        pfKeyName = PF_PAGE_UP;break;
            case XK_Delete:         pfKeyName = PF_DELETE;break;
            case XK_End:            pfKeyName = PF_END;break;
            case XK_Page_Down:      pfKeyName = PF_PAGE_DOWN;break;
            
            case XK_Up:             pfKeyName = PF_UP;break;
            case XK_Down:           pfKeyName = PF_DOWN;break;
            case XK_Left:           pfKeyName = PF_LEFT;break;
            case XK_Right:          pfKeyName = PF_RIGHT;break;
            
            case XK_Num_Lock:       pfKeyName = PF_NUM_LOCK;break;
            case XK_KP_Divide:      pfKeyName = PF_NUMPAD_DIVIDE;break;
            case XK_KP_Multiply:    pfKeyName = PF_NUMPAD_MULTIPLY;break;
            case XK_KP_Subtract:    pfKeyName = PF_NUMPAD_MINUS;break;
            
            case XK_KP_Home:        pfKeyName = PF_NUMPAD_7;break;
            case XK_KP_Up:          pfKeyName = PF_NUMPAD_8;break;
            case XK_KP_Page_Up:     pfKeyName = PF_NUMPAD_9;break;
            case XK_KP_Add:         pfKeyName = PF_NUMPAD_PLUS;break;
            case XK_KP_Left:        pfKeyName = PF_NUMPAD_4;break;
            case XK_KP_Begin:       pfKeyName = PF_NUMPAD_5;break;
            case XK_KP_Right:       pfKeyName = PF_NUMPAD_6;break;
            case XK_KP_End:         pfKeyName = PF_NUMPAD_1;break;
            case XK_KP_Down:        pfKeyName = PF_NUMPAD_2;break;
            case XK_KP_Page_Down:   pfKeyName = PF_NUMPAD_3;break;
            case XK_KP_Insert:      pfKeyName = PF_NUMPAD_0;break;
            case XK_KP_Delete:      pfKeyName = PF_NUMPAD_PERIOD;break;
            case XK_KP_Enter:       pfKeyName = PF_NUMPAD_ENTER;break;
            
            default:                pfKeyName = PF_NULL;break;
        }
        if(!x11KeyCodesToPositionCodeMapSet) globalX11KeyCodesToPositionCodeMap[i] = pfKeyName;
        globalX11KeyCodesToPfVkCodeMap[i] = pfKeyName;
    }
    XFree(keyCodeToKeySyms);
#endif
#endif
}

#if defined(PF_WINDOW_AND_INPUT)
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
    ASSERT(bufferSize >= 0, "Window buffer size cannot be negative");
    //void *tempOffscreenMemory = mmap(0, bufferSize, PROT_READ|PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    //ASSERT(tempOffscreenMemory != MAP_FAILED);
    
    if(bufferSize > 0)
    {
        window->offscreenBuffer.data = (char*)malloc(bufferSize);//tempOffscreenMemory;
        
        PfglMakeCurrent(window);
        GL_CALL(glViewport(0, 0, width, height));
        
        ASSERT(window->offscreenBuffer.data, "Window buffer allocation failed");
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
    ASSERT(chosenVisual != 0, "X11 Visual is null");
    
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
    
    PfglMakeCurrent(window);
    
    DEBUG_LOG( "OpenGL version: %s\n\n", glGetString(GL_VERSION));
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
    ASSERT(status != 0, "Could not register x11 window delete event atom");
    
    int32 saveContextResult = XSaveContext(window->display, window->windowHandle, globalXlibContext, (XPointer)window);
    ASSERT(saveContextResult == 0, "Could not save x11 context");
    
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
    ASSERT(successIndicator != 0, "Could not get x11 window dimensions"); 
    
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
    ASSERT(result != 0, "Could not send x11 fullscreen event");
}

int32 PfGetKeyState(PfKeyCode keyCode, bool isVkCode = false)
{
    int32 result;
    int32 arrayIndex = isVkCode ? 1 : 0;
    //KeyCode keyCode = XKeysymToKeycode(globalDisplay, vkCode);
    
    //if(keyCode == 0) keyCode = vkCode;
    //result = globalKeyboard[arrayIndex][keyCode];
    result = globalKeyboard[arrayIndex][keyCode];
    return result;
}

int32 PfGetKeyState(PfWindow *window, PfKeyCode keyCode, bool isVkCode = false)
{
    int32 result = 0;
    int32 arrayIndex = isVkCode ? 1 : 0;
    if(window->hasKeyboardFocus)
    {
        //KeyCode keyCode = XKeysymToKeycode(window->display, vkCode);
        //result = globalKeyboard[arrayIndex][keyCode];
        result = globalKeyboard[arrayIndex][keyCode];
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
    ASSERT(xQueryPointerResult, "Could not query x11 mouse coordinates");
    
    
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


bool IsKeyPadSym(KeySym keySym)
{
    bool result = false;
    switch(keySym)
    {
        case XK_KP_Space:
        case XK_KP_Tab  :   
        case XK_KP_Enter:
        case XK_KP_F1   :                        
        case XK_KP_F2:
        case XK_KP_F3:
        case XK_KP_F4:
        case XK_KP_Home:
        case XK_KP_Left:
        case XK_KP_Up:
        case XK_KP_Right:
        case XK_KP_Down:
        case XK_KP_Prior:
        case XK_KP_Next:                       
        case XK_KP_End:                        
        case XK_KP_Begin:                      
        case XK_KP_Insert:                     
        case XK_KP_Delete:                     
        case XK_KP_Equal:                      
        case XK_KP_Multiply:                   
        case XK_KP_Add:                        
        case XK_KP_Separator:                  
        case XK_KP_Subtract:                   
        case XK_KP_Decimal:                    
        case XK_KP_Divide:                     
        case XK_KP_0:                          
        case XK_KP_1:                          
        case XK_KP_2:                          
        case XK_KP_3:                          
        case XK_KP_4:                          
        case XK_KP_5:                          
        case XK_KP_6:                          
        case XK_KP_7:                          
        case XK_KP_8:                          
        case XK_KP_9:
        {
            result = true;
        }break;
    }
    return result;
}

void X11ProcessKeyCode(uint32 keyCode, bool isDown, bool modifierBitField)
{
    int positionCode = globalX11KeyCodesToPositionCodeMap[keyCode];
    int vkCode = globalX11KeyCodesToPfVkCodeMap[keyCode];
    
    if(vkCode == PF_NULL) vkCode = positionCode; // HACK(KARAN): To handle the case when we have a non English keyboard layout
    if(vkCode == PF_NUMPAD_ENTER) vkCode = PF_ENTER; // NOTE(KARAN): To mimic Window's vk code messages
    
    bool shift = modifierBitField & ShiftMask; 
    bool caps = modifierBitField & LockMask; // CAPS
    bool ctrl = modifierBitField & ControlMask; 
    bool alt = modifierBitField & Mod1Mask; // ALT
    bool numLock = modifierBitField & Mod2Mask; // NUM
    bool super = modifierBitField & Mod4Mask; // SUPER
    
    int keySymIndex = 0;
    KeySym secondkeySym = XKeycodeToKeysym(globalDisplay, keyCode, 1);
    bool isNumPadKey = IsKeyPadSym(secondkeySym);
    
    if(numLock && isNumPadKey)
    {
        if(!shift) keySymIndex = 1;
    }
    else if(!shift && !caps)
    {
        keySymIndex = 0;
    }
    else if(!shift && caps)
    {
        keySymIndex = 0;
    }
    else if(shift && caps)
    {
        keySymIndex = 1;
    }
    else if(shift)
    {
        keySymIndex = 1;
    }
    
    // HACK(KARAN): To mimic Windows vk code messages
    KeySym keySym = XKeycodeToKeysym(globalDisplay, keyCode, keySymIndex);
    if(keySymIndex == 0 && isNumPadKey)
    {
        switch(vkCode)
        {
            case PF_NUMPAD_7: vkCode = PF_HOME; break;
            case PF_NUMPAD_8: vkCode = PF_UP; break;
            case PF_NUMPAD_9: vkCode = PF_PAGE_UP; break;
            case PF_NUMPAD_4: vkCode = PF_LEFT; break;
            case PF_NUMPAD_5: vkCode = PF_CLEAR; break;
            case PF_NUMPAD_6: vkCode = PF_RIGHT; break;
            case PF_NUMPAD_1: vkCode = PF_END; break;
            case PF_NUMPAD_2: vkCode = PF_DOWN; break;
            case PF_NUMPAD_3: vkCode = PF_PAGE_DOWN; break;
            case PF_NUMPAD_0: vkCode = PF_INSERT; break;
            case PF_NUMPAD_PERIOD: vkCode = PF_DELETE; break;
        }
    }
    
    globalKeyboard[0][positionCode] = isDown;
    globalKeyboard[1][vkCode] = isDown;
#if 0
    char *keySymStr = XKeysymToString(keySym);
    if(true)
    {
        DEBUG_LOG( "ScanCode: %d | PfKeyCodeName: %s | PfVkCodeName: %s | XKeySym: ", keyCode, globalPfKeyCodeToStrMap[positionCode], globalPfKeyCodeToStrMap[vkCode]);
        if(keySymStr)
        {
            DEBUG_LOG( "%s\n", keySymStr);
        }
    }
#endif
}

void PfUpdate()
{
    // NOTE(KARAN):  Message Pump
    Display *display = globalDisplay;
    XEvent event;
    int32 queueLength;
    while((queueLength = XPending(display)) > 0)
    {
        XNextEvent(display, &event);
        PfWindow *eventWindow;
        int32 findContextResult = XFindContext(display, event.xany.window, globalXlibContext, (XPointer*)&eventWindow);
        ASSERT(findContextResult == 0, "Could not get PfWindow* from the x11 Window handle");
        
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
                
#if 1
                char keyStates[32];
                XQueryKeymap(globalDisplay, keyStates);
                
                XkbStateRec modifierStates;                       
                XkbGetState(globalDisplay, XkbUseCoreKbd, &modifierStates);
                
                for(int i = 0; i < 32; i++)
                {
                    for(int j = 0; j < 8; j++)
                    {
                        uint32 keyCode = (i * 8) + j;
                        bool isDown = (keyStates[i] >> j) & 0x01;
                        X11ProcessKeyCode(keyCode, isDown, modifierStates.mods);
                    }
                }
#endif
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
                
                X11ProcessKeyCode(keyEvent.keycode, event.type == KeyPress, keyEvent.state);
                
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
}


bool PfRequestSwapInterval(int32 frames)
{
    if(glXSwapInterval) 
    {
        int32 returnVal = glXSwapInterval(frames);
        return (returnVal == 0);
    }
    else
    {
        return false;
    }
}


#endif

#if defined(PF_TIME)
PfTimestamp PfGetTimestamp()
{
    PfTimestamp result = {};
    clock_gettime(CLOCK_MONOTONIC, &result);
    return result;
}

inline void HandleNanoSecondOverflow(PfTimestamp *timestamp)
{
    while(timestamp->tv_nsec >= 1000000000L)
    {
        timestamp->tv_nsec -= 1000000000L;       
        timestamp->tv_sec++;
    }
}

real32 PfGetSeconds(PfTimestamp startTime, PfTimestamp endTime)
{
    return ((real32)(endTime.tv_sec - startTime.tv_sec)
            + ((real32)(endTime.tv_nsec - startTime.tv_nsec) * 1e-9f));
    
    /*
    HandleNanoSecondOverflow(&startTime);
    HandleNanoSecondOverflow(&endTime);
    
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
    
    return seconds;*/
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

void PfSleep(int32 milliseconds)
{
    if(milliseconds > 0)
    {
        usleep(milliseconds * 1000);
    }
    // TODO(KARAN): Find which is better: clock_nanosleep or usleep
    /*
    if(milliseconds > 0)
    {
        timespec sleepTime = {0, milliseconds * 1000000};
        timespec sleepTimeEnd = AddTimespec(PfGetTimestamp(), sleepTime);
        while(clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &sleepTimeEnd,0) != 0)
        {
        }
    }
    */
}

#endif


#if defined(PF_FILE)
int64 PfWriteEntireFile(char *filename, void *data, uint32 size)
{
    ssize_t bytesWritten;
    int fileHandle = open(filename, O_WRONLY | O_CREAT, 0600);
    
    if(fileHandle == -1)
    {
        DEBUG_ERROR("Could not open %s for writing.", filename);
        return -1;
    }
    
    bytesWritten = write(fileHandle, data, size);
    if(bytesWritten != size)
    {
        DEBUG_ERROR("Requested bytes to write: %d | Actual bytes written: %ld.", size, bytesWritten);
    }
    
    close(fileHandle);
    return bytesWritten;
}


uint64 PfGetFileSize(char *filepath)
{
    struct stat stat_buf = {};
    stat(filepath, &stat_buf);
    
    return (uint64)(stat_buf.st_size);
}

int64 PfReadEntireFile(char *filename, void *data)
{
    ssize_t bytesRead;
    int64 fileHandle = open(filename, O_RDONLY);
    
    if(fileHandle == -1)
    {
        DEBUG_ERROR("Could not open %s for reading.", filename);
        return -1;
    }
    
    uint32 size = PfGetFileSize(filename);
    bytesRead = read(fileHandle, data, size);
    if(bytesRead != size)
    {
        DEBUG_ERROR("Requested bytes to read: %d | Actual bytes read: %ld.", size, bytesRead);
    }
    
    close(fileHandle);
    return bytesRead;
}


int64 PfCreateFile(char *filename, uint32 access, uint32 creationDisposition)
{
    int32 oFlags = 0;
    if(((access & PF_READ) != 0) && ((access & PF_WRITE) != 0))
    {
        oFlags |= O_RDWR;
    }
    else if((access & PF_READ) != 0)
    {
        oFlags |= O_RDONLY;
    }
    else if((access & PF_WRITE) != 0)
    {
        oFlags |= O_WRONLY;
    }
    
    if((creationDisposition & PF_CREATE) != 0)
    {
        oFlags |= O_CREAT;
    }
    
    int64 linuxFileHandle = open(filename, oFlags);
    return linuxFileHandle;
}

bool PfCloseFileHandle(int64 fileHandle)
{
    bool result = false;
    int32 successCode;
    successCode = close(fileHandle);
    result = (successCode == 0);
    return result;
}

bool PfDeleteFile(char *filename)
{
    bool result = false;
    int32 successCode;
    successCode = remove(filename);
    result = (successCode == 0);
    return result;
}


int64 PfReadFile(int64 fileHandle, void *data, uint32 size)
{
    int bytesRead = read(fileHandle, data, size);
    if(bytesRead == -1)
    {
        DEBUG_ERROR("Failed to read");
        return -1;
    }
    
    bool eof = (bytesRead == 0);
    if(!eof && bytesRead != size)
    {
        DEBUG_ERROR("Requested bytes to read: %d | Actual bytes read: %d.", size, bytesRead);
    }
    
    return bytesRead;
}


int64 PfWriteFile(int64 fileHandle, void *data, uint32 size)
{
    int bytesWritten = write(fileHandle, data, size);
    if(bytesWritten == -1)
    {
        DEBUG_ERROR("Failed to write");
        return -1;
    }
    
    if(bytesWritten != size)
    {
        DEBUG_ERROR("Requested bytes to write: %d | Actual bytes write: %d.", size, bytesWritten);
    }
    
    return bytesWritten;
}


bool PfFilepathExists(char *filepath)
{
    return (access(filepath, F_OK) == 0);
}


#endif


void* PfVirtualAlloc(void *baseAddress, size_t size)
{
    int32 flags = MAP_PRIVATE | MAP_ANONYMOUS;
    if(baseAddress)
    {
        flags = flags | MAP_FIXED;
    }
    
    return mmap(baseAddress, size, PROT_READ|PROT_WRITE, flags, -1, 0);
}


#if defined(PF_SOUND)
#define EXIT_ON_ALSA_ERROR(alsaReturnCode) if((alsaReturnCode) < 0){DEBUG_ERROR("%s\n", snd_strerror(alsaReturnCode)); exit(alsaReturnCode);}

PfSoundSystem PfInitializeSoundSystem(uint64 bufferDurationInFrames, uint32 bitsPerSample, uint32 numChannels, uint32 framesPerSecond)
{
    if(bitsPerSample != 16)
    {
        ASSERT(false, "Currently only supports 16bit audio");
    }
    
    real32 periodDurationInMS = 3.0f; //10.0f;
    int32 framesPerPeriod = CeilReal32ToInt32(periodDurationInMS * ((real32)framesPerSecond/1000.0f));
    uint32 numPeriods = CeilReal32ToUint32((real32)bufferDurationInFrames/(real32)framesPerPeriod);
    
    PfSoundSystem result = {};
    int32 alsaReturnCode;
    
    snd_pcm_hw_params_t *hardwareParams;
    snd_pcm_hw_params_alloca(&hardwareParams);
    
    alsaReturnCode = snd_pcm_open(&(result.soundDeviceHandle), (char*)"default", SND_PCM_STREAM_PLAYBACK, SND_PCM_NONBLOCK);
    EXIT_ON_ALSA_ERROR(alsaReturnCode);
    
    alsaReturnCode = snd_pcm_hw_params_any(result.soundDeviceHandle, hardwareParams);
    EXIT_ON_ALSA_ERROR(alsaReturnCode);
    
    //DEBUG_LOG("Before init:\n");
    //DebugLinuxPrintALSAParams(result.soundDeviceHandle, hardwareParams);
    //DEBUG_LOG("---------------------------\n");
    alsaReturnCode = snd_pcm_hw_params_set_access(result.soundDeviceHandle, hardwareParams, SND_PCM_ACCESS_RW_INTERLEAVED);
    EXIT_ON_ALSA_ERROR(alsaReturnCode);
    
    alsaReturnCode = snd_pcm_hw_params_set_format(result.soundDeviceHandle, hardwareParams, SND_PCM_FORMAT_S16_LE);
    EXIT_ON_ALSA_ERROR(alsaReturnCode);
    
    alsaReturnCode = snd_pcm_hw_params_set_channels(result.soundDeviceHandle, hardwareParams, numChannels);
    EXIT_ON_ALSA_ERROR(alsaReturnCode);
    
    alsaReturnCode = snd_pcm_hw_params_set_rate(result.soundDeviceHandle, hardwareParams, framesPerSecond, 0);
    EXIT_ON_ALSA_ERROR(alsaReturnCode);
    
    alsaReturnCode = snd_pcm_hw_params_set_buffer_size(result.soundDeviceHandle, hardwareParams, bufferDurationInFrames);
    EXIT_ON_ALSA_ERROR(alsaReturnCode);
    
    int32 dir = 0;
    alsaReturnCode = snd_pcm_hw_params_set_period_size_near(result.soundDeviceHandle, hardwareParams, (snd_pcm_uframes_t*)(&framesPerPeriod), &dir);
    EXIT_ON_ALSA_ERROR(alsaReturnCode);
    if(dir != 0)
    {
        DEBUG_ERROR("WARNING: Cannot set period size to the prefered value. Instead using %d samples per period.\n", framesPerPeriod);
    }
    
    alsaReturnCode = snd_pcm_hw_params_set_periods_near(result.soundDeviceHandle, hardwareParams, &numPeriods, &dir);
    EXIT_ON_ALSA_ERROR(alsaReturnCode);
    if(dir != 0)
    {
        DEBUG_ERROR("WARNING: Cannot set num periods to the prefered value. Instead using %d periods per buffer.\n", numPeriods);
    }
    
    alsaReturnCode = snd_pcm_hw_params(result.soundDeviceHandle, hardwareParams);
    EXIT_ON_ALSA_ERROR(alsaReturnCode);
    //DEBUG_LOG("After init:\n");
    //DebugLinuxPrintALSAParams(result.soundDeviceHandle, hardwareParams);
    result.secondarySoundBuffer = mmap(0, bufferDurationInFrames * numChannels * (bitsPerSample/8), PROT_READ|PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    result.bufferDurationInFrames = bufferDurationInFrames;
    return result;
}

uint64 PfGetPendingFrames(PfSoundSystem *soundSystem)
{
    int32 availableFrames = snd_pcm_avail(soundSystem->soundDeviceHandle);	
    
    if(availableFrames == -EPIPE)
    {
        DEBUG_ERROR("UNDERRUN: %s.\n", snd_strerror(availableFrames));
        snd_pcm_prepare(soundSystem->soundDeviceHandle);
        availableFrames = 0;
    }
    else if(availableFrames < 0)
    {
        DEBUG_ERROR("%s, PCM state: %s.\n", snd_strerror(availableFrames), snd_pcm_state_name(snd_pcm_state(soundSystem->soundDeviceHandle)));
        availableFrames = 0;
    }
    
    uint64 result = (uint64)(soundSystem->bufferDurationInFrames - availableFrames);
    
    return result;
}

PfSoundBuffer PfGetSoundBuffer(PfSoundSystem *soundSystem, uint64 framesRequired)
{
    PfSoundBuffer result = {};
    result.buffer = soundSystem->secondarySoundBuffer;
    result.frames = framesRequired;
    return result;
}

void PfDispatchSoundBuffer(PfSoundSystem *soundSystem, PfSoundBuffer *soundBuffer)
{
    int32 alsaReturnCode = snd_pcm_writei(soundSystem->soundDeviceHandle, soundBuffer->buffer, soundBuffer->frames);
    
    if(alsaReturnCode == -EPIPE)
    {
        DEBUG_ERROR("UNDERRUN: %s.\n", snd_strerror(alsaReturnCode));
        snd_pcm_prepare(soundSystem->soundDeviceHandle);
    }
    else if(alsaReturnCode < 0)
    {
        DEBUG_ERROR("%s, PCM state: %s.\n", snd_strerror(alsaReturnCode), snd_pcm_state_name(snd_pcm_state(soundSystem->soundDeviceHandle)));
    }
}


void PfStartSoundSystem(PfSoundSystem *soundSystem)
{
    snd_pcm_start(soundSystem->soundDeviceHandle);
}
#endif