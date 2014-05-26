#ifndef TYPES_H
#define TYPES_H

#include <memory>

class IAlbum;
class IAlbumTrack;
class IFile;
class ILabel;
class IMetadataService;
class IMovie;
class IShow;
class IShowEpisode;

typedef std::shared_ptr<IFile> FilePtr;
typedef std::shared_ptr<ILabel> LabelPtr;
typedef std::shared_ptr<IAlbum> AlbumPtr;
typedef std::shared_ptr<IAlbumTrack> AlbumTrackPtr;
typedef std::shared_ptr<IShow> ShowPtr;
typedef std::shared_ptr<IShowEpisode> ShowEpisodePtr;
typedef std::shared_ptr<IMovie> MoviePtr;

#endif // TYPES_H
