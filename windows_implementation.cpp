#include <stdio.h>
#include <intrin.h>  

#if !PF_GLEW_ENABLED
#include <GL/gl.h>
#endif

#include "utility.h"
#include "project_types.h"
#include "windows_platform_interface.h"
#define GLEW_STATIC

#if PF_GLEW_ENABLED
#include <GL/glew.h>
#endif

#include "pf_opengl.h"


/********* WGL specific stuff *******/
#define WGL_DRAW_TO_WINDOW_ARB            0x2001
#define WGL_SUPPORT_OPENGL_ARB            0x2010
#define WGL_DOUBLE_BUFFER_ARB             0x2011
#define WGL_PIXEL_TYPE_ARB                0x2013
#define WGL_COLOR_BITS_ARB                0x2014
#define WGL_TYPE_RGBA_ARB                 0x202B
#define WGL_DEPTH_BITS_ARB                0x2022
#define WGL_STENCIL_BITS_ARB              0x2023

#define WGL_CONTEXT_DEBUG_BIT_ARB         0x00000001
#define WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB 0x00000002
#define WGL_CONTEXT_MAJOR_VERSION_ARB     0x2091
#define WGL_CONTEXT_MINOR_VERSION_ARB     0x2092
#define WGL_CONTEXT_LAYER_PLANE_ARB       0x2093
#define WGL_CONTEXT_FLAGS_ARB             0x2094
#define WGL_CONTEXT_PROFILE_MASK_ARB      0x9126
#define WGL_CONTEXT_CORE_PROFILE_BIT_ARB  0x00000001
#define WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB 0x00000002
#define ERROR_INVALID_VERSION_ARB         0x2095

typedef  const char *WINAPI type_wglGetExtensionsStringARB(HDC hdc);
typedef BOOL WINAPI type_wglChoosePixelFormatARB(HDC hdc, const int *piAttribIList, const FLOAT *pfAttribFList, UINT nMaxFormats, int *piFormats, UINT *nNumFormats);
typedef HGLRC WINAPI type_wglCreateContextAttribsARB(HDC hDC, HGLRC hShareContext, const int *attribList);
typedef BOOL WINAPI type_wglSwapIntervalEXT(int interval);

global_variable type_wglGetExtensionsStringARB* wglGetExtensionsStringARB;  
global_variable type_wglChoosePixelFormatARB* wglChoosePixelFormatARB;
global_variable type_wglCreateContextAttribsARB* wglCreateContextAttribsARB;
global_variable type_wglSwapIntervalEXT* wglSwapIntervalEXT;


