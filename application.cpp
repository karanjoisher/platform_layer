#include "utility.h"
#include "project_types.h"
#include "math.h"

#define PF_WINDOW_AND_INPUT
#define PF_TIME
#define PF_FILE
#define PF_SOUND
#include "pf.h"

#if 1
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

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
    ASSERT(result, "Ran out of chunks");
    return result;
}


void FreeChunk(ChunkAllocator *ca, uint8 *chunk)
{
    int32 chunkIndex = (int32)(chunk - ca->base)/ca->chunkSize;
    *((int32*)chunk) = ca->nextFreeChunkIndex;
    ca->nextFreeChunkIndex = chunkIndex;
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


struct Sound
{
    uint32 id;
    int16 *data;
    uint64 sampleCount;
};

struct PlayingSound
{
    Sound *sound;
    real32 volume1;
    real32 volume2;
    bool loop;
    bool paused;
    
    // NOTE(KARAN):  Not to be used by API user
    uint32 id;
    PlayingSound *next;
    uint64 nextSampleIndexToPlay;
    bool destroyNow;
};

// NOTE(KARAN): Sorry for the bad naming
struct PlayingSoundId
{
    PlayingSound *playingSound;
    uint32 playingSoundId;
    uint32 soundId;
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
    
    ChunkAllocator playingSoundAllocator;
    uint32 playingSoundNextId;
    PlayingSound *soundsPlaying;
    
    real32 spaceshipRadius;
    real32 bulletRadius;
    real32 asteroidRadius;
    
    int32 playBackSlot;
    int32 recordSlot;
    int32 totalSlots;
};

bool IsValidPlayingSoundId(PlayingSoundId playingSoundId)
{
    return playingSoundId.playingSound;
}

PlayingSoundId PlaySound(GameState *gameState, Sound *sound, bool loop, real32 volume1, real32 volume2)
{
    PlayingSoundId result = {};
    PlayingSound *soundToPlay = (PlayingSound*)AllocateChunk(&(gameState->playingSoundAllocator));
    soundToPlay->id = gameState->playingSoundNextId++;
    if(gameState->playingSoundNextId == 0) gameState->playingSoundNextId = 1;
    soundToPlay->sound = sound;
    soundToPlay->volume1 = volume1;
    soundToPlay->volume2 = volume2;
    soundToPlay->nextSampleIndexToPlay = 0;
    soundToPlay->loop = loop;
    soundToPlay->paused = false;
    soundToPlay->destroyNow = false;
    
    soundToPlay->next = gameState->soundsPlaying;
    gameState->soundsPlaying = soundToPlay;
    
    result.playingSound = soundToPlay;
    result.playingSoundId = soundToPlay->id;
    result.soundId = sound->id;
    
    return result;
}

PlayingSound* GetPlayingSound(GameState *gameState, PlayingSoundId playingSoundId)
{
    PlayingSound *result = playingSoundId.playingSound;
    if(result && result->id == playingSoundId.playingSoundId && result->sound->id == playingSoundId.soundId)
    {
        
    }
    else
    {
        result = 0;
    }
#if 0
    if(IsValidPlayingSoundId(playingSoundId))
    {
        PlayingSound *playingSound = gameState->soundsPlaying;
        while(playingSound)
        {
            if(playingSound->id == playingSoundId.playingSoundId && playingSound->sound->id == playingSoundId.soundId)
            {
                result = playingSound;
                break;
            }
            playingSound = playingSound->next;
        }
    }
#endif
    return result;
}


void DestroySound(GameState *gameState, PlayingSoundId playingSoundId)
{
    PlayingSound *soundPlaying = GetPlayingSound(gameState, playingSoundId);
    if(soundPlaying) soundPlaying->destroyNow = true;
}


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
};

struct TextureInfo 
{
    uint32 textureId;
    int32 width;
    int32 height;
    int32 numChannels;
};

