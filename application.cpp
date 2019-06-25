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
#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_image.h"
#include "stb_truetype.h"




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

void FreeAllChunks(ChunkAllocator *ca)
{
    InitializeChunkAllocator(ca);
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
global_variable Memory globalTemporaryStorage = {};


#define PushStruct(memory, type) (type*)(PushSize(memory, sizeof(type)))
#define PushArray(memory, type, count) (type*)(PushSize(memory, sizeof(type) * count))
uint8* PushSize(Memory *memory, sizet required)
{
    ASSERT(memory->used + required <= memory->size, "Memory overflow");
    uint8* result = memory->base + memory->used;
    memory->used += required;
    return result;
}


char* ReadEntireFileAndNullTerminate(char *filepath, Memory *mem)
{
    uint64 fileSize = PfGetFileSize(filepath);
    char *fileData = (char *)PushSize(mem, fileSize + 1);
    int64 bytesRead = PfReadEntireFile(filepath, fileData);
    ASSERT(bytesRead == (int64)fileSize, "Did not read all bytes");
    fileData[fileSize] = 0;
    return fileData;
}


char* ReadEntireFile(char *filepath, Memory *mem)
{
    uint64 fileSize = PfGetFileSize(filepath);
    char *fileData = (char *)PushSize(mem, fileSize);
    int64 bytesRead = PfReadEntireFile(filepath, fileData);
    ASSERT(bytesRead == (int64)fileSize, "Did not read all bytes");
    return fileData;
}

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


struct TextureInfo 
{
    uint32 textureId;
    int32 width;
    int32 height;
    int32 numChannels;
};

struct Spritesheet
{
    TextureInfo textureInfo;
    int32 numFramesAlongWidth;
    int32 numFramesAlongHeight;
    int32 numFrames;
    real32 frameWidth;
    real32 frameHeight;
    real32 normalizedFrameWidth;
    real32 normalizedFrameHeight;
    uint32 vao;
};

struct AnimationClip
{
    Spritesheet *spritesheet;
    v3 pos;
    real32 msPerFrame;
    real32 timeSpentOnThisFrame;
    int32 startFrameIndex;
    int32 onePastEndFrameIndex;
    int32 currentFrameIndex;
    int32 stride;
    AnimationClip *next;
};

struct HitpointDisplay
{
    v3 pos;
    uint64 value;
    real32 durationSpent;
    HitpointDisplay *next;
};

struct GameState
{
    v2 spaceshipDim;
    v3 spaceshipPos;
    v3 spaceshipVel;
    real32 safetyRadius;
    real32 spaceshipRotation;
    
    v3 debrisPos;
    v3 debrisVel;
    
    ChunkAllocator bulletAllocator;
    Bullet *bullets;
    real32 firingCoolDown;
    
    ChunkAllocator asteroidAllocator;
    Asteroid *asteroids;
    real32 asteroidSpawnClock;
    real32 asteroidSpawnDelay;
    int32 asteroidsCount;
    int32 desiredAsteroidsCount;
    
    ChunkAllocator playingSoundAllocator;
    uint32 playingSoundNextId;
    PlayingSound *soundsPlaying;
    
    ChunkAllocator animationClipsAllocator;
    AnimationClip *animationClips;
    
    ChunkAllocator hitpointDisplayAllocator;
    HitpointDisplay *hitpointDisplays;
    
    real32 spaceshipRadius;
    real32 bulletRadius;
    real32 asteroidRadius;
    
    int32 playBackSlot;
    int32 recordSlot;
    int32 totalSlots;
    
    bool paused;
    int32 livesRemaining;
    uint64 score;
};


void PlayAnimation(GameState *gameState, Spritesheet *spritesheet, v3 pos, real32 msPerFrame, int32 startFrameIndex, int32 onePastEndFrameIndex, int32 stride)
{
    AnimationClip *clip = (AnimationClip *)AllocateChunk(&gameState->animationClipsAllocator);
    clip->spritesheet = spritesheet;
    clip->pos = pos;
    clip->msPerFrame = msPerFrame;
    clip->timeSpentOnThisFrame = 0.0f;
    clip->startFrameIndex = startFrameIndex;
    clip->onePastEndFrameIndex = onePastEndFrameIndex;
    clip->currentFrameIndex = startFrameIndex;
    clip->stride = stride;
    
    clip->next = gameState->animationClips;
    gameState->animationClips = clip;
}


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


struct HotCodeRelaunchPersistentData
{
    int32 playBackSlot;
};

TextureInfo OpenGLGenTexture(char *imagePath, GLuint wrapping = GL_CLAMP_TO_BORDER)
{
    // NOTE(KARAN): Only supports RGBA8
    TextureInfo result = {};
    
    stbi_set_flip_vertically_on_load(true);
    int imageWidth, imageHeight, imageChannels;
    unsigned char *imageData = stbi_load(imagePath, &imageWidth, &imageHeight, &imageChannels, 4);
    //ASSERT(imageChannels == 4, "Only supports RGBA textures");
    ASSERT(imageData, "Couldn't load image");
    
    uint32 textureId;
    GL_CALL(glActiveTexture(GL_TEXTURE0));
    GL_CALL(glGenTextures(1, &textureId));
    GL_CALL(glBindTexture(GL_TEXTURE_2D, textureId));
    GL_CALL(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, imageWidth, imageHeight, 0,
                         GL_RGBA, GL_UNSIGNED_BYTE, imageData));
    
    GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
    GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
    GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrapping));
    GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrapping));
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

