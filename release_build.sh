mkdir -p build
cd build

FLAGS='-DSLOW_BUILD=0 -DDEBUG_BUILD=0 -DPLATFORM_LINUX=1 -DHOT_CODE_RELOADABLE=0 -DINPUT_RECORDING_PLAYBACK=0'

# 32-bit build
g++ -O2 -std=c++11 -m32 -g ../application.cpp -o application32 ${FLAGS} -L/usr/X11/lib -lX11 -L/usr/lib/i386-linux-gnu -lasound -lGL -Wno-write-strings -Wno-deprecated-declarations

# 64-bit build
g++ -O2 -std=c++11 -m64 -g ../application.cpp -o application64 ${FLAGS} -L/usr/X11/lib -lX11 -L/usr/lib/x86_64-linux-gnu -lasound -lGL -Wno-write-strings -Wno-deprecated-declarations

cd ..
