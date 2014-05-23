#ifndef IALBUM_H
#define IALBUM_H

#include "IMediaLibrary.h"

class IAlbumTrack;

class IAlbum
{
    public:
        virtual ~IAlbum() {}
        virtual unsigned int id() const = 0;
        virtual const std::string& name() = 0;
        virtual unsigned int releaseYear() = 0;
        virtual const std::string& shortSummary() = 0;
        virtual const std::string& artworkUrl() = 0;
        virtual bool tracks( std::vector<std::shared_ptr<IAlbumTrack>>& tracks ) = 0;
        virtual AlbumTrackPtr addTrack( const std::string& name, unsigned int trackId ) = 0;
};

#endif // IALBUM_H
