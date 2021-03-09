# cmake_minimum_required(VERSION <specify CMake version here>)
project(lutf C)

set(CMAKE_C_STANDARD 99)
file(GLOB SRCS core/worker/**)
find_package(nng CONFIG REQUIRED)
find_package(Threads)
add_executable(lutf ${SRCS})
target_link_libraries(lutf nng::nng dl ${CMAKE_THREAD_LIBS_INIT} m)

add_executable(worker_test core/worker/worker_test.c)
target_link_libraries(worker_test nng::nng dl ${CMAKE_THREAD_LIBS_INIT})

add_library(work SHARED core/worker/work_load.c)
target_link_libraries(work m)