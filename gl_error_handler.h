#pragma once

#include <GL/glx.h>
#include "utility.h"

#define GL_CALL(glFunctionCall) glClearErrors(); glFunctionCall; glCheckErrors((char*)#glFunctionCall,(char*) __FILE__,  __LINE__);

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
        DEBUG_LOG(stderr, "[OPENGL_ERROR: %d], [FILE: %s], [LINE: %d], [FUNCTION: %s]\n", error, file, line, functionName); 
        
        if((error = glGetError()) == GL_NO_ERROR)
        {
            ASSERT(!"Encountered an error in openGL");
        }
    }
}


