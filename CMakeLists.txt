cmake_minimum_required(VERSION 4.0)

project(shuff C)

# set(CMAKE_C_COMPILER clang)
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -s")
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

add_executable(shuff
        src/main.c
        src/huffman.c
        src/huffman.h
        src/progress_bar.c
        src/progress_bar.h
)

# Compiler warnings
if (MSVC)
    target_compile_options(shuff PRIVATE /W4)
else ()
    target_compile_options(shuff PRIVATE
            -Wall
            -Wextra
            -Wpedantic
    )
endif ()
