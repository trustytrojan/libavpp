cmake_minimum_required(VERSION 3.16)
project(libavpp LANGUAGES CXX)
cmake_policy(SET CMP0135 NEW)
cmake_policy(SET CMP0167 NEW)

set(BUILD_SHARED_LIBS OFF)
set(CMAKE_CXX_STANDARD 23)
set(FFMPEG_LIBS avfilter avformat avcodec avutil swresample swscale)

option(LIBAVPP_BUILD_EXAMPLES "Build libavpp example programs" OFF)

foreach(LIB IN LISTS FFMPEG_LIBS)
    find_library(${LIB} NAMES ${LIB} REQUIRED)
endforeach()

add_library(libavpp INTERFACE)
target_include_directories(libavpp INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(libavpp INTERFACE ${FFMPEG_LIBS})

if(LIBAVPP_BUILD_EXAMPLES)
	add_subdirectory(examples)
endif()
