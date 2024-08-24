cmake_minimum_required(VERSION 3.16)
project(libavpp LANGUAGES CXX)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
option(BUILD_SHARED_LIBS "Build shared libraries" ON)

include(FetchContent)
FetchContent_Declare(SFML
    GIT_REPOSITORY https://github.com/SFML/SFML.git
    GIT_TAG master)
FetchContent_MakeAvailable(SFML)

FetchContent_Declare(portaudio-pp
	GIT_REPOSITORY https://github.com/trustytrojan/portaudio-pp
	GIT_TAG main)
FetchContent_MakeAvailable(portaudio-pp)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(audio-player src/tests/audio-player.cpp)
add_executable(display-attached-pic src/tests/display-attached-pic.cpp)
add_executable(video-player src/tests/video-player.cpp)

target_include_directories(audio-player PRIVATE ${portaudio-pp_SOURCE_DIR}/include)
target_include_directories(video-player PRIVATE ${portaudio-pp_SOURCE_DIR}/include)

target_link_libraries(audio-player PRIVATE portaudio avfilter avformat avcodec avutil swresample)
target_link_libraries(display-attached-pic PRIVATE sfml-graphics avfilter avformat avcodec avutil)
target_link_libraries(video-player PRIVATE sfml-graphics portaudio avfilter avformat avcodec avutil swscale)
