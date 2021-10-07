# SDL 2
FetchContent_Declare(
  SDL2
  GIT_REPOSITORY https://github.com/libsdl-org/SDL
  GIT_TAG dd95c9c8a23b3c1bb3081e802fe63d00978950e9
)
message(STATUS "Fetching SDL2...")
FetchContent_MakeAvailable(SDL2)

# SDL 2 Mixer
FetchContent_Declare(
  SDL2_MIXER
  GIT_REPOSITORY https://github.com/tris790/SDL_mixer
  GIT_TAG 7bd6995695593832f46639133812676342324189
)
message(STATUS "Fetching SDL2 Mixer...")
FetchContent_MakeAvailable(SDL2_MIXER)

# uvgrtp
SET(uvgrtp_GIT_HASH b729c931c0a9041b27f032ed2d3caab824ef65c5)
FetchContent_Declare(
  UVGRTP
  GIT_REPOSITORY "https://github.com/ultravideo/uvgRTP"
  GIT_TAG ${uvgrtp_GIT_HASH}
)
message(STATUS "Fetching UVGRTP...")
FetchContent_MakeAvailable(UVGRTP)