uint32 OpenGLGenVAOForQuadrilateralCenteredAtOrigin(int32 width, int32 height, v2 textureOrigin = {0.0f, 0.0f}, v2 textureDim = {1.0f, 1.0f})
{
    uint32 result = 0;
    
    v2 max = {(real32)width/2.0f, (real32)height/2.0f};
    v2 min = {-((real32)width/2.0f), -((real32)height/2.0f)};
    
    real32 vertices[] = 
    {
        // positions          // texture coords
        max.x, max.y, 0.0f,   (textureOrigin.x + textureDim.x), (textureOrigin.y + textureDim.y), // top right
        max.x, min.y, 0.0f,   (textureOrigin.x + textureDim.x), textureOrigin.y, // bottom right
        min.x, min.y, 0.0f,   textureOrigin.x, textureOrigin.y, // bottom left
        min.x, max.y, 0.0f,   textureOrigin.x, (textureOrigin.y + textureDim.y)  // top left 
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


void OpenGLSetUniform2f(char *uniformName, v2 val, uint32 programId)
{
    GL_CALL(GLint uniformLocation = glGetUniformLocation(programId, uniformName));
    GL_CALL(glUniform2f(uniformLocation, val.x, val.y)); 
}


void OpenGLSetUniform4f(char *uniformName, v4 val, uint32 programId)
{
    GL_CALL(GLint uniformLocation = glGetUniformLocation(programId, uniformName));
    GL_CALL(glUniform4f(uniformLocation, val.x, val.y, val.w, val.h)); 
}

void OpenGLSetUniform1i(char *uniformName, int32 val, uint32 programId)
{
    GL_CALL(GLint uniformLocation = glGetUniformLocation(programId, uniformName));
    GL_CALL(glUniform1i(uniformLocation, val)); 
}


void OpenGLSetUniform4fv(char *uniformName, real32 *val, uint32 programId)
{
    GL_CALL(GLint uniformLocation = glGetUniformLocation(programId, uniformName));
    GL_CALL(glUniformMatrix4fv(uniformLocation, 1, GL_TRUE, val)); 
}

enum shader_type
{
    basic_shader,
    single_channel_shader,
    solid_color_shader
};


struct ShaderInputs
{
    shader_type type;
    uint32 textureId;
    uint32 vao;
    uint32 programId;
    int32 sampler;
    v2 textureOffset;
    mat4 rotationMatAboutZAxis;
    mat4 translationMat;
    mat4 orthoProjection;
    v4 color;
};



void OpenGLSetShaderUniforms(ShaderInputs *input)
{
    if(input->type == basic_shader || input->type == single_channel_shader || input->type == solid_color_shader)
    {
        OpenGLSetUniform4fv("rotationMatAboutZAxis", input->rotationMatAboutZAxis.data, input->programId); 
        OpenGLSetUniform4fv("translationMat", input->translationMat.data, input->programId);
        OpenGLSetUniform4fv("orthoProjection", input->orthoProjection.data, input->programId); 
    }
    if(input->type == basic_shader || input->type == single_channel_shader)
    {
        OpenGLSetUniform2f("textureOffset", input->textureOffset, input->programId);
        OpenGLSetUniform1i("texture1", input->sampler, input->programId);
    }
    if(input->type == single_channel_shader || input->type == solid_color_shader)
    {
        OpenGLSetUniform4f("color", input->color, input->programId);
    }
    
}

void OpenGLBind_Texture_VAO_Program(ShaderInputs *shaderInputs)
{
    GL_CALL(glActiveTexture(GL_TEXTURE0 + shaderInputs->sampler));
    GL_CALL(glBindTexture(GL_TEXTURE_2D, shaderInputs->textureId));
    GL_CALL(glBindVertexArray(shaderInputs->vao));
    GL_CALL(glUseProgram(shaderInputs->programId));
}

void OpenGLUnbind_Texture_VAO_Program()
{
    GL_CALL(glBindTexture(GL_TEXTURE_2D, 0));
    GL_CALL(glBindVertexArray(0));
    GL_CALL(glUseProgram(0));
}



struct FontInfo
{
    int32 bitmapWidth;
    int32 bitmapHeight;
    int32 bytesPerPixel;
    real32 scale;
    int32 firstCodepoint;
    int32 numCodepoints;
    stbtt_packedchar *charData;
    
    uint32 textureId;
    uint32 vao;
    uint32 vbo;
    uint32 ebo;
};

void CreateFont(FontInfo *font, char* filepath, int32 firstCodepoint, int32 numCodepoints, real32 scale, Memory *storage)
{
    font->bitmapWidth = 512;
    font->scale = scale;
    font->bitmapHeight = 512;
    font->bytesPerPixel = 1;
    font->firstCodepoint = 32;
    font->numCodepoints = 95;
    font->charData = (stbtt_packedchar *)PushArray(storage, stbtt_packedchar, font->numCodepoints);
    
    Memory restorePoint = globalTemporaryStorage;
    uint8* fontFileData = (uint8*)ReadEntireFile(filepath, &globalTemporaryStorage); 
    int32 fontBitmapPitch = font->bytesPerPixel * font->bitmapWidth;
    uint8* fontBitmap = (uint8 *)PushSize(&globalTemporaryStorage, font->bitmapHeight * font->bitmapWidth * font->bytesPerPixel); 
    
    stbtt_pack_context packContext;
    real32 fontScale = 80;
    int32 ret = stbtt_PackBegin(&packContext, fontBitmap, font->bitmapWidth, font->bitmapHeight, 0, 1, 0);
    ASSERT(ret == 1 , "");
    ret = stbtt_PackFontRange(&packContext, fontFileData, 0, font->scale, font->firstCodepoint, font->numCodepoints, font->charData);
    ASSERT(ret == 1 , "");
    stbtt_PackEnd(&packContext);
    
    GL_CALL(glGenTextures(1, &font->textureId));
    GL_CALL(glBindTexture(GL_TEXTURE_2D, font->textureId));
    GL_CALL(glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, font->bitmapWidth, font->bitmapHeight, 0, GL_RED, GL_UNSIGNED_BYTE, fontBitmap));
    GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
    
    globalTemporaryStorage = restorePoint;
    
    real32 vertices[20] = {};
    uint32 quadIndices[] = 
    {
        0, 1, 3, // first triangle
        1, 2, 3  // second triangle
    };
    
    GL_CALL(glGenVertexArrays(1, &font->vao));
    GL_CALL(glGenBuffers(1, &font->vbo));
    GL_CALL(glGenBuffers(1, &font->ebo));
    GL_CALL(glBindVertexArray(font->vao));
    GL_CALL(glBindBuffer(GL_ARRAY_BUFFER, font->vbo));
    GL_CALL(glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW));
    GL_CALL(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, font->ebo));
    GL_CALL(glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(quadIndices), quadIndices, GL_STATIC_DRAW));
    GL_CALL(glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(real32), (void*)0));
    GL_CALL(glEnableVertexAttribArray(0));
    GL_CALL(glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(real32), (void*)(3 * sizeof(real32))));
    GL_CALL(glEnableVertexAttribArray(1));
    GL_CALL(glBindVertexArray(0));
    GL_CALL(glBindBuffer(GL_ARRAY_BUFFER, 0));
    GL_CALL(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0));
}


