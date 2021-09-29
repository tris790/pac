set(APP_NAME pac-client)

if(MSVC)
    add_executable(${APP_NAME} main.cpp)
    #set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS} /SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup")
else()
    add_executable(${APP_NAME} main.cpp)
endif()
target_compile_features(${APP_NAME} PRIVATE cxx_std_17)

# SDL 2
SET(SDL2_PATH ${CMAKE_SOURCE_DIR}/libs/SDL2)
target_include_directories(${APP_NAME} PUBLIC ${SDL2_PATH}/include)
target_link_directories(${APP_NAME} PUBLIC ${SDL2_PATH}/lib/x64)
target_link_libraries(${APP_NAME} PUBLIC SDL2)

add_custom_command(TARGET ${APP_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${SDL2_PATH}/lib/x64/SDL2.dll"
        $<TARGET_FILE_DIR:${APP_NAME}>)

# WINSOCK2
target_link_libraries(${APP_NAME} PUBLIC Ws2_32)

# UVG
SET(UVGRTP_PATH ${CMAKE_SOURCE_DIR}/libs/uvgRTP)
target_include_directories(${APP_NAME} PUBLIC ${UVGRTP_PATH}/include)
target_link_directories(${APP_NAME} PUBLIC ${UVGRTP_PATH}/build/Release)
target_link_libraries(${APP_NAME} PUBLIC uvgrtp)