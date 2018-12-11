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

#include <string>

#include "medialibrary/ILabel.h"
#include "database/DatabaseHelpers.h"

namespace medialibrary
{

class Media;
class Label;

class Label : public ILabel, public DatabaseHelpers<Label>
{
    public:
        struct Table
        {
            static const std::string Name;
            static const std::string PrimaryKeyColumn;
            static int64_t Label::*const PrimaryKey;
        };
        Label( MediaLibraryPtr ml, sqlite::Row& row );
        Label( MediaLibraryPtr ml, const std::string& name );

    public:
        virtual int64_t id() const override;
        virtual const std::string& name() const override;
        virtual Query<IMedia> media() override;

        static LabelPtr create( MediaLibraryPtr ml, const std::string& name );
        static void createTable( sqlite::Connection* dbConnection );
        static void createTriggers( sqlite::Connection* dbConnection );

    private:
        MediaLibraryPtr m_ml;
        int64_t m_id;
        const std::string m_name;

        friend struct Label::Table;
};

}
