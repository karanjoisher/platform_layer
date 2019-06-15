## How to use the platform layer

1. Download the project
2. Define symbols for the modules you intend to use.
For example, if you want to use file api, write `#define PF_FILE`.
Here is a list of modules currently present in this project:
    * PF_WINDOW_AND_INPUT
    *  PF_TIME
    * PF_FILE
    * PF_SOUND
3. Include pf.h after defining the modules (`#include pf.h`).

## How to build demo(application.cpp)

1. Comment out `python opengl_function_generator.py` line from the build.bat(On Windows)/build.sh(On Linux).
2. Change all absolute filepaths to relative filepaths in the build files. (Currently they are absolute filepaths as it helps to generate proper error messages during development.)
3. Run the build.bat/.sh file.

## Credits for asteroids demo:

* Art Assets: Created by Kim Lathrop, may be freely re-used in non-commercial projects, please credit Kim
* Music:

    * ambient_space.wav : https://freesound.org/people/Suburbanwizard/sounds/442181/
    * rings_of_saturn.wav : https://freesound.org/people/rhodesmas/sounds/324252/
    * thrust_1.wav: https://freesound.org/people/LimitSnap_Creations/sounds/318688/
    * pew_1.wav: https://freesound.org/people/Quonux/sounds/166418/
    * boom_1.wav: https://freesound.org/people/deleted_user_5405837/sounds/399303/