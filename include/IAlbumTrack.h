#ifndef IALBUMTRACK_H
#define IALBUMTRACK_H

#include "IMediaLibrary.h"

class IAlbum;

class IAlbumTrack
{
    public:
        virtual ~IAlbumTrack() {}

        virtual unsigned int id() const = 0;
        virtual const std::string& genre() = 0;
        virtual bool setGenre( const std::string& genre ) = 0;
        virtual const std::string& title() = 0;
        virtual unsigned int trackNumber() = 0;
        virtual std::shared_ptr<IAlbum> album() = 0;
        virtual bool files( std::vector<FilePtr>& files ) = 0;
        /**
         * @brief destroy Deletes the album track and the file(s) associated
         */
        virtual bool destroy() = 0;
};

#endif // IALBUMTRACK_H