global_variable WNDCLASS globalWindowClass;
global_variable int32 globalKeyboard[2][PF_ONE_PAST_LAST];
global_variable int32 globalMouseButtons[5];
global_variable int64 globalQueryPerformanceHZ;
global_variable int32 globalGLMajorVersion = 3;
global_variable int32 globalGLMinorVersion = 3;
global_variable bool globalCoreProfile       = false;
global_variable int globalScanCodesToPositionCodeMap[512];
global_variable int globalWindowsVkCodeToPfVkCodeMap[256];
global_variable char* globalWindowsVkCodeToStrMap[]= 
{
    "VK_NULL",
    "VK_LBUTTON",
    "VK_RBUTTON",
    "VK_CANCEL",
    "VK_MBUTTON",
    "VK_XBUTTON1",
    "VK_XBUTTON2",
    "VK_UNDEFINED",
    "VK_BACK",
    "VK_TAB",
    "VK_RESERVED",
    "VK_RESERVED",
    "VK_CLEAR",
    "VK_RETURN",
    "VK_UNDEFINED",
    "VK_UNDEFINED",
    "VK_SHIFT",
    "VK_CONTROL",
    "VK_MENU",
    "VK_PAUSE",
    "VK_CAPITAL",
    "VK_KANA_HANGUEL_HANGUL",
    "VK_UNDEFINED",
    "VK_JUNJA",
    "VK_FINAL",
    "VK_HANJA_KANJI",
    "VK_UNDEFINED",
    "VK_ESCAPE",
    "VK_CONVERT",
    "VK_NONCONVERT",
    "VK_ACCEPT",
    "VK_MODECHANGE",
    "VK_SPACE",
    "VK_PRIOR",
    "VK_NEXT",
    "VK_END",
    "VK_HOME",
    "VK_LEFT",
    "VK_UP",
    "VK_RIGHT",
    "VK_DOWN",
    "VK_SELECT",
    "VK_PRINT",
    "VK_EXECUTE",
    "VK_SNAPSHOT",
    "VK_INSERT",
    "VK_DELETE",
    "VK_HELP",
    "0",
    "1",
    "2",
    "3",
    "4",
    "5",
    "6",
    "7",
    "8",
    "9",
    "VK_UNDEFINED",
    "VK_UNDEFINED",
    "VK_UNDEFINED",
    "VK_UNDEFINED",
    "VK_UNDEFINED",
    "VK_UNDEFINED",
    "VK_UNDEFINED",
    "A",
    "B",
    "C",
    "D",
    "E",
    "F",
    "G",
    "H",
    "I",
    "J",
    "K",
    "L",
    "M",
    "N",
    "O",
    "P",
    "Q",
    "R",
    "S",
    "T",
    "U",
    "V",
    "W",
    "X",
    "Y",
    "Z",
    "VK_LWIN",
    "VK_RWIN",
    "VK_APPS",
    "VK_RESERVED",
    "VK_SLEEP",
    "VK_NUMPAD0",
    "VK_NUMPAD1",
    "VK_NUMPAD2",
    "VK_NUMPAD3",
    "VK_NUMPAD4",
    "VK_NUMPAD5",
    "VK_NUMPAD6",
    "VK_NUMPAD7",
    "VK_NUMPAD8", 
    "VK_NUMPAD9",
    "VK_MULTIPLY",
    "VK_ADD",
    "VK_SEPARATOR",
    "VK_SUBTRACT", 
    "VK_DECIMAL",	
    "VK_DIVIDE",	
    "VK_F1",
    "VK_F2",
    "VK_F3",
    "VK_F4",
    "VK_F5",
    "VK_F6", 
    "VK_F7", 
    "VK_F8",
    "VK_F9", 
    "VK_F10",
    "VK_F11",
    "VK_F12",
    "VK_F13",
    "VK_F14",
    "VK_F15",
    "VK_F16",
    "VK_F17",
    "VK_F18",
    "VK_F19",
    "VK_F20",
    "VK_F21",
    "VK_F22",
    "VK_F23",
    "VK_F24",
    "VK_UNASSIGNED",
    "VK_UNASSIGNED",
    "VK_UNASSIGNED",
    "VK_UNASSIGNED",
    "VK_UNASSIGNED",
    "VK_UNASSIGNED",
    "VK_UNASSIGNED",
    "VK_UNASSIGNED",
    "VK_NUMLOCK",
    "VK_SCROLL",
    "VK_OEM_SPECIFIC",
    "VK_OEM_SPECIFIC",
    "VK_OEM_SPECIFIC",
    "VK_OEM_SPECIFIC",
    "VK_OEM_SPECIFIC",
    "VK_UNASSIGNED",
    "VK_UNASSIGNED",
    "VK_UNASSIGNED",
    "VK_UNASSIGNED",
    "VK_UNASSIGNED",
    "VK_UNASSIGNED",
    "VK_UNASSIGNED",
    "VK_UNASSIGNED",
    "VK_UNASSIGNED",
    "VK_LSHIFT",	
    "VK_RSHIFT",	
    "VK_LCONTROL",	
    "VK_RCONTROL",	
    "VK_LMENU",	
    "VK_RMENU",	
    "VK_BROWSER_BACK",
    "VK_BROWSER_FORWARD",
    "VK_BROWSER_REFRESH",
    "VK_BROWSER_STOP",	
    "VK_BROWSER_SEARCH",	
    "VK_BROWSER_FAVORITES", 
    "VK_BROWSER_HOME",
    "VK_VOLUME_MUTE",
    "VK_VOLUME_DOWN",
    "VK_VOLUME_UP",	
    "VK_MEDIA_NEXT_TRACK", 
    "VK_MEDIA_PREV_TRACK",
    "VK_MEDIA_STOP",
    "VK_MEDIA_PLAY_PAUSE",
    "VK_LAUNCH_MAIL",
    "VK_LAUNCH_MEDIA_SELECT",
    "VK_LAUNCH_APP1",	
    "VK_LAUNCH_APP2",	
    "VK_RESERVED",
    "VK_RESERVED",
    "VK_OEM_SEMICOLON",
    "VK_OEM_PLUS",
    "VK_OEM_COMMA",
    "VK_OEM_MINUS",
    "VK_OEM_PERIOD",	
    "VK_OEM_FORWARD_SLASH",
    "VK_OEM_TILDE",
    "VK_RESERVED",
    "VK_RESERVED",
    "VK_RESERVED",
    "VK_RESERVED",
    "VK_RESERVED",
    "VK_RESERVED",
    "VK_RESERVED",
    "VK_RESERVED",
    "VK_RESERVED",
    "VK_RESERVED",
    "VK_RESERVED",
    "VK_RESERVED",
    "VK_RESERVED",
    "VK_RESERVED",
    "VK_RESERVED",
    "VK_RESERVED",
    "VK_RESERVED",
    "VK_RESERVED",
    "VK_RESERVED",
    "VK_RESERVED",
    "VK_RESERVED",
    "VK_RESERVED",
    "VK_RESERVED",
    "VK_UNASSIGNED",
    "VK_UNASSIGNED",
    "VK_UNASSIGNED",
    "VK_OEM_OPEN_SQUARE_BRACKET",
    "VK_OEM_BACK_SLASH",
    "VK_OEM_CLOSE_SQUARE_BRACKET",
    "VK_OEM_APOSTROPHE",	
    "VK_OEM_8",
    "VK_RESERVED",
    "VK_OEM_SPECIFIC",
    "VK_OEM_ANGULAR_OR_BACKSLASH",
    "VK_OEM_SPECIFIC",
    "VK_OEM_SPECIFIC",
    "VK_PROCESSKEY",
    "VK_OEM_SPECIFIC",
    "VK_PACKET",	
    "VK_UNASSIGNED",
    "VK_OEM_SPECIFIC",
    "VK_OEM_SPECIFIC",
    "VK_OEM_SPECIFIC",
    "VK_OEM_SPECIFIC",
    "VK_OEM_SPECIFIC",
    "VK_OEM_SPECIFIC",
    "VK_OEM_SPECIFIC",
    "VK_OEM_SPECIFIC",
    "VK_OEM_SPECIFIC",
    "VK_OEM_SPECIFIC",
    "VK_OEM_SPECIFIC",
    "VK_OEM_SPECIFIC",
    "VK_OEM_SPECIFIC",
    "VK_ATTN",
    "VK_CRSEL",
    "VK_EXSEL",
    "VK_EREOF",
    "VK_PLAY",
    "VK_ZOOM",
    "VK_NONAME",
    "VK_PA1",
    "VK_OEM_CLEAR"
};

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
        case WM_KILLFOCUS:
        {
            if(window) window->hasKeyboardFocus = (message == WM_SETFOCUS);
#if 1
            /*
                        if(message == WM_SETFOCUS)
                        {*/
            for(int scanCode = 0; scanCode < ARRAY_COUNT(globalScanCodesToPositionCodeMap); scanCode++)
            {
                PfKeyCode positionCode = (PfKeyCode)globalScanCodesToPositionCodeMap[scanCode];
                if(positionCode != PF_NULL)
                {
                    int extendedScanCode = scanCode;
                    if(extendedScanCode & 0x0100)
                    {
                        extendedScanCode = extendedScanCode & 0x0FF;
                        extendedScanCode = extendedScanCode | 0xE000;
                    }
                    int vkCode = MapVirtualKeyEx(extendedScanCode, MAPVK_VSC_TO_VK_EX, 0);
                    uint16 keyState = GetAsyncKeyState(vkCode);
                    
                    bool isDown = (keyState & 0x08000) != 0;
                    globalKeyboard[0][positionCode] = isDown;
                }
            }
            
            for(int vkCode = 0; vkCode < ARRAY_COUNT(globalWindowsVkCodeToPfVkCodeMap); vkCode++)
            {
                PfKeyCode pfVkCode = (PfKeyCode)globalWindowsVkCodeToPfVkCodeMap[vkCode];
                if(pfVkCode != PF_NULL)
                {
                    uint16 keyState = GetAsyncKeyState(vkCode);
                    
                    bool isDown = (keyState & 0x08000) != 0;
                    globalKeyboard[1][pfVkCode] = isDown;
                }
            }
            /*}
            else
            {
                ClearArray((char*)globalKeyboard, sizeof(globalKeyboard));
            }*/
#endif
        }break;
        case WM_KEYUP:
        case WM_KEYDOWN:
        case WM_SYSKEYDOWN:
        case WM_SYSKEYUP:
        {
            int32 scanCode = (lParam >> 16) & 0x07F;
            WPARAM vkCode = wParam;
            
            // TODO(KARAN): Separate keyboard updating from this callback
            
            int32 repeatCount = lParam & 0x07FFF;
            int32 isExtendedKey = (lParam >> 23) & 0x01;
            
            int32 reserved = (lParam >> 25) & 0x07;
            int32 isAltDown = (lParam >> 29) & 0x01;
            
            bool wasKeyDown = ((lParam >> 30) & 0x01) != 0;
            bool isKeyDown = ((lParam >> 31) & 0x01) == 0;
            
            int32 extendedScanCode = (lParam >> 16) & 0x01FF;
            
            int tempKeyState = isKeyDown ? 0x8000 : 0;
            if(vkCode == VK_SHIFT)
            {
                vkCode = VK_LSHIFT;
                uint16 keyState = GetKeyState((int32)vkCode);
                if((keyState & 0x8000) != tempKeyState)
                {
                    vkCode = VK_RSHIFT;
                }
            }
            else if(vkCode == VK_CONTROL)
            {
                vkCode = VK_LCONTROL;
                uint16 keyState = GetKeyState((int32)vkCode);
                if((keyState & 0x8000) != tempKeyState)
                {
                    vkCode = VK_RCONTROL;
                }
            }
            else if(vkCode == VK_MENU)
            {
                vkCode = VK_LMENU;
                uint16 keyState = GetKeyState((int32)vkCode);
                if((keyState & 0x8000) != tempKeyState)
                {
                    vkCode = VK_RMENU;
                }
            }
            
            int32 positionCodeIndex = globalScanCodesToPositionCodeMap[extendedScanCode];
            int32 vkCodeIndex = globalWindowsVkCodeToPfVkCodeMap[vkCode];
            
            globalKeyboard[1][vkCodeIndex] = isKeyDown;
            globalKeyboard[0][positionCodeIndex] = isKeyDown;
            
#if 0
            if(message == WM_KEYDOWN || message == WM_SYSKEYDOWN)
            {
                char str[512] = {};
                if(GetKeyNameTextA((LONG)lParam, str, 512))
                {
                    DEBUG_LOG("PositionCode: %s | PfVkCode: %s | WinVkCode: %s | ExtendedScanCode: %03x\n", globalPfKeyCodeToStrMap[positionCodeIndex], globalPfKeyCodeToStrMap[vkCodeIndex], globalWindowsVkCodeToStrMap[vkCode], extendedScanCode);
                }
            }
#endif
            
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
            
            if(message == WM_SYSKEYUP || message == WM_SYSKEYDOWN)
            {
                result = DefWindowProc(windowHandle, message, wParam, lParam); 
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
    // 1. Set up sleep timer resolution
    // 2. Get Query Performance Counter 
    // 3. Register Window Class
    // 4. Get WGL extensions and load opengl functions
    // 5. KeyCodes setup
    
    //// 1. Set up sleep timer resolution
    UINT sleepResolution = 1; //ms
    if(timeBeginPeriod(sleepResolution) != TIMERR_NOERROR)
    {
        DEBUG_ERROR("Could not set the resolution of sleep timer"); 
    }
    
    //// 2. Get Query Performance Counter Frequency
    LARGE_INTEGER queryPerformanceHZResult;
    
    BOOL result = QueryPerformanceFrequency(&queryPerformanceHZResult);
    ASSERT(result != 0, "Failed to get performance counter frequency");
    
    globalQueryPerformanceHZ  = queryPerformanceHZResult.QuadPart;
    
    //// 3. Register Window Class
    // NOTE(KARAN): CS_OWNDC necessary for OpenGL context creation
    globalWindowClass.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    globalWindowClass.lpfnWndProc = WinWindowCallback;
    globalWindowClass.hInstance = GetModuleHandle(0);globalWindowClass.lpszClassName = "windowsPlatformLayer";
    
    if(RegisterClassA(&globalWindowClass) == 0)
    {
        fprintf(stderr,"ERROR: Could not register window class. LINE: %d, FUNCTION:%s, FILE:%s\n", __LINE__, __func__, __FILE__);
    }
    
    //// 4. Get WGL extensions and load opengl functions
    PfWindow dummyWindow = {};
    WNDCLASS dummyWindowClass = {};
    dummyWindowClass.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    dummyWindowClass.lpfnWndProc = DefWindowProcA;
    dummyWindowClass.hInstance = GetModuleHandle(0);
    dummyWindowClass.lpszClassName = "DummyWindowClass";
    
    ATOM registerSuccess = RegisterClassA(&dummyWindowClass);
    ASSERT(registerSuccess, "Couldn't register dummy window class");
    
    dummyWindow.windowHandle = CreateWindowEx(0, dummyWindowClass.lpszClassName, "Dummy Window", WS_OVERLAPPEDWINDOW, 
                                              CW_USEDEFAULT, CW_USEDEFAULT,
                                              CW_USEDEFAULT, CW_USEDEFAULT,
                                              0, 0, dummyWindowClass.hInstance, 0);
    ASSERT(dummyWindow.windowHandle, "Couldn't create dummy window");
    
    dummyWindow.deviceContext = GetDC(dummyWindow.windowHandle);
    PIXELFORMATDESCRIPTOR desiredPixelFormat =
    {
        sizeof(PIXELFORMATDESCRIPTOR), 1,
        PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,    // Flags
        PFD_TYPE_RGBA,        // The kind of framebuffer. RGBA or palette.
        32,                   // Colordepth of the framebuffer.
        0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0,
        24,                   // Number of bits for the depthbuffer
        8,                    // Number of bits for the stencilbuffer
        0,                    // Number of Aux buffers in the framebuffer.
        PFD_MAIN_PLANE,
        0,0, 0, 0
    };
    
    int32 closestPixelFormatIndex = ChoosePixelFormat(dummyWindow.deviceContext, &desiredPixelFormat);
    ASSERT(closestPixelFormatIndex, "Couldn't find desired pixel format");
    /*
An application can only set the pixel format of a window one time. 
Once a window's pixel format is set, it cannot be changed.
*/
    BOOL setPixelFormatSuccess = SetPixelFormat(dummyWindow.deviceContext, closestPixelFormatIndex, &desiredPixelFormat);
    ASSERT(setPixelFormatSuccess == TRUE, "Couldn't set window DC's pixel format");
    
    dummyWindow.glContext = wglCreateContext(dummyWindow.deviceContext);
    ASSERT(dummyWindow.glContext, "Couldn't create dummy OpenGL context");
    
    BOOL makeCurrentSuccess = wglMakeCurrent(dummyWindow.deviceContext, dummyWindow.glContext);
    ASSERT(makeCurrentSuccess == TRUE, "Couldn't make dummy OpenGL context current");
    
#if !PF_GLEW_ENABLED
    GrabOpenGLFuncPointers();
#endif
    //TODO(KARAN): Need to parse the extension strings before getting the pointer to the functions
    wglGetExtensionsStringARB = (type_wglGetExtensionsStringARB*)wglGetProcAddress("wglGetExtensionsStringARB");
    
    const char *wglExtensions = wglGetExtensionsStringARB(dummyWindow.deviceContext);
    
    // TODO(KARAN): WGL_ARB_pixel_format extension
    wglChoosePixelFormatARB = (type_wglChoosePixelFormatARB*)wglGetProcAddress("wglChoosePixelFormatARB");
    
    // TODO(KARAN): WGL_ARB_create_context extension
    wglCreateContextAttribsARB = (type_wglCreateContextAttribsARB*)wglGetProcAddress("wglCreateContextAttribsARB");
    
    // TODO(KARAN): WGL_EXT_swap_control extension
    wglSwapIntervalEXT = (type_wglSwapIntervalEXT*)wglGetProcAddress("wglSwapIntervalEXT");
    
    BOOL deleteSuccess = wglDeleteContext(dummyWindow.glContext);
    
    deleteSuccess = ReleaseDC(dummyWindow.windowHandle, dummyWindow.deviceContext);
    
    deleteSuccess = DestroyWindow(dummyWindow.windowHandle);
    
    //// 5. KeyCodes setup
    
    // Position Code Mapping: https://docs.microsoft.com/en-us/previous-versions/visualstudio/visual-studio-6.0/aa299374(v=vs.60)
    globalScanCodesToPositionCodeMap[0] = PF_NULL;
    globalScanCodesToPositionCodeMap[1] = PF_ESC;
    globalScanCodesToPositionCodeMap[59] = PF_F1;
    globalScanCodesToPositionCodeMap[60] = PF_F2;
    globalScanCodesToPositionCodeMap[61] = PF_F3;
    globalScanCodesToPositionCodeMap[62] = PF_F4;
    globalScanCodesToPositionCodeMap[63] = PF_F5;
    globalScanCodesToPositionCodeMap[64] = PF_F6;
    globalScanCodesToPositionCodeMap[65] = PF_F7;
    globalScanCodesToPositionCodeMap[66] = PF_F8;
    globalScanCodesToPositionCodeMap[67] = PF_F9;
    globalScanCodesToPositionCodeMap[68] = PF_F10;
    globalScanCodesToPositionCodeMap[87] = PF_F11;
    globalScanCodesToPositionCodeMap[88] = PF_F12;
    globalScanCodesToPositionCodeMap[311] = PF_PRINT_SCREEN;
    globalScanCodesToPositionCodeMap[70] = PF_SCROLL_LOCK;
    globalScanCodesToPositionCodeMap[69] = PF_PAUSE;
    globalScanCodesToPositionCodeMap[326] = PF_PAUSE;
    
    globalScanCodesToPositionCodeMap[41] = PF_TILDE;
    globalScanCodesToPositionCodeMap[2] = PF_1;
    globalScanCodesToPositionCodeMap[3] = PF_2;
    globalScanCodesToPositionCodeMap[4] = PF_3;
    globalScanCodesToPositionCodeMap[5] = PF_4;
    globalScanCodesToPositionCodeMap[6] = PF_5;
    globalScanCodesToPositionCodeMap[7] = PF_6;
    globalScanCodesToPositionCodeMap[8] = PF_7;
    globalScanCodesToPositionCodeMap[9] = PF_8;
    globalScanCodesToPositionCodeMap[10] = PF_9;
    globalScanCodesToPositionCodeMap[11] = PF_0;
    globalScanCodesToPositionCodeMap[12] = PF_MINUS;
    globalScanCodesToPositionCodeMap[13] = PF_EQUALS;
    globalScanCodesToPositionCodeMap[14] = PF_BACKSPACE;
    
    globalScanCodesToPositionCodeMap[15] = PF_TAB;
    globalScanCodesToPositionCodeMap[16] = PF_Q;
    globalScanCodesToPositionCodeMap[17] = PF_W;
    globalScanCodesToPositionCodeMap[18] = PF_E;
    globalScanCodesToPositionCodeMap[19] = PF_R;
    globalScanCodesToPositionCodeMap[20] = PF_T;
    globalScanCodesToPositionCodeMap[21] = PF_Y;
    globalScanCodesToPositionCodeMap[22] = PF_U;
    globalScanCodesToPositionCodeMap[23] = PF_I;
    globalScanCodesToPositionCodeMap[24] = PF_O;
    globalScanCodesToPositionCodeMap[25] = PF_P;
    globalScanCodesToPositionCodeMap[26] = PF_OPEN_SQUARE_BRACKET;
    globalScanCodesToPositionCodeMap[27] = PF_CLOSE_SQUARE_BRACKET;
    globalScanCodesToPositionCodeMap[43] = PF_BACKSLASH;
    
    globalScanCodesToPositionCodeMap[58] = PF_CAPS_LOCK;
    globalScanCodesToPositionCodeMap[30] = PF_A;
    globalScanCodesToPositionCodeMap[31] = PF_S;
    globalScanCodesToPositionCodeMap[32] = PF_D;
    globalScanCodesToPositionCodeMap[33] = PF_F;
    globalScanCodesToPositionCodeMap[34] = PF_G;
    globalScanCodesToPositionCodeMap[35] = PF_H;
    globalScanCodesToPositionCodeMap[36] = PF_J;
    globalScanCodesToPositionCodeMap[37] = PF_K;
    globalScanCodesToPositionCodeMap[38] = PF_L;
    globalScanCodesToPositionCodeMap[39] = PF_SEMICOLON;
    globalScanCodesToPositionCodeMap[40] = PF_APOSTROPHE;
    globalScanCodesToPositionCodeMap[28] = PF_ENTER;
    
    globalScanCodesToPositionCodeMap[42] = PF_LEFT_SHIFT;
    globalScanCodesToPositionCodeMap[44] = PF_Z;
    globalScanCodesToPositionCodeMap[45] = PF_X;
    globalScanCodesToPositionCodeMap[46] = PF_C;
    globalScanCodesToPositionCodeMap[47] = PF_V;
    globalScanCodesToPositionCodeMap[48] = PF_B;
    globalScanCodesToPositionCodeMap[49] = PF_N;
    globalScanCodesToPositionCodeMap[50] = PF_M;
    globalScanCodesToPositionCodeMap[51] = PF_COMMA;
    globalScanCodesToPositionCodeMap[52] = PF_PERIOD;
    globalScanCodesToPositionCodeMap[53] = PF_FORWARD_SLASH;
    globalScanCodesToPositionCodeMap[54] = PF_RIGHT_SHIFT;
    
    globalScanCodesToPositionCodeMap[29] = PF_LEFT_CTRL;
    globalScanCodesToPositionCodeMap[347] = PF_LEFT_WIN;
    globalScanCodesToPositionCodeMap[56] = PF_LEFT_ALT;
    globalScanCodesToPositionCodeMap[57] = PF_SPACEBAR;
    globalScanCodesToPositionCodeMap[312] = PF_RIGHT_ALT;
    globalScanCodesToPositionCodeMap[348] = PF_RIGHT_WIN;
    globalScanCodesToPositionCodeMap[349] = PF_MENU;
    globalScanCodesToPositionCodeMap[285] = PF_RIGHT_CTRL;
    
    globalScanCodesToPositionCodeMap[338] = PF_INSERT;
    globalScanCodesToPositionCodeMap[327] = PF_HOME;
    globalScanCodesToPositionCodeMap[329] = PF_PAGE_UP;
    globalScanCodesToPositionCodeMap[339] = PF_DELETE;
    globalScanCodesToPositionCodeMap[335] = PF_END;
    globalScanCodesToPositionCodeMap[337] = PF_PAGE_DOWN;
    
    globalScanCodesToPositionCodeMap[328] = PF_UP;
    globalScanCodesToPositionCodeMap[336] = PF_DOWN;
    globalScanCodesToPositionCodeMap[331] = PF_LEFT;
    globalScanCodesToPositionCodeMap[333] = PF_RIGHT;
    
    globalScanCodesToPositionCodeMap[325] = PF_NUM_LOCK;
    globalScanCodesToPositionCodeMap[309] = PF_NUMPAD_DIVIDE;
    globalScanCodesToPositionCodeMap[55] = PF_NUMPAD_MULTIPLY;
    globalScanCodesToPositionCodeMap[74] = PF_NUMPAD_MINUS;
    globalScanCodesToPositionCodeMap[71] = PF_NUMPAD_7;
    globalScanCodesToPositionCodeMap[72] = PF_NUMPAD_8;
    globalScanCodesToPositionCodeMap[73] = PF_NUMPAD_9;
    globalScanCodesToPositionCodeMap[78] = PF_NUMPAD_PLUS;
    globalScanCodesToPositionCodeMap[75] = PF_NUMPAD_4;
    globalScanCodesToPositionCodeMap[76] = PF_NUMPAD_5;
    globalScanCodesToPositionCodeMap[77] = PF_NUMPAD_6;
    globalScanCodesToPositionCodeMap[79] = PF_NUMPAD_1;
    globalScanCodesToPositionCodeMap[80] = PF_NUMPAD_2;
    globalScanCodesToPositionCodeMap[81] = PF_NUMPAD_3;
    globalScanCodesToPositionCodeMap[82] = PF_NUMPAD_0;
    globalScanCodesToPositionCodeMap[83] = PF_NUMPAD_PERIOD;
    globalScanCodesToPositionCodeMap[284] = PF_NUMPAD_ENTER;
    
    // Virtual code mapping
    globalWindowsVkCodeToPfVkCodeMap[0] = PF_NULL;
    globalWindowsVkCodeToPfVkCodeMap[VK_ESCAPE] = PF_ESC;
    globalWindowsVkCodeToPfVkCodeMap[VK_F1] = PF_F1;
    globalWindowsVkCodeToPfVkCodeMap[VK_F2] = PF_F2;
    globalWindowsVkCodeToPfVkCodeMap[VK_F3] = PF_F3;
    globalWindowsVkCodeToPfVkCodeMap[VK_F4] = PF_F4;
    globalWindowsVkCodeToPfVkCodeMap[VK_F5] = PF_F5;
    globalWindowsVkCodeToPfVkCodeMap[VK_F6] = PF_F6;
    globalWindowsVkCodeToPfVkCodeMap[VK_F7] = PF_F7;
    globalWindowsVkCodeToPfVkCodeMap[VK_F8] = PF_F8;
    globalWindowsVkCodeToPfVkCodeMap[VK_F9] = PF_F9;
    globalWindowsVkCodeToPfVkCodeMap[VK_F10] = PF_F10;
    globalWindowsVkCodeToPfVkCodeMap[VK_F11] = PF_F11;
    globalWindowsVkCodeToPfVkCodeMap[VK_F12] = PF_F12;
    globalWindowsVkCodeToPfVkCodeMap[VK_PRINT] = PF_PRINT_SCREEN;
    globalWindowsVkCodeToPfVkCodeMap[VK_SCROLL] = PF_SCROLL_LOCK;
    globalWindowsVkCodeToPfVkCodeMap[VK_PAUSE] = PF_PAUSE;
    
    globalWindowsVkCodeToPfVkCodeMap[VK_OEM_3] = PF_TILDE;
    globalWindowsVkCodeToPfVkCodeMap[49] = PF_1;
    globalWindowsVkCodeToPfVkCodeMap[50] = PF_2;
    globalWindowsVkCodeToPfVkCodeMap[51] = PF_3;
    globalWindowsVkCodeToPfVkCodeMap[52] = PF_4;
    globalWindowsVkCodeToPfVkCodeMap[53] = PF_5;
    globalWindowsVkCodeToPfVkCodeMap[54] = PF_6;
    globalWindowsVkCodeToPfVkCodeMap[55] = PF_7;
    globalWindowsVkCodeToPfVkCodeMap[56] = PF_8;
    globalWindowsVkCodeToPfVkCodeMap[57] = PF_9;
    globalWindowsVkCodeToPfVkCodeMap[48] = PF_0;
    globalWindowsVkCodeToPfVkCodeMap[VK_OEM_MINUS] = PF_MINUS;
    globalWindowsVkCodeToPfVkCodeMap[VK_OEM_PLUS] = PF_EQUALS;
    globalWindowsVkCodeToPfVkCodeMap[VK_BACK] = PF_BACKSPACE;
    
    globalWindowsVkCodeToPfVkCodeMap[VK_TAB] = PF_TAB;
    globalWindowsVkCodeToPfVkCodeMap['Q'] = PF_Q;
    globalWindowsVkCodeToPfVkCodeMap['W'] = PF_W;
    globalWindowsVkCodeToPfVkCodeMap['E'] = PF_E;
    globalWindowsVkCodeToPfVkCodeMap['R'] = PF_R;
    globalWindowsVkCodeToPfVkCodeMap['T'] = PF_T;
    globalWindowsVkCodeToPfVkCodeMap['Y'] = PF_Y;
    globalWindowsVkCodeToPfVkCodeMap['U'] = PF_U;
    globalWindowsVkCodeToPfVkCodeMap['I'] = PF_I;
    globalWindowsVkCodeToPfVkCodeMap['O'] = PF_O;
    globalWindowsVkCodeToPfVkCodeMap['P'] = PF_P;
    globalWindowsVkCodeToPfVkCodeMap[VK_OEM_4] = PF_OPEN_SQUARE_BRACKET;
    globalWindowsVkCodeToPfVkCodeMap[VK_OEM_6] = PF_CLOSE_SQUARE_BRACKET;
    globalWindowsVkCodeToPfVkCodeMap[VK_OEM_5] = PF_BACKSLASH;
    
    globalWindowsVkCodeToPfVkCodeMap[VK_CAPITAL] = PF_CAPS_LOCK;
    globalWindowsVkCodeToPfVkCodeMap['A'] = PF_A;
    globalWindowsVkCodeToPfVkCodeMap['S'] = PF_S;
    globalWindowsVkCodeToPfVkCodeMap['D'] = PF_D;
    globalWindowsVkCodeToPfVkCodeMap['F'] = PF_F;
    globalWindowsVkCodeToPfVkCodeMap['G'] = PF_G;
    globalWindowsVkCodeToPfVkCodeMap['H'] = PF_H;
    globalWindowsVkCodeToPfVkCodeMap['J'] = PF_J;
    globalWindowsVkCodeToPfVkCodeMap['K'] = PF_K;
    globalWindowsVkCodeToPfVkCodeMap['L'] = PF_L;
    globalWindowsVkCodeToPfVkCodeMap[VK_OEM_1] = PF_SEMICOLON;
    globalWindowsVkCodeToPfVkCodeMap[VK_OEM_7] = PF_APOSTROPHE;
    globalWindowsVkCodeToPfVkCodeMap[VK_RETURN] = PF_ENTER;
    
    globalWindowsVkCodeToPfVkCodeMap[VK_LSHIFT] = PF_LEFT_SHIFT;
    globalWindowsVkCodeToPfVkCodeMap['Z'] = PF_Z;
    globalWindowsVkCodeToPfVkCodeMap['X'] = PF_X;
    globalWindowsVkCodeToPfVkCodeMap['C'] = PF_C;
    globalWindowsVkCodeToPfVkCodeMap['V'] = PF_V;
    globalWindowsVkCodeToPfVkCodeMap['B'] = PF_B;
    globalWindowsVkCodeToPfVkCodeMap['N'] = PF_N;
    globalWindowsVkCodeToPfVkCodeMap['M'] = PF_M;
    globalWindowsVkCodeToPfVkCodeMap[VK_OEM_COMMA] = PF_COMMA;
    globalWindowsVkCodeToPfVkCodeMap[VK_OEM_PERIOD] = PF_PERIOD;
    globalWindowsVkCodeToPfVkCodeMap[VK_OEM_2] = PF_FORWARD_SLASH;
    globalWindowsVkCodeToPfVkCodeMap[VK_RSHIFT] = PF_RIGHT_SHIFT;
    
    globalWindowsVkCodeToPfVkCodeMap[VK_LCONTROL] = PF_LEFT_CTRL;
    globalWindowsVkCodeToPfVkCodeMap[VK_LWIN] = PF_LEFT_WIN;
    globalWindowsVkCodeToPfVkCodeMap[VK_LMENU] = PF_LEFT_ALT;
    globalWindowsVkCodeToPfVkCodeMap[VK_SPACE] = PF_SPACEBAR;
    globalWindowsVkCodeToPfVkCodeMap[VK_RMENU] = PF_RIGHT_ALT;
    globalWindowsVkCodeToPfVkCodeMap[VK_RWIN] = PF_RIGHT_WIN;
    //TODO(KARAN)] VkCode for PF_MENU?? globalWindowsVkCodeToPfVkCodeMap[VK_] = PF_MENU;
    globalWindowsVkCodeToPfVkCodeMap[VK_RCONTROL] = PF_RIGHT_CTRL;
    
    globalWindowsVkCodeToPfVkCodeMap[VK_INSERT] = PF_INSERT;
    globalWindowsVkCodeToPfVkCodeMap[VK_HOME] = PF_HOME;
    globalWindowsVkCodeToPfVkCodeMap[VK_PRIOR] = PF_PAGE_UP;
    globalWindowsVkCodeToPfVkCodeMap[VK_DELETE] = PF_DELETE;
    globalWindowsVkCodeToPfVkCodeMap[VK_END] = PF_END;
    globalWindowsVkCodeToPfVkCodeMap[VK_NEXT] = PF_PAGE_DOWN;
    globalWindowsVkCodeToPfVkCodeMap[VK_CLEAR] = PF_CLEAR;
    
    globalWindowsVkCodeToPfVkCodeMap[VK_UP] = PF_UP;
    globalWindowsVkCodeToPfVkCodeMap[VK_DOWN] = PF_DOWN;
    globalWindowsVkCodeToPfVkCodeMap[VK_LEFT] = PF_LEFT;
    globalWindowsVkCodeToPfVkCodeMap[VK_RIGHT] = PF_RIGHT;
    
    globalWindowsVkCodeToPfVkCodeMap[VK_NUMLOCK] = PF_NUM_LOCK;
    globalWindowsVkCodeToPfVkCodeMap[VK_DIVIDE] = PF_NUMPAD_DIVIDE;
    globalWindowsVkCodeToPfVkCodeMap[VK_MULTIPLY] = PF_NUMPAD_MULTIPLY;
    globalWindowsVkCodeToPfVkCodeMap[VK_SUBTRACT] = PF_NUMPAD_MINUS;
    globalWindowsVkCodeToPfVkCodeMap[VK_NUMPAD7] = PF_NUMPAD_7;
    globalWindowsVkCodeToPfVkCodeMap[VK_NUMPAD8] = PF_NUMPAD_8;
    globalWindowsVkCodeToPfVkCodeMap[VK_NUMPAD9] = PF_NUMPAD_9;
    globalWindowsVkCodeToPfVkCodeMap[VK_ADD] = PF_NUMPAD_PLUS;
    globalWindowsVkCodeToPfVkCodeMap[VK_NUMPAD4] = PF_NUMPAD_4;
    globalWindowsVkCodeToPfVkCodeMap[VK_NUMPAD5] = PF_NUMPAD_5;
    globalWindowsVkCodeToPfVkCodeMap[VK_NUMPAD6] = PF_NUMPAD_6;
    globalWindowsVkCodeToPfVkCodeMap[VK_NUMPAD1] = PF_NUMPAD_1;
    globalWindowsVkCodeToPfVkCodeMap[VK_NUMPAD2] = PF_NUMPAD_2;
    globalWindowsVkCodeToPfVkCodeMap[VK_NUMPAD3] = PF_NUMPAD_3;
    globalWindowsVkCodeToPfVkCodeMap[VK_NUMPAD0] = PF_NUMPAD_0;
    globalWindowsVkCodeToPfVkCodeMap[VK_DECIMAL] = PF_NUMPAD_PERIOD;
    
    
#if 0
    // For every virtual keycode, find a scan code.
    for(int i = 0; i < 256; i++)
    {
        uint32 vkCode = i;
        PfKeyCode pfVirtualKeyCode = PF_NULL;
        switch(vkCode)
        {
            case 0: symbolicKeyCode = PF_NULL; break;
            case VK_ESCAPE: symbolicKeyCode = PF_ESC; break;
            case VK_F1: symbolicKeyCode = PF_F1; break;
            case VK_F2: symbolicKeyCode = PF_F2; break;
            case VK_F3: symbolicKeyCode = PF_F3; break;
            case VK_F4: symbolicKeyCode = PF_F4; break;
            case VK_F5: symbolicKeyCode = PF_F5; break;
            case VK_F6: symbolicKeyCode = PF_F6; break;
            case VK_F7: symbolicKeyCode = PF_F7; break;
            case VK_F8: symbolicKeyCode = PF_F8; break;
            case VK_F9: symbolicKeyCode = PF_F9; break;
            case VK_F10: symbolicKeyCode = PF_F10; break;
            case VK_F11: symbolicKeyCode = PF_F11; break;
            case VK_F12: symbolicKeyCode = PF_F12; break;
            case VK_PRINT: symbolicKeyCode = PF_PRINT_SCREEN; break;
            case VK_SCROLL: symbolicKeyCode = PF_SCROLL_LOCK; break;
            case VK_PAUSE: symbolicKeyCode = PF_PAUSE; break;
            
            case VK_OEM_3: symbolicKeyCode = PF_TILDE; break;
            case 49: symbolicKeyCode = PF_1; break;
            case 50: symbolicKeyCode = PF_2; break;
            case 51: symbolicKeyCode = PF_3; break;
            case 52: symbolicKeyCode = PF_4; break;
            case 53: symbolicKeyCode = PF_5; break;
            case 54: symbolicKeyCode = PF_6; break;
            case 55: symbolicKeyCode = PF_7; break;
            case 56: symbolicKeyCode = PF_8; break;
            case 57: symbolicKeyCode = PF_9; break;
            case 48: symbolicKeyCode = PF_0; break;
            case VK_OEM_MINUS: symbolicKeyCode = PF_MINUS; break;
            case VK_OEM_PLUS: symbolicKeyCode = PF_EQUALS; break;
            case VK_BACK: symbolicKeyCode = PF_BACKSPACE; break;
            
            case VK_TAB: symbolicKeyCode = PF_TAB; break;
            case 'Q': symbolicKeyCode = PF_Q; break;
            case 'W': symbolicKeyCode = PF_W; break;
            case 'E': symbolicKeyCode = PF_E; break;
            case 'R': symbolicKeyCode = PF_R; break;
            case 'T': symbolicKeyCode = PF_T; break;
            case 'Y': symbolicKeyCode = PF_Y; break;
            case 'U': symbolicKeyCode = PF_U; break;
            case 'I': symbolicKeyCode = PF_I; break;
            case 'O': symbolicKeyCode = PF_O; break;
            case 'P': symbolicKeyCode = PF_P; break;
            case VK_OEM_4: symbolicKeyCode = PF_OPEN_SQUARE_BRACKET; break;
            case VK_OEM_6: symbolicKeyCode = PF_CLOSE_SQUARE_BRACKET; break;
            case VK_OEM_5: symbolicKeyCode = PF_BACKSLASH; break;
            
            case VK_CAPITAL: symbolicKeyCode = PF_CAPS_LOCK; break;
            case 'A': symbolicKeyCode = PF_A; break;
            case 'S': symbolicKeyCode = PF_S; break;
            case 'D': symbolicKeyCode = PF_D; break;
            case 'F': symbolicKeyCode = PF_F; break;
            case 'G': symbolicKeyCode = PF_G; break;
            case 'H': symbolicKeyCode = PF_H; break;
            case 'J': symbolicKeyCode = PF_J; break;
            case 'K': symbolicKeyCode = PF_K; break;
            case 'L': symbolicKeyCode = PF_L; break;
            case VK_OEM_1: symbolicKeyCode = PF_SEMICOLON; break;
            case VK_OEM_7: symbolicKeyCode = PF_APOSTROPHE; break;
            case VK_RETURN: symbolicKeyCode = PF_ENTER; break;
            
            case VK_LSHIFT: symbolicKeyCode = PF_LEFT_SHIFT; break;
            case 'Z': symbolicKeyCode = PF_Z; break;
            case 'X': symbolicKeyCode = PF_X; break;
            case 'C': symbolicKeyCode = PF_C; break;
            case 'V': symbolicKeyCode = PF_V; break;
            case 'B': symbolicKeyCode = PF_B; break;
            case 'N': symbolicKeyCode = PF_N; break;
            case 'M': symbolicKeyCode = PF_M; break;
            case VK_OEM_COMMA: symbolicKeyCode = PF_COMMA; break;
            case VK_OEM_PERIOD: symbolicKeyCode = PF_PERIOD; break;
            case VK_OEM_2: symbolicKeyCode = PF_FORWARD_SLASH; break;
            case VK_RSHIFT: symbolicKeyCode = PF_RIGHT_SHIFT; break;
            
            case VK_LCONTROL: symbolicKeyCode = PF_LEFT_CTRL; break;
            case VK_LWIN: symbolicKeyCode = PF_LEFT_WIN; break;
            case VK_LMENU: symbolicKeyCode = PF_LEFT_ALT; break;
            case VK_SPACE: symbolicKeyCode = PF_SPACEBAR; break;
            case VK_RMENU: symbolicKeyCode = PF_RIGHT_ALT; break;
            case VK_RWIN: symbolicKeyCode = PF_RIGHT_WIN; break;
            //TODO(KARAN): VkCode for PF_MENU?? case VK_: symbolicKeyCode = PF_MENU; break;
            case VK_RCONTROL: symbolicKeyCode = PF_RIGHT_CTRL; break;
            
            case VK_INSERT: symbolicKeyCode = PF_INSERT; break;
            case VK_HOME: symbolicKeyCode = PF_HOME; break;
            case VK_PRIOR: symbolicKeyCode = PF_PAGE_UP; break;
            case VK_DELETE: symbolicKeyCode = PF_DELETE; break;
            case VK_END: symbolicKeyCode = PF_END; break;
            case VK_NEXT: symbolicKeyCode = PF_PAGE_DOWN; break;
            
            case VK_UP: symbolicKeyCode = PF_UP; break;
            case VK_DOWN: symbolicKeyCode = PF_DOWN; break;
            case VK_LEFT: symbolicKeyCode = PF_LEFT; break;
            case VK_RIGHT: symbolicKeyCode = PF_RIGHT; break;
            
            case VK_NUMLOCK: symbolicKeyCode = PF_NUM_LOCK; break;
            case VK_DIVIDE: symbolicKeyCode = PF_NUMPAD_DIVIDE; break;
            case VK_MULTIPLY: symbolicKeyCode = PF_NUMPAD_MULTIPLY; break;
            case VK_SUBTRACT: symbolicKeyCode = PF_NUMPAD_MINUS; break;
            case VK_NUMPAD7: symbolicKeyCode = PF_NUMPAD_7; break;
            case VK_NUMPAD8: symbolicKeyCode = PF_NUMPAD_8; break;
            case VK_NUMPAD9: symbolicKeyCode = PF_NUMPAD_9; break;
            case VK_ADD: symbolicKeyCode = PF_NUMPAD_PLUS; break;
            case VK_NUMPAD4: symbolicKeyCode = PF_NUMPAD_4; break;
            case VK_NUMPAD5: symbolicKeyCode = PF_NUMPAD_5; break;
            case VK_NUMPAD6: symbolicKeyCode = PF_NUMPAD_6; break;
            case VK_NUMPAD1: symbolicKeyCode = PF_NUMPAD_1; break;
            case VK_NUMPAD2: symbolicKeyCode = PF_NUMPAD_2; break;
            case VK_NUMPAD3: symbolicKeyCode = PF_NUMPAD_3; break;
            case VK_NUMPAD0: symbolicKeyCode = PF_NUMPAD_0; break;
            case VK_DECIMAL: symbolicKeyCode = PF_NUMPAD_PERIOD; break;
            //TODO(KARAN): Handle separately case VK_: symbolicKeyCode = PF_NUMPAD_ENTER; break;
            
            default: symbolicKeyCode = PF_NULL; break;
        }
        
        globalScanCodesToSymbolCodeMap[scanCode] = symbolicKeyCode;
        
        char str[512] = {};
        GetKeyNameTextA((LONG)(scanCode << 16), str, 512);
        if(globalScanCodesToPositionCodeMap[scanCode] != PF_NULL)
        {
            DEBUG_LOG(stdout, "%03x | %02x | %s | %s | %s\n", scanCode, vkCode, pfKeyCodeStr[globalScanCodesToPositionCodeMap[scanCode]], pfKeyCodeStr[globalScanCodesToSymbolCodeMap[scanCode]], str);
        }
    }
    
    DEBUG_LOG(stdout, "\n---------------------------------\n\n");
    
    DEBUG_LOG(stdout, "SCAN CODE | VK CODE | POSITION CODE | SYMBOL CODE | TEXT NAME\n");
    for(int i = 0; i < ARRAY_COUNT(globalScanCodesToPositionCodeMap); i++)
    {
        uint32 scanCode = i;
        if(globalScanCodesToSymbolCodeMap[i] == PF_NULL)
        {
            PfKeyCode symbolicKeyCode = PF_NULL;
            uint32 vkCode = MapVirtualKeyA(scanCode, MAPVK_VSC_TO_VK_EX);
            
            if(vkCode == 0 && ((scanCode & 0x100) != 0))
            {
                vkCode = MapVirtualKeyA(scanCode & 0x0FF, MAPVK_VSC_TO_VK_EX);
            }
            
            switch(vkCode)
            {
                case 0: symbolicKeyCode = PF_NULL; break;
                case VK_ESCAPE: symbolicKeyCode = PF_ESC; break;
                case VK_F1: symbolicKeyCode = PF_F1; break;
                case VK_F2: symbolicKeyCode = PF_F2; break;
                case VK_F3: symbolicKeyCode = PF_F3; break;
                case VK_F4: symbolicKeyCode = PF_F4; break;
                case VK_F5: symbolicKeyCode = PF_F5; break;
                case VK_F6: symbolicKeyCode = PF_F6; break;
                case VK_F7: symbolicKeyCode = PF_F7; break;
                case VK_F8: symbolicKeyCode = PF_F8; break;
                case VK_F9: symbolicKeyCode = PF_F9; break;
                case VK_F10: symbolicKeyCode = PF_F10; break;
                case VK_F11: symbolicKeyCode = PF_F11; break;
                case VK_F12: symbolicKeyCode = PF_F12; break;
                case VK_PRINT: symbolicKeyCode = PF_PRINT_SCREEN; break;
                case VK_SCROLL: symbolicKeyCode = PF_SCROLL_LOCK; break;
                case VK_PAUSE: symbolicKeyCode = PF_PAUSE; break;
                
                case VK_OEM_3: symbolicKeyCode = PF_TILDE; break;
                case 49: symbolicKeyCode = PF_1; break;
                case 50: symbolicKeyCode = PF_2; break;
                case 51: symbolicKeyCode = PF_3; break;
                case 52: symbolicKeyCode = PF_4; break;
                case 53: symbolicKeyCode = PF_5; break;
                case 54: symbolicKeyCode = PF_6; break;
                case 55: symbolicKeyCode = PF_7; break;
                case 56: symbolicKeyCode = PF_8; break;
                case 57: symbolicKeyCode = PF_9; break;
                case 48: symbolicKeyCode = PF_0; break;
                case VK_OEM_MINUS: symbolicKeyCode = PF_MINUS; break;
                case VK_OEM_PLUS: symbolicKeyCode = PF_EQUALS; break;
                case VK_BACK: symbolicKeyCode = PF_BACKSPACE; break;
                
                case VK_TAB: symbolicKeyCode = PF_TAB; break;
                case 'Q': symbolicKeyCode = PF_Q; break;
                case 'W': symbolicKeyCode = PF_W; break;
                case 'E': symbolicKeyCode = PF_E; break;
                case 'R': symbolicKeyCode = PF_R; break;
                case 'T': symbolicKeyCode = PF_T; break;
                case 'Y': symbolicKeyCode = PF_Y; break;
                case 'U': symbolicKeyCode = PF_U; break;
                case 'I': symbolicKeyCode = PF_I; break;
                case 'O': symbolicKeyCode = PF_O; break;
                case 'P': symbolicKeyCode = PF_P; break;
                case VK_OEM_4: symbolicKeyCode = PF_OPEN_SQUARE_BRACKET; break;
                case VK_OEM_6: symbolicKeyCode = PF_CLOSE_SQUARE_BRACKET; break;
                case VK_OEM_5: symbolicKeyCode = PF_BACKSLASH; break;
                
                case VK_CAPITAL: symbolicKeyCode = PF_CAPS_LOCK; break;
                case 'A': symbolicKeyCode = PF_A; break;
                case 'S': symbolicKeyCode = PF_S; break;
                case 'D': symbolicKeyCode = PF_D; break;
                case 'F': symbolicKeyCode = PF_F; break;
                case 'G': symbolicKeyCode = PF_G; break;
                case 'H': symbolicKeyCode = PF_H; break;
                case 'J': symbolicKeyCode = PF_J; break;
                case 'K': symbolicKeyCode = PF_K; break;
                case 'L': symbolicKeyCode = PF_L; break;
                case VK_OEM_1: symbolicKeyCode = PF_SEMICOLON; break;
                case VK_OEM_7: symbolicKeyCode = PF_APOSTROPHE; break;
                case VK_RETURN: symbolicKeyCode = PF_ENTER; break;
                
                case VK_LSHIFT: symbolicKeyCode = PF_LEFT_SHIFT; break;
                case 'Z': symbolicKeyCode = PF_Z; break;
                case 'X': symbolicKeyCode = PF_X; break;
                case 'C': symbolicKeyCode = PF_C; break;
                case 'V': symbolicKeyCode = PF_V; break;
                case 'B': symbolicKeyCode = PF_B; break;
                case 'N': symbolicKeyCode = PF_N; break;
                case 'M': symbolicKeyCode = PF_M; break;
                case VK_OEM_COMMA: symbolicKeyCode = PF_COMMA; break;
                case VK_OEM_PERIOD: symbolicKeyCode = PF_PERIOD; break;
                case VK_OEM_2: symbolicKeyCode = PF_FORWARD_SLASH; break;
                case VK_RSHIFT: symbolicKeyCode = PF_RIGHT_SHIFT; break;
                
                case VK_LCONTROL: symbolicKeyCode = PF_LEFT_CTRL; break;
                case VK_LWIN: symbolicKeyCode = PF_LEFT_WIN; break;
                case VK_LMENU: symbolicKeyCode = PF_LEFT_ALT; break;
                case VK_SPACE: symbolicKeyCode = PF_SPACEBAR; break;
                case VK_RMENU: symbolicKeyCode = PF_RIGHT_ALT; break;
                case VK_RWIN: symbolicKeyCode = PF_RIGHT_WIN; break;
                //TODO(KARAN): VkCode for PF_MENU?? case VK_: symbolicKeyCode = PF_MENU; break;
                case VK_RCONTROL: symbolicKeyCode = PF_RIGHT_CTRL; break;
                
                case VK_INSERT: symbolicKeyCode = PF_INSERT; break;
                case VK_HOME: symbolicKeyCode = PF_HOME; break;
                case VK_PRIOR: symbolicKeyCode = PF_PAGE_UP; break;
                case VK_DELETE: symbolicKeyCode = PF_DELETE; break;
                case VK_END: symbolicKeyCode = PF_END; break;
                case VK_NEXT: symbolicKeyCode = PF_PAGE_DOWN; break;
                
                case VK_UP: symbolicKeyCode = PF_UP; break;
                case VK_DOWN: symbolicKeyCode = PF_DOWN; break;
                case VK_LEFT: symbolicKeyCode = PF_LEFT; break;
                case VK_RIGHT: symbolicKeyCode = PF_RIGHT; break;
                
                case VK_NUMLOCK: symbolicKeyCode = PF_NUM_LOCK; break;
                case VK_DIVIDE: symbolicKeyCode = PF_NUMPAD_DIVIDE; break;
                case VK_MULTIPLY: symbolicKeyCode = PF_NUMPAD_MULTIPLY; break;
                case VK_SUBTRACT: symbolicKeyCode = PF_NUMPAD_MINUS; break;
                case VK_NUMPAD7: symbolicKeyCode = PF_NUMPAD_7; break;
                case VK_NUMPAD8: symbolicKeyCode = PF_NUMPAD_8; break;
                case VK_NUMPAD9: symbolicKeyCode = PF_NUMPAD_9; break;
                case VK_ADD: symbolicKeyCode = PF_NUMPAD_PLUS; break;
                case VK_NUMPAD4: symbolicKeyCode = PF_NUMPAD_4; break;
                case VK_NUMPAD5: symbolicKeyCode = PF_NUMPAD_5; break;
                case VK_NUMPAD6: symbolicKeyCode = PF_NUMPAD_6; break;
                case VK_NUMPAD1: symbolicKeyCode = PF_NUMPAD_1; break;
                case VK_NUMPAD2: symbolicKeyCode = PF_NUMPAD_2; break;
                case VK_NUMPAD3: symbolicKeyCode = PF_NUMPAD_3; break;
                case VK_NUMPAD0: symbolicKeyCode = PF_NUMPAD_0; break;
                case VK_DECIMAL: symbolicKeyCode = PF_NUMPAD_PERIOD; break;
                //TODO(KARAN): Handle separately case VK_: symbolicKeyCode = PF_NUMPAD_ENTER; break;
                
                default: symbolicKeyCode = PF_NULL; break;
            }
            
            globalScanCodesToSymbolCodeMap[scanCode] = symbolicKeyCode;
            char str[512] = {};
            GetKeyNameTextA((LONG)(scanCode << 16), str, 512);
            if(globalScanCodesToPositionCodeMap[scanCode] != PF_NULL)
            {
                DEBUG_LOG(stdout, "%03x | %02x | %s | %s | %s\n", scanCode, vkCode, pfKeyCodeStr[globalScanCodesToPositionCodeMap[scanCode]], pfKeyCodeStr[globalScanCodesToSymbolCodeMap[scanCode]], str);
            }
        }
    }
#endif
}

#if 1
void WinCreateOpenGLContext(PfWindow *window)
{
    int32 desiredPixelFormatARB[] =
    {
        WGL_DRAW_TO_WINDOW_ARB, GL_TRUE,
        WGL_SUPPORT_OPENGL_ARB, GL_TRUE,
        WGL_DOUBLE_BUFFER_ARB, GL_TRUE,
        WGL_PIXEL_TYPE_ARB, WGL_TYPE_RGBA_ARB,
        WGL_COLOR_BITS_ARB, 32,
        WGL_DEPTH_BITS_ARB, 24,
        WGL_STENCIL_BITS_ARB, 8,
        0, // End
    };
    
    int32 closestPixelFormatIndex;
    uint32 numFormats;
    BOOL choosingSuccess = wglChoosePixelFormatARB(window->deviceContext, desiredPixelFormatARB, NULL, 1, &closestPixelFormatIndex, &numFormats);
    ASSERT(choosingSuccess == TRUE, "Couldn't choose desired pixel format");
    
    PIXELFORMATDESCRIPTOR desiredPixelFormat;
    int32 describeSuccess =  DescribePixelFormat(window->deviceContext, closestPixelFormatIndex, sizeof(desiredPixelFormat), &desiredPixelFormat);
    ASSERT(describeSuccess, "Couldn't get the description of choosen pixel format");
    
    BOOL setPixelFormatSuccess = SetPixelFormat(window->deviceContext, closestPixelFormatIndex, &desiredPixelFormat);
    ASSERT(setPixelFormatSuccess == TRUE, "Couldn't set window's DC pixel format");
    
    // TODO(KARAN): Shared context for async texture uploads
    HGLRC sharedContext = 0;
    
    //WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
    int32 profile = WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB;
    if(globalCoreProfile)
    {
        profile = WGL_CONTEXT_CORE_PROFILE_BIT_ARB;
    }
    
    int32 desiredContextAttribs[] = 
    {
        WGL_CONTEXT_MAJOR_VERSION_ARB, globalGLMajorVersion,
        WGL_CONTEXT_MINOR_VERSION_ARB, globalGLMinorVersion,
#if DEBUG_BUILD 
        WGL_CONTEXT_FLAGS_ARB, WGL_CONTEXT_DEBUG_BIT_ARB,
#endif
        WGL_CONTEXT_PROFILE_MASK_ARB, profile,
        0
    };
    
    if(wglCreateContextAttribsARB)
    {
        window->glContext = wglCreateContextAttribsARB(window->deviceContext, sharedContext, desiredContextAttribs);
    }
    else
    {
        window->glContext = wglCreateContext(window->deviceContext);
        DEBUG_ERROR("Couldn't create a modern OpenGL context. wgl create context arb extension not available"); 
    }
    
    ASSERT(window->glContext, "Couldn't create OpenGL context");
}
#endif

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
    
    ASSERT(bitmapMemorySize >= 0, "Window buffer size cannot be negative");
    
    (window->offscreenBuffer.info).bmiHeader.biSize = sizeof((window->offscreenBuffer.info).bmiHeader);
    (window->offscreenBuffer.info).bmiHeader.biWidth = width;
    
    // NOTE(KARAN): Negative height to set origin at top left corner
    (window->offscreenBuffer.info).bmiHeader.biHeight = -height;
    (window->offscreenBuffer.info).bmiHeader.biPlanes = 1;
    (window->offscreenBuffer.info).bmiHeader.biBitCount = 32;
    
    if(bitmapMemorySize > 0)
    {
        window->offscreenBuffer.data = VirtualAlloc(0, bitmapMemorySize, MEM_COMMIT | MEM_RESERVE,PAGE_READWRITE);
        
        if(window->glContext)
        {
            PfglMakeCurrent(window);
            GL_CALL(glViewport(0, 0, width, height));
        }
        ASSERT(window->offscreenBuffer.data, "Window buffer allocation failed");
    }
    
}


