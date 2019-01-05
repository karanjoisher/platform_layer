#include "utility.h"
#include "project_types.h"
#include "math.h"

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


real32 RemapRange(real32 initialMin, real32 initialMax, real32 newMin, real32 newMax, real32 initialValue)
{
    real32 result;
    result = (((initialValue - initialMin)/(initialMax - initialMin)) * (newMax - newMin)) + newMin;
    return result;
}

#if 1
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

struct GameInput
{
    bool throttle;
    bool left;
    bool brake;
    bool right;
};

struct GameState
{
    v3 spaceshipPos;
    v3 spaceshipVel;
    real32 spaceshipRotation;
};


int main(int argc, char **argv)
{
    PfInitialize();
    PfGLConfig(4, 3, true);
    
    PfWindow window[1] = {};
    //PfCreateWindow(&window[1], (char*)"WINDOW 1", 256, 0, 256, 256);
    PfCreateWindow(&window[0], (char*)"WINDOW 0", 0, 0, 800, 800);
    
    real32 fps = 60.0f;
    real32 targetMillisecondsPerFrame = (1000.0f/fps);
    real32 dT = 1.0f/fps;
    
#if 1
    char *imagePath = "../data/asteroid_blend.png";
    PfglMakeCurrent(&window[0]);
    int imageWidth, imageHeight, imageChannels;
    stbi_set_flip_vertically_on_load(true);
    unsigned char *imageData = stbi_load(imagePath, &imageWidth, &imageHeight, &imageChannels, 4);
    uint32 textureId;
    ASSERT(imageData, "Couldn't load image");
    GL_CALL(glActiveTexture(GL_TEXTURE0));
    GL_CALL(glGenTextures(1, &textureId));
    GL_CALL(glBindTexture(GL_TEXTURE_2D, textureId));
    GL_CALL(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, imageWidth, imageHeight, 0,
                         GL_RGBA, GL_UNSIGNED_BYTE, imageData));
    
    GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
    GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
    GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER));
    GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER));
    GLfloat debugColor[] = {1.0f, 0.0f, 1.0f, 1.0f};
    GL_CALL(glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, debugColor));
    GL_CALL(glBindTexture(GL_TEXTURE_2D, 0));
    stbi_image_free(imageData);
    
    PfRect clientRect = PfGetClientRect(&window[0]);
    v2 max = {(real32)imageWidth/2.0f, (real32)imageHeight/2.0f};
    v2 min = {-((real32)imageWidth/2.0f), -((real32)imageHeight/2.0f)};
    real32 vertices[] = 
    {
        // positions          // texture coords
        max.x, max.y, 0.0f,   1.0f, 1.0f, // top right
        max.x, min.y, 0.0f,   1.0f, 0.0f, // bottom right
        min.x, min.y, 0.0f,   0.0f, 0.0f, // bottom left
        min.x, max.y, 0.0f,   0.0f, 1.0f  // top left 
    };
    
    uint32 indices[] = 
    {
        0, 1, 3, // first triangle
        1, 2, 3  // second triangle
    };
    
    uint32 vao, vbo, ebo;
    GL_CALL(glGenVertexArrays(1, &vao));
    GL_CALL(glGenBuffers(1, &vbo));
    GL_CALL(glGenBuffers(1, &ebo));
    GL_CALL(glBindVertexArray(vao));
    GL_CALL(glBindBuffer(GL_ARRAY_BUFFER, vbo));
    GL_CALL(glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW));
    GL_CALL(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo));
    GL_CALL(glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW));
    GL_CALL(glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0));
    GL_CALL(glEnableVertexAttribArray(0));
    GL_CALL(glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float))));
    GL_CALL(glEnableVertexAttribArray(1));
    GL_CALL(glBindVertexArray(0));
    GL_CALL(glBindBuffer(GL_ARRAY_BUFFER, 0));
    GL_CALL(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0));
    
    char *vertexShaderSource = 
        "#version 430 core\nlayout (location = 0) in vec3 aPos;\nlayout (location = 1) in vec2 aTexCoord;\nout vec3 ourColor;\nout vec2 TexCoord;\nuniform mat4 orthoProjection;\nuniform mat4 translationMat;\nuniform mat4 rotationMatAboutZAxis;\nvoid main(){gl_Position =  orthoProjection * translationMat * rotationMatAboutZAxis * vec4(aPos, 1.0f);ourColor = vec3(1.0f, 1.0f, 1.0f);TexCoord = vec2(aTexCoord.x, aTexCoord.y);}";
    
    char *fragmentShaderSource = 
        "#version 430 core\nout vec4 FragColor;in vec3 ourColor;in vec2 TexCoord;uniform sampler2D texture1;void main(){FragColor = texture(texture1, TexCoord);}";
    
    uint32 vertexShader, fragmentShader;
    GL_CALL(vertexShader = glCreateShader(GL_VERTEX_SHADER));
    GL_CALL(fragmentShader = glCreateShader(GL_FRAGMENT_SHADER));
    
    GL_CALL(glShaderSource(vertexShader, 1, &vertexShaderSource, 0));
    GL_CALL(glShaderSource(fragmentShader, 1, &fragmentShaderSource, 0));
    GL_CALL(glCompileShader(vertexShader));
    GL_CALL(glCompileShader(fragmentShader));
    
    int32 vertexCompilationSuccess;
    int32 fragmentCompilationSuccess;
    GL_CALL(glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &vertexCompilationSuccess));
    GL_CALL(glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &fragmentCompilationSuccess));
    if (!vertexCompilationSuccess)
    {
        char infoLog[512] = {};
        GL_CALL(glGetShaderInfoLog(vertexShader, 512, NULL, infoLog));
        DEBUG_ERROR("%s", infoLog);
    }
    if (!fragmentCompilationSuccess)
    {
        char infoLog[512] = {};
        GL_CALL(glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog));
        DEBUG_ERROR("%s", infoLog);
    }
    
    uint32 programId;
    GL_CALL(programId = glCreateProgram());
    GL_CALL(glAttachShader(programId, vertexShader));
    GL_CALL(glAttachShader(programId, fragmentShader));
    GL_CALL(glLinkProgram(programId));
    GL_CALL(glDeleteShader(vertexShader));
    GL_CALL(glDeleteShader(fragmentShader));  
    
