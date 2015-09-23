project(medialibrary)

cmake_minimum_required(VERSION 2.8)
include(ExternalProject)
include(cpp11)

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

EnableCpp11()
add_definitions("-Wall -Wextra -pedantic")

include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR}/src)
include_directories(${LIBVLC_INCLUDE_DIR})

list(APPEND TEST_SRCS
    Files.cpp
    Folders.cpp
    Labels.cpp
    Albums.cpp
    Tests.cpp
    Shows.cpp
    Movies.cpp
    VideoTracks.cpp
    AudioTracks.cpp
    VLCMetadataServices.cpp
    FsUtilsTests.cpp

    mocks/FileSystem.h
)

if (${EMOTION_FOUND})
list (APPEND TEST_SRCS
    EmotionMetadataService_Test.cpp
)
endif()

add_executable(unittest ${TEST_SRCS})
add_dependencies(unittest gtest-dependency)

target_link_libraries(unittest medialibrary)
target_link_libraries(unittest gtest gtest_main)
include_directories(${LIBVLCPP_DIR})
# Also add pthread, as gtest requires it
if(UNIX)
    target_link_libraries(unittest "pthread")
endif()

## Download files for parsing unit tests
file(DOWNLOAD "http://streams.videolan.org/streams/mp3/05-Mr.%20Zebra.mp3" "${CMAKE_CURRENT_BINARY_DIR}/mr-zebra.mp3")
file(DOWNLOAD "http://streams.videolan.org/streams/mp4/Mr_MrsSmith-h264_aac.mp4" "${CMAKE_CURRENT_BINARY_DIR}/mrmssmith.mp4")
