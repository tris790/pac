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
