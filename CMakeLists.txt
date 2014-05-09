project(medialibrary)
cmake_minimum_required(VERSION 2.8)
list(APPEND SRC_LIST
    include/IAudioTrackInformation.h
    include/ITrackInformation.h
    include/IDescription.h
    include/IVideoTrackInformation.h
    include/IShow.h
    include/IShowEpisode.h
    include/IAlbum.h
    include/IAlbumTrack.h
    include/IFile.h
)
add_executable(${PROJECT_NAME} ${SRC_LIST})

