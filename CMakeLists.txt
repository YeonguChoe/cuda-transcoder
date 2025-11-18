cmake_minimum_required(VERSION 4.0)
project(transcoder)

set(CMAKE_CXX_STANDARD 20)

add_executable(transcoder
main.cpp
transcoder.cpp
)

target_include_directories(${PROJECT_NAME} PRIVATE ${PROJECT_SOURCE_DIR}/ffmpeg/include)

target_link_directories(${PROJECT_NAME} PRIVATE ${PROJECT_SOURCE_DIR}/ffmpeg/lib)
target_link_libraries(${PROJECT_NAME} PRIVATE
        avutil
        swscale
        swresample
        avcodec
        avformat
        avdevice
        avfilter
)