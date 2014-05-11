project(medialibrary)

cmake_minimum_required(VERSION 2.8)
include(ExternalProject)

# Fetch gtest
ExternalProject_Add(
    gtest-dependency
    SVN_REPOSITORY http://googletest.googlecode.com/svn/trunk/
    TIMEOUT 10
    # Disable install step
    INSTALL_COMMAND ""
    UPDATE_COMMAND ""
    # Wrap download, configure and build steps in a script to log output
    LOG_DOWNLOAD ON
    LOG_CONFIGURE ON
    LOG_BUILD ON
)

ExternalProject_Get_Property(gtest-dependency source_dir)
include_directories(${source_dir}/include)
# Also link with gtest:
# fetch the directory which contains the built libraries (gtest & gtest_main)
ExternalProject_Get_Property(gtest-dependency binary_dir)
link_directories(${binary_dir})

add_definitions("-g")
add_definitions("-Wall -Wextra")
include_directories(${CMAKE_SOURCE_DIR}/include)

list(APPEND TEST_SRCS
    Tests.cpp
)

add_executable(unittest ${TEST_SRCS})
add_dependencies(unittest gtest-dependency)

target_link_libraries(unittest medialibrary)
target_link_libraries(unittest gtest gtest_main)
# Also add pthread, as gtest requires it
if(UNIX)
    target_link_libraries(unittest "pthread")
endif()
