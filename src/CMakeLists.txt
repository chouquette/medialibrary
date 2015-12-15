project(medialibrary)
include(cpp11)
cmake_minimum_required(VERSION 2.8)
set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake ${CMAKE_MODULE_PATH})
option(WITH_VLC "Include VLC metadata services" ON)

EnableCpp11()

include(FindPkgConfig)
pkg_check_modules(EVAS evas)

add_definitions("-Wall -Wextra -pedantic")

if(UNIX)
    set(ARCH_FOLDER "unix")
elseif(WIN32)
    set(ARCH_FOLDER "win32")
endif()

list(APPEND HEADERS_LIST
    ${CMAKE_SOURCE_DIR}/include/IShow.h
    ${CMAKE_SOURCE_DIR}/include/IShowEpisode.h
    ${CMAKE_SOURCE_DIR}/include/IAlbum.h
    ${CMAKE_SOURCE_DIR}/include/IAlbumTrack.h
    ${CMAKE_SOURCE_DIR}/include/IMedia.h
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

    database/SqliteTools.h

    filesystem/${ARCH_FOLDER}/Directory.h
    filesystem/${ARCH_FOLDER}/File.h

    discoverer/FsDiscoverer.h

    factory/FileSystem.h
    utils/Filename.h
)

include_directories("${CMAKE_SOURCE_DIR}/include")
include_directories("${CMAKE_SOURCE_DIR}/src")

list(APPEND SRC_LIST ${HEADERS_LIST}
    MediaLibrary.cpp
    Media.cpp
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
    Settings.cpp
    Device.cpp

    factory/FileSystem.cpp
    filesystem/common/CommonFile.cpp
    filesystem/${ARCH_FOLDER}/Directory.cpp
    filesystem/${ARCH_FOLDER}/File.cpp
    filesystem/${ARCH_FOLDER}/Device.cpp

    discoverer/FsDiscoverer.cpp
    discoverer/DiscovererWorker.cpp

    utils/Filename.cpp

    database/SqliteConnection.cpp
    database/SqliteTransaction.cpp

    logging/IostreamLogger.cpp
    logging/Logger.cpp
)

if (${WITH_VLC})
    list(APPEND SRC_LIST
        metadata_services/vlc/VLCMetadataService.cpp
        metadata_services/vlc/VLCThumbnailer.cpp
    )
endif()

find_package(Sqlite3 REQUIRED)
include_directories(${SQLITE3_INCLUDE_DIR})

add_library(${PROJECT_NAME} SHARED ${SRC_LIST})
target_link_libraries(${PROJECT_NAME} ${SQLITE3_LIBRARIES})

if (${WITH_VLC})
    find_package(LIBVLC REQUIRED)
    # Since VLC include a thumbnailer, we need libjpeg to encode the result
    find_package(JPEG)
    target_link_libraries( ${PROJECT_NAME} ${LIBVLC_LIBRARY} )
    include_directories(${LIBVLC_INCLUDE_DIR})
    if (${EVAS_FOUND})
        include_directories(${EVAS_INCLUDE_DIRS})
        target_link_libraries(${PROJECT_NAME} ${EVAS_LIBRARIES})
        add_definitions("-DWITH_EVAS")
    elseif (${JPEG_FOUND})
        include_directories(${JPEG_INCLUDE_DIR})
        target_link_libraries( ${PROJECT_NAME} ${JPEG_LIBRARIES} )
        add_definitions("-DWITH_JPEG")
    endif()
    set(LIBVLCPP_DIR "" CACHE PATH "Path to libvlcpp's directory")
    include_directories(${LIBVLCPP_DIR})
endif()

# Work around for tizen platform
if (NOT "${EXTRA_LIBS}" STREQUAL "")
    target_link_libraries( ${PROJECT_NAME} ${EXTRA_LIBS} )
endif()
