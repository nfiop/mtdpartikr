cmake_minimum_required(VERSION 3.20)

add_library(mtdpartctl STATIC
    dev.c
)

target_include_directories(mtdpartctl PUBLIC
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/user/common
)

target_compile_options(mtdpartctl PRIVATE -Wall -Wextra -Werror)

# Optional: debug build support
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_options(mtdpartctl PRIVATE -O0 -g)
endif()

