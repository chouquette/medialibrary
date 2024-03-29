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

    size_t executeCount( const std::string& req ) const
    {
        auto dbConn = m_ml->getConn();
        OPEN_READ_CONTEXT( ctx, dbConn );
        sqlite::QueryTimer qt{ req };
        sqlite::Statement stmt( req );
        stmt.execute( m_params );
        auto row = stmt.row();
        size_t count;
        row >> count;
        assert( stmt.row() == nullptr );
        return count;
    }

    Result executeFetchItems( const std::string& req, uint32_t nbItems, uint32_t offset ) const
    {
        return Impl::template fetchAll<Intf>( m_ml, req, m_params, nbItems, offset );
    }

    Result executeFetchAll( const std::string& req ) const
    {
        return Impl::template fetchAll<Intf>( m_ml, req, m_params );
    }

private:
    MediaLibraryPtr m_ml;
    const std::tuple<typename std::decay<RequestParams>::type...> m_params;
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

    void markPublic( bool isPublic )
    {
        if ( isPublic == true )
            m_field += ", TRUE";
    }

    virtual size_t count() override
    {
        std::string req = "SELECT COUNT(DISTINCT " + Impl::Table::PrimaryKeyColumn +
                " ) " + m_base;
        return Base::executeCount( req );
    }

    virtual Result items( uint32_t nbItems, uint32_t offset ) override
    {
        if ( nbItems == 0 && offset == 0 )
            return all();
        const std::string req = "SELECT " + m_field + " " + m_base + " " +
                m_groupAndOrderBy + " LIMIT ? OFFSET ?";
        return Base::executeFetchItems( req, nbItems, offset );
    }

    virtual Result all() override
    {
        const std::string req = "SELECT " + m_field + " " + m_base + " " +
                m_groupAndOrderBy;
        return Base::executeFetchAll( req );
    }

private:
    std::string m_field;
    const std::string m_base;
    const std::string m_groupAndOrderBy;
};

/**
 * Alternate query implementation, with 2 full blown requests for counting & listing
 * This can be more efficient if the listing query needs to join with multiple tables
 * while counting can be achieved by a simple SELECT COUNT(*) FROM Table;
 */
template <typename Impl, typename Intf = Impl, typename... RequestParams>
class SqliteQueryWithCount : public SqliteQueryBase<Impl, Intf, RequestParams...>
{
public:
    using Base = SqliteQueryBase<Impl, Intf, RequestParams...>;
    using Result = typename Base::Result;

    template <typename... Args>
    SqliteQueryWithCount( MediaLibraryPtr ml, std::string countReq,
                          std::string req, Args&&... args )
        : SqliteQueryBase<Impl, Intf, Args...>( ml, std::forward<Args>( args )... )
        , m_countReq( std::move( countReq ) )
        , m_req ( std::move( req ) )
    {
    }

    virtual size_t count() override
    {
        return Base::executeCount( m_countReq );
    }

    virtual Result items( uint32_t nbItems, uint32_t offset ) override
    {
        if ( nbItems == 0 && offset == 0 )
            return all();
        return Base::executeFetchItems( m_req + " LIMIT ? OFFSET ?", nbItems, offset );
    }

    virtual Result all() override
    {
        return Base::executeFetchAll( m_req );
    }

private:
    const std::string m_countReq;
    const std::string m_req;
};

template <typename Impl, typename Intf = Impl, typename... Args>
class QueryBuilder
{
public:
    using Query = std::unique_ptr<SqliteQuery<Impl, Intf, Args...>>;
    QueryBuilder( Query q ) : m_query( std::move( q ) ) {}
    QueryBuilder& markPublic( bool p ) { m_query->markPublic( p ); return *this; }
    Query build() { return std::move( m_query ); }

private:
    Query m_query;
};

template <typename Impl, typename Intf = Impl, typename... Args>
QueryBuilder<Impl, Intf, Args...>
make_query( MediaLibraryPtr ml, std::string field, std::string base,
            std::string orderAndGroupBy, Args&&... args )
{
    return QueryBuilder<Impl, Intf, Args...>{
            std::make_unique<SqliteQuery<Impl, Intf, Args...>>( ml, std::move( field ),
                                            std::move( base ),
                                            std::move( orderAndGroupBy ),
                                            std::forward<Args>( args )... )
    };
}

template <typename Impl, typename Intf = Impl, typename... Args>
std::unique_ptr<SqliteQueryWithCount<Impl, Intf, Args...>>
make_query_with_count( MediaLibraryPtr ml, std::string countReq, std::string req,
                       Args&&... args )
{
    return std::make_unique<SqliteQueryWithCount<Impl, Intf, Args...>>(
                    ml, std::move( countReq ), std::move( req ),
                    std::forward<Args>( args )... );
}




}