v2 GetTextBounds(char* text, FontInfo *font)
{
    v2 bounds = {};
    v2 basepoint = {0.0f, 0.0f};
    real32 x = basepoint.x;
    real32 y = basepoint.y;
    for(int32 i = 0; text[i] != 0; i++)
    {
        stbtt_aligned_quad q;
        int32 relativeIndex = text[i] - font->firstCodepoint;
        stbtt_packedchar a = font->charData[relativeIndex];
        
        if(i == 0) x -= a.xoff;
        
        real32 tempX = x;
        real32 tempY = y;
        stbtt_GetPackedQuad(font->charData, font->bitmapWidth, font->bitmapHeight, relativeIndex, &tempX, &tempY, &q, 0);
        
        real32 fontCharHeight = q.y1 - q.y0;
        if(bounds.y < fontCharHeight)
        {
            bounds.y = fontCharHeight;
        }
        
        x = tempX;
        y = tempY;
    }
    
    bounds.x = x;
    return bounds;
}

void OpenGLDrawTextAt(v2 basepoint, v2 screenDim, char *text, v4 color, FontInfo *font, uint32 programId)
{
    ShaderInputs shaderInputs = {};
    
    shaderInputs.orthoProjection.data[0] = 2.0f/screenDim.x;
    shaderInputs.orthoProjection.data[3] = -1.0f;
    shaderInputs.orthoProjection.data[5] = 2.0f/screenDim.y;
    shaderInputs.orthoProjection.data[7] = -1.0f;
    shaderInputs.orthoProjection.data[10] = 1.0f;
    shaderInputs.orthoProjection.data[15] = 1.0f;
    
    
    shaderInputs.type = single_channel_shader;
    shaderInputs.textureId = font->textureId;
    shaderInputs.vao = font->vao;
    shaderInputs.programId = programId;
    
    real32 x = basepoint.x;
    real32 y = basepoint.y;
    
    OpenGLBind_Texture_VAO_Program(&shaderInputs);
    for(int32 i = 0; text[i] != 0; i++)
    {
        stbtt_aligned_quad q;
        int32 relativeIndex = text[i] - font->firstCodepoint;
        stbtt_packedchar a = font->charData[relativeIndex];
        
        if(i == 0) x -= a.xoff;
        
        real32 tempX = x;
        real32 tempY = y;
        stbtt_GetPackedQuad(font->charData, font->bitmapWidth, font->bitmapHeight, relativeIndex, &tempX, &tempY, &q, 0);
        
        real32 newVertices[] = 
        {
            q.x0,q.y0, 0.0f, q.s0,q.t1,
            q.x1,q.y0, 0.0f, q.s1,q.t1,
            q.x1,q.y1, 0.0f, q.s1,q.t0,
            q.x0,q.y1, 0.0f, q.s0,q.t0
        };
        
        GL_CALL(glBindBuffer(GL_ARRAY_BUFFER, font->vbo));
        GL_CALL(glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(newVertices), newVertices));
        
        Identity(&(shaderInputs.rotationMatAboutZAxis));
        TranslationMat(&(shaderInputs.translationMat), {0.0F, -a.yoff});
        shaderInputs.textureOffset = {};
        shaderInputs.color = color;
        OpenGLSetShaderUniforms(&shaderInputs);
        GL_CALL(glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0));
        
        x = tempX;
        y = tempY;
    }
    
    OpenGLUnbind_Texture_VAO_Program();
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




void OpenGLDrawRectangle(v4 rect, v2 screenDim, v4 color, uint32 programId, real32 rotateDegrees = 0.0f)
{
    
    static bool firstCall = true;
    static uint32 vao;
    static uint32 vbo;
    static uint32 ebo;
    
    real32 width = rect.w;
    real32 height = rect.h;
    
    v2 max = {width/2.0f, height/2.0f};
    v2 min = {-(width/2.0f), -(height/2.0f)};
    
    real32 vertices[] = 
    {
        // positions          
        max.x, max.y, 0.0f,   // top right
        max.x, min.y, 0.0f,   // bottom right
        min.x, min.y, 0.0f,    // bottom left
        min.x, max.y, 0.0,    // top left 
    };
    
    
    if(firstCall)
    {
        firstCall = false;
        uint32 quadIndices[] = 
        {
            0, 1, 3, // first triangle
            1, 2, 3  // second triangle
        };
        
        GL_CALL(glGenVertexArrays(1, &vao));
        GL_CALL(glGenBuffers(1, &vbo));
        GL_CALL(glGenBuffers(1, &ebo));
        GL_CALL(glBindVertexArray(vao));
        GL_CALL(glBindBuffer(GL_ARRAY_BUFFER, vbo));
        GL_CALL(glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW));
        GL_CALL(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo));
        GL_CALL(glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(quadIndices), quadIndices, GL_STATIC_DRAW));
        GL_CALL(glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0));
        GL_CALL(glEnableVertexAttribArray(0));
    }
    else
    {
        GL_CALL(glBindBuffer(GL_ARRAY_BUFFER, vbo));
        GL_CALL(glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices));
    }
    
    
    ShaderInputs shaderInputs = {};
    shaderInputs.type = solid_color_shader;
    shaderInputs.vao = vao;
    shaderInputs.programId = programId;
    RotationAboutZAxis(&(shaderInputs.rotationMatAboutZAxis), rotateDegrees);
    TranslationMat(&shaderInputs.translationMat, {rect.x + max.x, rect.y + max.y});
    
    shaderInputs.orthoProjection.data[0] = 2.0f/screenDim.x;
    shaderInputs.orthoProjection.data[3] = -1.0f;
    shaderInputs.orthoProjection.data[5] = 2.0f/screenDim.y;
    shaderInputs.orthoProjection.data[7] = -1.0f;
    shaderInputs.orthoProjection.data[10] = 1.0f;
    shaderInputs.orthoProjection.data[15] = 1.0f;
    
    shaderInputs.color = color;
    
    OpenGLBind_Texture_VAO_Program(&shaderInputs);
    OpenGLSetShaderUniforms(&shaderInputs);
    GL_CALL(glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0));
    OpenGLUnbind_Texture_VAO_Program();
}


