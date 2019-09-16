/*****************************************************************************
 * Media Library
 *****************************************************************************
 * Copyright (C) 2017-2019 Hugo Beauzée-Luyssen, Videolabs, VideoLAN
 *
 * Authors: Hugo Beauzée-Luyssen <hugo@beauzee.fr>
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

#include <cassert>
#include <utility>

#include "discoverer/probe/PathProbe.h"

#include "utils/Filename.h"
#include "utils/Url.h"
#include "medialibrary/filesystem/IDirectory.h"
#include "medialibrary/filesystem/IFile.h"

namespace medialibrary
{

namespace prober
{

PathProbe::PathProbe( std::string path, bool isDirectory,
                      std::shared_ptr<Folder> parentFolder, const std::string& parentFolderPath,
                      int64_t parentPlaylistId, int64_t playlistIndex, bool reload )
    : m_isDirectory( isDirectory )
    , m_isDiscoveryEnded( false )
    , m_entryPointHandled( false )
    , m_parentFolder( std::move( parentFolder ) )
    , m_path( std::move( path ) )
    , m_parentPlaylistId( parentPlaylistId )
    , m_playlistIndex( playlistIndex )
{
    assert( m_path.size() >= parentFolderPath.size() );

    m_splitPath = utils::file::splitPath( m_path, isDirectory );

    // If the parent folder exists, we shorten the stack to contain only new folders
    if ( m_parentFolder != nullptr && m_splitPath.empty() == false )
    {
        auto parentSplit = utils::file::splitPath( parentFolderPath, true );
        while ( parentSplit.empty() == false )
        {
            assert( parentSplit.top() == m_splitPath.top() );
            parentSplit.pop();
            m_splitPath.pop();
        }
    }
    else if ( reload == true && m_splitPath.empty() == false )
        // An existing folder is being reloaded, and won't be unstacked by FsDiscoverer::discover(), so it's done here
        m_splitPath.pop();
}

bool PathProbe::proceedOnEntryPoint( const fs::IDirectory& entryPoint )
{
    if ( m_splitPath.empty() == true )
        return true;
    auto directoryPath = utils::file::toLocalPath( entryPoint.mrl() );
#ifndef _WIN32
    // In case we are discovering from "/", the root folder isn't part of the
    // splitted path stack, which would cause the prober to reject it.
    if ( directoryPath == "/" )
        return true;
#endif
    auto splitDirectoryPath = utils::file::splitPath( directoryPath, true );
    while ( !splitDirectoryPath.empty() )
    {
        if (m_splitPath.top() != splitDirectoryPath.top() )
            return false;

        m_splitPath.pop();
        if ( m_splitPath.empty() )
            return true;

        splitDirectoryPath.pop();
    }
    return true;
}

bool PathProbe::proceedOnDirectory( const fs::IDirectory& directory )
{
    if ( !m_entryPointHandled )
    {
        m_entryPointHandled = true;
        return proceedOnEntryPoint(directory);
    }
    if ( m_isDirectory && m_splitPath.empty() == true )
    {
        auto directoryPath = utils::file::toLocalPath( directory.mrl() );
        auto comp = std::mismatch( m_path.begin(), m_path.end(), directoryPath.begin());
        if ( comp.first == m_path.end() )
            return true;
        m_isDiscoveryEnded = true;
        return false;
    }
    if ( m_splitPath.empty() == true )
        return true;
    auto directoryPath = utils::file::toLocalPath( directory.mrl() );
#ifndef _WIN32
    // In case we are discovering from "/", the root folder isn't part of the
    // splitted path stack, which would cause the prober to reject it.
    if ( directoryPath == "/" )
        return true;
#endif
    if ( m_splitPath.top() != utils::file::directoryName( directoryPath ) )
        return false;
    m_splitPath.pop();
    return true;
}

bool PathProbe::proceedOnFile( const fs::IFile& file )
{
    auto path = utils::file::toLocalPath( file.mrl() );
    if ( m_isDirectory == true && m_isDiscoveryEnded == false && m_splitPath.empty() == true )
    {
        if ( path != m_path )
            return true;
        m_isDiscoveryEnded = true;
        return false;
    }

    if ( m_path == path )
    {
        assert( m_splitPath.size() == 1 &&
                utils::url::decode( file.name() ) == m_splitPath.top() );
        m_splitPath.pop();
        m_isDiscoveryEnded = true;
        return true;
    }
    return false;
}

}

}
