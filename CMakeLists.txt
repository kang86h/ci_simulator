cmake_minimum_required(VERSION 3.20)
project(ci_rt_simulator LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

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
target_link_libraries(ci_rt_sim PRIVATE portaudio)

if(MSVC)
  target_compile_options(ci_rt_sim PRIVATE /W4 /permissive-)
else()
  target_compile_options(ci_rt_sim PRIVATE -Wall -Wextra -Wpedantic)
endif()


