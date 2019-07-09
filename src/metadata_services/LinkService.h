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

#include "medialibrary/parser/IParserService.h"
#include "Types.h"

namespace medialibrary
{
namespace parser
{

class LinkService : public IParserService
{
public:
    virtual Status run(IItem& item) override;
    virtual const char*name() const override;
    virtual Step targetedStep() const override;
    virtual bool initialize(IMediaLibrary* ml) override;
    virtual void onFlushing() override;
    virtual void onRestarted() override;
    virtual void stop() override;

private:
    Status linkToPlaylist( IItem& item );

private:
    MediaLibraryPtr m_ml;
};

}
}
