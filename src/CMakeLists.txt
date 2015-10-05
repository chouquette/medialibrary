project(medialibrary)
include(cpp11)
cmake_minimum_required(VERSION 2.8)
set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake ${CMAKE_MODULE_PATH})
option(WITH_VLC "Include VLC metadata services" ON)

EnableCpp11()

include(FindPkgConfig)
pkg_check_modules(EMOTION emotion)
pkg_check_modules(ECOREEVAS ecore-evas)
pkg_check_modules(EVAS evas)

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
    ${CMAKE_SOURCE_DIR}/include/IDiscoverer.h
    ${CMAKE_SOURCE_DIR}/include/filesystem/IDirectory.h
    ${CMAKE_SOURCE_DIR}/include/filesystem/IFile.h
    ${CMAKE_SOURCE_DIR}/include/factory/IFileSystem.h
    ${CMAKE_SOURCE_DIR}/include/ILogger.h
    ${CMAKE_SOURCE_DIR}/include/IArtist.h

    database/Cache.h
    database/SqliteTools.h

    filesystem/${ARCH_FOLDER}/Directory.h
    filesystem/${ARCH_FOLDER}/File.h

    discoverer/FsDiscoverer.h

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
    Artist.cpp

    factory/FileSystem.cpp
    filesystem/${ARCH_FOLDER}/Directory.cpp
    filesystem/${ARCH_FOLDER}/File.cpp

    discoverer/FsDiscoverer.cpp

    Utils.cpp

    logging/IostreamLogger.cpp
    logging/Logger.cpp
)

if (${EMOTION_FOUND})
    list(APPEND SRC_LIST
        metadata_services/emotion/Emotion.cpp
    )
endif()

if (${WITH_VLC})
    list(APPEND SRC_LIST
        metadata_services/vlc/VLCMetadataService.cpp
        metadata_services/vlc/VLCThumbnailer.cpp
    )
endif()

find_package(Sqlite3 REQUIRED)

add_library(${PROJECT_NAME} SHARED ${SRC_LIST})
target_link_libraries(${PROJECT_NAME} ${SQLITE3_LIBRARIES})

if (${WITH_VLC})
    find_package(LIBVLC REQUIRED)
    # Since VLC include a thumbnailer, we need libjpeg to encode the result
    find_package(JPEG)
    target_link_libraries( ${PROJECT_NAME} ${LIBVLC_LIBRARY} )
    include_directories(${LIBVLC_INCLUDE_DIR})
    if (${JPEG_FOUND})
        include_directories(${JPEG_INCLUDE_DIR})
        target_link_libraries( ${PROJECT_NAME} ${JPEG_LIBRARIES} )
        add_definitions(-DWITH_JPEG)
    endif()
    set(LIBVLCPP_DIR "" CACHE PATH "Path to libvlcpp's directory")
    include_directories(${LIBVLCPP_DIR})
endif()

if (${EMOTION_FOUND})
    include_directories(${EMOTION_INCLUDE_DIRS})
    include_directories(${ECOREEVAS_INCLUDE_DIRS})
    target_link_libraries( ${PROJECT_NAME} ${EMOTION_LIBRARIES} )
    target_link_libraries( ${PROJECT_NAME} ${ECOREEVAS_LIBRARIES} )
    target_link_libraries( ${PROJECT_NAME} ${EVAS_LIBRARIES} )
endif()
