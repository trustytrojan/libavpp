include(FetchContent)

## sfml (without audio and network libs)
find_package(SFML COMPONENTS Graphics Window System)
if(NOT SFML_FOUND)
	message(STATUS "SFML not found, using FetchContent...")
    set(SFML_BUILD_AUDIO OFF)
    set(SFML_BUILD_NETWORK OFF)
	FetchContent_Declare(SFML URL https://github.com/SFML/SFML/archive/3.0.1.zip)
	FetchContent_MakeAvailable(SFML)
	find_package(SFML COMPONENTS Graphics Window System REQUIRED)
endif()

## portaudio
if(WIN32)
	file(DOWNLOAD https://github.com/spatialaudio/portaudio-binaries/raw/refs/heads/master/libportaudio64bit.dll ${CMAKE_BINARY_DIR}/portaudio_x64.dll)
	file(DOWNLOAD https://github.com/PortAudio/portaudio/raw/refs/tags/v19.7.0/include/portaudio.h ${CMAKE_BINARY_DIR}/portaudio.h)
	link_directories(${CMAKE_BINARY_DIR})
endif()

FetchContent_Declare(portaudio-pp URL https://github.com/trustytrojan/portaudio-pp/archive/main.zip)
FetchContent_MakeAvailable(portaudio-pp)

if(WIN32)
	link_libraries(portaudio_x64)
elseif(LINUX)
	link_libraries(portaudio)
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
link_libraries(libavpp SFML::Graphics portaudio-pp::portaudio-pp)

## tests
add_executable(audio-player audio-player.cpp)
add_executable(display-attached-pic display-attached-pic.cpp)
add_executable(filter-audio filter-audio.cpp)
add_executable(video-player video-player.cpp)
add_executable(vaapi-encode vaapi-encode.cpp)
add_executable(vaapi-encode-mux vaapi-encode-mux.cpp)
add_executable(vaapi-transcode vaapi-transcode.cpp)
add_executable(hw-decode hw-decode.cpp)
