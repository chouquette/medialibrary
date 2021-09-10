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

#include "compat/ConditionVariable.h"
#include <vlcpp/vlc.hpp>

#include "medialibrary/parser/IParserService.h"

namespace medialibrary
{
namespace parser
{

#if LIBVLC_VERSION_INT >= LIBVLC_VERSION(4, 0, 0, 0)

class VLCEmbeddedThumbnail4_0 : public IEmbeddedThumbnail
{
public:
    explicit VLCEmbeddedThumbnail4_0( VLC::Picture pic );
    bool save( const std::string& path ) override;
    virtual size_t size() const override;
    virtual std::string hash() const override;
    virtual std::string extension() const override;

private:
    VLC::Picture m_pic;
};

#elif defined(FORCE_ATTACHMENTS_API)

class VLCEmbeddedThumbnailForced : public IEmbeddedThumbnail
{
public:
    explicit VLCEmbeddedThumbnailForced( libvlc_picture_t* pic );
    virtual ~VLCEmbeddedThumbnailForced();
    bool save( const std::string& path ) override;
    virtual size_t size() const override;
    virtual std::string hash() const override;
    virtual std::string extension() const override;

private:
    libvlc_picture_t *m_pic;
};

#else

class VLCEmbeddedThumbnail3_0 : public IEmbeddedThumbnail
{
public:
    explicit VLCEmbeddedThumbnail3_0( std::string path );
    bool save( const std::string& path ) override;
    virtual size_t size() const override;
    virtual std::string hash() const override;
    virtual std::string extension() const override;

private:
    std::string m_thumbnailPath;
};

#endif

class VLCMetadataService : public IParserService
{
public:
    VLCMetadataService() = default;

private:
    virtual bool initialize( IMediaLibrary* ml ) override;
    virtual parser::Status run( parser::IItem& item ) override;
    virtual const char* name() const override;
    virtual void onFlushing() override;
    virtual void onRestarted() override;
    virtual parser::Step targetedStep() const override;
    virtual void stop() override;

    void mediaToItem( VLC::Media& media, parser::IItem& item );

#if defined(FORCE_ATTACHMENTS_API)
    static void onAttachedThumbnailsFound( const libvlc_event_t* e, void* data );
#endif

private:
    compat::Mutex m_mutex;
    compat::ConditionVariable m_cond;
    VLC::Media m_currentMedia;
};

}
}