TextureInfo OpenGLGenTexture(char *imagePath)
{
    // NOTE(KARAN): Only supports RGBA8
    TextureInfo result = {};
    
    stbi_set_flip_vertically_on_load(true);
    int imageWidth, imageHeight, imageChannels;
    unsigned char *imageData = stbi_load(imagePath, &imageWidth, &imageHeight, &imageChannels, 4);
    ASSERT(imageChannels == 4, "Only supports RGBA textures");
    ASSERT(imageData, "Couldn't load image");
    
    uint32 textureId;
    GL_CALL(glActiveTexture(GL_TEXTURE0));
    GL_CALL(glGenTextures(1, &textureId));
    GL_CALL(glBindTexture(GL_TEXTURE_2D, textureId));
    GL_CALL(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, imageWidth, imageHeight, 0,
                         GL_RGBA, GL_UNSIGNED_BYTE, imageData));
    
    GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
    GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
    GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER));
    GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER));
    //GLfloat debugColor[] = {1.0f, 0.0f, 1.0f, 1.0f};
    //GL_CALL(glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, debugColor));
    GL_CALL(glBindTexture(GL_TEXTURE_2D, 0));
    stbi_image_free(imageData);
    
    result.textureId = textureId;
    result.width = imageWidth;
    result.height = imageHeight;
    result.numChannels = imageChannels;
    
    return result;
}

uint32 OpenGLGenVAOForQuadrilateralCenteredAtOrigin(int32 width, int32 height)
{
    uint32 result = 0;
    
    v2 max = {(real32)width/2.0f, (real32)height/2.0f};
    v2 min = {-((real32)width/2.0f), -((real32)height/2.0f)};
    
    real32 vertices[] = 
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
    
    uint32 vbo, ebo;
    GL_CALL(glGenVertexArrays(1, &result));
    GL_CALL(glGenBuffers(1, &vbo));
    GL_CALL(glGenBuffers(1, &ebo));
    GL_CALL(glBindVertexArray(result));
    GL_CALL(glBindBuffer(GL_ARRAY_BUFFER, vbo));
    GL_CALL(glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW));
    GL_CALL(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo));
    GL_CALL(glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(quadIndices), quadIndices, GL_STATIC_DRAW));
    GL_CALL(glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0));
    GL_CALL(glEnableVertexAttribArray(0));
    GL_CALL(glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float))));
    GL_CALL(glEnableVertexAttribArray(1));
    GL_CALL(glBindVertexArray(0));
    GL_CALL(glBindBuffer(GL_ARRAY_BUFFER, 0));
    GL_CALL(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0));
    
    return result;
}


uint32 OpenGLGenProgramId(char *vertexShaderSource, char *fragmentShaderSource)
{
    uint32 result = 0;
    
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
    result = programId;
    
    return result;
}



