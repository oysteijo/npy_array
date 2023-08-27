cmake_minimum_required(VERSION 3.18)

project(npy_arrays LANGUAGES C)

set(CMAKE_C_STANDARD 11)

include(FetchContent)
FetchContent_Declare(
    libzip
    GIT_REPOSITORY https://github.com/nih-at/libzip.git
    GIT_TAG v1.10.0
)
FetchContent_MakeAvailable(libzip)

add_library(npy_arrays STATIC
    npy_array.c
    npy_array.h
    npy_array_list.c
    npy_array_list.h
)

set(libzip_INCLUDE_DIR ${libzip_SOURCE_DIR}/lib)
include_directories(
    ${libzip_INCLUDE_DIR}
    ${libzip_BINARY_DIR}
)

target_link_libraries(npy_arrays zip)
