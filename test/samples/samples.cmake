list(APPEND SAMPLE_TEST_SRCS
    samples/main.cpp
    samples/Tester.cpp
)

add_executable(samples ${SAMPLE_TEST_SRCS})
add_dependencies(samples gtest-dependency)
target_link_libraries(samples medialibrary)
target_link_libraries(samples gtest gtest_main)