#endif
    
    PfRequestSwapInterval(1);
    
    PfTimestamp start = PfGetTimestamp();
    uint64 startCycles = PfRdtsc();
    
    
    bool recording = false;
    bool playing = false;
    GameInput input = {};
    GameState gameState = {};
    gameState.spaceshipPos = {(real32)clientRect.width/2.0f, (real32)clientRect.height/2.0f, 0.0f};
    gameState.spaceshipVel = {};
    gameState.spaceshipRotation = 0.0f;
    
    HANDLE inputFileHandle = INVALID_HANDLE_VALUE;
    int32 recordSlot = 0;
    int32 playBackSlot = 0;
    bool playingRestart = false;
#if HOT_CODE_RELOADABLE
    if(argc >= 2)
    {
        DEBUG_LOG("%s %s\n", argv[0], argv[1]);
        if(AreStringsSame(argv[1], "hcr_reloaded"))
        {
            
            DWORD bytesRead;
            HANDLE hotCodeRelaunch = CreateFileA("hot_code_relaunch_persistent_data", GENERIC_READ, NULL, NULL, OPEN_EXISTING, NULL, NULL);
            if(GetLastError() == ERROR_FILE_NOT_FOUND)
            {
            }
            else
            {
                ASSERT(hotCodeRelaunch != INVALID_HANDLE_VALUE, "File handle to read the persistent data is invalid");
                ASSERT(ReadFile(hotCodeRelaunch, (void *)&playBackSlot, sizeof(playBackSlot), &bytesRead, NULL), "Failed to read the persistent data.");
                ASSERT(bytesRead == sizeof(playBackSlot), "The bytes read from the file is not the same as the size of the playBackSlot");
                CloseHandle(hotCodeRelaunch);
                playingRestart = true;
            }
        }
    }
