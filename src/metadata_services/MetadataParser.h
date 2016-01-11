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

class AlbumTrack;

class MetadataParser : public ParserService
{
public:
    MetadataParser( DBConnection dbConnection, std::shared_ptr<factory::IFileSystem> fsFactory );
protected:
    virtual bool initialize() override;
    virtual parser::Task::Status run( parser::Task& task ) override;
    virtual const char* name() const override;
    virtual uint8_t nbThreads() const override;

    std::shared_ptr<Album> findAlbum(File& file, VLC::Media& vlcMedia, const std::string& title, Artist* albumArtist ) const;
    bool parseAudioFile( std::shared_ptr<Media> media, File& file, VLC::Media &vlcMedia ) const;
    bool parseVideoFile( std::shared_ptr<Media> media, VLC::Media& vlcMedia ) const;
    std::pair<std::shared_ptr<Artist>, std::shared_ptr<Artist>> handleArtists(VLC::Media& vlcMedia ) const;
    std::shared_ptr<AlbumTrack> handleTrack(std::shared_ptr<Album> album, std::shared_ptr<Media> media, VLC::Media& vlcMedia , std::shared_ptr<Artist> artist) const;
    bool link(Media& media, std::shared_ptr<Album> album, std::shared_ptr<Artist> albumArtist, std::shared_ptr<Artist> artist ) const;
    std::shared_ptr<Album> handleAlbum( std::shared_ptr<Media> media, File& file, VLC::Media& vlcMedia, std::shared_ptr<Artist> albumArtist, std::shared_ptr<Artist> artist ) const;

private:
    MediaLibrary* m_ml;
    std::shared_ptr<Artist> m_unknownArtist;
    DBConnection m_dbConn;
    std::shared_ptr<factory::IFileSystem> m_fsFactory;

};
