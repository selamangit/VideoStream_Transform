# opencv
# cmake_minimum_required(VERSION 3.10)
# project(VideoStream_transorm)
# find_package(OpenCV REQUIRED)

# # 添加Opencv头文件
# include_directories(${OpenCV_INCLUDE_DIRS})

# # message(STATUS "OpenCV_INCLUDE_DIRS = ${OpenCV_INCLUDE_DIRS}")
# # message(STATUS "OpenCV_LIBS = ${OpenCV_LIBS}")

# aux_source_directory(. DIR_SRCS)

# add_executable(main ${DIR_SRCS})
# # 将库文件链接到可执行程序上
# target_link_libraries(main ${OpenCV_LIBS})


# ffmpeg
cmake_minimum_required(VERSION 2.8)
project(VideoStream_transorm)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)

include_directories("/usr/local/ffmpeg/include")

link_directories("/usr/local/ffmpeg/lib")

aux_source_directory(. DIR_SRCS)

add_executable(main ${DIR_SRCS})