void OpenGLDrawRectangle(v2 pt1, v2 pt2, v2 screenDim, v4 color, uint32 programId, real32 rotateDegrees = 0.0f)
{
    v4 rect = {pt1.x, pt1.y, pt2.x - pt1.x, pt2.y - pt1.y};
    OpenGLDrawRectangle(rect, screenDim, color, programId, rotateDegrees);
}


void CreateSpritesheet(char *imagePath, Spritesheet *spritesheet, int32 numFramesAlongWidth, int32 numFramesAlongHeight)
{
    spritesheet->textureInfo = OpenGLGenTexture(imagePath);
    spritesheet->numFramesAlongWidth = numFramesAlongWidth;
    spritesheet->numFramesAlongHeight = numFramesAlongHeight;
    spritesheet->numFrames = spritesheet->numFramesAlongWidth * spritesheet->numFramesAlongHeight;
    spritesheet->frameWidth = (real32)spritesheet->textureInfo.width/(real32)spritesheet->numFramesAlongWidth;
    spritesheet->frameHeight = (real32)spritesheet->textureInfo.height/(real32)spritesheet->numFramesAlongHeight;
    spritesheet->normalizedFrameWidth = spritesheet->frameWidth/(real32)spritesheet->textureInfo.width;
    spritesheet->normalizedFrameHeight = spritesheet->frameHeight/(real32)spritesheet->textureInfo.height;
    spritesheet->vao = OpenGLGenVAOForQuadrilateralCenteredAtOrigin((int32)spritesheet->frameWidth, (int32)spritesheet->frameHeight, {0.0f, 0.0f}, {spritesheet->normalizedFrameWidth, spritesheet->normalizedFrameHeight});
}


