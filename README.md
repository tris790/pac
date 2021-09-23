# PAC - Portals Across Computers

## Building
### Server
cd build
Run once: 
- Windows: `cmake -G "Visual Studio 16 2019" -A x64 -S ../ -B "build64"`
- Linux: `cmake -G "Ninja" -S ../ -B "build64"`

Run to build: 
- Debug: `cmake --build build64 --config Debug`
- Release: `cmake --build build64 --config Release` add: -DCMAKE_CXX_COMPILER=PATH_TO_YOUR_COMPILER (run: which g++)