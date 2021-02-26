cmake_minimum_required(VERSION 3.17)
project(fork_test C)

set(CMAKE_C_STANDARD 99)

add_executable(load load.c)
target_link_libraries(load dl)

add_executable(fork fork.c)

add_executable(work work.c)
target_link_libraries(work m)

add_executable(work_loop work_loop.c)
target_link_libraries(work_loop m)

add_library(work.so SHARED work.c)
target_link_libraries(work.so m)

add_executable(coroutine main.c coroutine.c coroutine.h)
target_link_libraries(coroutine dl)
