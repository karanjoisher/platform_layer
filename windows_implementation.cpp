#include <stdio.h>
#include <intrin.h>  
#include <GL/gl.h>

#include "utility.h"
#include "project_types.h"
#include "windows_platform_interface.h"
#include "pf_opengl.h"
#include "gl_error_handler.h"

global_variable WNDCLASS globalWindowClass;
global_variable int32 globalKeyboard[256];
global_variable int32 globalMouseButtons[5];
global_variable int64 globalQueryPerformanceHZ;
global_variable int32 globalGLMajorVersion = 3;
global_variable int32 globalGLMinorVersion = 3;
global_variable bool globalCoreProfile       = false;

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

global_variable type_wglGetExtensionsStringARB* wglGetExtensionsStringARB;  
global_variable type_wglChoosePixelFormatARB* wglChoosePixelFormatARB;
global_variable type_wglCreateContextAttribsARB* wglCreateContextAttribsARB;


void WinCreateDummyWindow(PfWindow *window)
{
    
    WNDCLASS dummyWindowClass = {};
    dummyWindowClass.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    dummyWindowClass.lpfnWndProc = DefWindowProcA;
    dummyWindowClass.hInstance = GetModuleHandle(0);
    dummyWindowClass.lpszClassName = "DummyWindowClass";
    
    if(RegisterClassA(&dummyWindowClass) == 0)
    {
        DEBUG_ERROR("Could not register window class.");
        
        ASSERT(!"Couldn't register window class");
    }
    
    
    window->windowHandle = CreateWindowEx(0, dummyWindowClass.lpszClassName, "Dummy Window", WS_OVERLAPPEDWINDOW | WS_VISIBLE, 
                                          CW_USEDEFAULT, CW_USEDEFAULT,
                                          CW_USEDEFAULT, CW_USEDEFAULT,
                                          0, 0, dummyWindowClass.hInstance, 0);
    
    if(window->windowHandle == 0)
    {
        DEBUG_ERROR("Could not create window.");
        
        ASSERT(!"Couldn't create window");
    }
    
    window->deviceContext = GetDC(window->windowHandle);
}


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
    
    // NOTE(KARAN): CS_OWNDC necessary for OpenGL context creation
    globalWindowClass.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    globalWindowClass.lpfnWndProc = WinWindowCallback;
    globalWindowClass.hInstance = GetModuleHandle(0);globalWindowClass.lpszClassName = "windowsPlatformLayer";
    
    if(RegisterClassA(&globalWindowClass) == 0)
    {
        fprintf(stderr,"ERROR: Could not register window class. LINE: %d, FUNCTION:%s, FILE:%s\n", __LINE__, __func__, __FILE__);
    }
    
    /***** Opengl context creation ******/
    
    PfWindow dummyWindow = {};
    WinCreateDummyWindow(&dummyWindow);
    
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
    ASSERT(closestPixelFormatIndex);
    
    /*
An application can only set the pixel format of a window one time. 
Once a window's pixel format is set, it cannot be changed.
*/
    BOOL setPixelFormatSuccess = SetPixelFormat(dummyWindow.deviceContext, closestPixelFormatIndex, &desiredPixelFormat);
    ASSERT(setPixelFormatSuccess == TRUE);
    
    dummyWindow.glContext = wglCreateContext(dummyWindow.deviceContext);
    ASSERT(dummyWindow.glContext);
    
    BOOL makeCurrentSuccess = wglMakeCurrent(dummyWindow.deviceContext, dummyWindow.glContext);
    ASSERT(makeCurrentSuccess == TRUE);
    
    GrabOpenGLFuncPointers();
    
    wglGetExtensionsStringARB = (type_wglGetExtensionsStringARB*)wglGetProcAddress("wglGetExtensionsStringARB");
    ASSERT(wglGetExtensionsStringARB);
    
    const char *wglExtensions = wglGetExtensionsStringARB(dummyWindow.deviceContext);
    //DEBUG_LOG(stdout, "WGL extensions: %s\n", wglExtensions);
    
    // TODO(KARAN): Part of WGL_ARB_pixel_format extension
    wglChoosePixelFormatARB = (type_wglChoosePixelFormatARB*)wglGetProcAddress("wglChoosePixelFormatARB");
    ASSERT(wglChoosePixelFormatARB);
    
    // TODO(KARAN): Part of WGL_ARB_create_context extension
    wglCreateContextAttribsARB = (type_wglCreateContextAttribsARB*)wglGetProcAddress("wglCreateContextAttribsARB");
    ASSERT(wglCreateContextAttribsARB);
    
    makeCurrentSuccess = wglMakeCurrent(0, 0);
    ASSERT(makeCurrentSuccess == TRUE);
    
    
    BOOL deleteSuccess = wglDeleteContext(dummyWindow.glContext);
    ASSERT(deleteSuccess == TRUE);
    
    deleteSuccess = ReleaseDC(dummyWindow.windowHandle, dummyWindow.deviceContext);
    ASSERT(deleteSuccess == 1);
    
    deleteSuccess = DestroyWindow(dummyWindow.windowHandle);
    ASSERT(deleteSuccess == TRUE);
    
    
    
}


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
    ASSERT(choosingSuccess == TRUE);
    
    PIXELFORMATDESCRIPTOR desiredPixelFormat;
    int32 describeSuccess =  DescribePixelFormat(window->deviceContext, closestPixelFormatIndex, sizeof(desiredPixelFormat), &desiredPixelFormat);
    ASSERT(describeSuccess);
    
    BOOL setPixelFormatSuccess = SetPixelFormat(window->deviceContext, closestPixelFormatIndex, &desiredPixelFormat);
    ASSERT(setPixelFormatSuccess == TRUE);
    
    // TODO(KARAN): Shared context for async texture uploads
    HGLRC sharedContext = 0;
    
    //WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
    // TODO(KARAN): Make these attribs configurable via API
    
    
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
    
    window->glContext = wglCreateContextAttribsARB(window->deviceContext, sharedContext, desiredContextAttribs);
    ASSERT(window->glContext);
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
        
        wglMakeCurrent(window->deviceContext, window->glContext);
        glViewport(0, 0, width, height);
        
        ASSERT(window->offscreenBuffer.data);
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
    
    ASSERT(windowRectResult != 0);
    
    windowRect.right = windowRect.right - windowRect.left;
    windowRect.bottom = windowRect.bottom - windowRect.top;
    
    window->windowHandle = CreateWindowEx(0,globalWindowClass.lpszClassName, title, windowStyle, xPos, yPos, windowRect.right, windowRect.bottom, 0, 0, globalWindowClass.hInstance,0);
    
    
    /* MSDN:  "To avoid retrieving a device context each time it needs to paint inside a window, an application can specify the CS_OWNDC style for the window class. This class style directs the system to create a private device context — that is, to allocate a unique device context for each window in the class. The application need only retrieve the context once and then use it for all subsequent painting." */
    window->deviceContext = GetDC(window->windowHandle);
    
    if(window->windowHandle == 0)
    {
        DEBUG_LOG(stderr,"ERROR: Could not create window. LINE: %d, FUNCTION:%s, FILE:%s\n", __LINE__, __func__, __FILE__);
    }
    ASSERT(window->windowHandle);
    BOOL propertySetResult = SetPropA(window->windowHandle, "PfWindow", window);
    HWND prevFocusWindowHandle = SetFocus(window->windowHandle);
    ASSERT(prevFocusWindowHandle != 0);
    
    // NOTE(KARAN): If the window already had keyboard focus, WM_SETFOCUS message isn't sent to the WINDOWPROC. Hence this hack.
    if(prevFocusWindowHandle == window->windowHandle)
    {
        window->hasKeyboardFocus = true;
    }
    
    ASSERT(propertySetResult != 0);
    
    WinCreateOpenGLContext(window);
    
    // HACK(KARAN): Creation of texture and vertices for rendering offscreenbuffer via opengl
    // Adding this so that code can be compiled.
    wglMakeCurrent(window->deviceContext, window->glContext);
    
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
    wglMakeCurrent(0, 0);
    
    PfResizeWindow(window, width, height);
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

inline void PfglMakeCurrent(PfWindow *window)
{
    BOOL makeCurrentSuccess;
    
    if(window)
    {
        makeCurrentSuccess = wglMakeCurrent(window->deviceContext, window->glContext);
    }
    else
    {
        makeCurrentSuccess = wglMakeCurrent(0, 0);
    }
    
    ASSERT(makeCurrentSuccess == TRUE);
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
    
    //GL_CALL(glDisable(GL_TEXTURE_2D));
    
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