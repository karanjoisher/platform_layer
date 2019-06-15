@echo off

REM python opengl_function_generator.py PLATFORM_WINDOWS .

IF NOT EXIST ./build mkdir ./build
cd build

set FLAGS=-DSLOW_BUILD=1 -DDEBUG_BUILD=1 -DPLATFORM_WINDOWS=1 -DPF_GLEW_ENABLED=0 -DHOT_CODE_RELOADABLE=1 -DINPUT_RECORDING_PLAYBACK=1

set CommonCompilerFlags= -MTd -Gm- -nologo -GR- -EHa- -Oi  -W4 -wd4201 -wd4100 -wd4127 -wd4505 -wd4189 -fp:fast  /Z7 

REM set GlewLibrary=/LIBPATH:W:/platform_layer/glew/lib/Win32 glew32s.lib
set CommonLinkerFlags=%GlewLibrary% user32.lib Gdi32.lib winmm.lib Ole32.lib openGL32.lib 

cl  /I W:/platform_layer/glew/include %FLAGS% %CommonCompilerFlags% -Fmapplication.map W:/platform_layer/application.cpp /link    %CommonLinkerFlags% -incremental:no

REM cl %FLAGS%  %CommonCompilerFlags% W:/platform_layer/wave_file_pcm_extractor.cpp /link %CommonLinkerFlags% -incremental:no

cd ..


