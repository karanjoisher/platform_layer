## Demo video for this cross platform interface

https://www.youtube.com/watch?v=RHCYtrhFRr4

## How to use the platform layer

1. Download the project
2. Define symbols for the modules you intend to use.
For example, if you want to use file api, write `#define PF_FILE`.
Here is a list of modules currently present in this project:
    * PF_WINDOW_AND_INPUT
    * PF_TIME
    * PF_FILE
    * PF_SOUND
3. Include pf.h after defining the modules (`#include pf.h`).
4. Required libraries for windows: user32.lib Gdi32.lib winmm.lib Ole32.lib openGL32.lib.
   Required libraries for linux: lX11(X11 windows), lasound(ALSA sound), lGL(OpenGL)

## How to build demo(application.cpp)

1. On windows, run release_build.bat using msvc compiler
2. On linux, run release_build.sh using g++ compiler

## Credits for asteroids demo:

* Art Assets: Created by Kim Lathrop, may be freely re-used in non-commercial projects, please credit Kim
* Music:

    * ambient_space.wav : https://freesound.org/people/Suburbanwizard/sounds/442181/
    * rings_of_saturn.wav : https://freesound.org/people/rhodesmas/sounds/324252/
    * thrust_1.wav: https://freesound.org/people/LimitSnap_Creations/sounds/318688/
    * pew_1.wav: https://freesound.org/people/Quonux/sounds/166418/
    * boom_1.wav: https://freesound.org/people/deleted_user_5405837/sounds/399303/
