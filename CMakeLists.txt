cmake_minimum_required(VERSION 3.14)
project(XExporter)

set(CMAKE_CXX_STANDARD 14)

set(SRC_DIR ./)

# 头文件
include_directories(${SRC_DIR})

# 源码
aux_source_directory(${SRC_DIR} SOURCE_DIR)

# 第三方库
include_directories(/usr/local/Cellar/ffmpeg/4.2.2-with-options_1/include)
link_directories(/usr/local/Cellar/ffmpeg/4.2.2-with-options_1/lib)

set(SOURCE ${SOURCE_DIR})

add_executable(XExporter ${SOURCE})

target_link_libraries(XExporter avformat avcodec swscale avutil swresample)