void DrawRectangle(uint8 *offscreenMemory, int width, int height, uint32 pitch,real32 rectMinX, real32 rectMinY, real32 rectMaxX, real32 rectMaxY, uint32 color)
{
    
    int32 minX = RoundReal32ToInt32(rectMinX);
    int32 minY = RoundReal32ToInt32(rectMinY);
    int32 maxX = RoundReal32ToInt32(rectMaxX);
    int32 maxY = RoundReal32ToInt32(rectMaxY);
    
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


int main(int argc, char **argv)
{
    v2 gameResolution = {1280.0f, 720.0f};
    
    real32 fps = 60.0f;
    real32 targetMillisecondsPerFrame = (1000.0f/fps);
    real32 dT = 1.0f/fps;
    
    PfInitialize();
    PfGLConfig(4, 3, true);
    
    PfWindow window[1] = {};
    PfCreateWindow(&window[0], (char*)"WINDOW 0", 0, 0, (int32)gameResolution.x, (int32)gameResolution.y);
    PfRect clientRect = PfGetClientRect(&window[0]);
    
    int32 playBackSlot = 0;
    bool playingRestart = false;
    bool hotCodeReloaded = false;
    
    Memory gameStateStorage = {};
    Memory assetStorage = {};
    
    HotCodeRelaunchPersistentData hcrData = {};
#if HOT_CODE_RELOADABLE
    if(argc >= 2 && AreStringsSame(argv[1], "hcr_reloaded") && PfFilepathExists("hot_code_relaunch_persistent_data"))
    {
        PfReadEntireFile("hot_code_relaunch_persistent_data", (void *)&hcrData);
        playBackSlot = hcrData.playBackSlot;
        playingRestart = true;
        hotCodeReloaded = true;
    }
#endif
    gameStateStorage.size = MEGABYTES((uint64)8);
    assetStorage.size = MEGABYTES((uint64)64);
    globalTemporaryStorage.size = MEGABYTES((uint64)16);
    
    void *baseAddressForGameMemoryStorage = (void*)0;
#if DEBUG_BUILD | HOT_CODE_RELOADABLE | INPUT_RECORDING_PLAYBACK
    baseAddressForGameMemoryStorage = (void*)TERABYTES((uint64)4);
#endif
    gameStateStorage.base = (uint8*)PfVirtualAlloc(baseAddressForGameMemoryStorage,(size_t)(gameStateStorage.size + assetStorage.size + globalTemporaryStorage.size));
    assetStorage.base = gameStateStorage.base + gameStateStorage.size;
    globalTemporaryStorage.base = assetStorage.base + assetStorage.size;
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
    
    gameState->bulletAllocator.length = 15;
    gameState->bulletAllocator.chunkSize = sizeof(Bullet);
    gameState->bulletAllocator.base = (uint8 *)PushArray(&gameStateStorage, Bullet, gameState->bulletAllocator.length);
    InitializeChunkAllocator(&(gameState->bulletAllocator));
    
    gameState->asteroidAllocator.length = 10;
    gameState->asteroidAllocator.chunkSize = sizeof(Asteroid);
    gameState->asteroidAllocator.base = (uint8 *)PushArray(&gameStateStorage, Asteroid, gameState->asteroidAllocator.length);
    InitializeChunkAllocator(&(gameState->asteroidAllocator));
    
    gameState->playingSoundAllocator.length = 30;
    gameState->playingSoundNextId = 1;
    gameState->playingSoundAllocator.chunkSize = sizeof(PlayingSound);
    gameState->playingSoundAllocator.base = (uint8 *)PushArray(&gameStateStorage, PlayingSound, gameState->playingSoundAllocator.length);
    InitializeChunkAllocator(&(gameState->playingSoundAllocator));
    
    gameState->animationClipsAllocator.length = 15;
    gameState->animationClipsAllocator.chunkSize = sizeof(AnimationClip);
    gameState->animationClipsAllocator.base = (uint8 *)PushArray(&gameStateStorage, AnimationClip, gameState->animationClipsAllocator.length);
    InitializeChunkAllocator(&(gameState->animationClipsAllocator));
    
    gameState->hitpointDisplayAllocator.length = 15;
    gameState->hitpointDisplayAllocator.chunkSize = sizeof(HitpointDisplay);
    gameState->hitpointDisplayAllocator.base = (uint8 *)PushArray(&gameStateStorage, HitpointDisplay, gameState->hitpointDisplayAllocator.length);
    InitializeChunkAllocator(&(gameState->hitpointDisplayAllocator));
    
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
        sound->id = i + 1;
        uint32 soundFileSize = (uint32)PfGetFileSize(filepaths[i]);
        sound->data = (int16*)ReadEntireFile(filepaths[i], &assetStorage);
        sound->sampleCount = soundFileSize/(bytesPerFrame/numChannels);
    }
    
    uint64 soundMixingBufferLength = requestedBufferFrames * numChannels;
    real32 *soundMixingBuffer = (real32*)PushArray(&assetStorage, real32, soundMixingBufferLength);
    
    PlayingSoundId backgroundPlayingSoundID = PlaySound(gameState, backgroundSound, true, 0.9f, 0.9f);
    PlayingSoundId thrustPlayingSoundId = {}; 
    
    GameInput input = {};
    int64 inputFileHandle = -1;
    
    gameState->spaceshipPos = {gameResolution.x/2.0f, gameResolution.y/2.0f, 0.0f};
    gameState->paused = true;
    gameState->livesRemaining = 3;
    gameState->score = 0;
    gameState->asteroidSpawnClock = 0.0f;
    gameState->asteroidSpawnDelay = 0.5f;
    gameState->asteroidsCount = 0;
    gameState->desiredAsteroidsCount = 7;
    gameState->debrisPos = {gameResolution.x/2.0f, gameResolution.y/2.0f, 0.0f};
    gameState->spaceshipVel = {};
    gameState->debrisVel = {20.0f, 0.0f, 0.0f};
    gameState->spaceshipRotation = 0.0f;
    gameState->bulletRadius = 3.0f;
    gameState->spaceshipRadius = 35.0f;
    gameState->safetyRadius = gameState->spaceshipRadius * 3.0f;
    gameState->asteroidRadius = 40.0f;
    gameState->playBackSlot = playBackSlot;
    gameState->totalSlots = 10;
    bool recording = false;
    bool playing = false;
    
    PfglMakeCurrent(&window[0]);
    
    TextureInfo spaceshipTextureInfo = OpenGLGenTexture("../data/images/double_ship.png");
    TextureInfo nebulaTextureInfo = OpenGLGenTexture("../data/images/nebula_brown.png");
    TextureInfo debrisTextureInfo = OpenGLGenTexture("../data/images/debris_blend.png", GL_REPEAT);
    TextureInfo bulletTextureInfo = OpenGLGenTexture("../data/images/shot1.png");
    TextureInfo asteroidTextureInfo = OpenGLGenTexture("../data/images/asteroid_blend.png");
    
    gameState->spaceshipDim = {(real32)spaceshipTextureInfo.width/2.0f, (real32)spaceshipTextureInfo.height};
    
    uint32 spaceshipVao = OpenGLGenVAOForQuadrilateralCenteredAtOrigin((int32)gameState->spaceshipDim.x, (int32)gameState->spaceshipDim.y, {0.0f, 0.0f}, {0.5f, 1.0f});
    uint32 bulletVao = OpenGLGenVAOForQuadrilateralCenteredAtOrigin(bulletTextureInfo.width, bulletTextureInfo.height);
    uint32 asteroidVao = OpenGLGenVAOForQuadrilateralCenteredAtOrigin(asteroidTextureInfo.width, asteroidTextureInfo.height);
    uint32 nebulaVao = OpenGLGenVAOForQuadrilateralCenteredAtOrigin((int32)gameResolution.x, (int32)gameResolution.y);
    real32 factor = gameResolution.x * 2.0f/(real32)debrisTextureInfo.width;
    real32 factor2 = gameResolution.y/(real32)debrisTextureInfo.height;
    uint32 debrisVao = OpenGLGenVAOForQuadrilateralCenteredAtOrigin((int32)(debrisTextureInfo.width * factor), (int32)(debrisTextureInfo.height * factor2), {0.0f, 0.0f}, {factor, factor2});
    
    Memory restorePoint = globalTemporaryStorage;
    
    char *vertexShaderSource = ReadEntireFileAndNullTerminate("../basic_vertex_shader.glsl", &globalTemporaryStorage);
    char *vertexShaderSource2 = ReadEntireFileAndNullTerminate("../solid_color_vertex_shader.glsl", &globalTemporaryStorage);
    char *fragmentShaderSource = ReadEntireFileAndNullTerminate("../basic_fragment_shader.glsl", &globalTemporaryStorage);
    char *fragmentShaderSource2 = ReadEntireFileAndNullTerminate("../single_channel_fragment_shader.glsl", &globalTemporaryStorage);
    char *fragmentShaderSource3 = ReadEntireFileAndNullTerminate("../solid_color_fragment_shader.glsl", &globalTemporaryStorage);
    
    uint32 programId = OpenGLGenProgramId(vertexShaderSource, fragmentShaderSource);
    uint32 program2Id = OpenGLGenProgramId(vertexShaderSource, fragmentShaderSource2);
    uint32 program3Id = OpenGLGenProgramId(vertexShaderSource2, fragmentShaderSource3);
    
    globalTemporaryStorage = restorePoint;
    
    //Fonts
    FontInfo *font = PushStruct(&assetStorage, FontInfo);
    CreateFont(font, "../data/fonts/pixel_font.ttf", 32, 95, 50, &assetStorage);
    
    FontInfo *font2 = PushStruct(&assetStorage, FontInfo);
    CreateFont(font2, "../data/fonts/pixel_font.ttf", 32, 95, 32, &assetStorage);
    
    FontInfo *font3 = PushStruct(&assetStorage, FontInfo);
    CreateFont(font3, "../data/fonts/pixel_font.ttf", 32, 95, 28, &assetStorage);
    
    // Sprite animation
    Spritesheet *explosionSpritesheet = PushStruct(&gameStateStorage, Spritesheet);
    CreateSpritesheet("../data/images/explosion_orange.png", explosionSpritesheet, 24, 1);
    
    Spritesheet *explosionSpritesheet2 = PushStruct(&gameStateStorage, Spritesheet);
    CreateSpritesheet("../data/images/explosion_alpha.png", explosionSpritesheet2, 24, 1);
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
            input.fire = PfGetKeyState(&window[0], PF_K);
#if INPUT_RECORDING_PLAYBACK
        }
        if(recording)
        {
            PfWriteFile(inputFileHandle, (void *)&input, sizeof(input));
        }
