# cmake_minimum_required(VERSION <specify CMake version here>)
project(lutf C)

set(CMAKE_C_STANDARD 99)
find_package(nng CONFIG REQUIRED)
find_package(Threads)
add_executable(lutf core/worker/work_server.c core/worker/func_server.c core/worker/worker.c
        core/worker/work_server.h core/worker/func_server.h core/worker/coroutine.c core/worker/coroutine.h)
target_link_libraries(lutf nng::nng dl ${CMAKE_THREAD_LIBS_INIT} m)

add_executable(worker_test core/worker/worker_test.c)
target_link_libraries(worker_test nng::nng dl ${CMAKE_THREAD_LIBS_INIT})

add_library(work SHARED core/worker/work_load.c)
target_link_libraries(work m)