int main(int argc, char **argv)
{
    real32 fps = 60.0f;
    real32 targetMillisecondsPerFrame = (1000.0f/fps);
    real32 dT = 1.0f/fps;
    
    PfInitialize();
    PfGLConfig(4, 3, true);
    
    PfWindow window[1] = {};
    PfCreateWindow(&window[0], (char*)"WINDOW 0", 0, 0, 800, 800);
    PfRect clientRect = PfGetClientRect(&window[0]);
    
    int32 playBackSlot = 0;
    bool playingRestart = false;
    
    Memory gameStateStorage = {};
    Memory assetStorage = {};
    HotCodeRelaunchPersistentData hcrData = {};
#if HOT_CODE_RELOADABLE
    if(argc >= 2 && AreStringsSame(argv[1], "hcr_reloaded") && PfFilepathExists("hot_code_relaunch_persistent_data"))
    {
        PfReadEntireFile("hot_code_relaunch_persistent_data", (void *)&hcrData);
        playBackSlot = hcrData.playBackSlot;
        playingRestart = true;
    }
#endif
    gameStateStorage.size = MEGABYTES((uint64)512);
    assetStorage.size = MEGABYTES((uint64)128);
    
    void *baseAddressForGameMemoryStorage = (void*)0;
#if DEBUG_BUILD | HOT_CODE_RELOADABLE | INPUT_RECORDING_PLAYBACK
    baseAddressForGameMemoryStorage = (void*)TERABYTES((uint64)4);
#endif
    gameStateStorage.base = (uint8*)PfVirtualAlloc(baseAddressForGameMemoryStorage,(size_t)(gameStateStorage.size + assetStorage.size));
    assetStorage.base = gameStateStorage.base + gameStateStorage.size;
#if DEBUG_BUILD
    ASSERT(((void*)gameStateStorage.base) == baseAddressForGameMemoryStorage, "Could not allocate game storage at predefined address location(To support hot code reload and input playback)");
#endif
    
    //Sound
    bool soundPlaying = false;
    uint32 soundFramesPerSecond = 48000;
    uint32 numChannels = 2;
    uint32 bitsPerSample = 16;
    uint32 bytesPerFrame = 4; 
    real32 bufferDurationInMS = 1000.0f;//targetMillisecondsPerFrame * 1.5f;
    uint64 requestedBufferFrames = CeilReal32ToUint64(bufferDurationInMS * ((real32)soundFramesPerSecond/1000.0f));
    //
    GameState *gameState = 0;
    gameState = PushStruct(&gameStateStorage, GameState);
    
    gameState->bulletAllocator.length = 20;
    gameState->bulletAllocator.chunkSize = sizeof(Bullet);
    gameState->bulletAllocator.base = (uint8 *)PushArray(&gameStateStorage, Bullet, gameState->bulletAllocator.length);
    InitializeChunkAllocator(&(gameState->bulletAllocator));
    
    gameState->asteroidAllocator.length = 30;
    gameState->asteroidAllocator.chunkSize = sizeof(Asteroid);
    gameState->asteroidAllocator.base = (uint8 *)PushArray(&gameStateStorage, Asteroid, gameState->asteroidAllocator.length);
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
    
    gameState->playingSoundAllocator.length = 30;
    gameState->playingSoundNextId = 1;
    gameState->playingSoundAllocator.chunkSize = sizeof(PlayingSound);
    gameState->playingSoundAllocator.base = (uint8 *)PushArray(&gameStateStorage, PlayingSound, gameState->playingSoundAllocator.length);
    InitializeChunkAllocator(&(gameState->playingSoundAllocator));
    
    Sound* backgroundSound = 0;
    Sound* pewSound = 0;
    Sound* thrustSound = 0;
    Sound* boomSound = 0;
    
    Sound **soundPointers[] = {&backgroundSound, &pewSound, &thrustSound, &boomSound};
    char* filepaths[] = {"../data/sounds/ambient_space.pcm_s16_il", "../data/sounds/pew_1.pcm_s16_il", "../data/sounds/thrust_1.pcm_s16_il", "../data/sounds/boom_1.pcm_s16_il"};
    
    int64 bytesRead = 0;
    for(int i = 0; i < ARRAY_COUNT(soundPointers); i++)
    {
        Sound **soundPointer = soundPointers[i];
        *soundPointer = PushStruct(&gameStateStorage, Sound);
        Sound *sound = *soundPointer;
        uint32 soundFileSize = (uint32)PfGetFileSize(filepaths[i]);
        sound->id = i + 1;
        sound->data = (int16*)PushSize(&assetStorage, soundFileSize);
        bytesRead = PfReadEntireFile(filepaths[i], (void *)(sound->data));
        ASSERT((uint32)bytesRead == soundFileSize, "Bytes read from sound file not equal to file size");
        sound->sampleCount = soundFileSize/(bytesPerFrame/numChannels);
    }
    
    uint64 soundMixingBufferLength = requestedBufferFrames * numChannels;
    real32 *soundMixingBuffer = (real32*)PushArray(&assetStorage, real32, soundMixingBufferLength);
    
    PlayingSoundId backgroundPlayingSoundID = PlaySound(gameState, backgroundSound, true, 0.9f, 0.9f);
    PlayingSoundId thrustPlayingSoundId = {}; 
    
    GameInput input = {};
    int64 inputFileHandle = -1;
    
    gameState->spaceshipPos = {(real32)clientRect.width/2.0f, (real32)clientRect.height/2.0f, 0.0f};
    gameState->spaceshipVel = {};
    gameState->spaceshipRotation = 0.0f;
    gameState->bulletRadius = 3.0f;
    gameState->spaceshipRadius = 35.0f;
    gameState->asteroidRadius = 40.0f;
    gameState->playBackSlot = playBackSlot;
    gameState->totalSlots = 10;
    bool recording = false;
    bool playing = false;
    
#if 1
    PfglMakeCurrent(&window[0]);
    
    TextureInfo spaceshipTextureInfo = OpenGLGenTexture("../data/images/spaceship.png");
    TextureInfo bulletTextureInfo = OpenGLGenTexture("../data/images/shot1.png");
    TextureInfo asteroidTextureInfo = OpenGLGenTexture("../data/images/asteroid_blend.png");
    
    gameState->spaceshipDim = {(real32)spaceshipTextureInfo.width, (real32)spaceshipTextureInfo.height};
    
    uint32 spaceshipVao = OpenGLGenVAOForQuadrilateralCenteredAtOrigin(spaceshipTextureInfo.width, spaceshipTextureInfo.height);
    uint32 bulletVao = OpenGLGenVAOForQuadrilateralCenteredAtOrigin(bulletTextureInfo.width, bulletTextureInfo.height);
    uint32 asteroidVao = OpenGLGenVAOForQuadrilateralCenteredAtOrigin(asteroidTextureInfo.width, asteroidTextureInfo.height);
    
    char *vertexShaderSource = 
        "#version 430 core\nlayout (location = 0) in vec3 aPos;\nlayout (location = 1) in vec2 aTexCoord;\nout vec3 ourColor;\nout vec2 TexCoord;\nuniform mat4 orthoProjection;\nuniform mat4 translationMat;\nuniform mat4 rotationMatAboutZAxis;\nvoid main(){gl_Position =  orthoProjection * translationMat * rotationMatAboutZAxis * vec4(aPos, 1.0f);ourColor = vec3(1.0f, 0.0f, 0.0f);TexCoord = vec2(aTexCoord.x, aTexCoord.y);}";
    
    char *fragmentShaderSource = 
        "#version 430 core\nout vec4 FragColor;in vec3 ourColor;in vec2 TexCoord;uniform sampler2D texture1;void main(){FragColor = texture(texture1, TexCoord);}";
    
    uint32 programId = OpenGLGenProgramId(vertexShaderSource, fragmentShaderSource);
#endif
    
    PfRequestSwapInterval(1);
    
    // Sound Initialization
    PfSoundSystem soundSystem = PfInitializeSoundSystem(requestedBufferFrames, bitsPerSample, numChannels, soundFramesPerSecond);
    
    PfTimestamp start = PfGetTimestamp();
    uint64 startCycles = PfRdtsc();
    //DEBUG_LOG("Input,Update,Render,Sound,Swap,Sleep\n");
    while(true)
    {
        PfTimestamp s0 = start;
        PfUpdate();
        if(window[0].shouldClose)
        {
            break;
        }
#if 1
        clientRect = PfGetClientRect(&window[0]);
        
#if INPUT_RECORDING_PLAYBACK
        if(!recording && PfGetKeyState(&window[0], PF_LEFT_CTRL) && (PfGetKeyState(&window[0], PF_LEFT_SHIFT) == false) && PfGetKeyState(&window[0], PF_R))
        {
            recording = true;
            char gameMemoryFileName[32] = {};
            char inputFileName[32] = {};
            sprintf(gameMemoryFileName, "game_memory_%d.memory", gameState->recordSlot);
            sprintf(inputFileName, "input_%d.input_series", gameState->recordSlot);
            gameState->recordSlot = (gameState->recordSlot + 1) % gameState->totalSlots;
            PfWriteEntireFile(gameMemoryFileName, (void *)gameStateStorage.base, (uint32)gameStateStorage.size);
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
            hcrData.playBackSlot = gameState->playBackSlot;
            PfWriteEntireFile("hot_code_relaunch_persistent_data", (void *)&hcrData, sizeof(HotCodeRelaunchPersistentData));
#endif
            char gameMemoryFileName[32] = {};
            char inputFileName[32] = {};
            sprintf(gameMemoryFileName, "game_memory_%d.memory", gameState->playBackSlot);
            sprintf(inputFileName, "input_%d.input_series", gameState->playBackSlot);
            
            PfReadEntireFile(gameMemoryFileName, (void *)gameStateStorage.base);
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
            
            if(PfGetKeyState(&window[0], PF_0))      { gameState->playBackSlot = 0; playBackSlotChanged = true; }
            else if(PfGetKeyState(&window[0], PF_1)) { gameState->playBackSlot = 1; playBackSlotChanged = true; }
            else if(PfGetKeyState(&window[0], PF_2)) { gameState->playBackSlot = 2; playBackSlotChanged = true; }
            else if(PfGetKeyState(&window[0], PF_3)) { gameState->playBackSlot = 3; playBackSlotChanged = true; }
            else if(PfGetKeyState(&window[0], PF_4)) { gameState->playBackSlot = 4; playBackSlotChanged = true; }
            else if(PfGetKeyState(&window[0], PF_5)) { gameState->playBackSlot = 5; playBackSlotChanged = true; }
            else if(PfGetKeyState(&window[0], PF_6)) { gameState->playBackSlot = 6; playBackSlotChanged = true; }
            else if(PfGetKeyState(&window[0], PF_7)) { gameState->playBackSlot = 7; playBackSlotChanged = true; }
            else if(PfGetKeyState(&window[0], PF_8)) { gameState->playBackSlot = 8; playBackSlotChanged = true; }
            else if(PfGetKeyState(&window[0], PF_9)) { gameState->playBackSlot = 9; playBackSlotChanged = true; }
            
            if(playBackSlotChanged)
            {
                PfCloseFileHandle(inputFileHandle);
                playingRestart = true;
            }
            else
            {
                bytesRead = PfReadFile(inputFileHandle, (void *)&input, sizeof(input));
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
#endif
            input.throttle = PfGetKeyState(&window[0], PF_W);
            input.left = PfGetKeyState(&window[0], PF_A);
            input.brake = PfGetKeyState(&window[0], PF_S);
            input.right = PfGetKeyState(&window[0], PF_D);
            input.fire = PfGetKeyState(&window[0], PF_SPACEBAR);
            
            if(PfGetKeyState(&window[0], PF_NUMPAD_8))
            {
                PlayingSound *backgroundPlayingSound = GetPlayingSound(gameState, backgroundPlayingSoundID);
                if(backgroundPlayingSound)
                {
                    backgroundPlayingSound->paused = true;
                }
            }
            else if(PfGetKeyState(&window[0], PF_NUMPAD_2))
            {
                PlayingSound *backgroundPlayingSound = GetPlayingSound(gameState, backgroundPlayingSoundID);
                if(backgroundPlayingSound)
                {
                    backgroundPlayingSound->paused = false;
                }
            }
            
            if(PfGetKeyState(&window[0], PF_NUMPAD_0))
            {
                DestroySound(gameState, backgroundPlayingSoundID);
            }
            else if(PfGetKeyState(&window[0], PF_NUMPAD_1))
            {
                backgroundPlayingSoundID = PlaySound(gameState, backgroundSound, true, 0.9f, 0.9f);
            }
#if INPUT_RECORDING_PLAYBACK
        }
        if(recording)
        {
            PfWriteFile(inputFileHandle, (void *)&input, sizeof(input));
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
        
        PfTimestamp s1 = PfGetTimestamp();
        real32 t0 = PfGetSeconds(s0, s1);
        
        if(input.left) gameState->spaceshipRotation += 2.0f; 
        if(input.right) gameState->spaceshipRotation -= 2.0f;
        
        v3 forward = v3{Cosine(gameState->spaceshipRotation * DEG_TO_RAD), Sine(gameState->spaceshipRotation * DEG_TO_RAD), 0.0f};
        v3 accelaration = {};
        real32 breakingFactor = 1.5f;
        real32 accelarationMultiplier = 750.0f;
        
        if(input.throttle)
        {
            accelaration = forward * accelarationMultiplier;
            if(!IsValidPlayingSoundId(thrustPlayingSoundId))
            {
                thrustPlayingSoundId = PlaySound(gameState, thrustSound, true, 0.2f, 0.2f);
            }
        }
        else
        {
            DestroySound(gameState, thrustPlayingSoundId);
            thrustPlayingSoundId = {};
        }
        
        if(input.brake) breakingFactor = 3.0f;
        
        accelaration = accelaration - (gameState->spaceshipVel * breakingFactor);
        v3 displacement = (gameState->spaceshipVel*dT) + (accelaration*0.5f*dT*dT);
        gameState->spaceshipPos = gameState->spaceshipPos + displacement;
        gameState->spaceshipVel = gameState->spaceshipVel + (accelaration * dT);
        WrapAroundIfOutOfBounds((v2*)(&(gameState->spaceshipPos)), {0.0f, 0.0f, (real32)clientRect.width, (real32)clientRect.height});
        
        gameState->firingCoolDown -= dT;
        if(gameState->firingCoolDown <= 0.0f) 
        {
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
                    
                    PlaySound(gameState, pewSound, false, 0.2f, 0.2f);
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
                WrapAroundIfOutOfBounds((v2*)(&(bullet->pos)), {0.0f, 0.0f, (real32)clientRect.width, (real32)clientRect.height});
                previous = bullet;
                bullet = bullet->next;
            }
        }
        
        Asteroid *asteroid = gameState->asteroids;
        while(asteroid)
        {
            asteroid->pos = asteroid->pos + (asteroid->vel * dT);
            asteroid->rotation = asteroid->rotation + (asteroid->rotationalVelocity * dT);
            WrapAroundIfOutOfBounds((v2*)(&(asteroid->pos)), {0.0f, 0.0f, (real32)clientRect.width, (real32)clientRect.height});
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
                    
                    PlaySound(gameState, boomSound, false, 0.2f, 0.2f);
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
                
                PlaySound(gameState, boomSound, false, 0.2f, 0.2f);
            }
            else
            {
                previousAsteroid = asteroid;
                asteroid = asteroid->next;
            }
        }
        PfTimestamp s2 = PfGetTimestamp();
        real32 t1 = PfGetSeconds(s1, s2);
#endif
        // Draw spaceship
        PfglMakeCurrent(&window[0]);
        GL_CALL(glClearColor(0.0f, 0.0f, 0.0f, 1.0f));
        GL_CALL(glClear(GL_COLOR_BUFFER_BIT));
        
#if 1
        GL_CALL(GLboolean wasBlendEnabled = glIsEnabled(GL_BLEND));
        GL_CALL(glEnable(GL_BLEND));
        GL_CALL(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));
        
        GL_CALL(glActiveTexture(GL_TEXTURE0));
        GL_CALL(glBindTexture(GL_TEXTURE_2D, spaceshipTextureInfo.textureId));
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
        GL_CALL(glBindTexture(GL_TEXTURE_2D, bulletTextureInfo.textureId));
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
        GL_CALL(glBindTexture(GL_TEXTURE_2D, asteroidTextureInfo.textureId));
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
#endif
        PfTimestamp s3 = PfGetTimestamp();
        real32 t2 = PfGetSeconds(s2, s3);
        
        // Render sound
        
        /*
        mrmixer's solution on https://hero.handmade.network/forums/code-discussion/t/102-day_19_-_audio_latency
        mmozeiko's solution on https://gist.github.com/mmozeiko/38c64bb65855d783645c
        
        int MaxSampleCount = (int)(SoundOutput.SecondaryBufferSize - SoundPaddingSize);
        SamplesToWrite = (int) SoundOutput.LatencySampleCount - SoundPaddingSize;
        if (SamplesToWrite < 0)
        {
        SamplesToWrite = 0;
        }
        assert(SamplesToWrite <= MaxSampleCount);
        */
        uint64 pendingFrames = PfGetPendingFrames(&soundSystem);
        uint64 availableFrames = requestedBufferFrames - pendingFrames;
        int64 framesToWrite = CeilReal32ToInt64(targetMillisecondsPerFrame * 1.8f * ((real32)soundFramesPerSecond/1000.0f)) - (int64)pendingFrames;
        if(framesToWrite < 0) framesToWrite = 0;
        ASSERT(framesToWrite <= (int64)availableFrames, "Frames to write is greater than free frames in the buffer");
        uint64 samplesToWrite = (uint64)(framesToWrite * numChannels);
        
        ClearArray((char*)soundMixingBuffer, (int)(soundMixingBufferLength * sizeof(real32))); 
        PlayingSound *currentSoundToPlay = gameState->soundsPlaying;
        PlayingSound *previousPlayedSound = 0;
        while(currentSoundToPlay)
        {
            if(!currentSoundToPlay->destroyNow && !currentSoundToPlay->paused)
            {
                uint64 samplesToMix = samplesToWrite;
                uint64 samplesPending = currentSoundToPlay->sound->sampleCount - currentSoundToPlay->nextSampleIndexToPlay;
                if(samplesPending < samplesToMix) samplesToMix = samplesPending;
                
                
                if(samplesToMix > 0)
                {
                    real32 *sampleToWrite = soundMixingBuffer;
                    uint64 samplesMixed = 0;
                    real32 volume1 = currentSoundToPlay->volume1;
                    real32 volume2 = currentSoundToPlay->volume2;
                    while(samplesMixed < samplesToMix)
                    {
                        *sampleToWrite++ += ((real32)(currentSoundToPlay->sound->data[currentSoundToPlay->nextSampleIndexToPlay++]) * volume1);
                        *sampleToWrite++ += ((real32)(currentSoundToPlay->sound->data[currentSoundToPlay->nextSampleIndexToPlay++]) * volume2);
                        
                        samplesMixed += 2;
                    }
                }
            }
            
            PlayingSound *nextSoundToPlay = currentSoundToPlay->next;
            if(currentSoundToPlay->destroyNow || currentSoundToPlay->nextSampleIndexToPlay == currentSoundToPlay->sound->sampleCount)
            {
                if(!currentSoundToPlay->destroyNow && currentSoundToPlay->loop) 
                {
                    currentSoundToPlay->nextSampleIndexToPlay = 0;
                    previousPlayedSound = currentSoundToPlay;
                }
                else
                {
                    if(previousPlayedSound)
                    {
                        previousPlayedSound->next = currentSoundToPlay->next;
                    }
                    else
                    {
                        gameState->soundsPlaying = currentSoundToPlay->next;
                    }
                    FreeChunk(&(gameState->playingSoundAllocator), (uint8*)currentSoundToPlay);
                }
            }
            else
            {
                previousPlayedSound = currentSoundToPlay;
            }
            
            currentSoundToPlay = nextSoundToPlay;
        }
        
        PfSoundBuffer soundBuffer = PfGetSoundBuffer(&soundSystem, framesToWrite);
        int16 *sampleToWrite = (int16*)soundBuffer.buffer;
        for(uint64 i = 0; i < samplesToWrite; i+=numChannels)
        {
            /*int16 value = (int16)(Sine(t) * amplitude * volumePercentage); 
            t += increment;
            
            *soundBuffer++ = value;
            *soundBuffer++ = value;
            */
            *sampleToWrite++ = (int16)soundMixingBuffer[i];
            *sampleToWrite++ = (int16)soundMixingBuffer[i + 1];
        }
        PfDispatchSoundBuffer(&soundSystem, &soundBuffer);
        
        if(!soundPlaying) 
        {
            PfStartSoundSystem(&soundSystem);
            soundPlaying = true;
        }
        
        PfTimestamp s4 = PfGetTimestamp();
        real32 t3 = PfGetSeconds(s3, s4);
        
        PfglSwapBuffers(&window[0]);
        
        PfTimestamp s5 = PfGetTimestamp();
        real32 t4 = PfGetSeconds(s4, s5);
        
        real32 msRequiredToRenderThisFrame = PfGetSeconds(start, PfGetTimestamp()) * 1000.0f;
        real32 sleepTime = targetMillisecondsPerFrame - msRequiredToRenderThisFrame;
        ASSERT((int32)sleepTime < targetMillisecondsPerFrame, "Sleep time is greater than target frame time");
        PfSleep((int32)sleepTime);
        
        PfTimestamp s6 = PfGetTimestamp();
        real32 t5 = PfGetSeconds(s5, s6);
        
        //DEBUG_LOG("%f,%f,%f,%f,%f,%f\n", t0 * 1000.0f, t1 * 1000.0f, t2 * 1000.0f, t3 * 1000.0f, t4 * 1000.0f, t5 * 1000.0f); 
        
        PfTimestamp end = PfGetTimestamp();
        uint64 endCycles = PfRdtsc();
        
        real32 secondsPerFrame = PfGetSeconds(start, end);
        real32 cyclesPerFrame = real32(endCycles - startCycles);
        real32 framesPerSecond = 1.0f/secondsPerFrame;
        
        startCycles = endCycles;
        start = end;
        
        char temp[256];
        sprintf(temp, "[%.2fms %.2FPS %.3fMHz %.3fms sleep] [RECORDING: %d PLAYING:%d]", secondsPerFrame * 1000.0f, 1.0f/secondsPerFrame, cyclesPerFrame/(secondsPerFrame * 1000.0f * 1000.0f), t5*1000.0f, recording, playing);
        PfSetWindowTitle(&window[0], temp);
        
    }
    
#if PLATFORM_LINUX
    XCloseDisplay(globalDisplay);
#endif
    //DEBUG_LOG("Exiting application.\n");
    return 0;
}
#endif