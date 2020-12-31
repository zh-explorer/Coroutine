cmake_minimum_required(VERSION 3.10)
project(coro)

set(CMAKE_CXX_STANDARD 14)
find_package(Threads)
#SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3")
#SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pg")
#SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -pg")
#SET(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -pg")
#set(CMAKE_CXX_FLAGS -fsanitize=address)
#set(CMAKE_CXX_FLAGS -fsanitize-recover=address)

add_library(Coroutine SHARED)

include(async/CMakeLists.txt)
include(asyncIO/CMakeLists.txt)
include(coro/CMakeLists.txt)
include(ThreadPool/CMakeLists.txt)
include(unit/CMakeLists.txt)

target_link_libraries(Coroutine ${CMAKE_THREAD_LIBS_INIT})

#add_library(Coroutine SHARED)
#target_link_libraries(Coroutine Coroutine)

add_executable(Coroutine_bin main.cpp)
target_link_libraries(Coroutine_bin Coroutine)

target_include_directories(Coroutine PUBLIC
        "${PROJECT_BINARY_DIR}"
        "${PROJECT_SOURCE_DIR}/async"
        "${PROJECT_SOURCE_DIR}/asyncIO"
        "${PROJECT_SOURCE_DIR}/coro"
        "${PROJECT_SOURCE_DIR}/ThreadPool"
        "${PROJECT_SOURCE_DIR}/unit"
        )

file(GLOB header */*.h */*.hpp)

set_target_properties(Coroutine PROPERTIES PUBLIC_HEADER "${header}")

target_include_directories(Coroutine_bin PUBLIC
        "${PROJECT_BINARY_DIR}"
        "${PROJECT_SOURCE_DIR}/async"
        "${PROJECT_SOURCE_DIR}/asyncIO"
        "${PROJECT_SOURCE_DIR}/coro"
        "${PROJECT_SOURCE_DIR}/ThreadPool"
        "${PROJECT_SOURCE_DIR}/unit"
        )

SET(CMAKE_INSTALL_PREFIX "/usr")

install(TARGETS Coroutine
        LIBRARY DESTINATION ${INSTALL_LIBDIR} lib
        PUBLIC_HEADER DESTINATION include/Coroutine)
