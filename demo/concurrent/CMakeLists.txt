cmake_minimum_required(VERSION 3.13)
if(COMMAND cmake_policy)
    cmake_policy(SET CMP0003 NEW)
endif(COMMAND cmake_policy)

project(sleep C)

SET(CMAKE_BUILD_TYPE "Debug")
SET(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -O0 -Wall -g -ggdb")
SET(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -O3 -Wall")

set(CMAKE_C_STANDARD 99)
find_package(nng CONFIG REQUIRED)
find_package(Threads)

add_executable(client ./client.c ../../core/worker/worker_tools.c ../../core/worker/worker_tools.h)
target_link_libraries(client nng::nng dl ${CMAKE_THREAD_LIBS_INIT})

add_executable(add_func ./add_func.c ../../core/worker/worker_tools.c ../../core/worker/worker_tools.h)
target_link_libraries(add_func nng::nng dl ${CMAKE_THREAD_LIBS_INIT})

add_library(sleep SHARED ./sleep.c)
set_target_properties(sleep PROPERTIES PREFIX "")
set_target_properties(sleep PROPERTIES SUFFIX ".lf")
