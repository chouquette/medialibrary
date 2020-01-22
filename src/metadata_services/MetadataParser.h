/*****************************************************************************
 * Media Library
 *****************************************************************************
 * Copyright (C) 2015-2019 Hugo Beauzée-Luyssen, Videolabs, VideoLAN
 *
 * Authors: Hugo Beauzée-Luyssen <hugo@beauzee.fr>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
 *****************************************************************************/

#pragma once

#include "medialibrary/parser/IParserService.h"
#include "medialibrary/IInterruptProbe.h"
#include "medialibrary/IMedia.h"

#include <atomic>

namespace medialibrary
{

class AlbumTrack;
class Thumbnail;
class Playlist;
class Media;
class File;
class Artist;
class Album;
class Genre;
class MediaLibrary;
class ModificationNotifier;
class Show;

namespace parser
{

class MetadataAnalyzer : public IParserService, public IInterruptProbe
{
public:
    MetadataAnalyzer();

protected:
    bool cacheUnknownArtist();
    bool cacheUnknownShow();
    virtual bool initialize( IMediaLibrary* ml ) override;
    virtual Status run( IItem& item ) override;
    virtual const char* name() const override;
    virtual void onFlushing() override;
    virtual void onRestarted() override;
    virtual Step targetedStep() const override;
    virtual void stop() override;

    virtual bool isInterrupted() const override;

    Status parsePlaylist( IItem& item ) const;
    void addPlaylistElement( IItem& item, std::shared_ptr<Playlist> playlistPtr,
                             const IItem& subitem ) const;
    Status parseAudioFile( IItem& task );
    bool parseVideoFile( IItem& task ) const;
    Status createFileAndMedia( IItem& item ) const;
    Status overrideExternalMedia(IItem& item, std::shared_ptr<Media> media,
                                  std::shared_ptr<File> file, IMedia::Type newType ) const;
    void createTracks( Media& m, const std::vector<IItem::Track>& tracks ) const;
    std::tuple<bool, bool> refreshFile( IItem& item ) const;
    std::tuple<bool, bool> refreshMedia( IItem& item ) const;
    std::tuple<bool, bool> refreshPlaylist( IItem& item ) const;
    std::pair<std::shared_ptr<Artist>, std::shared_ptr<Artist>> findOrCreateArtist( IItem& item ) const;
    std::shared_ptr<AlbumTrack> handleTrack( std::shared_ptr<Album> album, IItem& item,
                                             std::shared_ptr<Artist> artist, Genre* genre ) const;
    void link( IItem& item, Album& album, std::shared_ptr<Artist> albumArtist,
               std::shared_ptr<Artist> artist, std::shared_ptr<Thumbnail> thumbnail );
    std::shared_ptr<Album> findAlbum( IItem& item, std::shared_ptr<Artist> albumArtist,
                                        std::shared_ptr<Artist> artist );
    std::shared_ptr<Genre> handleGenre( IItem& item ) const;
    std::shared_ptr<Thumbnail> findAlbumArtwork( IItem& item );
    std::shared_ptr<Show> findShow( const std::string& showName ) const;
    bool assignMediaToGroup( IItem& item ) const;

private:
    static int toInt( IItem& item, IItem::Metadata meta );

private:
    MediaLibrary* m_ml;
    std::shared_ptr<ModificationNotifier> m_notifier;

    std::shared_ptr<Artist> m_unknownArtist;
    std::shared_ptr<Artist> m_variousArtists;
    std::shared_ptr<Album> m_previousAlbum;
    std::shared_ptr<Show> m_unknownShow;
    int64_t m_previousFolderId;
    std::atomic_bool m_stopped;
};

}
}
