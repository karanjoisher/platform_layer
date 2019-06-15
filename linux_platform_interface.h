#pragma once

#include <X11/Xutil.h>
#include "utility.h"
#include "project_types.h"
#include <GL/glx.h>
#include <alsa/asoundlib.h>


#if defined(PF_TIME)
typedef timespec PfTimestamp;
#endif

#if defined(PF_WINDOW_AND_INPUT)
typedef XImage LinuxOffscreenBuffer;

struct PfOffscreenBuffer
{
    void *data;
    int width;
    int height;
    int pitch;
    int bytesPerPixel;
};

struct PfWindow
{
    Window windowHandle; //Window = unsigned long 
    Display *display; 
    Screen *screen;
    int32 screenNum;
    GC graphicsContext;
    GLXContext glxContext;
    LinuxOffscreenBuffer offscreenBuffer;
    bool shouldClose;
    bool hasKeyboardFocus;
    bool isWindowUnderMouse;
    
    // HACK(KARAN): This is temporarily added to support 
    // offscreenbuffer rendering via opengl modern contexts.
    GLuint offscreenBufferTextureId;
    int32 vao;
    uint32 programId;
};


struct PfRect
{
    int32 x;
    int32 y; 
    int32 width;
    int32 height;
};
#endif

#if defined(PF_SOUND)
struct PfSoundSystem
{
    snd_pcm_t *soundDeviceHandle;
    void *secondarySoundBuffer;
    uint64 bufferDurationInFrames;
};

struct PfSoundBuffer
{
    void *buffer;
    uint64 frames;
};
#endif

#include "api_interface.h"