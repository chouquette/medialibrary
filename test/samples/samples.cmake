list(APPEND SAMPLE_TEST_SRCS
    samples/main.cpp
)

add_executable(samples ${SAMPLE_TEST_SRCS})

add_dependencies(samples gtest-dependency)
target_link_libraries(samples medialibrary)
target_link_libraries(samples gtest gtest_main)

add_definitions("-DSRC_DIR=\"${CMAKE_SOURCE_DIR}\"")
