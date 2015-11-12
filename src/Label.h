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

#ifndef LABEL_H
#define LABEL_H

#include <sqlite3.h>
#include <string>

class Media;
class Label;

#include "ILabel.h"
#include "database/Cache.h"

namespace policy
{
struct LabelTable
{
    static const std::string Name;
    static const std::string CacheColumn;
    static unsigned int Label::*const PrimaryKey;
};

struct LabelCachePolicy
{
    typedef std::string KeyType;
    static const std::string& key(const ILabel* self );
    static std::string key( sqlite::Row& row );
};

}

class Label : public ILabel, public Cache<Label, ILabel, policy::LabelTable, policy::LabelCachePolicy>
{
    private:
        typedef Cache<Label, ILabel, policy::LabelTable, policy::LabelCachePolicy> _Cache;
    public:
        Label( DBConnection dbConnection, sqlite::Row& row );
        Label( const std::string& name );

    public:
        virtual unsigned int id() const override;
        virtual const std::string& name() const override;
        virtual std::vector<MediaPtr> files() override;

        static LabelPtr create( DBConnection dbConnection, const std::string& name );
        static bool createTable( DBConnection dbConnection );
    private:
        DBConnection m_dbConnection;
        unsigned int m_id;
        std::string m_name;

        friend class Cache<Label, ILabel, policy::LabelTable>;
        friend struct policy::LabelTable;
};

#endif // LABEL_H
