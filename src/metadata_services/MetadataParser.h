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

#include "medialibrary/parser/IItem.h"
#include "medialibrary/parser/IParserService.h"
#include "medialibrary/IMedia.h"
#include "compat/Mutex.h"

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

class MetadataAnalyzer : public IParserService
{
public:
    MetadataAnalyzer();

protected:
    struct Cache
    {
        std::shared_ptr<Album> previousAlbum;
        int64_t previousFolderId = 0;
    };
    bool cacheUnknownArtist();
    bool cacheUnknownShow();
    virtual bool initialize( IMediaLibrary* ml ) override;
    virtual Status run( IItem& item ) override;
    virtual const char* name() const override;
    virtual void onFlushing() override;
    virtual void onRestarted() override;
    virtual Step targetedStep() const override;
    virtual void stop() override;

    Status parsePlaylist( IItem& item ) const;
    void addPlaylistElement(IItem& item, int64_t playlistId,
                             const std::string& mrl, const std::string& itemTitle,
                             int64_t itemIdx ) const;
    void addFolderToPlaylist(IItem& item, std::shared_ptr<Playlist> playlistPtr , const IItem& subitem) const;
    Status parseAudioFile( IItem& task, Cache& cache );
    bool parseVideoFile( IItem& task ) const;
    Status createFileAndMedia( IItem& item ) const;
    Status overrideExternalMedia( IItem& item, Media& media,
                                  File& file, IMedia::Type newType ) const;
    void createTracks( Media& m, const std::vector<IItem::Track>& tracks ) const;
    std::tuple<bool, bool> refreshFile( IItem& item ) const;
    std::tuple<bool, bool> refreshMedia( IItem& item ) const;
    std::tuple<bool, bool> refreshPlaylist( IItem& item ) const;
    std::tuple<bool, bool> refreshSubscription( IItem& item ) const;
    std::pair<std::shared_ptr<Artist>, std::shared_ptr<Artist>> findOrCreateArtist( IItem& item ) const;
    std::shared_ptr<Media> handleTrack(Album& album, IItem& item,
                                             int64_t artistId, Genre* genre ) const;
    void link( IItem& item, Album& album, std::shared_ptr<Artist> albumArtist,
               std::shared_ptr<Artist> artist, bool newAlbum,
               std::shared_ptr<Thumbnail> thumbnail );
    std::shared_ptr<Thumbnail> fetchThumbnail( IItem& item, Album* album );
    void assignThumbnails( Media& media, Album &album,
                           Artist& albumArtist, bool newAlbum,
                           std::shared_ptr<Thumbnail> thumbnail );
    std::shared_ptr<Album> findAlbum(IItem& item, const std::string& albumName,
                                      Artist* albumArtist, Artist* artist , Cache& cache);
    std::shared_ptr<Album> handleUnknownAlbum( Artist* albumArtist,
                                               Artist* trackArtist );
    std::shared_ptr<Album> createUnknownAlbum( Artist* albumArtist,
                                               Artist* trackArtist );
    std::shared_ptr<Genre> handleGenre( IItem& item ) const;
    std::shared_ptr<Thumbnail> findAlbumArtwork( IItem& item );
    std::shared_ptr<Show> findShow( const std::string& showName ) const;
    bool assignMediaToGroup( IItem& item ) const;
    Status parseSubscription( IItem& item ) const;
    void addSubscriptionElement( const IItem& item, int64_t subscriptionId,
                                 std::string mrl, std::string title ) const;

private:
    static int toInt( IItem& item, IItem::Metadata meta );
    IMedia::Type guessMediaType( const IItem &item ) const;

    // The value at which we consider a date to be a timestamp and not a year
    static constexpr uint32_t TimestampThreshold = 3000;

private:
    MediaLibrary* m_ml;
    std::shared_ptr<ModificationNotifier> m_notifier;

    std::shared_ptr<Artist> m_unknownArtist;
    std::shared_ptr<Artist> m_variousArtists;
    std::shared_ptr<Show> m_unknownShow;
    /*
     * Protects the previous album & previous folder ID which can change during
     * the current task executes.
     */
    compat::Mutex m_cacheMutex;
    std::shared_ptr<Album> m_previousAlbum;
    int64_t m_previousFolderId;
    bool m_flushed;
    std::atomic_bool m_stopped;
};

}
}
