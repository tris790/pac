# PAC - Portals Across Computers
<p align="center"><a href="https://vuejs.org" target="_blank" rel="noopener noreferrer"><img width="100" src="assets/tupac3.png" alt="Pac logo"></a></p>

## Prerequisite
- [SDL2](https://www.libsdl.org/release/SDL2-devel-2.0.16-VC.zip)
- [SDL2 mixer](https://www.libsdl.org/projects/SDL_mixer/release/SDL2_mixer-devel-2.0.4-VC.zip)

## Install SDL2
### Windows
1. Download [SDL2](https://www.libsdl.org/release/SDL2-devel-2.0.16-VC.zip) development libraries
2. Extract the .zip file in the `src\lib` folder
3. Rename the extracted folder to `SDL2`
4. You should now have that hierarchy
   
   ![SDL2 hierarchy after unzip](assets/SDL2Hierarchy.png?raw=true "Title")

## Install SDL2 mixer
1. Download [SDL2 mixer](https://www.libsdl.org/projects/SDL_mixer/release/SDL2_mixer-devel-2.0.4-VC.zip) development libraries
2. Extract the .zip file in the `src\lib` folder
3. Rename the extracted folder to `SDL2_mixer`
4. You should now have that hierarchy
   
   ![SDL2_mixer hierarchy after unzip](assets/sdl2_mixerHierarchy.png?raw=true "Title2")
## Building
### Server
cd build
Run once: 
- Windows: `cmake -G "Visual Studio 16 2019" -A x64 -S ../ -B "build64"`
- Linux: `cmake -G "Ninja" -S ../ -B "build64"`

Run to build: 
- Debug: `cmake --build build64 --config Debug`
- Release: `cmake --build build64 --config Release` add: -DCMAKE_CXX_COMPILER=PATH_TO_YOUR_COMPILER (run: which g++)

### Gstreamer screen capture

- Windows: `gst-launch-1.0 dxgiscreencapsrc cursor=true ! video/x-raw,framerate=60/1 ! videoscale method=0 ! videoconvert ! autovideosink sync=false`
- Linux: `gst-launch-1.0 ximagesrc startx=0 starty=0 use-damage=0 xid=83886082 ! video/x-raw,framerate=60/1 ! videoscale method=0 ! video/x-raw,width=1920,height=1080  ! autovideosink sync=false`