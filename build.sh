mkdir -p build
cd build

FLAGS='-DSLOW_BUILD=1 -DDEBUG_BUILD=1'
ARCHITECTURE=32

g++ -m${ARCHITECTURE} -g ../linux_platform_layer.cpp -o linux_platform_layer ${FLAGS} -L/usr/X11/lib -lX11 

#-lglut -lGL -lGLU 
cd ..

