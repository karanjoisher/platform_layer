#pragma once

// NOTE(KARAN): Do not delete or move these markers. They are being used by opengl_function_generator.py for outputing code required to import opengl functions
#define INSERT_NEW_FUNCTION_TYPES_HERE
#define INSERT_NEW_FUNCTION_DECLARATIONS_HERE
#define INSERT_NEW_FUNCTION_GRABS_HERE

#if PLATFORM_WINDOWS
#define GL_CLAMP_TO_BORDER                0x812D
#define GL_BGRA                           0x80E1
#define GL_ARRAY_BUFFER                   0x8892
#define GL_ELEMENT_ARRAY_BUFFER           0x8893
#define GL_STATIC_DRAW                    0x88E4
#define GL_FRAGMENT_SHADER                0x8B30
#define GL_VERTEX_SHADER                  0x8B31
#define GL_COMPILE_STATUS                 0x8B81
#define GL_TEXTURE0                       0x84C0

typedef char GLchar;
typedef ptrdiff_t GLsizeiptr;
typedef ptrdiff_t GLintptr;
#endif

INSERT_NEW_FUNCTION_TYPES_HERE
typedef void APIENTRY type_glGenVertexArrays (GLsizei n, GLuint *arrays);
typedef void APIENTRY type_glBindVertexArray (GLuint array);
typedef void APIENTRY type_glGenBuffers (GLsizei n, GLuint *buffers);
typedef void APIENTRY type_glBindBuffer (GLenum target, GLuint buffer);
typedef void APIENTRY type_glBufferData (GLenum target, GLsizeiptr size, const void *data, GLenum usage);
typedef void APIENTRY type_glVertexAttribPointer (GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void *pointer);
typedef void APIENTRY type_glEnableVertexAttribArray (GLuint index);
typedef GLuint APIENTRY type_glCreateShader (GLenum type);
typedef void APIENTRY type_glShaderSource (GLuint shader, GLsizei count, const GLchar *const*string, const GLint *length);
typedef void APIENTRY type_glCompileShader (GLuint shader);
typedef void APIENTRY type_glGetShaderiv (GLuint shader, GLenum pname, GLint *params);
typedef void APIENTRY type_glGetShaderInfoLog (GLuint shader, GLsizei bufSize, GLsizei *length, GLchar *infoLog);
typedef GLuint APIENTRY type_glCreateProgram (void);
typedef void APIENTRY type_glAttachShader (GLuint program, GLuint shader);
typedef void APIENTRY type_glLinkProgram (GLuint program);
typedef void APIENTRY type_glDeleteShader (GLuint shader);
typedef void APIENTRY type_glUseProgram (GLuint program);
typedef void APIENTRY type_glActiveTexture (GLenum texture);
typedef GLint APIENTRY type_glGetUniformLocation (GLuint program, const GLchar *name);
typedef void APIENTRY type_glUniform1i (GLint location, GLint v0);


#define GL_FUNCTION(name) global_variable type_##name *name

INSERT_NEW_FUNCTION_DECLARATIONS_HERE
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
GL_FUNCTION(glGetShaderiv);
GL_FUNCTION(glGetShaderInfoLog);
GL_FUNCTION(glCreateProgram);
GL_FUNCTION(glAttachShader);
GL_FUNCTION(glLinkProgram);
GL_FUNCTION(glDeleteShader);
GL_FUNCTION(glUseProgram);
GL_FUNCTION(glActiveTexture);
GL_FUNCTION(glGetUniformLocation);
GL_FUNCTION(glUniform1i);


#if PLATFORM_LINUX
#define GL_GET_PROC_ADDRESS(name) name = (type_##name *)glXGetProcAddressARB((const GLubyte*) #name);
#elif PLATFORM_WINDOWS
#define GL_GET_PROC_ADDRESS(name) name = (type_##name *)wglGetProcAddress(#name);
#endif

void GrabOpenGLFuncPointers()
{
    INSERT_NEW_FUNCTION_GRABS_HERE;
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
    GL_GET_PROC_ADDRESS(glGetShaderiv);
    GL_GET_PROC_ADDRESS(glGetShaderInfoLog);
    GL_GET_PROC_ADDRESS(glCreateProgram);
    GL_GET_PROC_ADDRESS(glAttachShader);
    GL_GET_PROC_ADDRESS(glLinkProgram);
    GL_GET_PROC_ADDRESS(glDeleteShader);
    GL_GET_PROC_ADDRESS(glUseProgram);
    GL_GET_PROC_ADDRESS(glActiveTexture);
    GL_GET_PROC_ADDRESS(glGetUniformLocation);
    GL_GET_PROC_ADDRESS(glUniform1i);
    
}

#undef INSERT_NEW_FUNCTION_TYPES_HERE
#undef INSERT_NEW_FUNCTION_DECLARATIONS_HERE
#undef INSERT_NEW_FUNCTION_GRABS_HERE
