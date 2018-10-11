#pragma once

// NOTE(KARAN): Do not delete or move these markers. They are being used by opengl_function_generator.py for outputing code required to import opengl functions
#define INSERT_NEW_FUNCTION_TYPES_HERE
#define INSERT_NEW_FUNCTION_DECLARATIONS_HERE
#define INSERT_NEW_FUNCTION_GRABS_HERE

INSERT_NEW_FUNCTION_TYPES_HERE
typedef void  type_glGetShaderiv (GLuint shader, GLenum pname, GLint *params);
typedef void  type_glGetShaderInfoLog (GLuint shader, GLsizei bufSize, GLsizei *length, GLchar *infoLog);
typedef void  type_glGenVertexArrays (GLsizei n, GLuint *arrays);
typedef void  type_glBindVertexArray (GLuint array);
typedef void  type_glGenBuffers (GLsizei n, GLuint *buffers);
typedef void  type_glBindBuffer (GLenum target, GLuint buffer);
typedef void  type_glBufferData (GLenum target, GLsizeiptr size, const void *data, GLenum usage);
typedef void  type_glVertexAttribPointer (GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void *pointer);
typedef void  type_glEnableVertexAttribArray (GLuint index);
typedef GLuint  type_glCreateShader (GLenum type);
typedef void  type_glShaderSource (GLuint shader, GLsizei count, const GLchar *const*string, const GLint *length);
typedef void  type_glCompileShader (GLuint shader);
typedef GLuint  type_glCreateProgram (void);
typedef void  type_glAttachShader (GLuint program, GLuint shader);
typedef void  type_glLinkProgram (GLuint program);
typedef void  type_glUseProgram (GLuint program);
typedef void  type_glDeleteShader (GLuint shader);

#define GL_FUNCTION(name) global_variable type_##name *name

INSERT_NEW_FUNCTION_DECLARATIONS_HERE
GL_FUNCTION(glGetShaderiv);
GL_FUNCTION(glGetShaderInfoLog);
GL_FUNCTION(glGenVertexArrays);
GL_FUNCTION(glBindVertexArray);
GL_FUNCTION(glGenBuffers);
GL_FUNCTION(glBindBuffer);
GL_FUNCTION(glBufferData);
GL_FUNCTION(glVertexAttribPointer);
GL_FUNCTION(glEnableVertexAttribArray);
GL_FUNCTION(glCreateShader);
GL_FUNCTION(glShaderSource);
GL_FUNCTION(glCompileShader);
GL_FUNCTION(glCreateProgram);
GL_FUNCTION(glAttachShader);
GL_FUNCTION(glLinkProgram);
GL_FUNCTION(glUseProgram);
GL_FUNCTION(glDeleteShader);

#if PLATFORM_LINUX
#define GL_GET_PROC_ADDRESS(name) name = (type_##name *)glXGetProcAddressARB((const GLubyte*) #name);
#endif

void GrabOpenGLFuncPointers()
{
    INSERT_NEW_FUNCTION_GRABS_HERE;
    GL_GET_PROC_ADDRESS(glGetShaderiv);
    GL_GET_PROC_ADDRESS(glGetShaderInfoLog);
    GL_GET_PROC_ADDRESS(glGenVertexArrays);
    GL_GET_PROC_ADDRESS(glBindVertexArray);
    GL_GET_PROC_ADDRESS(glGenBuffers);
    GL_GET_PROC_ADDRESS(glBindBuffer);
    GL_GET_PROC_ADDRESS(glBufferData);
    GL_GET_PROC_ADDRESS(glVertexAttribPointer);
    GL_GET_PROC_ADDRESS(glEnableVertexAttribArray);
    GL_GET_PROC_ADDRESS(glCreateShader);
    GL_GET_PROC_ADDRESS(glShaderSource);
    GL_GET_PROC_ADDRESS(glCompileShader);
    GL_GET_PROC_ADDRESS(glCreateProgram);
    GL_GET_PROC_ADDRESS(glAttachShader);
    GL_GET_PROC_ADDRESS(glLinkProgram);
    GL_GET_PROC_ADDRESS(glUseProgram);
    GL_GET_PROC_ADDRESS(glDeleteShader);
    
}


#undef INSERT_NEW_FUNCTION_TYPES_HERE
#undef INSERT_NEW_FUNCTION_DECLARATIONS_HERE
#undef INSERT_NEW_FUNCTION_GRABS_HERE