void PfCreateWindow(PfWindow *window, char *title, int32 xPos, int32 yPos, int32 width, int32 height)
{
    PfWindow clear  = {};
    *window = clear;
    
    DWORD windowStyle = WS_OVERLAPPEDWINDOW | WS_VISIBLE;
    
    RECT windowRect = {};
    windowRect.left = 0;
    windowRect.top = 0;
    windowRect.right = width;
    windowRect.bottom = height;
    
    BOOL windowRectResult = AdjustWindowRect(&windowRect, windowStyle, 0);
    
    windowRect.right = windowRect.right - windowRect.left;
    windowRect.bottom = windowRect.bottom - windowRect.top;
    
    window->windowHandle = CreateWindowEx(0,globalWindowClass.lpszClassName, title, windowStyle, xPos, yPos, windowRect.right, windowRect.bottom, 0, 0, globalWindowClass.hInstance,0);
    
    
    /* MSDN:  "To avoid retrieving a device context each time it needs to paint inside a window, an application can specify the CS_OWNDC style for the window class. This class style directs the system to create a private device context — that is, to allocate a unique device context for each window in the class. The application need only retrieve the context once and then use it for all subsequent painting." */
    window->deviceContext = GetDC(window->windowHandle);
    
    if(window->windowHandle == 0)
    {
        DEBUG_ERROR("Could not create window");
    }
    PfResizeWindow(window, width, height);
    
    BOOL propertySetResult = SetPropA(window->windowHandle, "PfWindow", window);
    HWND prevFocusWindowHandle = SetFocus(window->windowHandle);
    ASSERT(prevFocusWindowHandle != 0, "");
    
    // NOTE(KARAN): If the window already had keyboard focus, WM_SETFOCUS message isn't sent to the WINDOWPROC. Hence this hack.
    if(prevFocusWindowHandle == window->windowHandle)
    {
        window->hasKeyboardFocus = true;
    }
    
    ASSERT(propertySetResult != 0, "Couldn't associate PfWindow* with HWND window handle");
    
    WinCreateOpenGLContext(window);
    // HACK(KARAN): Creation of texture and vertices for rendering offscreenbuffer via opengl
    // Adding this so that code can be compiled.
    PfglMakeCurrent(window);
    
#if PF_GLEW_ENABLED
    GLenum err = glewInit();
    if (GLEW_OK == err)
    {
        DEBUG_ERROR("GLEW INTIALIZATION FAILED");
    }
#endif
    
    DEBUG_LOG("OpenGL version: %s\n\n", glGetString(GL_VERSION));
    
    uint32 vbo, vao, ebo;
    GL_CALL(glGenVertexArrays(1, &vao));
    GL_CALL(glGenBuffers(1, &vbo));
    GL_CALL(glGenBuffers(1, &ebo));
    
    window->vao = vao;
    GL_CALL(glBindVertexArray(vao));
    
    real32 xMax = 1.0f;
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
    
    GL_CALL(glBindBuffer(GL_ARRAY_BUFFER, vbo));
    GL_CALL(glBufferData(GL_ARRAY_BUFFER, ARRAY_COUNT(vertices) * sizeof(real32), vertices, GL_STATIC_DRAW));
    
    GL_CALL(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo));
    GL_CALL(glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW));
    
    // position attribute
    GL_CALL(glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0));
    GL_CALL(glEnableVertexAttribArray(0));
    // texture coord attribute
    GL_CALL(glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float))));
    GL_CALL(glEnableVertexAttribArray(1));
    
    GL_CALL(glBindVertexArray(0));
    
    uint32 vertexShader;
    GL_CALL(vertexShader = glCreateShader(GL_VERTEX_SHADER));
    char *vertexShaderSource = "#version 430 core\nlayout (location = 0) in vec3 aPos;layout (location = 1) in vec2 aTexCoord;out vec3 ourColor; out vec2 TexCoord; void main() { gl_Position = vec4(aPos, 1.0); ourColor = vec3(1.0f, 1.0f, 1.0f); TexCoord = vec2(aTexCoord.x, aTexCoord.y);}\0";
    GL_CALL(glShaderSource(vertexShader, 1, &vertexShaderSource, 0));
    GL_CALL(glCompileShader(vertexShader));
    
    int32 success;
    GL_CALL(glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success));
    if (!success)
    {
        char infoLog[512] = {};
        GL_CALL(glGetShaderInfoLog(vertexShader, 512, NULL, infoLog));
        DEBUG_ERROR("%s", infoLog);
    }
    
    
    uint32 fragmentShader;
    GL_CALL(fragmentShader = glCreateShader(GL_FRAGMENT_SHADER));
    char *fragmentShaderSource = "#version 430 core\nout vec4 FragColor; in vec3 ourColor; in vec2 TexCoord; uniform sampler2D texture1; void main(){FragColor = texture(texture1, TexCoord);}\0";
    GL_CALL(glShaderSource(fragmentShader, 1, &fragmentShaderSource, 0));
    GL_CALL(glCompileShader(fragmentShader));
    
    GL_CALL(glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success));
    if (!success)
    {
        char infoLog[512] = {};
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
    
    GL_CALL(glViewport(0, 0, width, height));
    
    GL_CALL(glActiveTexture(GL_TEXTURE0));
    GL_CALL(glGenTextures(1, &(window->offscreenBufferTextureId)));
    GL_CALL(glBindTexture(GL_TEXTURE_2D, window->offscreenBufferTextureId));
    
    GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST));
    GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST));
    GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER));
    GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER));
    GLfloat debugColor[] = {1.0f, 0.0f, 1.0f, 1.0f};
    GL_CALL(glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, debugColor));
    
    GL_CALL(glBindTexture(GL_TEXTURE_2D, 0));
    
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
    /* HACK(KARAN): stretchdibits doesn't work in fullscreen mode
    when the window has an opengl context. So when in fullscreen mode
    and if we have a glContext, use opengl texture rendering path.
    */
