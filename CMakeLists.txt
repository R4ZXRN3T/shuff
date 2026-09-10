cmake_minimum_required(VERSION 4.3)

project(shuff C)

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

add_executable(shuff
        src/main.c
        src/huffman.c
        src/huffman.h
)

# Compiler warnings
if(MSVC)
    target_compile_options(shuff PRIVATE /W4)
else()
    target_compile_options(shuff PRIVATE
            -Wall
            -Wextra
            -Wpedantic
    )
endif()