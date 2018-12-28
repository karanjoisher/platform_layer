#pragma once

#if PLATFORM_WINDOWS
#include <GL/gl.h>
#elif PLATFORM_LINUX
#include <GL/glx.h>
#endif

#include "utility.h"
#define GL_CALL(glFunctionCall) glClearErrors();glFunctionCall; glCheckErrors((char*)#glFunctionCall,(char*) __FILE__,  __LINE__);

inline void glClearErrors()
{
    while(glGetError() != GL_NO_ERROR);
}

inline void glCheckErrors(char *functionName, char *file, int line)
{
    GLenum error;
    
    error = glGetError();
    while(error != GL_NO_ERROR)
    {
        DEBUG_ERROR("[OPENGL_ERROR: %d Function:%s File:%s Line:%d]", error, functionName, file, line); 
        
        if((error = glGetError()) == GL_NO_ERROR)
        {
            ASSERT(false, "Encountered an error in openGL");
        }
    }
}


#if PLATFORM_WINDOWS
#include "windows_opengl.h"
#elif PLATFORM_LINUX
#include "linux_opengl.h"
#endif