#define DEBUG_ENABLE_HARDWARE_BLIT 1
#if DEBUG_ENABLE_HARDWARE_BLIT
    if(window->fullscreen && (window->glContext != 0))
    {
        PfglMakeCurrent(window);
        PfglRenderWindow(window);
        PfglSwapBuffers(window);
        PfglMakeCurrent(0);
    }
    else
    {
#endif
        HDC deviceContextHandle = window->deviceContext;//GetDC(window->windowHandle);
        int32 scanLines = StretchDIBits(deviceContextHandle,
                                        0, 0,window->offscreenBuffer.width, window->offscreenBuffer.height,
                                        0,0,window->offscreenBuffer.width, window->offscreenBuffer.height,
                                        window->offscreenBuffer.data, &(window->offscreenBuffer.info),DIB_RGB_COLORS,SRCCOPY);
        
        ASSERT(scanLines == window->offscreenBuffer.height, "Did not blit the entire height of the buffer");
#if DEBUG_ENABLE_HARDWARE_BLIT
    }
#endif
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
            window->fullscreen = true;
        }
    }
    else
    {
        SetWindowLong(window->windowHandle, GWL_STYLE, windowStyle | WS_OVERLAPPEDWINDOW);
        SetWindowPlacement(window->windowHandle, &(window->prevWindowPlacement));
        SetWindowPos(window->windowHandle, 0, 0, 0, 0, 0,
                     SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER |
                     SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
        window->fullscreen = false;
    }
    PfglMakeCurrent(window);
    PfRect clientRect = PfGetClientRect(window);
    GL_CALL(glViewport(0, 0, clientRect.width, clientRect.height));
}


