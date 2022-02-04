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

#include "SqliteTransaction.h"

#include "SqliteTools.h"

namespace medialibrary
{

namespace sqlite
{

thread_local Transaction* Transaction::CurrentTransaction = nullptr;

ActualTransaction::ActualTransaction( sqlite::Connection* dbConn)
    : m_dbConn( dbConn )
    , m_ctx( dbConn )
{
    assert( CurrentTransaction == nullptr );
    LOG_VERBOSE( "Starting SQLite transaction" );
    Statement s( m_ctx.handle(), "BEGIN EXCLUSIVE" );
    s.execute();
    while ( s.row() != nullptr )
        ;
    CurrentTransaction = this;
}

void ActualTransaction::commit()
{
    assert( CurrentTransaction != nullptr );
    auto chrono = std::chrono::steady_clock::now();
    Statement s( m_ctx.handle(), "COMMIT" );
    s.execute();
    while ( s.row() != nullptr )
        ;
    auto duration = std::chrono::steady_clock::now() - chrono;
    LOG_VERBOSE( "Flushed transaction in ",
             std::chrono::duration_cast<std::chrono::microseconds>( duration ).count(), "µs" );
    CurrentTransaction = nullptr;
    m_ctx.unlock();
}

bool Transaction::isInProgress()
{
    return CurrentTransaction != nullptr;
}

ActualTransaction::~ActualTransaction()
{
    if ( CurrentTransaction != nullptr )
    {
        try
        {
            Statement s( m_ctx.handle(), "ROLLBACK" );
            s.execute();
            while ( s.row() != nullptr )
                ;
        }
        // Ignore a rollback failure as it is most likely innocuous (see
        // http://www.sqlite.org/lang_transaction.html
        catch( const std::exception& ex )
        {
            LOG_WARN( "Failed to rollback transaction: ", ex.what() );
        }
        CurrentTransaction = nullptr;
    }
}

NoopTransaction::NoopTransaction()
{
    assert( Transaction::isInProgress() == true );
}

NoopTransaction::~NoopTransaction()
{
    assert( Transaction::isInProgress() == true );
}

void NoopTransaction::commit()
{
}

}

}
