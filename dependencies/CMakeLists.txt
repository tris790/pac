# SDL 2
FetchContent_Declare(
  SDL2
  URL https://www.libsdl.org/release/SDL2-2.0.16.zip
)
message(STATUS "Fetching SDL2...")
FetchContent_MakeAvailable(SDL2)

# uvgrtp
SET(uvgrtp_GIT_HASH b729c931c0a9041b27f032ed2d3caab824ef65c5)
FetchContent_Declare(
  UVGRTP
  GIT_REPOSITORY "https://github.com/ultravideo/uvgRTP"
  GIT_TAG ${uvgrtp_GIT_HASH}
)
message(STATUS "Fetching UVGRTP...")
FetchContent_MakeAvailable(UVGRTP)