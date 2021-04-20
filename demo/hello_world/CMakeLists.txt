cmake_minimum_required(VERSION 3.13)
if(COMMAND cmake_policy)
    cmake_policy(SET CMP0003 NEW)
endif(COMMAND cmake_policy)

project(hello C)

SET(CMAKE_BUILD_TYPE "Debug")
SET(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -O0 -Wall -g -ggdb")
SET(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -O3 -Wall")

set(CMAKE_C_STANDARD 99)
find_package(nng CONFIG REQUIRED)
find_package(Threads)

add_executable(hello_client ./hello_client.c ../../core/worker/worker_tools.c ../../core/worker/worker_tools.h)
target_link_libraries(hello_client nng::nng dl ${CMAKE_THREAD_LIBS_INIT})

add_library(hello_world SHARED ./hello_world.c)
set_target_properties(hello_world PROPERTIES PREFIX "")
set_target_properties(hello_world PROPERTIES SUFFIX ".lf")
