cmake_minimum_required(VERSION 3.16)

project(bmpconvert)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_compile_options(-O3)

add_executable(bmpconvert
    src/main.cpp
    src/format/bmp.cpp
    src/format/pixel_array.cpp
    src/format/pixel_array/packed.cpp
    src/format/pixel_array/expanded.cpp
)

target_include_directories(bmpconvert PRIVATE
    src
)