cmake_minimum_required(VERSION 3.20)

add_library(mtdpartctl_if STATIC
    dev.c
)

target_include_directories(mtdpartctl_if PUBLIC
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/user/common
)

target_compile_options(mtdpartctl_if PRIVATE -Wall -Wextra -Werror)

# Optional: debug build support
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_options(mtdpartctl_if PRIVATE -O0 -g)
endif()

