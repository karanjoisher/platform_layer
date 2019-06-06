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