#endif
        
        if(gameState->paused)
        {
            if(PfGetKeyState(&window[0], PF_SPACEBAR, true))
            {
                gameState->paused = false;
                gameState->livesRemaining = 3;
                gameState->score = 0;
            }
            
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
        
        PfTimestamp s1 = PfGetTimestamp();
        real32 t0 = PfGetSeconds(s0, s1);
        
        if(input.left) gameState->spaceshipRotation += 5.0f; 
        if(input.right) gameState->spaceshipRotation -= 5.0f;
        
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
        WrapAroundIfOutOfBounds((v2*)(&(gameState->spaceshipPos)), {0.0f, 0.0f, gameResolution.x, gameResolution.y});
        
        gameState->debrisPos = gameState->debrisPos + gameState->debrisVel*dT;
        WrapAroundIfOutOfBounds((v2*)(&(gameState->debrisPos)), {0.0f, 0.0f, gameResolution.x, gameResolution.y});
        
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
                WrapAroundIfOutOfBounds((v2*)(&(bullet->pos)), {0.0f, 0.0f, gameResolution.x, gameResolution.y});
                previous = bullet;
                bullet = bullet->next;
            }
        }
        
        if(!gameState->paused)
        {
            gameState->asteroidSpawnClock += dT;
            while(gameState->asteroidSpawnClock >= gameState->asteroidSpawnDelay)
            {
                gameState->asteroidSpawnClock -= gameState->asteroidSpawnDelay;
                if(gameState->asteroidsCount < gameState->desiredAsteroidsCount)
                {
                    Asteroid *asteroid = (Asteroid*)AllocateChunk(&(gameState->asteroidAllocator));
                    if(asteroid)
                    {
                        real32 randomSign = rand() % 2 == 0 ? 1.0f : -1.0f;
                        real32 randomAngle = gameState->spaceshipRotation + Rand(45, 315);
                        v2 dir = {Cosine(randomAngle * DEG_TO_RAD), Sine(randomAngle * DEG_TO_RAD)};
                        
                        v2 distanceToWalls = {INFINITY, INFINITY};
                        bool setDistanceToAtleastOneWall = false;
                        if(dir.y != 0)
                        {
                            v2 b = {0.0f, gameResolution.y};
                            v2 c = {gameState->spaceshipPos.y, gameState->spaceshipPos.y};
                            v2 a = (b - c)/dir.y;
                            ASSERT(a.x * a.y < 0.0f, "Not possible to reach both ends of screen when moving along a dir");
                            distanceToWalls.y = a.x > 0.0f ? a.x : a.y;
                            setDistanceToAtleastOneWall = true;
                        }
                        
                        if(dir.x != 0)
                        {
                            v2 b = {0.0f, gameResolution.x};
                            v2 c = {gameState->spaceshipPos.x, gameState->spaceshipPos.x};
                            v2 a = (b - c)/dir.x;
                            ASSERT(a.x * a.y < 0.0f, "Not possible to reach both ends of screen when moving along a dir");
                            distanceToWalls.x = a.x > 0.0f ? a.x : a.y;
                            setDistanceToAtleastOneWall = true;
                        }
                        
                        ASSERT(setDistanceToAtleastOneWall, "Dir might be 0 as we did not reach any wall along the dir");
                        real32 distanceToNearestWall = distanceToWalls.x < distanceToWalls.y ? distanceToWalls.x : distanceToWalls.y;
                        real32 randomDistance;
                        if(distanceToNearestWall < gameState->safetyRadius)
                        {
                            randomDistance = gameState->safetyRadius;
                        }
                        else
                        {
                            randomDistance = Rand((int32)gameState->safetyRadius, (int32)distanceToNearestWall);
                        }
                        
                        v3 dir3 = {dir.x, dir.y ,0.0f};
                        asteroid->pos = gameState->spaceshipPos + (dir3 * randomDistance);
                        asteroid->vel = dir3 * randomSign * Rand(50, 100) ;
                        asteroid->rotation = 0.0f;
                        asteroid->rotationalVelocity = Rand(50, 500) * randomSign;
                        asteroid->next = gameState->asteroids;
                        gameState->asteroids = asteroid;
                        gameState->asteroidsCount++;
                    }
                }
            }
        }
        
        Asteroid *asteroid = gameState->asteroids;
        while(asteroid)
        {
            asteroid->pos = asteroid->pos + (asteroid->vel * dT);
            asteroid->rotation = asteroid->rotation + (asteroid->rotationalVelocity * dT);
            WrapAroundIfOutOfBounds((v2*)(&(asteroid->pos)), {0.0f, 0.0f, gameResolution.x, gameResolution.y});
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
            int32 asteroidsDestroyed = 0;
            uint64 pointsGained = 0;
            while(asteroid)
            {
                v2 asteroidPos = {asteroid->pos.x, asteroid->pos.y};
                bool collidedWithBullet = CircleWithCircleCollisionTest(bulletPos, gameState->bulletRadius, asteroidPos, gameState->asteroidRadius);
                if(collidedWithBullet)
                {
                    deleteThisBullet = true;
                    v3 explosionPos = asteroid->pos;
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
                    gameState->asteroidsCount--;
                    PlaySound(gameState, boomSound, false, 0.1f, 0.1f);
                    PlayAnimation(gameState, explosionSpritesheet, explosionPos, 16.0f, 0, 24, 1);
                    asteroidsDestroyed++;
                }
                else
                {
                    previousAsteroid = asteroid;
                    asteroid = asteroid->next;
                }
            }
            
            pointsGained = asteroidsDestroyed * 10;
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
                
                HitpointDisplay *hitpointDisplay = (HitpointDisplay *)AllocateChunk(&gameState->hitpointDisplayAllocator);
                if(hitpointDisplay)
                {
                    hitpointDisplay->pos = {bulletPos.x, bulletPos.y, 0.0f};
                    hitpointDisplay->value = pointsGained;
                    hitpointDisplay->durationSpent = 0.0f;
                    
                    hitpointDisplay->next = gameState->hitpointDisplays;
                    gameState->hitpointDisplays = hitpointDisplay;
                }
            }
            else
            {
                previousBullet = bullet;
                bullet = bullet->next;
            }
            
            
            gameState->score += pointsGained;
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
                v3 explosion1Pos = asteroid->pos;
                v3 explosion2Pos = gameState->spaceshipPos;
                
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
                
                gameState->asteroidsCount--;
                gameState->livesRemaining--;
                if(gameState->livesRemaining <= 0) 
                {
                    gameState->livesRemaining = 0; 
                    gameState->paused = true;
                }
                
                PlaySound(gameState, boomSound, false, 0.1f, 0.1f);
                PlayAnimation(gameState, explosionSpritesheet, explosion1Pos, 16.0f, 0, 24, 1);
                PlayAnimation(gameState, explosionSpritesheet2, explosion2Pos, 16.0f, 0, 24, 1);
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
        PfglMakeCurrent(&window[0]);
        GL_CALL(glClearColor(1.0f, 0.0f, 0.0f, 1.0f));
        GL_CALL(glClear(GL_COLOR_BUFFER_BIT));
        
