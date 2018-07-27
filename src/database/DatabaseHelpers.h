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

#include <memory>
#include <unordered_map>
#include <vector>

#include "compat/Mutex.h"
#include "SqliteTools.h"
#include "SqliteTransaction.h"

namespace medialibrary
{

template <typename IMPL>
class DatabaseHelpers
{
    public:
        template <typename... Args>
        static std::shared_ptr<IMPL> fetch( MediaLibraryPtr ml, const std::string& req, Args&&... args )
        {
            try
            {
                return sqlite::Tools::fetchOne<IMPL>( ml, req, std::forward<Args>( args )... );
            }
            catch ( const sqlite::errors::GenericExecution& ex )
            {
                if ( sqlite::errors::isInnocuous( ex ) == false )
                    throw;
                LOG_WARN( "Ignoring innocuous error: ", ex.what() );
            }
            return {};
        }

        static std::shared_ptr<IMPL> fetch( MediaLibraryPtr ml, int64_t pkValue )
        {
            static std::string req = "SELECT * FROM " + IMPL::Table::Name + " WHERE " +
                    IMPL::Table::PrimaryKeyColumn + " = ?";
            try
            {
                return sqlite::Tools::fetchOne<IMPL>( ml, req, pkValue );
            }
            catch ( const sqlite::errors::GenericExecution& ex )
            {
                if ( sqlite::errors::isInnocuous( ex ) == false )
                    throw;
            }
            return {};
        }

        /*
         * Will fetch all elements from the database & cache them.
         */
        template <typename INTF = IMPL>
        static std::vector<std::shared_ptr<INTF>> fetchAll( MediaLibraryPtr ml )
        {
            static const std::string req = "SELECT * FROM " + IMPL::Table::Name;
            try
            {
                return sqlite::Tools::fetchAll<IMPL, INTF>( ml, req );
            }
            catch ( const sqlite::errors::GenericExecution& ex )
            {
                if ( sqlite::errors::isInnocuous( ex ) == false )
                    throw;
                LOG_WARN( "Ignoring innocuous error: ", ex.what() );
            }
            return {};
        }

        template <typename INTF, typename... Args>
        static std::vector<std::shared_ptr<INTF>> fetchAll( MediaLibraryPtr ml, const std::string& req, Args&&... args )
        {
            try
            {
                return sqlite::Tools::fetchAll<IMPL, INTF>( ml, req, std::forward<Args>( args )... );
            }
            catch ( const sqlite::errors::GenericExecution& ex )
            {
                if ( sqlite::errors::isInnocuous( ex ) == false )
                    throw;
                LOG_WARN( "Ignoring innocuous error: ", ex.what() );
            }
            return {};
        }

        static bool destroy( MediaLibraryPtr ml, int64_t pkValue )
        {
            static const std::string req = "DELETE FROM " + IMPL::Table::Name + " WHERE "
                    + IMPL::Table::PrimaryKeyColumn + " = ?";
            return sqlite::Tools::executeDelete( ml->getConn(), req, pkValue );
        }

        static bool deleteAll( MediaLibraryPtr ml )
        {
            static const std::string req = "DELETE FROM " + IMPL::Table::Name;
            return sqlite::Tools::executeDelete( ml->getConn(), req );
        }

    protected:
        /*
         * Create a new instance of the cache class.
         */
        template <typename... Args>
        static bool insert( MediaLibraryPtr ml, std::shared_ptr<IMPL> self, const std::string& req, Args&&... args )
        {
            int64_t pKey = sqlite::Tools::executeInsert( ml->getConn(), req, std::forward<Args>( args )... );
            if ( pKey == 0 )
                return false;
            (self.get())->*IMPL::Table::PrimaryKey = pKey;
            return true;
        }


    protected:
        DatabaseHelpers() : m_deleted( false ) {}
        /**
         * @brief m_deleted reflects if this entity has been deleted from the DB
         * This is not reliable, and isn't synchronized. You can only assume that if it is set to true
         * then the entity has been deleted.
         * This can be false, but become true the cycle after you checked it.
         */
        std::atomic_bool m_deleted;

    public:
        bool isDeleted() const
        {
            return m_deleted.load( std::memory_order_relaxed );
        }

        void markDeleted()
        {
            auto del = false;
            auto res = m_deleted.compare_exchange_strong( del, true, std::memory_order_relaxed, std::memory_order_relaxed );
            assert(res);
            (void)res;
        }

};

}
