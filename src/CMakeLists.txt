project(medialibrary)
include(cpp11)
cmake_minimum_required(VERSION 2.8)
set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake ${CMAKE_MODULE_PATH})

EnableCpp11()
add_definitions("-Wall -Wextra -pedantic -O0")

list(APPEND HEADERS_LIST
    ${CMAKE_SOURCE_DIR}/include/IAudioTrackInformation.h
    ${CMAKE_SOURCE_DIR}/include/ITrackInformation.h
    ${CMAKE_SOURCE_DIR}/include/IDescription.h
    ${CMAKE_SOURCE_DIR}/include/IVideoTrackInformation.h
    ${CMAKE_SOURCE_DIR}/include/IShow.h
    ${CMAKE_SOURCE_DIR}/include/IShowEpisode.h
    ${CMAKE_SOURCE_DIR}/include/IAlbum.h
    ${CMAKE_SOURCE_DIR}/include/IAlbumTrack.h
    ${CMAKE_SOURCE_DIR}/include/IFile.h
    ${CMAKE_SOURCE_DIR}/include/IMediaLibrary.h
    ${CMAKE_SOURCE_DIR}/include/ILabel.h
)
include_directories("${CMAKE_SOURCE_DIR}/include")

list(APPEND SRC_LIST ${HEADERS_LIST}
    MediaLibrary.cpp
    File.cpp
    Album.cpp
    Show.cpp
    Label.cpp
    AlbumTrack.cpp
    ShowEpisode.cpp
    Factory.cpp
)

find_package(Sqlite3 REQUIRED)

add_library(${PROJECT_NAME} ${SRC_LIST})
target_link_libraries(${PROJECT_NAME} ${SQLITE3_LIBRARIES})

