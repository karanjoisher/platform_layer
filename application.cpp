#include "utility.h"
#include "project_types.h"
#include "math.h"

#define PF_WINDOW_AND_INPUT
#define PF_TIME
#define PF_FILE
#include "pf.h"

#if 1
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

struct Queue
{
    uint8 *base;
    int32 length;
    int32 elementSize;
    int32 head;
    int32 nextFreeSlot;
    int32 currentlyQueued;
};


struct ChunkAllocator
{
    uint8 *base;
    int32 chunkSize;
    int32 length;
    int32 nextFreeChunkIndex;
};

void InitializeChunkAllocator(ChunkAllocator *ca)
{
    ca->nextFreeChunkIndex = 0;
    for(int i = 0; i < ca->length; i++)
    {
        int32 *chunk = (int32*)(ca->base + (i * ca->chunkSize));
        *chunk = i + 1;
    }
}


uint8 *AllocateChunk(ChunkAllocator *ca)
{
    uint8 *result = 0;
    if(ca->nextFreeChunkIndex < ca->length)
    {
        result = ca->base + (ca->nextFreeChunkIndex * ca->chunkSize);
        ca->nextFreeChunkIndex = *((int32*)result);
    }
    return result;
}


void FreeChunk(ChunkAllocator *ca, uint8 *chunk)
{
    int32 chunkIndex = (int32)(chunk - ca->base)/ca->chunkSize;
    *((int32*)chunk) = ca->nextFreeChunkIndex;
    ca->nextFreeChunkIndex = chunkIndex;
}


uint8 *GetNthElementFromQueueHead(Queue *q, int32 i)
{
    uint8 *result = 0;
    if(i < q->currentlyQueued)
    {
        result = q->base + (((q->head + i) % q->length) * q->elementSize);
    }
    return result;
}

uint8* Enqueue(Queue *q)
{
    uint8* result = 0;
    if(q->currentlyQueued < q->length)
    {
        result = (q->base + (q->elementSize * q->nextFreeSlot));
        q->nextFreeSlot = (q->nextFreeSlot + 1) % q->length;
        ++(q->currentlyQueued);
    }
    return result;
}

uint8 *Peekqueue(Queue *q)
{
    uint8* result = 0;
    if(q->currentlyQueued > 0)
    {
        result = (q->base + (q->elementSize * q->head));
    }
    return result;
}

void Dequeue(Queue *q)
{
    if(q->currentlyQueued > 0)
    {
        q->head = (q->head + 1) % q->length;
        --(q->currentlyQueued);
    }
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
                color = blueColor;
            }
            else
            {
                color = redColor;
            }
            
        }
        row += offscreenBuffer->pitch;
    }
}

struct GameInput
{
    bool throttle;
    bool left;
    bool brake;
    bool right;
    bool fire;
};

struct Bullet
{
    v3 pos;
    v3 vel;
    real32 duration;
    Bullet *next;
};

struct Asteroid
{
    v3 pos;
    v3 vel;
    real32 rotation;
    real32 rotationalVelocity;
    Asteroid *next;
};

struct Memory
{
    uint8* base;
    sizet size;
    sizet used;
};

struct GameState
{
    v2 spaceshipDim;
    v3 spaceshipPos;
    v3 spaceshipVel;
    real32 spaceshipRotation;
    
    ChunkAllocator bulletAllocator;
    Bullet *bullets;
    real32 firingCoolDown;
    
    ChunkAllocator asteroidAllocator;
    Asteroid *asteroids;
    
    real32 spaceshipRadius;
    real32 bulletRadius;
    real32 asteroidRadius;
};

#define PushStruct(memory, type) (type*)(PushSize(memory, sizeof(type)))
#define PushArray(memory, type, count) (type*)(PushSize(memory, sizeof(type) * count))
uint8* PushSize(Memory *memory, sizet required)
{
    ASSERT(memory->used + required <= memory->size, "Memory overflow");
    uint8* result = memory->base + memory->used;
    memory->used += required;
    return result;
}

struct HotCodeRelaunchPersistentData
{
    int32 playBackSlot;
    Memory mem;
};

