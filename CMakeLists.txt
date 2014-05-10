project(medialibrary)
cmake_minimum_required(VERSION 2.8)
set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake ${CMAKE_MODULE_PATH})

list(APPEND HEADERS_LIST
    include/IAudioTrackInformation.h
    include/ITrackInformation.h
    include/IDescription.h
    include/IVideoTrackInformation.h
    include/IShow.h
    include/IShowEpisode.h
    include/IAlbum.h
    include/IAlbumTrack.h
    include/IFile.h
    include/IMediaLibrary.h
)
include_directories("include")

list(APPEND SRC_LIST ${HEADERS_LIST}
    src/MediaLibrary.cpp
    src/SqliteTools.cpp
    src/File.cpp
    src/Album.cpp
    src/Show.cpp
    src/Label.cpp
    src/AlbumTrack.cpp
)

find_package(Sqlite3 REQUIRED)

add_library(${PROJECT_NAME} ${SRC_LIST})
target_link_libraries(${PROJECT_NAME} ${SQLITE3_LIBRARIES})

