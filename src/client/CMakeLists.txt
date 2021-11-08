set(APP_NAME pac-client)
add_executable(${APP_NAME} main.cpp)

# Hide terminal in debug for Windows
if(WIN32)
    set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS} /SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup")
endif()

target_compile_features(${APP_NAME} PRIVATE cxx_std_17)

# SDL 2
target_link_libraries(${APP_NAME} PUBLIC SDL2)

# SDL 2 Mixer
target_link_libraries(${APP_NAME} PUBLIC SDL2_MIXER)

# WINSOCK2
if(WIN32)
    target_link_libraries(${APP_NAME} PUBLIC Ws2_32)
endif()

# UVG
target_link_libraries(${APP_NAME} PUBLIC uvgrtp)

# GSTREAMER
if(WIN32)
    target_link_directories(${APP_NAME} PUBLIC $ENV{GSTREAMER_1_0_ROOT_MSVC_X86_64}/lib)
    target_include_directories(${APP_NAME} PUBLIC $ENV{GSTREAMER_1_0_ROOT_MSVC_X86_64}/include/gstreamer-1.0)
    target_include_directories(${APP_NAME} PUBLIC $ENV{GSTREAMER_1_0_ROOT_MSVC_X86_64}/include/glib-2.0)
    target_include_directories(${APP_NAME} PUBLIC $ENV{GSTREAMER_1_0_ROOT_MSVC_X86_64}/lib/glib-2.0/include)
    target_include_directories(${APP_NAME} PUBLIC $ENV{GSTREAMER_1_0_ROOT_MSVC_X86_64}/lib/gstreamer-1.0/include)
else()
    target_include_directories(${APP_NAME} PUBLIC /usr/include/gstreamer-1.0)	
    target_include_directories(${APP_NAME} PUBLIC /usr/include/glib-2.0)	
    target_include_directories(${APP_NAME} PUBLIC /usr/lib/glib-2.0/include)
    target_include_directories(${APP_NAME} PUBLIC /usr/lib/gstreamer-1.0/include)
endif()

target_link_libraries(${APP_NAME} PUBLIC gstreamer-1.0 glib-2.0 gstapp-1.0 gstvideo-1.0 gobject-2.0)


# Copying DLLS
if(WIN32)
    message(STATUS "Copying DLLS...")
    add_custom_command(TARGET ${APP_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:SDL2>
            $<TARGET_FILE_DIR:${APP_NAME}>)
endif()