int main(int argc, char **argv)
{
    PfInitialize();
    PfGLConfig(4, 3, true);
    
    PfWindow window[1] = {};
    //PfCreateWindow(&window[1], (char*)"WINDOW 1", 256, 0, 256, 256);
    PfCreateWindow(&window[0], (char*)"WINDOW 0", 0, 0, 800, 800);
    PfRect clientRect = PfGetClientRect(&window[0]);
    
    int32 playBackSlot = 0;
    bool playingRestart = false;
    
    Memory mem = {};
    mem.size = MEGABYTES((uint64)512);
    
    HotCodeRelaunchPersistentData hcrData = {};
#if HOT_CODE_RELOADABLE
    if(argc >= 2)
    {
        if(AreStringsSame(argv[1], "hcr_reloaded"))
        {
            int64 fileHandle = PfCreateFile("hot_code_relaunch_persistent_data", PF_READ, PF_OPEN);
            if(fileHandle == -1)
            {
            }
            else
            {
                PfCloseFileHandle(fileHandle);
                PfReadEntireFile("hot_code_relaunch_persistent_data", (void *)&hcrData, sizeof(HotCodeRelaunchPersistentData));
                playBackSlot = hcrData.playBackSlot;
                mem = hcrData.mem;
                playingRestart = true;
            }
        }
    }
#endif
    
    
#if PLATFORM_WINDOWS
#if defined(DEBUG_BUILD)
    LPVOID baseAddressForGameMemoryStorage = (LPVOID)TERABYTES((uint64)1);
#else
    LPVOID baseAddressForGameMemoryStorage = (LPVOID)0;
#endif
    mem.base = (uint8*)VirtualAlloc(baseAddressForGameMemoryStorage,(size_t)mem.size, MEM_COMMIT | MEM_RESERVE,PAGE_READWRITE);
#endif
    
    GameState *gameState = 0;
    if(playingRestart)
    {
        gameState = (GameState*)mem.base;
    }
    else
    {
        gameState = PushStruct(&mem, GameState);
        
        gameState->bulletAllocator.length = 20;
        gameState->bulletAllocator.chunkSize = sizeof(Bullet);
        gameState->bulletAllocator.base = (uint8 *)PushArray(&mem, Bullet, gameState->bulletAllocator.length);
        InitializeChunkAllocator(&(gameState->bulletAllocator));
        
        gameState->asteroidAllocator.length = 30;
        gameState->asteroidAllocator.chunkSize = sizeof(Asteroid);
        gameState->asteroidAllocator.base = (uint8 *)PushArray(&mem, Asteroid, gameState->asteroidAllocator.length);
        InitializeChunkAllocator(&(gameState->asteroidAllocator));
        
        for(int i = 0; i < 10; i++)
        {
            Asteroid *asteroid = (Asteroid*)AllocateChunk(&(gameState->asteroidAllocator));
            if(asteroid)
            {
                asteroid->pos = {(real32)(rand() % clientRect.width), (real32)(rand() % clientRect.height)};
                
                real32 signMultiplier1 = (rand() % 2 == 0) ? 1.0f : -1.0f;
                real32 signMultiplier2 = (rand() % 2 == 0) ? 1.0f : -1.0f;
                
                asteroid->vel = {(real32)((rand() % 100) + 50) * signMultiplier1, (real32)((rand() % 100) + 50) * signMultiplier2};
                asteroid->rotation = 0.0f;
                asteroid->rotationalVelocity = (real32)((rand() % 500) + 50) * signMultiplier1;
                asteroid->next = gameState->asteroids;
                gameState->asteroids = asteroid;
            }
        }
    }
    
    
    real32 fps = 60.0f;
    real32 targetMillisecondsPerFrame = (1000.0f/fps);
    real32 dT = 1.0f/fps;
    
#if 1
    PfglMakeCurrent(&window[0]);
    
    char *imagePath = "../data/spaceship.png";
    int imageWidth, imageHeight, imageChannels;
    stbi_set_flip_vertically_on_load(true);
    unsigned char *imageData = stbi_load(imagePath, &imageWidth, &imageHeight, &imageChannels, 4);
    ASSERT(imageData, "Couldn't load image");
    
    uint32 spaceshipTextureId;
    gameState->spaceshipDim = {(real32)imageWidth, (real32)imageHeight};
    GL_CALL(glActiveTexture(GL_TEXTURE0));
    GL_CALL(glGenTextures(1, &spaceshipTextureId));
    GL_CALL(glBindTexture(GL_TEXTURE_2D, spaceshipTextureId));
    GL_CALL(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, imageWidth, imageHeight, 0,
                         GL_RGBA, GL_UNSIGNED_BYTE, imageData));
    
    GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
    GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
    GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER));
    GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER));
    GLfloat debugColor[] = {1.0f, 0.0f, 1.0f, 1.0f};
    //GL_CALL(glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, debugColor));
    GL_CALL(glBindTexture(GL_TEXTURE_2D, 0));
    stbi_image_free(imageData);
    
    
    v2 max = {(real32)imageWidth/2.0f, (real32)imageHeight/2.0f};
    v2 min = {-((real32)imageWidth/2.0f), -((real32)imageHeight/2.0f)};
    real32 spaceshipVertices[] = 
    {
        // positions          // texture coords
        max.x, max.y, 0.0f,   1.0f, 1.0f, // top right
        max.x, min.y, 0.0f,   1.0f, 0.0f, // bottom right
        min.x, min.y, 0.0f,   0.0f, 0.0f, // bottom left
        min.x, max.y, 0.0f,   0.0f, 1.0f  // top left 
    };
    
    uint32 quadIndices[] = 
    {
        0, 1, 3, // first triangle
        1, 2, 3  // second triangle
    };
    
    uint32 spaceshipVao, vbo, ebo;
    GL_CALL(glGenVertexArrays(1, &spaceshipVao));
    GL_CALL(glGenBuffers(1, &vbo));
    GL_CALL(glGenBuffers(1, &ebo));
    GL_CALL(glBindVertexArray(spaceshipVao));
    GL_CALL(glBindBuffer(GL_ARRAY_BUFFER, vbo));
    GL_CALL(glBufferData(GL_ARRAY_BUFFER, sizeof(spaceshipVertices), spaceshipVertices, GL_STATIC_DRAW));
    GL_CALL(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo));
    GL_CALL(glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(quadIndices), quadIndices, GL_STATIC_DRAW));
    GL_CALL(glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0));
    GL_CALL(glEnableVertexAttribArray(0));
    GL_CALL(glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float))));
    GL_CALL(glEnableVertexAttribArray(1));
    GL_CALL(glBindVertexArray(0));
    GL_CALL(glBindBuffer(GL_ARRAY_BUFFER, 0));
    GL_CALL(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0));
    
    char *vertexShaderSource = 
        "#version 430 core\nlayout (location = 0) in vec3 aPos;\nlayout (location = 1) in vec2 aTexCoord;\nout vec3 ourColor;\nout vec2 TexCoord;\nuniform mat4 orthoProjection;\nuniform mat4 translationMat;\nuniform mat4 rotationMatAboutZAxis;\nvoid main(){gl_Position =  orthoProjection * translationMat * rotationMatAboutZAxis * vec4(aPos, 1.0f);ourColor = vec3(1.0f, 0.0f, 0.0f);TexCoord = vec2(aTexCoord.x, aTexCoord.y);}";
    
    char *fragmentShaderSource = 
        "#version 430 core\nout vec4 FragColor;in vec3 ourColor;in vec2 TexCoord;uniform sampler2D texture1;void main(){FragColor = vec4(((texture(texture1, TexCoord) * (1.0f - 0.0f)) + (ourColor * 0.0f)).rgb, texture(texture1, TexCoord).a) ;}";
    
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
    
    // Load and upload image to the GPU
    imagePath = "../data/shot1.png";
    stbi_set_flip_vertically_on_load(true);
    imageData = stbi_load(imagePath, &imageWidth, &imageHeight, &imageChannels, 4);
    ASSERT(imageData, "Couldn't load image");
    uint32 bulletTextureId;
    GL_CALL(glActiveTexture(GL_TEXTURE0));
    GL_CALL(glGenTextures(1, &bulletTextureId));
    GL_CALL(glBindTexture(GL_TEXTURE_2D, bulletTextureId));
    GL_CALL(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, imageWidth, imageHeight, 0,
                         GL_RGBA, GL_UNSIGNED_BYTE, imageData));
    
    GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
    GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
    GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER));
    GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER));
    //GL_CALL(glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, debugColor));
    GL_CALL(glBindTexture(GL_TEXTURE_2D, 0));
    stbi_image_free(imageData);
    
    // Setting up VAO 
    max = {(real32)imageWidth/2.0f, (real32)imageHeight/2.0f};
    min = {-((real32)imageWidth/2.0f), -((real32)imageHeight/2.0f)};
    real32 bulletVertices[] = 
    {
        // positions          // texture coords
        max.x, max.y, 0.0f,   1.0f, 1.0f, // top right
        max.x, min.y, 0.0f,   1.0f, 0.0f, // bottom right
        min.x, min.y, 0.0f,   0.0f, 0.0f, // bottom left
        min.x, max.y, 0.0f,   0.0f, 1.0f  // top left 
    };
    
    uint32 bulletVao;
    GL_CALL(glGenVertexArrays(1, &bulletVao));
    GL_CALL(glGenBuffers(1, &vbo));
    GL_CALL(glBindVertexArray(bulletVao));
    GL_CALL(glBindBuffer(GL_ARRAY_BUFFER, vbo));
    GL_CALL(glBufferData(GL_ARRAY_BUFFER, sizeof(bulletVertices), bulletVertices, GL_STATIC_DRAW));
    GL_CALL(glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0));
    GL_CALL(glEnableVertexAttribArray(0));
    GL_CALL(glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float))));
    GL_CALL(glEnableVertexAttribArray(1));
    GL_CALL(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo));
    GL_CALL(glBindVertexArray(0));
    GL_CALL(glBindBuffer(GL_ARRAY_BUFFER, 0));
    GL_CALL(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0));
    
    
    // Load asteroids image
    imagePath = "../data/asteroid_blend.png";
    stbi_set_flip_vertically_on_load(true);
    imageData = stbi_load(imagePath, &imageWidth, &imageHeight, &imageChannels, 4);
    ASSERT(imageData, "Couldn't load image");
    uint32 asteroidTextureId;
    GL_CALL(glActiveTexture(GL_TEXTURE0));
    GL_CALL(glGenTextures(1, &asteroidTextureId));
    GL_CALL(glBindTexture(GL_TEXTURE_2D, asteroidTextureId));
    GL_CALL(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, imageWidth, imageHeight, 0,
                         GL_RGBA, GL_UNSIGNED_BYTE, imageData));
    
    GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
    GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
    GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER));
    GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER));
    //GL_CALL(glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, debugColor));
    GL_CALL(glBindTexture(GL_TEXTURE_2D, 0));
    stbi_image_free(imageData);
    
    // Setting up VAO 
    max = {(real32)imageWidth/2.0f, (real32)imageHeight/2.0f};
    min = {-((real32)imageWidth/2.0f), -((real32)imageHeight/2.0f)};
    real32 asteroidVertices[] = 
    {
        // positions          // texture coords
        max.x, max.y, 0.0f,   1.0f, 1.0f, // top right
        max.x, min.y, 0.0f,   1.0f, 0.0f, // bottom right
        min.x, min.y, 0.0f,   0.0f, 0.0f, // bottom left
        min.x, max.y, 0.0f,   0.0f, 1.0f  // top left 
    };
    
    uint32 asteroidVao;
    GL_CALL(glGenVertexArrays(1, &asteroidVao));
    GL_CALL(glGenBuffers(1, &vbo));
    GL_CALL(glBindVertexArray(asteroidVao));
    GL_CALL(glBindBuffer(GL_ARRAY_BUFFER, vbo));
    GL_CALL(glBufferData(GL_ARRAY_BUFFER, sizeof(asteroidVertices), asteroidVertices, GL_STATIC_DRAW));
    GL_CALL(glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0));
    GL_CALL(glEnableVertexAttribArray(0));
    GL_CALL(glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float))));
    GL_CALL(glEnableVertexAttribArray(1));
    GL_CALL(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo));
    GL_CALL(glBindVertexArray(0));
    GL_CALL(glBindBuffer(GL_ARRAY_BUFFER, 0));
    GL_CALL(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0));
