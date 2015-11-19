project(medialibrary)

include(ExternalProject)

# Fetch gtest
ExternalProject_Add(
    gtest-dependency
    CMAKE_COMMAND cmake -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}
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

cmake_minimum_required(VERSION 2.8)
include(cpp11)

if (UNIX)
    option(WITH_ASAN "Build with address sanitizer" OFF)
    option(WITH_TSAN "Build with thread sanitizer" OFF)
    option(WITH_UBSAN "Build with undefined behavior sanitizer" OFF)
endif()

EnableCpp11()
add_definitions("-Wall -Wextra -pedantic")

include_directories(${SQLITE3_INCLUDE_DIR})

include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR}/src)
include_directories(${CMAKE_SOURCE_DIR}/test)
include_directories(${LIBVLC_INCLUDE_DIR})

include(unittest/unittest.cmake)
include(samples/samples.cmake)

include_directories(${LIBVLCPP_DIR})
# Also add pthread, as gtest requires it
if(UNIX)
    target_link_libraries(unittest "pthread")
    target_link_libraries(samples "pthread")

    if(${WITH_ASAN})
        if(${WITH_TSAN} OR ${WITH_UBSAN})
            message(SEND_ERROR "Sanitizers are mutually exclusive")
        endif()
        add_definitions("-fsanitize=address -fno-omit-frame-pointer")
        target_link_libraries(unittest "asan")
        target_link_libraries(samples "asan")
    endif()

    if(${WITH_TSAN})
        if(${WITH_ASAN} OR ${WITH_UBSAN})
            message(SEND_ERROR "Sanitizers are mutually exclusive")
        endif()
        add_definitions("-fsanitize=thread -fno-omit-frame-pointer")
        target_link_libraries(unittest "tsan")
        target_link_libraries(samples "tsan")
    endif()

    if(${WITH_UBSAN})
        if(${WITH_ASAN} OR ${WITH_TSAN})
            message(SEND_ERROR "Sanitizers are mutually exclusive")
        endif()
        add_definitions("-fsanitize=undefined -fno-omit-frame-pointer")
        target_link_libraries(unittest "ubsan")
        target_link_libraries(samples "ubsan")
    endif()
endif()
