list(APPEND TEST_SRCS
    unittest/MediaTests.cpp
    unittest/FolderTests.cpp
    unittest/LabelTests.cpp
    unittest/AlbumTests.cpp
    unittest/Tests.cpp
    unittest/ShowTests.cpp
    unittest/MovieTests.cpp
    unittest/VideoTrackTests.cpp
    unittest/AudioTrackTests.cpp
    unittest/FsUtilsTests.cpp
    unittest/ArtistTests.cpp
    unittest/AlbumTrackTests.cpp
    unittest/DeviceTests.cpp
    unittest/FileTests.cpp

    mocks/FileSystem.h
    mocks/FileSystem.cpp
    mocks/DiscovererCbMock.h
)

add_executable(unittest ${TEST_SRCS})
add_dependencies(unittest gtest-dependency)
target_link_libraries(unittest medialibrary)
target_link_libraries(unittest gtest gtest_main)
