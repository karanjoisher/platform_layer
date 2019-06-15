#pragma once

#if defined(PF_WINDOW_AND_INPUT)
global_variable char *globalPfKeyCodeToStrMap[]
{
    "PF_NULL",
    "PF_ESC",
    "PF_F1",
    "PF_F2",
    "PF_F3",
    "PF_F4",
    "PF_F5",
    "PF_F6",
    "PF_F7",
    "PF_F8",
    "PF_F9",
    "PF_F10",
    "PF_F11",
    "PF_F12",
    "PF_PRINT_SCREEN",
    "PF_SCROLL_LOCK",
    "PF_PAUSE",
    
    "PF_TILDE",
    "PF_1",
    "PF_2",
    "PF_3",
    "PF_4",
    "PF_5",
    "PF_6",
    "PF_7",
    "PF_8",
    "PF_9",
    "PF_0",
    "PF_MINUS",
    "PF_EQUALS",
    "PF_BACKSPACE",
    
    "PF_TAB",
    "PF_Q",
    "PF_W",
    "PF_E",
    "PF_R",
    "PF_T",
    "PF_Y",
    "PF_U",
    "PF_I",
    "PF_O",
    "PF_P",
    "PF_OPEN_SQUARE_BRACKET",
    "PF_CLOSE_SQUARE_BRACKET",
    "PF_BACKSLASH",
    
    "PF_CAPS_LOCK",
    "PF_A",
    "PF_S",
    "PF_D",
    "PF_F",
    "PF_G",
    "PF_H",
    "PF_J",
    "PF_K",
    "PF_L",
    "PF_SEMICOLON",
    "PF_APOSTROPHE",
    "PF_ENTER",
    
    "PF_LEFT_SHIFT",
    "PF_Z",
    "PF_X",
    "PF_C",
    "PF_V",
    "PF_B",
    "PF_N",
    "PF_M",
    "PF_COMMA",
    "PF_PERIOD",
    "PF_FORWARD_SLASH",
    "PF_RIGHT_SHIFT",
    
    "PF_LEFT_CTRL",
    "PF_LEFT_WIN",
    "PF_LEFT_ALT",
    "PF_SPACEBAR",
    "PF_RIGHT_ALT",
    "PF_RIGHT_WIN",
    "PF_MENU",
    "PF_RIGHT_CTRL",
    
    "PF_INSERT",
    "PF_HOME",
    "PF_PAGE_UP",
    "PF_DELETE",
    "PF_END",
    "PF_PAGE_DOWN",
    "PF_CLEAR",
    
    "PF_UP",
    "PF_DOWN",
    "PF_LEFT",
    "PF_RIGHT",
    
    "PF_NUM_LOCK",
    "PF_NUMPAD_DIVIDE",
    "PF_NUMPAD_MULTIPLY",
    "PF_NUMPAD_MINUS",
    "PF_NUMPAD_7",
    "PF_NUMPAD_8",
    "PF_NUMPAD_9",
    "PF_NUMPAD_PLUS",
    "PF_NUMPAD_4",
    "PF_NUMPAD_5",
    "PF_NUMPAD_6",
    "PF_NUMPAD_1",
    "PF_NUMPAD_2",
    "PF_NUMPAD_3",
    "PF_NUMPAD_0",
    "PF_NUMPAD_PERIOD",
    "PF_NUMPAD_ENTER",
    "PF_ONE_PAST_LAST",
};


enum PfKeyCode
{
    PF_NULL,
    PF_ESC,
    PF_F1,
    PF_F2,
    PF_F3,
    PF_F4,
    PF_F5,
    PF_F6,
    PF_F7,
    PF_F8,
    PF_F9,
    PF_F10,
    PF_F11,
    PF_F12,
    PF_PRINT_SCREEN,
    PF_SCROLL_LOCK,
    PF_PAUSE,
    
    PF_TILDE,
    PF_1,
    PF_2,
    PF_3,
    PF_4,
    PF_5,
    PF_6,
    PF_7,
    PF_8,
    PF_9,
    PF_0,
    PF_MINUS,
    PF_EQUALS,
    PF_BACKSPACE,
    
    PF_TAB,
    PF_Q,
    PF_W,
    PF_E,
    PF_R,
    PF_T,
    PF_Y,
    PF_U,
    PF_I,
    PF_O,
    PF_P,
    PF_OPEN_SQUARE_BRACKET,
    PF_CLOSE_SQUARE_BRACKET,
    PF_BACKSLASH,
    
    PF_CAPS_LOCK,
    PF_A,
    PF_S,
    PF_D,
    PF_F,
    PF_G,
    PF_H,
    PF_J,
    PF_K,
    PF_L,
    PF_SEMICOLON,
    PF_APOSTROPHE,
    PF_ENTER,
    
