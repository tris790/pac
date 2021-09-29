set(APP_NAME pac-server)
add_executable(${APP_NAME} main.cpp)
target_compile_features(${APP_NAME} PRIVATE cxx_std_17)

#WINSOCK2
target_link_libraries(${APP_NAME} PUBLIC Ws2_32)

# UVG
SET(UVGRTP_PATH ${CMAKE_SOURCE_DIR}/libs/uvgRTP)
target_include_directories(${APP_NAME} PUBLIC ${UVGRTP_PATH}/include)
target_link_directories(${APP_NAME} PUBLIC ${UVGRTP_PATH}/build/Release)
target_link_libraries(${APP_NAME} PUBLIC uvgrtp)
