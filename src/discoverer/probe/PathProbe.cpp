/*****************************************************************************
 * Media Library
 *****************************************************************************
 * Copyright (C) 2017 Hugo Beauzée-Luyssen, Videolabs
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

#include <cassert>
#include <utility>

#include "discoverer/probe/PathProbe.h"

#include "utils/Filename.h"
#include "medialibrary/filesystem/IDirectory.h"
#include "medialibrary/filesystem/IFile.h"

namespace medialibrary
{

namespace prober
{

PathProbe::PathProbe( const std::string& path, bool isDirectory, std::shared_ptr<Playlist> parentPlaylist,
                      std::shared_ptr<Folder> parentFolder, const std::string& parentFolderPath,
                      unsigned int playlistIndex, bool reload )
    : m_isDirectory( isDirectory )
    , m_isDiscoveryEnded( false )
    , m_parentPlaylist( std::move( parentPlaylist ) )
    , m_parentFolder( std::move( parentFolder ) )
    , m_path( path )
    , m_playlistIndex( playlistIndex )
{
    assert( path.size() >= parentFolderPath.size() );

    m_splitPath = utils::file::splitPath( path, isDirectory );

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

bool PathProbe::proceedOnDirectory( const fs::IDirectory& directory )
{
    if ( m_isDirectory && m_splitPath.empty() == true )
    {
        auto directoryPath = utils::file::toPath( directory.mrl() );
        auto comp = std::mismatch( m_path.begin(), m_path.end(), directoryPath.begin());
        if ( comp.first == m_path.end() )
            return true;
        m_isDiscoveryEnded = true;
        return false;
    }
    if ( m_splitPath.empty() == true
         || m_splitPath.top() != utils::file::directoryName( utils::file::stripScheme( directory.mrl() ) ) )
        return false;
    m_splitPath.pop();
    return true;
}

bool PathProbe::proceedOnFile( const fs::IFile& file )
{
    if ( m_isDirectory == true && m_isDiscoveryEnded == false && m_splitPath.empty() == true )
    {
        if ( utils::file::stripScheme( file.mrl() ) != m_path )
            return true;
        m_isDiscoveryEnded = true;
        return false;
    }

    if ( m_path == utils::file::stripScheme( file.mrl() ) )
    {
        assert( m_splitPath.size() == 1 && file.name() == m_splitPath.top() );
        m_splitPath.pop();
        m_isDiscoveryEnded = true;
        return true;
    }
    return false;
}

}

}
