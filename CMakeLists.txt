project(medialibrary)
cmake_minimum_required(VERSION 2.8)
set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake ${CMAKE_MODULE_PATH})

option(BUILD_TESTS "Shall unit tests be built?" ON)

add_subdirectory( src )
if (${BUILD_TESTS})
    add_subdirectory( test )
endif()
