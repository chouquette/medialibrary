project(medialibrary)
include(cpp11)
cmake_minimum_required(VERSION 2.8)
set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake ${CMAKE_MODULE_PATH})

EnableCpp11()
add_definitions("-Wall -Wextra -pedantic")

if(UNIX)
    set(ARCH_FOLDER "unix")
endif()

list(APPEND HEADERS_LIST
    ${CMAKE_SOURCE_DIR}/include/IShow.h
    ${CMAKE_SOURCE_DIR}/include/IShowEpisode.h
    ${CMAKE_SOURCE_DIR}/include/IAlbum.h
    ${CMAKE_SOURCE_DIR}/include/IAlbumTrack.h
    ${CMAKE_SOURCE_DIR}/include/IFile.h
    ${CMAKE_SOURCE_DIR}/include/IFolder.h
    ${CMAKE_SOURCE_DIR}/include/IMediaLibrary.h
    ${CMAKE_SOURCE_DIR}/include/ILabel.h
    ${CMAKE_SOURCE_DIR}/include/IAudioTrack.h
    ${CMAKE_SOURCE_DIR}/include/IVideoTrack.h
    ${CMAKE_SOURCE_DIR}/include/IMovie.h

    ${CMAKE_SOURCE_DIR}/include/factory/IFileSystem.h

    Cache.h
    SqliteTools.h

    filesystem/IDirectory.h
    filesystem/IFile.h

    filesystem/${ARCH_FOLDER}/Directory.h
    filesystem/${ARCH_FOLDER}/File.h

    factory/FileSystem.h
    Utils.h
)

include_directories("${CMAKE_SOURCE_DIR}/include")
include_directories("${CMAKE_SOURCE_DIR}/src")

list(APPEND SRC_LIST ${HEADERS_LIST}
    MediaLibrary.cpp
    File.cpp
    Folder.cpp
    Album.cpp
    Show.cpp
    Label.cpp
    AlbumTrack.cpp
    ShowEpisode.cpp
    Factory.cpp
    Movie.cpp
    VideoTrack.cpp
    AudioTrack.cpp
    Parser.cpp

    metadata_services/VLCMetadataService.cpp

    factory/FileSystem.cpp
    filesystem/${ARCH_FOLDER}/Directory.cpp
    filesystem/${ARCH_FOLDER}/File.cpp
    Utils.cpp
)

find_package(Sqlite3 REQUIRED)

add_library(${PROJECT_NAME} ${SRC_LIST})
target_link_libraries(${PROJECT_NAME} ${SQLITE3_LIBRARIES})


find_package(LIBVLC REQUIRED)
target_link_libraries( ${PROJECT_NAME} ${LIBVLC_LIBRARY} ${LIBVLCCORE_LIBRARY} )
include_directories(${LIBVLC_INCLUDE_DIR})

find_package(vlcpp 2.2.0 REQUIRED)
target_link_libraries( ${PROJECT_NAME} ${LIBVLCPP_LIBRARIES})
include_directories(${LIBVLCPP_INCLUDE_DIR})
