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

#include <stack>
#include <string>

namespace medialibrary
{

namespace utils
{

namespace file
{
    /**
     * @brief extension Returns the file's extension, without the leading '.'
     * @param fileName A filename
     */
    std::string extension( const std::string& fileName );
    std::string stripExtension( const std::string& fileName );
    /**
     * @brief directory Returns the mrl of the folder containing the provided file
     * @param fileMrl An mrl pointing to a file
     * If the mrl points to a directory, this function will return the same mrl.
     */
    std::string directory( const std::string& fileMrl );

    /**
     * @brief directoryName  Extract the folder name from a path
     * @param directoryPath  Path pointing to a directory
     */
    std::string directoryName( const std::string& directoryPath );
    std::string fileName( const std::string& filePath );
    std::string firstFolder( const std::string& path );
    std::string removePath( const std::string& fullPath, const std::string& toRemove );
    std::string parentDirectory( const std::string& path );
    /**
     * @brief toFolder  Ensures a path is a folder path; ie. it has a terminal '/'
     * @param path      The path to sanitize
     */
    std::string& toFolderPath( std::string& path );
    std::string  toFolderPath( const std::string& path );
    /**
     * @brief toLocalPath Converts an MRL to a local path.
     * This will strip the file:// scheme and URL decode the MRL
     */
    std::string  toLocalPath( const std::string& mrl );

    /**
     * @brief stripScheme Remove the scheme from a MRL
     * @param mrl
     */
    std::string stripScheme( const std::string& mrl );

    /**
     * @brief scheme Returns the scheme used in an MRL
     * ie. for seaOtter://foo.bar it will return seaOtter://
     */
    std::string  scheme( const std::string& mrl );
    /**
     * @brief toMrl Convert a filepath to an MRL
     */
    std::string toMrl( const std::string& path );

    std::stack<std::string> splitPath( const std::string& path, bool isDirectory );

    /**
     * @brief schemeIs Check if a mrl start with a specific scheme
     */
    bool schemeIs( const std::string& scheme, const std::string& mrl );
}

}

}
