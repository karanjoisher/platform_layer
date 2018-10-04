mkdir -p build
cd build

FLAGS='-DSLOW_BUILD=1 -DDEBUG_BUILD=1 -DPLATFORM_LINUX=1'

#g++ -m32 -g ../application.cpp -o application ${FLAGS} -L/usr/X11/lib -lX11 -L/usr/lib/i386-linux-gnu -lasound

g++ -std=c++11 -m64 -g /media/karan/9AA41D2CA41D0BFF/workspace/platform_layer/application.cpp -o application ${FLAGS} -L/usr/X11/lib -lX11 -L/usr/lib/x86_64-linux-gnu -lasound

#-lglut -lGL -lGLU 
cd ..



