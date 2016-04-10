/*****************************************************************************
 * Media Library
 *****************************************************************************
 * Copyright (C) 2015 Hugo Beauzée-Luyssen, Videolabs
 *
 * Authors: Hugo Beauzée-Luyssen<hugo@beauzee.fr>
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

#ifndef IMEDIALIBRARY_H
#define IMEDIALIBRARY_H

#include <vector>
#include <string>

#include "Types.h"

namespace medialibrary
{
    static constexpr auto UnknownArtistID = 1u;
    static constexpr auto VariousArtistID = 2u;

    struct MediaSearchAggregate
    {
        std::vector<MediaPtr> episodes;
        std::vector<MediaPtr> movies;
        std::vector<MediaPtr> others;
        std::vector<MediaPtr> tracks;
    };

    struct SearchAggregate
    {
        std::vector<AlbumPtr> albums;
        std::vector<ArtistPtr> artists;
        std::vector<GenrePtr> genres;
        MediaSearchAggregate media;
        std::vector<PlaylistPtr> playlists;
    };

    enum class SortingCriteria
    {
        /*
         * Default depends on the entity type:
         * - By track number (and disc number) for album tracks
         * - Alphabetical order for others
         */
        Default,
        Alpha,
        Duration,
        InsertionDate,
        LastModificationDate,
        ReleaseDate,
    };

class IMediaLibraryCb
{
public:
    virtual ~IMediaLibraryCb() = default;
    /**
     * @brief onFileAdded Will be called when some media get added.
     * Depending if the media is being restored or was just discovered,
     * the media type might be a best effort guess. If the media was freshly
     * discovered, it is extremely likely that no metadata will be
     * available yet.
     * The number of media is undefined, but is guaranteed to be at least 1.
     */
    virtual void onMediaAdded( std::vector<MediaPtr> media ) = 0;
    /**
     * @brief onFileUpdated Will be called when a file metadata gets updated.
     */
    virtual void onMediaUpdated( std::vector<MediaPtr> media ) = 0;

    virtual void onMediaDeleted( std::vector<int64_t> ids ) = 0;

    virtual void onArtistsAdded( std::vector<ArtistPtr> artists ) = 0;
    virtual void onArtistsModified( std::vector<ArtistPtr> artist ) = 0;
    virtual void onArtistsDeleted( std::vector<int64_t> ids ) = 0;

    virtual void onAlbumsAdded( std::vector<AlbumPtr> albums ) = 0;
    virtual void onAlbumsModified( std::vector<AlbumPtr> albums ) = 0;
    virtual void onAlbumsDeleted( std::vector<int64_t> ids ) = 0;
    /**
     * @brief onTrackAdded Called when a media gets detected as an album track
     * and after it has been added to the album representation
     */
    virtual void onTracksAdded( std::vector<AlbumTrackPtr> tracks ) = 0;
    // Tracks are never modified after their creation, so there is no tracksModified event
    virtual void onTracksDeleted( std::vector<int64_t> trackIds ) = 0;

    virtual void onDiscoveryStarted( const std::string& entryPoint ) = 0;
    virtual void onDiscoveryCompleted( const std::string& entryPoint ) = 0;
    virtual void onReloadStarted( const std::string& entryPoint ) = 0;
    virtual void onReloadCompleted( const std::string& entryPoint ) = 0;
    /**
     * @brief onParsingStatsUpdated Called when the parser statistics are updated
     *
     * There is no waranty about how often this will be called.
     * @param percent The progress percentage [0,100]
     *
     */
    virtual void onParsingStatsUpdated( uint32_t percent) = 0;
};

class IMediaLibrary
{
    public:
        virtual ~IMediaLibrary() = default;
        ///
        /// \brief  initialize Initializes the media library.
        ///         This will use the provided discoverer to search for new media asynchronously.
        ///
        /// \param dbPath       Path to the database
        /// \return true in case of success, false otherwise
        ///
        virtual bool initialize( const std::string& dbPath, const std::string& thumbnailPath, IMediaLibraryCb* metadataCb ) = 0;
        virtual void setVerbosity( LogLevel v ) = 0;