#endif
    
    PfRequestSwapInterval(1);
    
    PfTimestamp start = PfGetTimestamp();
    uint64 startCycles = PfRdtsc();
    
    bool recording = false;
    bool playing = false;
    GameInput input = {};
    gameState->spaceshipPos = {(real32)clientRect.width/2.0f, (real32)clientRect.height/2.0f, 0.0f};
    gameState->spaceshipVel = {};
    gameState->spaceshipRotation = 0.0f;
    gameState->bulletRadius = 3.0f;
    gameState->spaceshipRadius = 35.0f;
    gameState->asteroidRadius = 40.0f;
    
    int64 inputFileHandle = -1;
    int32 recordSlot = 0;
    
    int32 totalSlots = 10; 
    while(!window[0].shouldClose)
    {
        clientRect = PfGetClientRect(&window[0]);
        
        if(!recording && PfGetKeyState(&window[0], PF_LEFT_CTRL) && (PfGetKeyState(&window[0], PF_LEFT_SHIFT) == false) && PfGetKeyState(&window[0], PF_R))
        {
            recording = true;
            char gameMemoryFileName[32] = {};
            char inputFileName[32] = {};
            sprintf(gameMemoryFileName, "game_memory_%d.memory", recordSlot);
            sprintf(inputFileName, "input_%d.input_series", recordSlot);
            recordSlot = (recordSlot + 1) % totalSlots;
            PfWriteEntireFile(gameMemoryFileName, (void *)mem.base, (uint32)mem.size);
            inputFileHandle = PfCreateFile(inputFileName, PF_WRITE, PF_CREATE);
            ASSERT(inputFileHandle != -1, "File handle to record the input is invalid");
        }
        
        if(recording && PfGetKeyState(&window[0], PF_LEFT_CTRL) &&  PfGetKeyState(&window[0], PF_LEFT_SHIFT) && PfGetKeyState(&window[0], PF_R))
        {
            recording = false;
            PfCloseFileHandle(inputFileHandle);
        }
        
        if(playingRestart || (!playing && PfGetKeyState(&window[0], PF_LEFT_CTRL) && (PfGetKeyState(&window[0], PF_LEFT_SHIFT) == false) && PfGetKeyState(&window[0], PF_P)))
        {
            playingRestart = false;
            playing = true;
            
#if HOT_CODE_RELOADABLE
            hcrData.playBackSlot = playBackSlot;
            hcrData.mem = mem;
            PfWriteEntireFile("hot_code_relaunch_persistent_data", (void *)&hcrData, sizeof(HotCodeRelaunchPersistentData));
#endif
            char gameMemoryFileName[32] = {};
            char inputFileName[32] = {};
            sprintf(gameMemoryFileName, "game_memory_%d.memory", playBackSlot);
            sprintf(inputFileName, "input_%d.input_series", playBackSlot);
            
            PfReadEntireFile(gameMemoryFileName, (void *)mem.base, (uint32)mem.size);
            //gameState->bullets = 0;
            inputFileHandle = PfCreateFile(inputFileName, PF_READ, PF_OPEN);
            ASSERT(inputFileHandle != -1, "File handle to read the input is invalid");
        }
        
        if(playing && PfGetKeyState(&window[0], PF_LEFT_CTRL) &&  PfGetKeyState(&window[0], PF_LEFT_SHIFT) && PfGetKeyState(&window[0], PF_P))
        {
            playing = false;
            PfCloseFileHandle(inputFileHandle);
            
#if HOT_CODE_RELOADABLE
            PfDeleteFile("hot_code_relaunch_persistent_data");
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
                PfCloseFileHandle(inputFileHandle);
                playingRestart = true;
            }
            else
            {
                int64 bytesRead = PfReadFile(inputFileHandle, (void *)&input, sizeof(input));
                if(bytesRead == 0)
                {
                    input = {};
                    PfCloseFileHandle(inputFileHandle);
                    playingRestart = true;
                }
            }
        }
        else
        {
            input.throttle = PfGetKeyState(&window[0], PF_W);
            input.left = PfGetKeyState(&window[0], PF_A);
            input.brake = PfGetKeyState(&window[0], PF_S);
            input.right = PfGetKeyState(&window[0], PF_D);
            input.fire = PfGetKeyState(&window[0], PF_SPACEBAR);
        }
        
        if(recording)
        {
            PfWriteFile(inputFileHandle, (void *)&input, sizeof(input));
        }
        
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
        
        if(input.left) gameState->spaceshipRotation += 2.0f; 
        if(input.right) gameState->spaceshipRotation -= 2.0f;
        
        v3 forward = v3{Cosine(gameState->spaceshipRotation * DEG_TO_RAD), Sine(gameState->spaceshipRotation * DEG_TO_RAD), 0.0f};
        v3 accelaration = {};
        real32 breakingFactor = 1.5f;
        real32 accelarationMultiplier = 750.0f;
        if(input.throttle) accelaration = forward * accelarationMultiplier;
        if(input.brake) breakingFactor = 3.0f;
        
        accelaration = accelaration - (gameState->spaceshipVel * breakingFactor);
        v3 displacement = (gameState->spaceshipVel*dT) + (accelaration*0.5f*dT*dT);
        gameState->spaceshipPos = gameState->spaceshipPos + displacement;
        gameState->spaceshipVel = gameState->spaceshipVel + (accelaration * dT);
        
        if(gameState->spaceshipPos.x < 0.0f)
        {
            gameState->spaceshipPos.x = (real32)clientRect.width + gameState->spaceshipPos.x;
        }
        else if(gameState->spaceshipPos.x >= clientRect.width)
        {
            gameState->spaceshipPos.x = (real32)clientRect.width - gameState->spaceshipPos.x;
        }
        if(gameState->spaceshipPos.y < 0.0f)
        {
            gameState->spaceshipPos.y = (real32)clientRect.height + gameState->spaceshipPos.y;
        }
        else if(gameState->spaceshipPos.y >= clientRect.height)
        {
            gameState->spaceshipPos.y = (real32)clientRect.height - gameState->spaceshipPos.y;
        }
        
        gameState->firingCoolDown -= dT;
        real32 balance = 0.0f;
        if(gameState->firingCoolDown <= 0.0f) 
        {
            balance = -gameState->firingCoolDown;
            gameState->firingCoolDown = 0.0f; 
        }
        
        if(input.fire)
        {
            if(gameState->firingCoolDown == 0.0f)
            {
                Bullet *bullet = (Bullet*)AllocateChunk(&(gameState->bulletAllocator));
                if(bullet)
                {
                    bullet->pos = gameState->spaceshipPos + (forward * ((gameState->spaceshipDim.x * 0.5f) + 5.0f));
                    bullet->vel = forward * 1000.0f; // Multiply the velocity by 1000.0f
                    bullet->duration = 1.0f; // Change duration to 1.0f
                    bullet->next = gameState->bullets;
                    gameState->firingCoolDown = 0.3f;
                    
                    gameState->bullets = bullet;
                }
            }
        }
        else
        {
            gameState->firingCoolDown = 0.0f; 
        }
        
        Bullet *previous = 0;
        Bullet *bullet = gameState->bullets;
        while(bullet)
        {
            bullet->duration -= dT;
            if(bullet->duration <= 0.0f)
            {
                Bullet *toDelete = bullet;
                if(previous)
                {
                    previous->next = bullet->next;
                }
                else
                {
                    gameState->bullets = bullet->next;
                }
                bullet = bullet->next;
                FreeChunk(&(gameState->bulletAllocator), (uint8*)toDelete);
            }
            else
            {
                bullet->pos = bullet->pos + (bullet->vel * dT);
                if(bullet->pos.x < 0.0f)
                {
                    bullet->pos.x = (real32)clientRect.width + bullet->pos.x;
                }
                else if(bullet->pos.x >= clientRect.width)
                {
                    bullet->pos.x = (real32)clientRect.width - bullet->pos.x;
                }
                if(bullet->pos.y < 0.0f)
                {
                    bullet->pos.y = (real32)clientRect.height + bullet->pos.y;
                }
                else if(bullet->pos.y >= clientRect.height)
                {
                    bullet->pos.y = (real32)clientRect.height - bullet->pos.y;
                }
                
                previous = bullet;
                bullet = bullet->next;
            }
        }
        
        Asteroid *asteroid = gameState->asteroids;
        while(asteroid)
        {
            asteroid->pos = asteroid->pos + (asteroid->vel * dT);
            asteroid->rotation = asteroid->rotation + (asteroid->rotationalVelocity * dT);
            if(asteroid->pos.x < 0.0f)
            {
                asteroid->pos.x = (real32)clientRect.width + asteroid->pos.x;
            }
            else if(asteroid->pos.x >= clientRect.width)
            {
                asteroid->pos.x = (real32)clientRect.width - asteroid->pos.x;
            }
            if(asteroid->pos.y < 0.0f)
            {
                asteroid->pos.y = (real32)clientRect.height + asteroid->pos.y;
            }
            else if(asteroid->pos.y >= clientRect.height)
            {
                asteroid->pos.y = (real32)clientRect.height - asteroid->pos.y;
            }
            asteroid = asteroid->next;
        }
        
        Bullet *previousBullet = 0;
        bullet = gameState->bullets;
        while(bullet)
        {
            v2 bulletPos = {bullet->pos.x, bullet->pos.y};
            Asteroid *previousAsteroid = 0;
            asteroid = gameState->asteroids;
            bool deleteThisBullet = false;
            while(asteroid)
            {
                v2 asteroidPos = {asteroid->pos.x, asteroid->pos.y};
                bool collidedWithBullet = CircleWithCircleCollisionTest(bulletPos, gameState->bulletRadius, asteroidPos, gameState->asteroidRadius);
                if(collidedWithBullet)
                {
                    deleteThisBullet = true;
                    if(previousAsteroid)
                    {
                        previousAsteroid->next = asteroid->next;
                    }
                    else
                    {
                        gameState->asteroids = asteroid->next;
                    }
                    Asteroid *next = asteroid->next;
                    FreeChunk(&(gameState->asteroidAllocator), (uint8 *)asteroid);
                    asteroid = next;
                }
                else
                {
                    previousAsteroid = asteroid;
                    asteroid = asteroid->next;
                }
            }
            
            if(deleteThisBullet)
            {
                if(previousBullet)
                {
                    previousBullet->next = bullet->next;
                }
                else
                {
                    gameState->bullets = bullet->next;
                }
                Bullet *next = bullet->next;
                FreeChunk(&(gameState->bulletAllocator), (uint8*)bullet);
                bullet = next;
            }
            else
            {
                previousBullet = bullet;
                bullet = bullet->next;
            }
        }
        
        Asteroid *previousAsteroid = 0;
        asteroid = gameState->asteroids;
        v2 spaceshipPos = {gameState->spaceshipPos.x, gameState->spaceshipPos.y};
        while(asteroid)
        {
            v2 asteroidPos = {asteroid->pos.x, asteroid->pos.y};
            bool collidedWithSpaceship = CircleWithCircleCollisionTest(spaceshipPos, gameState->spaceshipRadius, asteroidPos, gameState->asteroidRadius);
            if(collidedWithSpaceship)
            {
                if(previousAsteroid)
                {
                    previousAsteroid->next = asteroid->next;
                }
                else
                {
                    gameState->asteroids = asteroid->next;
                }
                Asteroid *next = asteroid->next;
                FreeChunk(&(gameState->asteroidAllocator), (uint8 *)asteroid);
                asteroid = next;
            }
            else
            {
                previousAsteroid = asteroid;
                asteroid = asteroid->next;
            }
        }
        
        
        
        
        PfglMakeCurrent(&window[0]);
        GL_CALL(glClearColor(0.0f, 0.0f, 0.0f, 1.0f));
        GL_CALL(glClear(GL_COLOR_BUFFER_BIT));
        GL_CALL(GLboolean wasBlendEnabled = glIsEnabled(GL_BLEND));
        GL_CALL(glEnable(GL_BLEND));
        GL_CALL(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));
        
        GL_CALL(glActiveTexture(GL_TEXTURE0));
        GL_CALL(glBindTexture(GL_TEXTURE_2D, spaceshipTextureId));
        GL_CALL(glBindVertexArray(spaceshipVao));
        GL_CALL(glUseProgram(programId));
        GL_CALL(GLint samplerUniformLocation = glGetUniformLocation(programId, "texture1"));
        GL_CALL(GLint rotationMatAboutZAxisUniformLocation = glGetUniformLocation(programId, "rotationMatAboutZAxis"));
        GL_CALL(GLint translationMatUniformLocation = glGetUniformLocation(programId, "translationMat"));
        GL_CALL(GLint orthoProjectionUniformLocation = glGetUniformLocation(programId, "orthoProjection"));
        
        GL_CALL(glUniform1i(samplerUniformLocation, 0)); // Setting the texture unit
        
        mat4 rotationMatAboutZAxis = {};
        RotationAboutZAxis(&rotationMatAboutZAxis, gameState->spaceshipRotation);
        
        mat4 translationMat = {};
        TranslationMat(&translationMat, gameState->spaceshipPos);
        
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
        
        // Draw bullet
        GL_CALL(glActiveTexture(GL_TEXTURE0));
        GL_CALL(glBindTexture(GL_TEXTURE_2D, bulletTextureId));
        GL_CALL(glBindVertexArray(bulletVao));
        
        Identity(&rotationMatAboutZAxis);
        
        bullet = gameState->bullets;
        while(bullet)
        {
            TranslationMat(&translationMat, bullet->pos);
            GL_CALL(glUniformMatrix4fv(rotationMatAboutZAxisUniformLocation, 1, GL_TRUE, rotationMatAboutZAxis.data)); 
            GL_CALL(glUniformMatrix4fv(translationMatUniformLocation, 1, GL_TRUE, translationMat.data)); 
            GL_CALL(glUniformMatrix4fv(orthoProjectionUniformLocation, 1, GL_TRUE, orthoProjection.data)); 
            GL_CALL(glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0));
            bullet = bullet->next;
        }
        
        // Draw asteroid
        GL_CALL(glActiveTexture(GL_TEXTURE0));
        GL_CALL(glBindTexture(GL_TEXTURE_2D, asteroidTextureId));
        GL_CALL(glBindVertexArray(asteroidVao));
        
        asteroid = gameState->asteroids;
        while(asteroid)
        {
            RotationAboutZAxis(&rotationMatAboutZAxis, asteroid->rotation);
            TranslationMat(&translationMat, asteroid->pos);
            GL_CALL(glUniformMatrix4fv(rotationMatAboutZAxisUniformLocation, 1, GL_TRUE, rotationMatAboutZAxis.data)); 
            GL_CALL(glUniformMatrix4fv(translationMatUniformLocation, 1, GL_TRUE, translationMat.data)); 
            GL_CALL(glUniformMatrix4fv(orthoProjectionUniformLocation, 1, GL_TRUE, orthoProjection.data)); 
            GL_CALL(glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0));
            asteroid = asteroid->next;
        }
        
        // Unbind stuff
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
        sprintf(temp, "%.2fms %.2FPS %.3fMHz Spaceship:(%.2f, %.2f, %.2f) RECORDING: %d PLAYING:%d", secondsPerFrame * 1000.0f, 1.0f/secondsPerFrame, cyclesPerFrame/(secondsPerFrame * 1000.0f * 1000.0f), gameState->spaceshipPos.x, gameState->spaceshipPos.y, gameState->spaceshipPos.z, recording, playing);
        
        PfSetWindowTitle(&window[0], temp);
        PfUpdate();
    }
    
#if PLATFORM_LINUX
    XCloseDisplay(globalDisplay);
#endif
    DEBUG_LOG("Exiting application.\n");
    return 0;
}

#endif