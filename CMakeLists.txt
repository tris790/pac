cmake_minimum_required(VERSION 3.11)

project(
  Pac
  VERSION 0.1
  DESCRIPTION "Portals Across Computers"
  LANGUAGES CXX)

if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
  set(CMAKE_CXX_EXTENSIONS OFF)
  set_property(GLOBAL PROPERTY USE_FOLDERS ON)
endif()

include(FetchContent)
if(${CMAKE_VERSION} VERSION_LESS 3.14)
  include(cmake/add_FetchContent_MakeAvailable.cmake)
endif()

include_directories("include")
add_subdirectory(dependencies)
add_subdirectory(src)

if(MSVC)
  add_compile_options(/W3 /WX)
else()
  add_compile_options(-W -Wall -Werror)
endif()