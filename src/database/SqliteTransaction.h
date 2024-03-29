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

/**
 * @brief The Transaction class represents an abstract transaction.
 *
 * When no transaction is opened, the associated factory (Connection::newTransaction)
 * will instantiate an ActualTransaction, which executes begin/commit
 * Later instantiation while a transaction is already opened will result in a
 * NoopTransaction which does nothing but simplifies the code path for the caller.
 */
class Transaction
{
public:
    Transaction() = default;
    /**
     * If a transaction is in progress and hasn't been committed, the destructor
     * will issue a ROLLBACK
     */
    virtual ~Transaction() = default;
    /**
     * @brief commit Commits all pending changes to the database
     */
    virtual void commit() = 0;
    /**
     * @brief commitNoUnlock This will commit the current transaction but will
     * keep the lock held until the transaction object is destroyed
     */
    virtual void commitNoUnlock() = 0;

    /**
     * @brief isInProgress Returns true if a transaction is opened by the current thread
     */
    static bool isInProgress();

    Transaction( const Transaction& ) = delete;
    Transaction( Transaction&& ) = delete;
    Transaction& operator=( const Transaction& ) = delete;
    Transaction& operator=( Transaction&& ) = delete;

protected:
    static thread_local Transaction* CurrentTransaction;
};

class ActualTransaction : public Transaction
{
public:
    explicit ActualTransaction( sqlite::Connection* dbConn );
    virtual void commit() override;
    virtual void commitNoUnlock() override;

    virtual ~ActualTransaction();

private:
    Connection::WriteContext m_ctx;
};

class NoopTransaction : public Transaction
{
public:
    NoopTransaction();
    virtual ~NoopTransaction();
    virtual void commit() override;
    virtual void commitNoUnlock() override;
};

}

}
