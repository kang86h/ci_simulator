cmake_minimum_required(VERSION 3.20)
project(ci_rt_simulator LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Prefer static runtime on MSVC for portable single-EXE builds
if(MSVC)
  # /MT for Release, /MTd for Debug
  set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# Find PortAudio (via vcpkg recommended)
find_package(portaudio CONFIG REQUIRED)

add_executable(ci_rt_sim
  src/main.cpp
  src/biquad.h
  src/filters.h
  src/ci_processor.h
  src/ci_processor.cpp
  src/audio_io.h
  src/audio_io.cpp
)

target_include_directories(ci_rt_sim PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
# Resolve PortAudio target name across static/dynamic builds
if (TARGET portaudio)
  set(PORTAUDIO_TGT portaudio)
elseif (TARGET portaudio_static)
  set(PORTAUDIO_TGT portaudio_static)
elseif (TARGET PortAudio::PortAudio)
  set(PORTAUDIO_TGT PortAudio::PortAudio)
else()
  message(FATAL_ERROR "No suitable PortAudio target found after find_package")
endif()

target_link_libraries(ci_rt_sim PRIVATE ${PORTAUDIO_TGT})

if(MSVC)
  target_compile_options(ci_rt_sim PRIVATE /W4 /permissive-)
else()
  target_compile_options(ci_rt_sim PRIVATE -Wall -Wextra -Wpedantic)
endif()


