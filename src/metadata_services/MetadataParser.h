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
#include "parser/ParserService.h"

namespace medialibrary
{

class AlbumTrack;

class MetadataParser : public ParserService
{
protected:
    virtual bool initialize() override;
    virtual parser::Task::Status run( parser::Task& task ) override;
    virtual const char* name() const override;
    virtual uint8_t nbThreads() const override;

    std::shared_ptr<Album> findAlbum(parser::Task& task, Artist* albumArtist ) const;
    bool parseAudioFile(parser::Task& task) const;
    bool parseVideoFile(parser::Task& task) const;
    std::pair<std::shared_ptr<Artist>, std::shared_ptr<Artist>> findOrCreateArtist( parser::Task& vlcMedia ) const;
    std::shared_ptr<AlbumTrack> handleTrack( std::shared_ptr<Album> album, parser::Task& task,
                                             std::shared_ptr<Artist> artist, Genre* genre ) const;
    bool link(Media& media, std::shared_ptr<Album> album, std::shared_ptr<Artist> albumArtist, std::shared_ptr<Artist> artist ) const;
    std::shared_ptr<Album> handleAlbum( parser::Task& task, std::shared_ptr<Artist> albumArtist,
                                        std::shared_ptr<Artist> artist, Genre* genre ) const;
    std::shared_ptr<Genre> handleGenre( parser::Task& task ) const;

private:
    std::shared_ptr<Artist> m_unknownArtist;
};

}
