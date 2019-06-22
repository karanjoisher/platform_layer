# python opengl_function_generator.py PLATFORM_LINUX .

mkdir -p build
cd build

FLAGS='-DSLOW_BUILD=0 -DDEBUG_BUILD=0 -DPLATFORM_LINUX=1 -DHOT_CODE_RELOADABLE=0 -DINPUT_RECORDING_PLAYBACK=0'

#g++ -std=c++11 -m32 -g /media/karan/9AA41D2CA41D0BFF/workspace/platform_layer/application.cpp -o application ${FLAGS} -L/usr/X11/lib -lX11 -L/usr/lib/i386-linux-gnu -lasound -lGL -Wno-write-strings -Wno-deprecated-declarations

g++ -std=c++11 -m64 -g /media/karan/9AA41D2CA41D0BFF/workspace/platform_layer/application.cpp -o application ${FLAGS} -L/usr/X11/lib -lX11 -L/usr/lib/x86_64-linux-gnu -lasound -lGL -Wno-write-strings -Wno-deprecated-declarations

#-lglut -lGL -lGLU 
cd ..



