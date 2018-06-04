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

#pragma once

#include "MediaLibrary.h"
#include "parser/ParserWorker.h"

namespace medialibrary
{

class AlbumTrack;

class MetadataParser : public IParserService
{
public:
    MetadataParser();

protected:
    bool cacheUnknownArtist();
    virtual bool initialize( IMediaLibrary* ml ) override;
    virtual parser::Status run( parser::IItem& item ) override;
    virtual const char* name() const override;
    virtual uint8_t nbThreads() const override;
    virtual void onFlushing() override;
    virtual void onRestarted() override;
    virtual parser::Step targetedStep() const override;

    bool addPlaylistMedias( parser::IItem& item ) const;
    void addPlaylistElement( parser::IItem& item, std::shared_ptr<Playlist> playlistPtr,
                             const parser::IItem& subitem ) const;
    bool parseAudioFile( parser::IItem& task );
    bool parseVideoFile( parser::IItem& task ) const;
    std::pair<std::shared_ptr<Artist>, std::shared_ptr<Artist>> findOrCreateArtist( parser::IItem& item ) const;
    std::shared_ptr<AlbumTrack> handleTrack( std::shared_ptr<Album> album, parser::IItem& item,
                                             std::shared_ptr<Artist> artist, Genre* genre ) const;
    bool link(Media& media, std::shared_ptr<Album> album, std::shared_ptr<Artist> albumArtist, std::shared_ptr<Artist> artist );
    std::shared_ptr<Album> findAlbum( parser::IItem& item, std::shared_ptr<Artist> albumArtist,
                                        std::shared_ptr<Artist> artist );
    std::shared_ptr<Genre> handleGenre( parser::IItem& item ) const;

private:
    static int toInt( parser::IItem& item, parser::IItem::Metadata meta );

private:
    MediaLibrary* m_ml;
    std::shared_ptr<ModificationNotifier> m_notifier;

    std::shared_ptr<Artist> m_unknownArtist;
    std::shared_ptr<Artist> m_variousArtists;
    std::shared_ptr<Album> m_previousAlbum;
    int64_t m_previousFolderId;
};

}