int32 PfGetKeyState(PfKeyCode keyCode, bool isVkCode = false)
{
    int32 result;
    int32 arrayIndex = isVkCode ? 1 : 0;
    result = globalKeyboard[arrayIndex][keyCode];
    return result;
}

int32 PfGetKeyState(PfWindow *window, PfKeyCode keyCode, bool isVkCode = false)
{
    int32 result = 0;
    int32 arrayIndex = isVkCode ? 1 : 0;
    if(window->hasKeyboardFocus)//windowHandle == globalKeyboardFocusWindowHandle)
    {
        result = globalKeyboard[arrayIndex][keyCode];
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

inline void PfglMakeCurrent(PfWindow *window)
{
    BOOL makeCurrentSuccess = TRUE;
    
    if(window)
    {
        makeCurrentSuccess = wglMakeCurrent(window->deviceContext, window->glContext);
    }
    
    ASSERT(makeCurrentSuccess == TRUE, "Failed to make window's OpenGL context current");
}

void PfglRenderWindow(PfWindow *window)
{
    GL_CALL(GLboolean wasBlendEnabled = glIsEnabled(GL_BLEND));
    
    GL_CALL(glEnable(GL_BLEND));
    GL_CALL(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));
    
    GL_CALL(glActiveTexture(GL_TEXTURE0));
    GL_CALL(glBindTexture(GL_TEXTURE_2D, window->offscreenBufferTextureId));
    GL_CALL(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, window[0].offscreenBuffer.width, window[0].offscreenBuffer.height, 0,
                         GL_BGRA, GL_UNSIGNED_BYTE, window[0].offscreenBuffer.data));
    
    GL_CALL(glBindVertexArray(window->vao));
    GL_CALL(glUseProgram(window->programId));
    GL_CALL(GLint samplerUniformLocation = glGetUniformLocation(window->programId, "texture1"));
    GL_CALL(glUniform1i(samplerUniformLocation, 0)); // Setting the texture unit
    GL_CALL(glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0));
    GL_CALL(glUseProgram(0));
    GL_CALL(glBindVertexArray(0));
    GL_CALL(glBindTexture(GL_TEXTURE_2D, 0));
    
    if(wasBlendEnabled == GL_FALSE)
    {
        glDisable(GL_BLEND);
    }
}


