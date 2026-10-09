cmake_minimum_required(VERSION 3.16)
project(OS_lab2 C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_executable(child1 child1.c)
add_executable(child2 child2.c)
add_executable(parent parent.c)

target_compile_options(child1 PRIVATE -Wall -Wextra)
target_compile_options(child2 PRIVATE -Wall -Wextra)
target_compile_options(parent  PRIVATE -Wall -Wextra)

