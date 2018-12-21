#include "utility.h"
#include "project_types.h"

#if PLATFORM_WINDOWS
#include "windows_platform_interface.h"
#include "windows_implementation.cpp"
#endif

#if PLATFORM_LINUX
#include "linux_platform_interface.h"
#include "linux_implementation.cpp"
#include <GL/glu.h>
#endif

#if PLATFORM_WINDOWS
#define sprintf sprintf_s
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
    
    if(PfGetKeyState(window, PF_LEFT_ALT))
    {
        rect->y  -= 2;
    }
    
    if(PfGetKeyState(window, PF_A))
    {
        rect->x  -= 2;
    }
    
    if(PfGetKeyState(window, PF_S))
    {
        rect->y  += 2;
    }
    
    if(PfGetKeyState(window, PF_D))
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
    uint32 blueColor = 0xFF00FF00;
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

#if 1
int main()
{
    PfInitialize();
    PfGLConfig(4, 3, true);
    
    int x = 0;
    int y = 0;
    int width = 256;
    int height = 256;
    
    PfWindow window[2] = {};
    
    PfCreateWindow(&window[0], (char*)"WINDOW 0", x, y, width, height);
    
    PfCreateWindow(&window[1], (char*)"WINDOW 1", 256, 0, width, height);
    
#if 1
    
    // NOTE(KARAN): Modern opengl testing
    
    //
    PfglMakeCurrent(&window[1]);
    
    real32 triangleVertices[] = 
    {
        -0.5f, -0.5f, 0.0f,
        0.5f, -0.5f, 0.0f,
        0.0f,  0.5f, 0.0f
    };  
    
    uint32 vao;
    
    GL_CALL(glGenVertexArrays(1, &vao));  
    GL_CALL(glBindVertexArray(vao));
    
    uint32 vbo;
    GL_CALL(glGenBuffers(1, &vbo));
    GL_CALL(glBindBuffer(GL_ARRAY_BUFFER, vbo));
    GL_CALL(glBufferData(GL_ARRAY_BUFFER, ARRAY_COUNT(triangleVertices) * sizeof(triangleVertices[0]), triangleVertices, GL_STATIC_DRAW));
    GL_CALL(glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(real32), (void*)0));
    GL_CALL(glEnableVertexAttribArray(0));
    
    char *vertexShaderSource = "#version 330 core\nlayout (location = 0) in vec3 aPos;\nvoid main(){gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);}";
    
    char *fragmentShaderSource = "#version 330 core\nout vec4 FragColor;void main(){FragColor = vec4(1.0f, 0.5f, 0.2f, 1.0f);}";
    
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
    
    GL_CALL(glAttachShader(shaderProgram, vertexShader));
    GL_CALL(glAttachShader(shaderProgram, fragmentShader));
    GL_CALL(glLinkProgram(shaderProgram));
    
    GL_CALL(glDeleteShader(vertexShader));
    GL_CALL(glDeleteShader(fragmentShader));  
    GL_CALL(glBindVertexArray(0));
#endif
    
    PfRect rect1 = {0, 0, 30, 30};
    PfRect rect2 = {0, 0, 10, 10};
    
    PfTimestamp start = PfGetTimestamp();
    uint64 startCycles = PfRdtsc();
    while(!window[0].shouldClose || !window[1].shouldClose)
    {
        PfUpdate();
        
        static bool toggled = false;
        if(PfGetKeyState(PF_LEFT_ALT, true) == 0 || PfGetKeyState(PF_ENTER, true) == 0)
        {
            toggled = false;
        }
        
        if(!toggled && PfGetKeyState(&window[0], PF_LEFT_ALT, true) && PfGetKeyState(&window[0], PF_ENTER, true) != 0)
        {
            toggled = true;
            PfToggleFullscreen(&window[0]);
            
        }
        
        if(!toggled && PfGetKeyState(&window[1], PF_LEFT_ALT, true) && PfGetKeyState(&window[1], PF_ENTER, true) != 0)
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
        
        DrawRectangle(&offscreenBuffer2, PfRect{0, 0, offscreenBuffer2.width, offscreenBuffer2.height}, 0);
        PfRect rect2Border  = {rect2.x - 5, rect2.y - 5, rect2.width + 10, rect2.height + 10};
        DrawRectangle(&offscreenBuffer2, rect2Border, color2);
        DrawRectangle(&offscreenBuffer2, rect2, 0);
        
        if(!window[0].shouldClose) 
        {
            PfBlitToScreen(&window[0]);
        }
        
#if 1
        
        if(!window[1].shouldClose)
        {
            
            PfglMakeCurrent(&window[1]);
            
            glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);
            
            // Render Triangle
            
            GL_CALL(glUseProgram(shaderProgram));
            GL_CALL(glBindVertexArray(vao));
            GL_CALL(glDrawArrays(GL_TRIANGLES, 0, 3));
            GL_CALL(glUseProgram(0));
            GL_CALL(glBindVertexArray(0));
            
            PfglRenderWindow(&window[1]);
            
            PfglSwapBuffers(&window[1]);
            
        }
#endif
        
        real32 targetMillisecondsPerFrame = (1000.0f/30.0f);
        real32 msRequiredToRenderThisFrame = PfGetSeconds(start, PfGetTimestamp()) * 1000.0f;
        
        real32 sleepTime = targetMillisecondsPerFrame - msRequiredToRenderThisFrame;
        
        PfSleep((int32)sleepTime);
        
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
        bool inside = false;
        
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

#endif