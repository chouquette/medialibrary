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

#include "Task.h"

namespace medialibrary
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
    virtual parser::Task::Status run( parser::Task& task ) = 0;
    /**
     * @brief name returns the name of this service.
     *
     * This is for logging purposes only.
     */
    virtual const char* name() const = 0;
    /**
     * @brief nbThreads Returns the number of thread this service wishes to run
     *
     * @return Concurrency, including database coherence, is the service's
     * responsibility.
     */
    virtual uint8_t nbThreads() const = 0;
    /**
     * @brief isCompleted Probes a task for completion with regard to this service.
     * @param task The task probed for completion
     * @return true if the task is completed, false otherwise.
     */
    virtual bool isCompleted( const parser::Task& task ) const = 0;

    /**
     * @brief initialize Run service specific initialization.
     *
     * By the time this function is called, the database is fully initialized and
     * can be used.
     *
     * If false is returned, the service will be released and won't be used.
     */
    virtual bool initialize() = 0;
};

}
