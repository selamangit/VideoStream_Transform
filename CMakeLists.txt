cmake_minimum_required(VERSION 3.10)
project(VideoStream_transorm)

#cv
find_package(OpenCV REQUIRED)

set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -O3  -Wall -std=c++11")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)

# 在根目录下导入之后子目录下的CMakeLists.txt就不需要写了，这是为什么？
set(ffmpeg_include_DIR /usr/local/ffmpeg/include)
set(ffmpeg_lib_DIR /usr/local/ffmpeg/lib)
#ffmpeg
include_directories(
    ${PROJECT_SOURCE_DIR}/include
    ${ffmpeg_include_DIR}
    ${OpenCV_INCLUDE_DIRS}
)

link_directories(
    ${PROJECT_SOURCE_DIR}/lib
    ${ffmpeg_lib_DIR}
)

# add_subdirectory(${PROJECT_SOURCE_DIR}/src)

aux_source_directory(src/ DIR_SRCS)

add_executable(main ${DIR_SRCS})


# 必须要这样写？
target_link_libraries(main avcodec avformat avutil swresample swscale avfilter x264)