        virtual LabelPtr createLabel( const std::string& label ) = 0;
        virtual bool deleteLabel( LabelPtr label ) = 0;
        virtual std::vector<MediaPtr> audioFiles( medialibrary::SortingCriteria sort = medialibrary::SortingCriteria::Default, bool desc = false ) const = 0;
        virtual std::vector<MediaPtr> videoFiles( medialibrary::SortingCriteria sort = medialibrary::SortingCriteria::Default, bool desc = false ) const = 0;
        virtual AlbumPtr album( int64_t id ) const = 0;
        virtual std::vector<AlbumPtr> albums( medialibrary::SortingCriteria sort = medialibrary::SortingCriteria::Default, bool desc = false ) const = 0;
        virtual ShowPtr show( const std::string& name ) const = 0;
        virtual MoviePtr movie( const std::string& title ) const = 0;
        virtual ArtistPtr artist( int64_t id ) const = 0;
        /**
         * @brief artists List all artists that have at least an album.
         * Artists that only appear on albums as guests won't be listed from here, but will be
         * returned when querying an album for all its appearing artists
         * @param sort A sorting criteria. So far, this is ignored, and artists are sorted by lexial order
         * @param desc If true, the provided sorting criteria will be reversed.
         */
        virtual std::vector<ArtistPtr> artists( medialibrary::SortingCriteria sort = medialibrary::SortingCriteria::Default, bool desc = false ) const = 0;
        /**
         * @brief genres Return the list of music genres
         * @param sort A sorting criteria. So far, this is ignored, and artists are sorted by lexial order
         * @param desc If true, the provided sorting criteria will be reversed.
         */
        virtual std::vector<GenrePtr> genres( medialibrary::SortingCriteria sort = medialibrary::SortingCriteria::Default, bool desc = false ) const = 0;
        virtual GenrePtr genre( int64_t id ) const = 0;
        /***
         *  Playlists
         */
        virtual PlaylistPtr createPlaylist( const std::string& name ) = 0;
        virtual std::vector<PlaylistPtr> playlists( medialibrary::SortingCriteria sort = medialibrary::SortingCriteria::Default, bool desc = false ) = 0;
        virtual PlaylistPtr playlist( int64_t id ) const = 0;
        virtual bool deletePlaylist( int64_t playlistId ) = 0;

        /**
         * History
         */
        virtual bool addToHistory( const std::string& mrl ) = 0;
        virtual std::vector<HistoryPtr> lastStreamsPlayed() const = 0;
        virtual std::vector<MediaPtr> lastMediaPlayed() const = 0;

        /**
         * Search
         */
        virtual medialibrary::MediaSearchAggregate searchMedia( const std::string& pattern ) const = 0;
        virtual std::vector<PlaylistPtr> searchPlaylists( const std::string& name ) const = 0;
        virtual std::vector<AlbumPtr> searchAlbums( const std::string& pattern ) const = 0;
        virtual std::vector<GenrePtr> searchGenre( const std::string& genre ) const = 0;
        virtual std::vector<ArtistPtr> searchArtists( const std::string& name ) const = 0;
        virtual medialibrary::SearchAggregate search( const std::string& pattern ) const = 0;

        /**
         * @brief discover Launch a discovery on the provided entry point.
         * The actuall discovery will run asynchronously, meaning this method will immediatly return.
         * Depending on which discoverer modules where provided, this might or might not work
         * @param entryPoint What to discover.
         */
        virtual void discover( const std::string& entryPoint ) = 0;
        /**
         * @brief banFolder will blacklist a folder for discovery
         */
        virtual bool banFolder( const std::string& path ) = 0;
        virtual bool unbanFolder( const std::string& path ) = 0;
        virtual const std::string& thumbnailPath() const = 0;
        virtual void setLogger( ILogger* logger ) = 0;
        /**
         * @brief pauseBackgroundOperations Will stop potentially CPU intensive background
         * operations, until resumeBackgroundOperations() is called.
         * If an operation is currently running, it will finish before pausing.
         */
        virtual void pauseBackgroundOperations() = 0;
        /**
         * @brief resumeBackgroundOperations Resumes background tasks, previously
         * interrupted by pauseBackgroundOperations().
         */
        virtual void resumeBackgroundOperations() = 0;
        virtual void reload() = 0;
        virtual void reload( const std::string& entryPoint ) = 0;
};

}

extern "C"
{
    medialibrary::IMediaLibrary* NewMediaLibrary();
}

#endif // IMEDIALIBRARY_H
