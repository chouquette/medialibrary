#ifndef IALBUMTRACK_H
#define IALBUMTRACK_H

#include <memory>
#include <string>

class IAlbum;

class IAlbumTrack
{
    public:
        virtual ~IAlbumTrack() {}

        virtual const std::string& genre() = 0;
        virtual const std::string& title() = 0;
        virtual unsigned int trackNumber() = 0;
        virtual std::shared_ptr<IAlbum> album() = 0;
};

#endif // IALBUMTRACK_H
