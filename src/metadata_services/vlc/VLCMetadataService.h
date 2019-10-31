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

class VLCMetadataService : public IParserService
{
public:
    explicit VLCMetadataService();

private:
    virtual bool initialize( IMediaLibrary* ml ) override;
    virtual parser::Status run( parser::IItem& item ) override;
    virtual const char* name() const override;
    virtual void onFlushing() override;
    virtual void onRestarted() override;
    virtual parser::Step targetedStep() const override;
    virtual void stop() override;

    void mediaToItem( VLC::Media& media, parser::IItem& item );

private:
    VLC::Instance m_instance;
    compat::Mutex m_mutex;
    compat::ConditionVariable m_cond;
    VLC::Media m_currentMedia;
};

}
}
