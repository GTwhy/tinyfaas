# cmake_minimum_required(VERSION <specify CMake version here>)
project(lutf C)

SET(CMAKE_BUILD_TYPE "Debug")
SET(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -O0 -Wall -g -ggdb")
SET(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -O3 -Wall")

set(CMAKE_C_STANDARD 99)
find_package(nng CONFIG REQUIRED)
find_package(Threads)

add_executable(lutf core/worker/work_server.c core/worker/func_server.c core/worker/worker.c
        core/worker/work_server.h core/worker/func_server.h core/worker/runtime.c core/worker/runtime.h)
target_link_libraries(lutf nng::nng dl ${CMAKE_THREAD_LIBS_INIT} m pthread)

add_executable(worker_test core/worker/worker_test.c)
target_link_libraries(worker_test nng::nng ${CMAKE_THREAD_LIBS_INIT} pthread)

add_library(work SHARED core/worker/work_load.c)
target_link_libraries(work m)