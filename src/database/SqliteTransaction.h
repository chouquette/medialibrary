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

#include "SqliteConnection.h"
#include "Types.h"

#include <functional>
#include <vector>

namespace medialibrary
{

namespace sqlite
{

class Transaction
{
public:
    Transaction() = default;
    virtual ~Transaction() = default;
    virtual void commit() = 0;

    static bool transactionInProgress();

    Transaction( const Transaction& ) = delete;
    Transaction( Transaction&& ) = delete;
    Transaction& operator=( const Transaction& ) = delete;
    Transaction& operator=( Transaction&& ) = delete;

    static thread_local Transaction* CurrentTransaction;
};

class ActualTransaction : public Transaction
{
public:
    explicit ActualTransaction( sqlite::Connection* dbConn );
    virtual void commit() override;

    virtual ~ActualTransaction();

private:
    sqlite::Connection* m_dbConn;
    Connection::WriteContext m_ctx;
};

class NoopTransaction : public Transaction
{
public:
    NoopTransaction();
    virtual ~NoopTransaction();
    virtual void commit() override;
};

}

}
