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

#include "Cache.h"

template <typename IMPL, typename TABLEPOLICY>
class Table
{
    using _Cache = Cache<IMPL>;

    public:
        template <typename... Args>
        static std::shared_ptr<IMPL> fetch( DBConnection dbConnection, const std::string& req, Args&&... args )
        {
            return sqlite::Tools::fetchOne<IMPL>( dbConnection, req, std::forward<Args>( args )... );
        }

        static std::shared_ptr<IMPL> fetch( DBConnection dbConnection, unsigned int pkValue )
        {
            static std::string req = "SELECT * FROM " + TABLEPOLICY::Name + " WHERE " +
                    TABLEPOLICY::PrimaryKeyColumn + " = ?";
            return sqlite::Tools::fetchOne<IMPL>( dbConnection, req, pkValue );
        }

        /*
         * Will fetch all elements from the database & cache them.
         */
        template <typename INTF = IMPL>
        static std::vector<std::shared_ptr<INTF>> fetchAll( DBConnection dbConnection )
        {
            static const std::string req = "SELECT * FROM " + TABLEPOLICY::Name;
            // Lock the cache mutex before attempting to acquire a context, otherwise
            // we could have a thread locking cache then DB, and a thread locking DB then cache
            auto l = _Cache::lock();
            return sqlite::Tools::fetchAll<IMPL, INTF>( dbConnection, req );
        }

        template <typename INTF, typename... Args>
        static std::vector<std::shared_ptr<INTF>> fetchAll( DBConnection dbConnection, const std::string &req, Args&&... args )
        {
            auto l = _Cache::lock();
            return sqlite::Tools::fetchAll<IMPL, INTF>( dbConnection, req, std::forward<Args>( args )... );
        }

        static std::shared_ptr<IMPL> load( DBConnection dbConnection, sqlite::Row& row )
        {
            auto l = _Cache::lock();

            auto key = row.load<unsigned int>( 0 );
            auto res = _Cache::load( key );
            if ( res != nullptr )
                return res;
            res = std::make_shared<IMPL>( dbConnection, row );
            _Cache::store( res );
            return res;
        }

        template <typename... Args>
        static bool destroy( DBConnection dbConnection, unsigned int pkValue )
        {
            auto l = _Cache::lock();
            static const std::string req = "DELETE FROM " + TABLEPOLICY::Name + " WHERE "
                    + TABLEPOLICY::PrimaryKeyColumn + " = ?";
            auto res = sqlite::Tools::executeDelete( dbConnection, req, pkValue );
            if ( res == true )
                _Cache::discard( pkValue );
            else
            {
                // Simply ensure nothing was cached for this value if there's nothing to
                // delete from DB
                assert( _Cache::discard( pkValue ) == false );
            }
            return res;
        }

        static void clear()
        {
            auto l = _Cache::lock();
            _Cache::clear();
        }

    protected:
        /*
         * Create a new instance of the cache class.
         */
        template <typename... Args>
        static bool insert( DBConnection dbConnection, std::shared_ptr<IMPL> self, const std::string& req, Args&&... args )
        {
            auto l = _Cache::lock();

            unsigned int pKey = sqlite::Tools::insert( dbConnection, req, std::forward<Args>( args )... );
            if ( pKey == 0 )
                return false;
            (self.get())->*TABLEPOLICY::PrimaryKey = pKey;
            _Cache::store( self );
            return true;
        }
};