void PfGLConfig(int32 glMajorVersion, int32 glMinorVersion, bool coreProfile)
{
    globalGLMajorVersion = glMajorVersion;
    globalGLMinorVersion = glMinorVersion;
    globalCoreProfile = coreProfile;
}


void PfglSwapBuffers(PfWindow *window)
{
    SwapBuffers(window->deviceContext);
}

void PfSleep(int32 milliseconds)
{
    if(milliseconds > 0)
    {
        Sleep(milliseconds);
    }
}

void PfUpdate()
{
    globalMouseButtons[0] = GetKeyState(VK_LBUTTON) >> 15;
    globalMouseButtons[1] = GetKeyState(VK_MBUTTON) >> 15;
    globalMouseButtons[2] = GetKeyState(VK_RBUTTON) >> 15;
    globalMouseButtons[3] = GetKeyState(VK_XBUTTON1) >> 15;
    globalMouseButtons[4] = GetKeyState(VK_XBUTTON2) >> 15;
    
    MSG message = {};
    
    // NOTE(KARAN): If hWnd is NULL, PeekMessage retrieves messages for any window that belongs to the current thread, and any messages on the current thread's message queue whose hwnd value is NULL
    HWND windowHandle = 0;
    while(PeekMessage(&message, windowHandle, 0, 0, PM_REMOVE) > 0)
    {
        switch(message.message)
        {
            case WM_QUIT:
            {
                PfWindow *window = (PfWindow*)GetPropA(message.hwnd, "PfWindow");
                if(window) window->shouldClose = true;
            }break;
            default:
            {
                TranslateMessage(&message);
                DispatchMessage(&message);
            }break;
        }
    }
}

bool PfRequestSwapInterval(int32 frames)
{
    bool result = false;
    if(wglSwapIntervalEXT)
    {
        result = wglSwapIntervalEXT(frames);
    }
    return result;
}