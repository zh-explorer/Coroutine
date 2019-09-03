cmake_minimum_required(VERSION 3.10)
project(Coroutine)

set(CMAKE_CXX_STANDARD 14)
find_package(Threads)
add_executable(Coroutine main.cpp Coroutine.cpp Coroutine.h async.cpp async.h aio.cpp aio.h unit/array_buf.cpp unit/array_buf.h log.cpp log.h poll.cpp poll.h aThread.cpp aThread.h)
target_link_libraries(Coroutine ${CMAKE_THREAD_LIBS_INIT})