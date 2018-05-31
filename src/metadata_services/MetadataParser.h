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
    virtual bool initialize( MediaLibrary* ml ) override;
    virtual parser::Task::Status run( parser::Task& task ) override;
    virtual const char* name() const override;
    virtual uint8_t nbThreads() const override;
    virtual void onFlushing() override;
    virtual void onRestarted() override;
    bool isCompleted( const parser::Task& task ) const override;

    bool addPlaylistMedias( parser::Task& task ) const;
    void addPlaylistElement( parser::Task& task, const std::shared_ptr<Playlist>& playlistPtr,
                             parser::Task::Item& subitem, unsigned int index ) const;
    bool parseAudioFile(parser::Task& task);
    bool parseVideoFile(parser::Task& task) const;
    std::pair<std::shared_ptr<Artist>, std::shared_ptr<Artist>> findOrCreateArtist( parser::Task& vlcMedia ) const;
    std::shared_ptr<AlbumTrack> handleTrack( std::shared_ptr<Album> album, parser::Task& task,
                                             std::shared_ptr<Artist> artist, Genre* genre ) const;
    bool link(Media& media, std::shared_ptr<Album> album, std::shared_ptr<Artist> albumArtist, std::shared_ptr<Artist> artist );
    std::shared_ptr<Album> findAlbum( parser::Task& task, std::shared_ptr<Artist> albumArtist,
                                        std::shared_ptr<Artist> artist );
    std::shared_ptr<Genre> handleGenre( parser::Task& task ) const;

private:
    static int toInt( parser::Task& task, parser::Task::Item::Metadata meta );

private:
    MediaLibrary* m_ml;
    std::shared_ptr<ModificationNotifier> m_notifier;

    std::shared_ptr<Artist> m_unknownArtist;
    std::shared_ptr<Artist> m_variousArtists;
    std::shared_ptr<Album> m_previousAlbum;
    int64_t m_previousFolderId;
};

}