    PF_LEFT_SHIFT,
    PF_Z,
    PF_X,
    PF_C,
    PF_V,
    PF_B,
    PF_N,
    PF_M,
    PF_COMMA,
    PF_PERIOD,
    PF_FORWARD_SLASH,
    PF_RIGHT_SHIFT,
    
    PF_LEFT_CTRL,
    PF_LEFT_WIN,
    PF_LEFT_ALT,
    PF_SPACEBAR,
    PF_RIGHT_ALT,
    PF_RIGHT_WIN,
    PF_MENU,
    PF_RIGHT_CTRL,
    
    PF_INSERT,
    PF_HOME,
    PF_PAGE_UP,
    PF_DELETE,
    PF_END,
    PF_PAGE_DOWN,
    PF_CLEAR,
    
    PF_UP,
    PF_DOWN,
    PF_LEFT,
    PF_RIGHT,
    
    PF_NUM_LOCK,
    PF_NUMPAD_DIVIDE,
    PF_NUMPAD_MULTIPLY,
    PF_NUMPAD_MINUS,
    PF_NUMPAD_7,
    PF_NUMPAD_8,
    PF_NUMPAD_9,
    PF_NUMPAD_PLUS,
    PF_NUMPAD_4,
    PF_NUMPAD_5,
    PF_NUMPAD_6,
    PF_NUMPAD_1,
    PF_NUMPAD_2,
    PF_NUMPAD_3,
    PF_NUMPAD_0,
    PF_NUMPAD_PERIOD,
    PF_NUMPAD_ENTER,
    PF_ONE_PAST_LAST,
};
#endif

#if defined(PF_FILE)
#define PF_READ ((uint32)(1 << 0))
#define PF_WRITE ((uint32)(1 << 1))
#define PF_CREATE ((uint32)(1 << 2))
#define PF_OPEN ((uint32)(1 << 3))
#endif

// Initialization
void PfInitialize();

#if defined(PF_WINDOW_AND_INPUT)
// Windowing, I/O
void PfCreateWindow(PfWindow *window, char *title, int32 xPos, int32 yPos, int32 width, int32 height);
void PfResizeWindow(PfWindow *window, int32 width, int32 height);
PfRect PfGetClientRect(PfWindow *window);
PfRect PfGetWindowRect(PfWindow *window); // TODO(KARAN): Linux 
void PfGetOffscreenBuffer(PfWindow *window, PfOffscreenBuffer *offscreenBuffer);
void PfBlitToScreen(PfWindow *window); 
void PfToggleFullscreen(PfWindow *window);
int32 PfGetKeyState(PfKeyCode keyCode, bool isVkCode);
int32 PfGetKeyState(PfWindow *window, PfKeyCode keyCode, bool isVkCode);
int32 PfGetMouseButtonState(PfWindow *window, int32 index);
int32 PfGetMouseButtonState(int32 index);
bool PfGetMouseCoordinates(PfWindow *window, int32 *x, int32 *y);
void PfSetWindowTitle(PfWindow *window, char *title);
void PfglRenderWindow(PfWindow *window);
void PfglMakeCurrent(PfWindow *window); 
void PfGLConfig(int32 glMajorVersion, int32 glMinorVersion, bool coreProfile);
void PfglSwapBuffers(PfWindow *window);
void PfUpdate();
bool PfRequestSwapInterval(int32 frames);
#endif

// Timing
#if defined(PF_TIME)
PfTimestamp PfGetTimestamp();
real32 PfGetSeconds(PfTimestamp startTime, PfTimestamp endTime);
uint64 PfRdtsc();
void PfSleep(int32 milliseconds);
#endif

// File I/O
#if defined(PF_FILE)
int64 PfWriteEntireFile(char *filename, void *data, uint32 size);
int64 PfReadEntireFile(char *filename, void *data); 
int64 PfWriteFile(int64 fileHandle, void *data, uint32 size);
int64 PfReadFile(int64 fileHandle, void *data, uint32 size);
int64 PfCreateFile(char *filename, uint32 access, uint32 creationDisposition);
bool PfCloseFileHandle(int64 fileHandle);
bool PfDeleteFile(char *filename);
bool PfFilepathExists(char *filepath); 
uint64 PfGetFileSize(char *filepath); 
#endif

//Memory
// TODO(KARAN): Make this function as versatile as Linux's mmap and Windows' VirtualAlloc
void* PfVirtualAlloc(void *baseAddress, size_t size);

#if defined(PF_SOUND)
PfSoundSystem PfInitializeSoundSystem(uint64 bufferDurationInFrames, uint32 bitsPerSample, uint32 numChannels, uint32 framesPerSecond);
uint64 PfGetPendingFrames(PfSoundSystem *soundSystem);
PfSoundBuffer PfGetSoundBuffer(PfSoundBuffer *soundSystem, uint64 framesRequired);

void PfDispatchSoundBuffer(PfSoundSystem *soundSystem, PfSoundBuffer *soundBuffer);
void PfStartSoundSystem(PfSoundSystem *soundSystem);
#endif