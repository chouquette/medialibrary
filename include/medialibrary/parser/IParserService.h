/*****************************************************************************
 * Media Library
 *****************************************************************************
 * Copyright (C) 2015-2018 Hugo Beauzée-Luyssen, Videolabs, VideoLAN
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

#include "medialibrary/parser/Parser.h"
#include "medialibrary/parser/IItem.h"

namespace medialibrary
{

class IMediaLibrary;

namespace parser
{

class IParserService
{
public:
    virtual ~IParserService() = default;

    /**
     * @brief run  Process a specific task
     * @param task The task to be processed
     * @return A status code
     */
    virtual parser::Status run( parser::IItem& item ) = 0;
    /**
     * @brief name returns the name of this service.
     *
     * This is for logging purposes only.
     */
    virtual const char* name() const = 0;

    /**
     * @brief targetedStep Returns the ParserStep targeted by this service.
     */
    virtual parser::Step targetedStep() const = 0;

    /**
     * @brief initialize Run service specific initialization.
     *
     * By the time this function is called, the database is fully initialized and
     * can be used.
     * A pointer the the medialibrary is provided to allow the service to
     * create/fetch entities.
     *
     * If false is returned, the service will be released and won't be used.
     */
    virtual bool initialize( IMediaLibrary* ml ) = 0;

    /**
     * @brief onFlushing will be invoked prior to restarting/flushing the service
     *
     * A service must release any database entity it might hold.
     * The service will be paused or unstarted when this method gets called.
     */
    virtual void onFlushing() = 0;

    /**
     * @brief onRestarted will be invoked prior to a service restart.
     *
     * A restart will always occur after a flush. Once this function gets called,
     * the service is free to cache some database entities, or interact with the
     * medialibrary again.
     * The thread(s) running the service itself won't have been restarted when this
     * function gets called.
     */
    virtual void onRestarted() = 0;

    /**
     * @brief stop Require the service to interrupt its processing as soon as possible
     */
    virtual void stop() = 0;
};

}
}
