# Devlog

## 29/9/2021
- Project name: PAC

- Dependancies in C++ are hard if you want to be crossplatform
    - Build dependancies or store them in the repo or make a script that pulls a zip of them or make the dev install all of them on his own
    - Some OS have **pakage managers**, some don't
    - Cmake debug/release for dependencies
 
- Screen recording is not as straight forward as we thought
    - Gstreamer is bigger than we'd like but seems to do what we want it to do (record at specified framerate low enough latency etc.)
    - We tried multiple open source projetcs but we weren't able to compile them (outdated/incomplete/poor documentation)


## 13/10/2021
- CMake
    FetchContent_Declare
    FetchContent_MakeAvailable
    Easy if the dependency can be built with CMake
    Sometime doesn't rebuild correctly ? (we need to delete build folder to fix it...) 

- Networking
    Using UVGRTP, an unencoded video frame is too big for an udp frame or even for the builtin packet splitting
    The connection can be established between two computers and then data can be transfered 

- Screen Capture
    Windows dxgiscreencapsrc or dx9screencapsrc work with stutters (probably caused by the size of the unencoded stream)
    Linux ximagesrc works with stutters on PopOs, not on Arch 