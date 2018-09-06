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

#include "medialibrary/IQuery.h"
#include "SqliteConnection.h"
#include "SqliteTools.h"

#include <vector>
#include <functional>
#include <string>

namespace medialibrary
{

template <typename Impl, typename Intf, typename... RequestParams>
class SqliteQueryBase : public IQuery<Intf>
{
protected:
    using Result = typename IQuery<Intf>::Result;

    template <typename... Params>
    SqliteQueryBase( MediaLibraryPtr ml, Params&&... params )
        : m_ml( ml )
        , m_params( std::forward<Params>( params )... )
    {
    }

    size_t count( const std::string& req )
    {
        auto dbConn = m_ml->getConn();
        auto ctx = dbConn->acquireReadContext();
        auto chrono = std::chrono::steady_clock::now();
        sqlite::Statement stmt( dbConn->handle(), req );
        stmt.execute( m_params );
        auto duration = std::chrono::steady_clock::now() - chrono;
        LOG_DEBUG("Executed ", req, " in ",
                 std::chrono::duration_cast<std::chrono::microseconds>( duration ).count(), "µs" );
        auto row = stmt.row();
        size_t count;
        row >> count;
        assert( stmt.row() == nullptr );
        return count;
    }

    Result items( const std::string& req, uint32_t nbItems, uint32_t offset )
    {
        return Impl::template fetchAll<Intf>( m_ml, req, m_params, nbItems, offset );
    }

    Result all( const std::string& req )
    {
        return Impl::template fetchAll<Intf>( m_ml, req, m_params );
    }

private:
    MediaLibraryPtr m_ml;
    std::tuple<typename std::decay<RequestParams>::type...> m_params;
};

template <typename Impl, typename Intf, typename... Args>
class SqliteQuery : public SqliteQueryBase<Impl, Intf, Args...>
{
public:
    using Base = SqliteQueryBase<Impl, Intf, Args...>;
    using Result = typename Base::Result;

    template <typename... Params>
    SqliteQuery( MediaLibraryPtr ml, std::string field, std::string base,
                 std::string groupAndOrderBy, Params&&... params )
        : SqliteQueryBase<Impl, Intf, Args...>( ml, std::forward<Params>( params )... )
        , m_field( std::move( field ) )
        , m_base( std::move( base ) )
        , m_groupAndOrderBy( std::move( groupAndOrderBy ) )
    {
    }

    virtual size_t count() override
    {
        std::string req = "SELECT COUNT(DISTINCT " + Impl::Table::PrimaryKeyColumn +
                " ) " + m_base;
        return Base::count( req );
    }

    virtual Result items( uint32_t nbItems, uint32_t offset ) override
    {
        if ( nbItems == 0 && offset == 0 )
            return all();
        const std::string req = "SELECT " + m_field + " " + m_base + " " +
                m_groupAndOrderBy + " LIMIT ? OFFSET ?";
        return Base::items( req, nbItems, offset );
    }

    virtual Result all() override
    {
        const std::string req = "SELECT " + m_field + " " + m_base + " " +
                m_groupAndOrderBy;
        return Base::all( req );
    }

private:
    std::string m_field;
    std::string m_base;
    std::string m_groupAndOrderBy;
};

template <typename Impl, typename Intf = Impl, typename... Args>
Query<Intf> make_query( MediaLibraryPtr ml, std::string field, std::string base,
                        std::string orderAndGroupBy, Args&&... args )
{
    return std::unique_ptr<IQuery<Intf>>(
        new SqliteQuery<Impl, Intf, Args...>( ml, std::move( field ),
                                            std::move( base ),
                                            std::move( orderAndGroupBy ),
                                            std::forward<Args>( args )... )
    );
}

}