#if 1
        GL_CALL(GLboolean wasBlendEnabled = glIsEnabled(GL_BLEND));
        GL_CALL(glEnable(GL_BLEND));
        GL_CALL(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));
        
        // Draw nebula
        ShaderInputs shaderInputs = {};
        shaderInputs.type = basic_shader;
        shaderInputs.textureId = nebulaTextureInfo.textureId;
        shaderInputs.vao = nebulaVao;
        shaderInputs.programId = programId;
        shaderInputs.orthoProjection.data[0] = 2.0f/gameResolution.x;
        shaderInputs.orthoProjection.data[3] = -1.0f;
        shaderInputs.orthoProjection.data[5] = 2.0f/gameResolution.y;
        shaderInputs.orthoProjection.data[7] = -1.0f;
        shaderInputs.orthoProjection.data[10] = 1.0f;
        shaderInputs.orthoProjection.data[15] = 1.0f;
        Identity(&(shaderInputs.rotationMatAboutZAxis));
        TranslationMat(&(shaderInputs.translationMat), {gameResolution.x/2.0f, gameResolution.y/2.0f, 0.0f});
        
        OpenGLBind_Texture_VAO_Program(&shaderInputs);
        OpenGLSetShaderUniforms(&shaderInputs);
        GL_CALL(glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0));
        
        // Draw debris
        shaderInputs.type = basic_shader;
        shaderInputs.textureId = debrisTextureInfo.textureId;
        shaderInputs.vao = debrisVao;
        Identity(&(shaderInputs.rotationMatAboutZAxis));
        TranslationMat(&(shaderInputs.translationMat), gameState->debrisPos);
        
        OpenGLBind_Texture_VAO_Program(&shaderInputs);
        OpenGLSetShaderUniforms(&shaderInputs);
        GL_CALL(glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0));
        
        // Draw spaceship
        shaderInputs.type = basic_shader;
        shaderInputs.textureId = spaceshipTextureInfo.textureId;
        shaderInputs.vao = spaceshipVao;
        RotationAboutZAxis(&(shaderInputs.rotationMatAboutZAxis), gameState->spaceshipRotation);
        TranslationMat(&(shaderInputs.translationMat), gameState->spaceshipPos);
        if(input.throttle)
        {
            shaderInputs.textureOffset = {0.5f, 0.0f};
        }
        else
        {
            shaderInputs.textureOffset = {0.0f, 0.0f};
        }
        OpenGLBind_Texture_VAO_Program(&shaderInputs);
        OpenGLSetShaderUniforms(&shaderInputs);
        GL_CALL(glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0));
        
        // Draw bullets
        shaderInputs.type = basic_shader;
        shaderInputs.textureId = bulletTextureInfo.textureId;
        shaderInputs.vao = bulletVao;
        shaderInputs.textureOffset = {};
        Identity(&(shaderInputs.rotationMatAboutZAxis));
        
        OpenGLBind_Texture_VAO_Program(&shaderInputs);
        bullet = gameState->bullets;
        while(bullet)
        {
            TranslationMat(&(shaderInputs.translationMat), bullet->pos);
            
            OpenGLSetShaderUniforms(&shaderInputs);
            GL_CALL(glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0));
            bullet = bullet->next;
        }
        
        // Draw asteroids
        shaderInputs.type = basic_shader;
        shaderInputs.textureId = asteroidTextureInfo.textureId;
        shaderInputs.vao = asteroidVao;
        
        OpenGLBind_Texture_VAO_Program(&shaderInputs);
        
        asteroid = gameState->asteroids;
        while(asteroid)
        {
            RotationAboutZAxis(&(shaderInputs.rotationMatAboutZAxis), asteroid->rotation);
            TranslationMat(&(shaderInputs.translationMat), asteroid->pos);
            OpenGLSetShaderUniforms(&shaderInputs);
            GL_CALL(glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0));
            asteroid = asteroid->next;
        }
        
        // Draw animations
        AnimationClip *previousClip = 0;
        AnimationClip *currentClip = gameState->animationClips;
        while(currentClip)
        {
            AnimationClip *nextClip = currentClip->next;
            currentClip->timeSpentOnThisFrame += targetMillisecondsPerFrame;
            if(currentClip->timeSpentOnThisFrame >= currentClip->msPerFrame)
            {
                currentClip->timeSpentOnThisFrame -= currentClip->msPerFrame;
                currentClip->currentFrameIndex = currentClip->currentFrameIndex + currentClip->stride;
            }
            
            if(currentClip->currentFrameIndex == currentClip->onePastEndFrameIndex) 
            {
                // Delete clip
                if(previousClip)
                {
                    previousClip->next = nextClip;
                }
                else
                {
                    gameState->animationClips = nextClip;
                }
                FreeChunk(&gameState->animationClipsAllocator, (uint8*)currentClip);
            }
            else
            {
                real32 row = Floor((real32)currentClip->currentFrameIndex/(real32)currentClip->spritesheet->numFramesAlongWidth);
                real32 column = (real32)currentClip->currentFrameIndex - (row * (real32)currentClip->spritesheet->numFramesAlongWidth);
                v2 textureOffset = {column*currentClip->spritesheet->normalizedFrameWidth, row*currentClip->spritesheet->normalizedFrameHeight};
                
                shaderInputs.type = basic_shader;
                shaderInputs.textureId = currentClip->spritesheet->textureInfo.textureId;
                shaderInputs.vao = currentClip->spritesheet->vao;
                Identity(&(shaderInputs.rotationMatAboutZAxis));
                TranslationMat(&(shaderInputs.translationMat), currentClip->pos);
                shaderInputs.textureOffset = textureOffset;
                
                OpenGLBind_Texture_VAO_Program(&shaderInputs);
                OpenGLSetShaderUniforms(&shaderInputs);
                GL_CALL(glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0));
                
                previousClip = currentClip;
            }
            
            currentClip = nextClip;
        }
        
        //Draw hitpoints
        v4 screenRect = {0.0f, 0.0f, gameResolution.x, gameResolution.y};
        HitpointDisplay *previousHitpointDisplay = 0;
        HitpointDisplay *currentHitpointDisplay = gameState->hitpointDisplays;
        real32 displayVelocity = 2.5f;
        real32 displayDuration = 0.5f;
        char hitpointString[1 + 20 + 1] = {};
        hitpointString[0] = '+';
        while(currentHitpointDisplay)
        {
            HitpointDisplay *nextHitpointDisplay = currentHitpointDisplay->next;
            if(currentHitpointDisplay->durationSpent >= displayDuration)
            {
                if(previousHitpointDisplay)
                {
                    previousHitpointDisplay->next = nextHitpointDisplay;
                }
                else
                {
                    gameState->hitpointDisplays = nextHitpointDisplay;
                }
                
                FreeChunk(&gameState->hitpointDisplayAllocator, (uint8*)currentHitpointDisplay);
            }
            else
            {
                currentHitpointDisplay->pos += {0.0f, displayVelocity, 0.0f}; 
                real32 opacity = 1.0f - (currentHitpointDisplay->durationSpent/displayDuration);
                sprintf(hitpointString, "+%" PRIu64, currentHitpointDisplay->value);
                OpenGLDrawTextAt({currentHitpointDisplay->pos.x , currentHitpointDisplay->pos.y}, {screenRect.w, screenRect.h}, hitpointString, {0.0f, 1.0f, 0.0f, opacity}, font3, program2Id);
                currentHitpointDisplay->durationSpent += dT;
                
                previousHitpointDisplay = currentHitpointDisplay;
            }
            
            currentHitpointDisplay = nextHitpointDisplay;
        }
        
        
        //Draw score
        char scoreString[7 + 20 + 1] = {};
        char livesRemainingString[8 + 1] = {};
        sprintf(scoreString, "SCORE: %" PRIu64, gameState->score);
        sprintf(livesRemainingString, "LIVES: %d" , gameState->livesRemaining);
        
        v2 textBounds = GetTextBounds(scoreString, font2);
        OpenGLDrawTextAt({screenRect.w - textBounds.x - 5.0f, screenRect.h - textBounds.y - 5.0f}, {screenRect.w, screenRect.h}, scoreString, {1.0f, 1.0f, 1.0f, 1.0f}, font2, program2Id);
        v4 color = {1.0f, 1.0f, 1.0f, 1.0f};
        if(gameState->livesRemaining == 1) color = {1.0f, 0.0f, 0.0f, 1.0f};
        textBounds = GetTextBounds(livesRemainingString, font2);
        OpenGLDrawTextAt({5.0f, gameResolution.y - textBounds.y - 5.0f}, {screenRect.w, screenRect.h}, livesRemainingString, color, font2, program2Id);
        
        if(gameState->paused)
        {
            FreeAllChunks(&gameState->asteroidAllocator);
            gameState->asteroidsCount = 0;
            gameState->asteroids = 0;
            gameState->asteroidSpawnClock = 0.0f;
            
            
            char *text = "PRESS SPACE TO START";
            real32 x = gameResolution.x/2.0f;
            real32 y = gameResolution.y/2.0f;
            v4 rect = {x, y, 20.0f, 20.0f};
            
            OpenGLDrawRectangle({0.0f, 0.0f}, {screenRect.w, screenRect.h}, {screenRect.w, screenRect.h}, {0.0f, 0.0f, 0.0f, 0.6f}, program3Id);
            
            textBounds = GetTextBounds(text, font);
            x -= textBounds.x/2.0f;
            y -= textBounds.y/2.0f;
#if 0
            //baseline
            OpenGLDrawRectangle({x, y}, {screenRect.w, y + 1.0f}, {screenRect.w, screenRect.h}, {1.0f, 1.0f, 1.0f, 1.0f}, program3Id);
            OpenGLDrawRectangle({x, y, 3.0f, font->scale}, {screenRect.w, screenRect.h}, {1.0f, 0.0f, 0.0f, 1.0f}, program3Id);
            OpenGLDrawRectangle({x, y, textBounds.x, textBounds.y}, {screenRect.w, screenRect.h}, {0.0f, 0.0f, 1.0f, 1.0f}, program3Id);
#endif
            OpenGLDrawTextAt({x, y}, {screenRect.w, screenRect.h}, text, {1.0f, 1.0f, 1.0f, 1.0f}, font, program2Id);
            
            char *text2 = "W,A,S,D TO MOVE | K TO SHOOT";
            x = gameResolution.x/2.0f;
            y -= textBounds.y + 5.0f;
            textBounds = GetTextBounds(text2, font3);
            x -= textBounds.x/2.0f;
            y -= textBounds.y/2.0f;
            OpenGLDrawTextAt({x, y}, {screenRect.w, screenRect.h}, text2, {1.0f, 1.0f, 1.0f, 1.0f}, font3, program2Id);
        }
        
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
        
        PfTimestamp end = PfGetTimestamp();
        uint64 endCycles = PfRdtsc();
        
        real32 secondsPerFrame = PfGetSeconds(start, end);
        real32 cyclesPerFrame = real32(endCycles - startCycles);
        real32 framesPerSecond = 1.0f/secondsPerFrame;
        
        startCycles = endCycles;
        start = end;
        
        /*char temp[256];
        sprintf(temp, "[%.2fms %.2FPS %.3fMHz %.3fms sleep] [RECORDING: %d PLAYING:%d]", secondsPerFrame * 1000.0f, 1.0f/secondsPerFrame, cyclesPerFrame/(secondsPerFrame * 1000.0f * 1000.0f), t5*1000.0f, recording, playing);*/
        PfSetWindowTitle(&window[0], "DEMO");
        
    }
    
    return 0;
}