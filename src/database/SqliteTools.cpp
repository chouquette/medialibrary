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

#if HAVE_CONFIG_H
# include "config.h"
#endif

#include "SqliteTools.h"

namespace medialibrary
{

namespace sqlite
{
std::unordered_map<Connection::Handle,
                   Statement::StatementsCacheMap> Statement::StatementsCache;

compat::Mutex Statement::StatementsCacheLock;

std::string Tools::sanitizePattern( const std::string& pattern )
{
    // This assumes the input pattern is a regular user input, ie. that it is not
    // supposed to contain anything that ressembles SQL. What this actually means
    // is that if the user provides an escaped double quote «""» it will result in 2
    // escaped double quotes («""""»)
    std::string res;
    res.reserve( pattern.size() + 3 );
    res.append( "\"" );
    for ( const auto c : pattern )
    {
        if ( c == '"' || c == '\'' )
            res += c;
        res += c;
    }
    res.append( "*\"" );
    return res;
}

Statement::Statement(Connection::Handle dbConnection, const std::string& req)
    : m_stmt( nullptr, [](sqlite3_stmt* stmt) {
        sqlite3_clear_bindings( stmt );
        sqlite3_reset( stmt );
    })
    , m_dbConn( dbConnection )
    , m_bindIdx( 0 )
    , m_isCommit( false )
{
    std::lock_guard<compat::Mutex> lock( StatementsCacheLock );
    auto& connMap = StatementsCache[ dbConnection ];
    auto it = connMap.find( req );
    if ( it == end( connMap ) )
    {
        sqlite3_stmt* stmt;
        int res = sqlite3_prepare_v2( dbConnection, req.c_str(),
                                      req.size() + 1, &stmt, nullptr );
        if ( res != SQLITE_OK )
        {
            errors::mapToException( req.c_str(), sqlite3_errmsg( dbConnection ),
                                    res );
        }
        m_stmt.reset( stmt );
        connMap.emplace( req, CachedStmtPtr( stmt, &sqlite3_finalize ) );
    }
    else
    {
        m_stmt.reset( it->second.get() );
    }
    if ( req == "COMMIT" )
        m_isCommit = true;
}

Statement::Statement( const std::string& req )
    : Statement( Connection::Context::handle(), req )
{
}

Row Statement::row()
{
    auto maxRetries = 10;
    while ( true )
    {
        auto extRes = sqlite3_step( m_stmt.get() );
        auto res = extRes & 0xFF;
        if ( res == SQLITE_ROW )
            return Row( m_stmt.get() );
        else if ( res == SQLITE_DONE )
            return Row{};
        else if ( ( Transaction::isInProgress() == false ||
                    m_isCommit == true ) &&
                  errors::isInnocuous( res ) && maxRetries-- > 0 )
            continue;
        auto errMsg = sqlite3_errmsg( m_dbConn );
        const char* reqStr = sqlite3_sql( m_stmt.get() );
        if ( reqStr == nullptr )
            reqStr = "<unknown request>";
        errors::mapToException( reqStr, errMsg, extRes );
        assert( !"unreachable" );
    }
}

void Statement::FlushStatementCache()
{
    std::lock_guard<compat::Mutex> lock( StatementsCacheLock );
    StatementsCache.clear();
}

void Statement::FlushConnectionStatementCache( Connection::Handle h )
{
    std::lock_guard<compat::Mutex> lock( StatementsCacheLock );
    auto it = StatementsCache.find( h );
    if ( it != end( StatementsCache ) )
        StatementsCache.erase( it );
}

}

}
