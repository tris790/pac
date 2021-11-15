set(APP_NAME pac-server)
add_executable(${APP_NAME} main.cpp)
target_compile_features(${APP_NAME} PRIVATE cxx_std_17)

#WINSOCK2
if(WIN32)
    target_link_libraries(${APP_NAME} PUBLIC Ws2_32)
endif()

# UVG
SET(UVGRTP_PATH ${CMAKE_SOURCE_DIR}/libs/uvgRTP)
target_link_libraries(${APP_NAME} PUBLIC uvgrtp)

# SDL 2
target_link_libraries(${APP_NAME} PUBLIC SDL2)

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

if (UNIX)
    target_link_Libraries(${APP_NAME} PUBLIC X11 Xtst)
endif()

target_link_libraries(${APP_NAME} PUBLIC gstreamer-1.0 glib-2.0 gstapp-1.0 gobject-2.0)

# Copying config
message(STATUS "Copying config...")
#add_custom_command(TARGET ${APP_NAME} POST_BUILD
    #COMMAND ${CMAKE_COMMAND} -E copy_if_different
    #    ${CMAKE_CURRENT_LIST_DIR}/server.conf
    #    $<TARGET_FILE_DIR:${APP_NAME}>)
