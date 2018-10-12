#pragma once

void PfInitialize();
void PfCreateWindow(PfWindow *window, char *title, int32 xPos, int32 yPos, int32 width, int32 height);
void PfResizeWindow(PfWindow *window, int32 width, int32 height);
PfRect PfGetClientRect(PfWindow *window);
PfRect PfGetWindowRect(PfWindow *window); // TODO(KARAN): Linux 
void PfGetOffscreenBuffer(PfWindow *window, PfOffscreenBuffer *offscreenBuffer);
void PfBlitToScreen(PfWindow *window); 
void PfToggleFullscreen(PfWindow *window);
int32 PfGetKeyState(int32 vkCode);
int32 PfGetKeyState(PfWindow *window, int32 vkCode);
int32 PfGetMouseButtonState(PfWindow *window, int32 index);
int32 PfGetMouseButtonState(int32 index);
bool PfGetMouseCoordinates(PfWindow *window, int32 *x, int32 *y);
PfTimestamp PfGetTimestamp();
real32 PfGetSeconds(PfTimestamp startTime, PfTimestamp endTime);
uint64 PfRdtsc();
void PfSetWindowTitle(PfWindow *window, char *title);
void PfglRenderWindow(PfWindow *window);
void PfglMakeCurrent(PfWindow *window); 
void PfGLConfig(int32 glMajorVersion, int32 glMinorVersion, bool coreProfile); 

void PfglSwapBuffers(PfWindow *window);