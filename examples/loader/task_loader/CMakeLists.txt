cmake_minimum_required(VERSION 3.10)
project(worker C)

set(CMAKE_C_STANDARD 99)
add_executable(worker main.c)
target_link_libraries(worker ${CMAKE_DL_LIBS} )