#endif
    int32 totalSlots = 10; 
    while(!window[0].shouldClose)
    {
        
#if PLATFORM_WINDOWS
        if(!recording && PfGetKeyState(&window[0], PF_LEFT_CTRL) && (PfGetKeyState(&window[0], PF_LEFT_SHIFT) == false) && PfGetKeyState(&window[0], PF_R))
        {
            recording = true;
            
            char gameStateFileName[32] = {};
            char inputFileName[32] = {};
            sprintf(gameStateFileName, "game_state_%d.state", recordSlot);
            sprintf(inputFileName, "input_%d.input_series", recordSlot);
            recordSlot = (recordSlot + 1) % totalSlots;
            
            DWORD bytesWritten;
            HANDLE gameStateFileHandle = CreateFileA(gameStateFileName, GENERIC_WRITE, NULL, NULL, CREATE_ALWAYS, NULL, NULL);
            ASSERT(gameStateFileHandle != INVALID_HANDLE_VALUE, "File handle to record the game state is invalid");
            ASSERT(WriteFile(gameStateFileHandle, (void *)&gameState, sizeof(gameState), &bytesWritten, NULL), "Failed to write the game state data to the recording file.");
            ASSERT(bytesWritten == sizeof(gameState), "The bytes written to file is not the same as the size of the gamestate");
            CloseHandle(gameStateFileHandle);
            
            inputFileHandle = CreateFileA(inputFileName, GENERIC_WRITE, NULL, NULL, CREATE_ALWAYS, NULL, NULL);
            ASSERT(inputFileHandle != INVALID_HANDLE_VALUE, "File handle to record the input is invalid");
        }
        
        if(recording && PfGetKeyState(&window[0], PF_LEFT_CTRL) &&  PfGetKeyState(&window[0], PF_LEFT_SHIFT) && PfGetKeyState(&window[0], PF_R))
        {
            recording = false;
            CloseHandle(inputFileHandle);
        }
        
        if(playingRestart || (!playing && PfGetKeyState(&window[0], PF_LEFT_CTRL) && (PfGetKeyState(&window[0], PF_LEFT_SHIFT) == false) && PfGetKeyState(&window[0], PF_P)))
        {
            playingRestart = false;
            playing = true;
            
#if HOT_CODE_RELOADABLE
            DWORD bytesWritten;
            HANDLE hotCodeRelaunch  = CreateFileA("hot_code_relaunch_persistent_data", GENERIC_WRITE, NULL, NULL, CREATE_ALWAYS, NULL, NULL);
            ASSERT(hotCodeRelaunch != INVALID_HANDLE_VALUE, "File handle to record the persistent data for hot code reload is invalid");
            ASSERT(WriteFile(hotCodeRelaunch, (void *)&playBackSlot, sizeof(playBackSlot), &bytesWritten, NULL), "Failed to store persistent data for hot code reload.");
            ASSERT(bytesWritten == sizeof(playBackSlot), "The bytes written to file is not the same as the size of the int");
            CloseHandle(hotCodeRelaunch);
#endif
            
            char gameStateFileName[32] = {};
            char inputFileName[32] = {};
            sprintf(gameStateFileName, "game_state_%d.state", playBackSlot);
            sprintf(inputFileName, "input_%d.input_series", playBackSlot);
            
            DWORD bytesRead;
            HANDLE gameStateFileHandle = CreateFileA(gameStateFileName, GENERIC_READ, NULL, NULL, OPEN_EXISTING, NULL, NULL);
            ASSERT(gameStateFileHandle != INVALID_HANDLE_VALUE, "File handle to read the game state is invalid");
            ASSERT(ReadFile(gameStateFileHandle, (void *)&gameState, sizeof(gameState), &bytesRead, NULL), "Failed to read the game state data from the recording file.");
            ASSERT(bytesRead == sizeof(gameState), "The bytes read from the file is not the same as the size of the gamestate");
            CloseHandle(gameStateFileHandle);
            
            inputFileHandle = CreateFileA(inputFileName, GENERIC_READ, NULL, NULL, OPEN_EXISTING, NULL, NULL);
            ASSERT(inputFileHandle != INVALID_HANDLE_VALUE, "File handle to read the input is invalid");
        }
        
        if(playing && PfGetKeyState(&window[0], PF_LEFT_CTRL) &&  PfGetKeyState(&window[0], PF_LEFT_SHIFT) && PfGetKeyState(&window[0], PF_P))
        {
            playing = false;
            CloseHandle(inputFileHandle);
            
            
#if HOT_CODE_RELOADABLE
            DeleteFileA("hot_code_relaunch_persistent_data");
#endif
        }
        
        if(playing)
        {
            bool playBackSlotChanged = false;
            
            if(PfGetKeyState(&window[0], PF_0))      { playBackSlot = 0; playBackSlotChanged = true; }
            else if(PfGetKeyState(&window[0], PF_1)) { playBackSlot = 1; playBackSlotChanged = true; }
            else if(PfGetKeyState(&window[0], PF_2)) { playBackSlot = 2; playBackSlotChanged = true; }
            else if(PfGetKeyState(&window[0], PF_3)) { playBackSlot = 3; playBackSlotChanged = true; }
            else if(PfGetKeyState(&window[0], PF_4)) { playBackSlot = 4; playBackSlotChanged = true; }
            else if(PfGetKeyState(&window[0], PF_5)) { playBackSlot = 5; playBackSlotChanged = true; }
            else if(PfGetKeyState(&window[0], PF_6)) { playBackSlot = 6; playBackSlotChanged = true; }
            else if(PfGetKeyState(&window[0], PF_7)) { playBackSlot = 7; playBackSlotChanged = true; }
            else if(PfGetKeyState(&window[0], PF_8)) { playBackSlot = 8; playBackSlotChanged = true; }
            else if(PfGetKeyState(&window[0], PF_9)) { playBackSlot = 9; playBackSlotChanged = true; }
            
            if(playBackSlotChanged)
            {
                CloseHandle(inputFileHandle);
                playingRestart = true;
            }
            else
            {
                DWORD bytesRead;
                BOOL readSuccess = ReadFile(inputFileHandle, (void *)&input, sizeof(input), &bytesRead, NULL);
                ASSERT(readSuccess == TRUE, "Failed to read the input data from the recording file.");
                
                bool eof = (readSuccess == TRUE) && (bytesRead == 0);
                if(eof)
                {
                    input = {};
                    CloseHandle(inputFileHandle);
                    playingRestart = true;
                }
            }
        }
        else
        {
            
#if PLATFORM_WINDOWS | PLATFORM_LINUX
            input.throttle = PfGetKeyState(&window[0], PF_W);
            input.left = PfGetKeyState(&window[0], PF_A);
            input.brake = PfGetKeyState(&window[0], PF_S);
            input.right = PfGetKeyState(&window[0], PF_D);
#endif
        }
        
        if(recording)
        {
            DWORD bytesWritten;
            ASSERT(WriteFile(inputFileHandle, (void *)&input, sizeof(input), &bytesWritten, NULL), "Failed to write the input data to the recording file.");
            ASSERT(bytesWritten == sizeof(input), "The bytes written to file is not the same as the size of the input");
        }
#endif
        
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
        
        if(input.left) gameState.spaceshipRotation += 2.0f; 
        if(input.right) gameState.spaceshipRotation -= 2.0f;
        
        v3 forward = v3{Cosine(gameState.spaceshipRotation * DEG_TO_RAD), Sine(gameState.spaceshipRotation * DEG_TO_RAD), 0.0f};
        v3 accelaration = {};
        real32 breakingFactor = 1.5f;
        real32 accelarationMultiplier = 750.0f;
        if(input.throttle) accelaration = forward * accelarationMultiplier;
        if(input.brake) breakingFactor = 3.0f;
        
        accelaration = accelaration - (gameState.spaceshipVel * breakingFactor);
        
        v3 displacement = (gameState.spaceshipVel*dT) + (accelaration*0.5f*dT*dT);
        
        gameState.spaceshipPos = gameState.spaceshipPos + displacement;
        
        if(gameState.spaceshipPos.x < 0.0f)
        {
            gameState.spaceshipPos.x = (real32)clientRect.width + gameState.spaceshipPos.x;
        }
        else if(gameState.spaceshipPos.x >= clientRect.width)
        {
            gameState.spaceshipPos.x = (real32)clientRect.width - gameState.spaceshipPos.x;
        }
        
        
        if(gameState.spaceshipPos.y < 0.0f)
        {
            gameState.spaceshipPos.y = (real32)clientRect.height + gameState.spaceshipPos.y;
        }
        else if(gameState.spaceshipPos.y >= clientRect.height)
        {
            gameState.spaceshipPos.y = (real32)clientRect.height - gameState.spaceshipPos.y;
        }
        
        
        gameState.spaceshipVel = gameState.spaceshipVel + (accelaration * dT);
        
        PfglMakeCurrent(&window[0]);
        GL_CALL(glClearColor(1.0f, 0.0f, 1.0f, 1.0f));
        GL_CALL(glClear(GL_COLOR_BUFFER_BIT));
        GL_CALL(GLboolean wasBlendEnabled = glIsEnabled(GL_BLEND));
        GL_CALL(glEnable(GL_BLEND));
        GL_CALL(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));
        
        GL_CALL(glActiveTexture(GL_TEXTURE0));
        GL_CALL(glBindTexture(GL_TEXTURE_2D, textureId));
        GL_CALL(glBindVertexArray(vao));
        GL_CALL(glUseProgram(programId));
        GL_CALL(GLint samplerUniformLocation = glGetUniformLocation(programId, "texture1"));
        GL_CALL(GLint rotationMatAboutZAxisUniformLocation = glGetUniformLocation(programId, "rotationMatAboutZAxis"));
        GL_CALL(GLint translationMatUniformLocation = glGetUniformLocation(programId, "translationMat"));
        GL_CALL(GLint orthoProjectionUniformLocation = glGetUniformLocation(programId, "orthoProjection"));
        
        GL_CALL(glUniform1i(samplerUniformLocation, 0)); // Setting the texture unit
        
        mat4 rotationMatAboutZAxis = {};
        real32 cosine = Cosine(DEG_TO_RAD * gameState.spaceshipRotation);
        real32 sine = Sine(DEG_TO_RAD * gameState.spaceshipRotation);
        rotationMatAboutZAxis.data[0] = cosine;
        rotationMatAboutZAxis.data[1] = -sine;
        rotationMatAboutZAxis.data[4] = sine;
        rotationMatAboutZAxis.data[5] = cosine;
        rotationMatAboutZAxis.data[10] = 1.0f;
        rotationMatAboutZAxis.data[15] = 1.0f;
        
        mat4 translationMat = {};
        translationMat.data[0] = 1.0f; 
        translationMat.data[5] = 1.0f; 
        translationMat.data[10] = 1.0f; 
        translationMat.data[3] = gameState.spaceshipPos.x;
        translationMat.data[7] = gameState.spaceshipPos.y;
        translationMat.data[11] = gameState.spaceshipPos.z;
        translationMat.data[15] = 1.0f;
        
        mat4 orthoProjection = {};
        orthoProjection.data[0] = 2.0f/(real32)clientRect.width;
        orthoProjection.data[3] = -1.0f;
        orthoProjection.data[5] = 2.0f/(real32)clientRect.height;
        orthoProjection.data[7] = -1.0f;
        orthoProjection.data[10] = 1.0f;
        orthoProjection.data[15] = 1.0f;
        
        GL_CALL(glUniformMatrix4fv(rotationMatAboutZAxisUniformLocation, 1, GL_TRUE, rotationMatAboutZAxis.data)); 
        GL_CALL(glUniformMatrix4fv(translationMatUniformLocation, 1, GL_TRUE, translationMat.data)); 
        GL_CALL(glUniformMatrix4fv(orthoProjectionUniformLocation, 1, GL_TRUE, orthoProjection.data)); 
        GL_CALL(glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0));
        GL_CALL(glUseProgram(0));
        GL_CALL(glBindVertexArray(0));
        GL_CALL(glBindTexture(GL_TEXTURE_2D, 0));
        
        if(wasBlendEnabled == GL_FALSE)
        {
            glDisable(GL_BLEND);
        }
        
        PfglSwapBuffers(&window[0]);
        
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
        
        inside = PfGetMouseCoordinates(&window[0], &xMouse, &yMouse);
        sprintf(temp, "%.2fms %.2FPS %.3fMHz Spaceship:(%.2f, %.2f, %.2f) RECORDING: %d PLAYING:%d", secondsPerFrame * 1000.0f, 1.0f/secondsPerFrame, cyclesPerFrame/(secondsPerFrame * 1000.0f * 1000.0f), gameState.spaceshipPos.x, gameState.spaceshipPos.y, gameState.spaceshipPos.z, recording, playing);
        
        PfSetWindowTitle(&window[0], temp);
        
        PfUpdate();
        clientRect = PfGetClientRect(&window[0]);
    }
    
#if PLATFORM_LINUX
    XCloseDisplay(globalDisplay);
#endif
    DEBUG_LOG("Exiting application.\n");
    return 0;
}

#endif