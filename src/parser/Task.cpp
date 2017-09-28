/*****************************************************************************
 * Media Library
 *****************************************************************************
 * Copyright (C) 2015 - 2017 Hugo Beauzée-Luyssen, Videolabs
 *
 * Authors: Hugo Beauzée-Luyssen<hugo@beauzee.fr>
 *          Alexandre Fernandez <nerf@boboop.fr>
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

#if HAVE_CONFIG_H
# include "config.h"
#endif

#include "Task.h"

#include "filesystem/IFile.h"
#include "File.h"

namespace medialibrary
{

namespace parser
{

Task::Task( std::shared_ptr<File> file, std::string mrl )
    : file( std::move( file ) )
    , mrl( std::move( mrl ) )
    , currentService( 0 )
    , step( this->file->parserStep() )
{
}

Task::Task( std::shared_ptr<fs::IFile> fileFs,
            std::shared_ptr<Folder> parentFolder,
            std::shared_ptr<fs::IDirectory> parentFolderFs,
            std::string mrl )
    : fileFs( std::move( fileFs ) )
    , parentFolder( std::move( parentFolder ) )
    , parentFolderFs( std::move( parentFolderFs ) )
    , mrl( std::move( mrl ) )
    , currentService( 0 )
    , step( ParserStep::None )
{
}

void Task::markStepCompleted( ParserStep stepCompleted )
{
    step = static_cast<ParserStep>( static_cast<uint8_t>( step ) |  static_cast<uint8_t>( stepCompleted ) );
    if ( file != nullptr )
        file->markStepCompleted( stepCompleted );
}

void Task::markStepUncompleted( ParserStep stepUncompleted )
{
    step = static_cast<ParserStep>( static_cast<uint8_t>( step ) & ( ~ static_cast<uint8_t>( stepUncompleted ) ) );
    if ( file != nullptr )
        file->markStepUncompleted( stepUncompleted );
}

}

}
