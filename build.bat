@echo off

REM python opengl_function_generator.py PLATFORM_WINDOWS .

if not exist build\. mkdir build
pushd build

set FLAGS=-DSLOW_BUILD=0 -DDEBUG_BUILD=0 -DPLATFORM_WINDOWS=1 -DPF_GLEW_ENABLED=0 -DHOT_CODE_RELOADABLE=0 -DINPUT_RECORDING_PLAYBACK=0

set CommonCompilerFlags= -MTd -Gm- -nologo -GR- -EHa- -Oi  -W4 -wd4201 -wd4100 -wd4127 -wd4505 -wd4189 -fp:fast  /Z7 -Ox

REM set GlewLibrary=/LIBPATH:W:/platform_layer/glew/lib/Win32 glew32s.lib
set CommonLinkerFlags=%GlewLibrary% user32.lib Gdi32.lib winmm.lib Ole32.lib openGL32.lib 

cl %FLAGS% %CommonCompilerFlags% -Fmapplication.map ..\application.cpp /link %CommonLinkerFlags% -incremental:no

popd
