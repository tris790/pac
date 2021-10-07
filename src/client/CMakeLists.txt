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

# Copying DLLS
if(WIN32)
    message(STATUS "Copying DLLS...")
    add_custom_command(TARGET ${APP_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:SDL2>
            $<TARGET_FILE_DIR:${APP_NAME}>)
endif()