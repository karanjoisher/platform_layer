@echo off

REM python opengl_function_generator.py .

IF NOT EXIST ./build mkdir ./build
cd build

set FLAGS=-DSLOW_BUILD=1 -DDEBUG_BUILD=1 -DPLATFORM_WINDOWS=1

set CommonCompilerFlags= -MTd -Gm- -nologo -GR- -EHa- -Oi  -W4 -wd4201 -wd4100 -wd4127 -wd4505 -wd4189 -fp:fast  /Z7 

set CommonLinkerFlags=user32.lib Gdi32.lib winmm.lib Ole32.lib 

cl %FLAGS% %CommonCompilerFlags% -Fmapplication.map W:/platform_layer/application.cpp /link %CommonLinkerFlags% -incremental:no

cd ..


