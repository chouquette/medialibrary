#ifndef IALBUM_H
#define IALBUM_H

#include <memory>
#include <string>
#include <vector>

class IAlbumTrack;

class IAlbum
{
    public:
        virtual ~IAlbum() {}
        virtual const std::string& name() = 0;
        virtual unsigned int releaseYear() = 0;
        virtual const std::string& shortSummary() = 0;
        virtual const std::string& artworkUrl() = 0;
        virtual const std::vector<std::shared_ptr<IAlbumTrack>>& tracks() = 0;
};

#endif // IALBUM_H
