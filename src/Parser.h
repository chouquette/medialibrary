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

#ifndef PARSER_H
#define PARSER_H

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <memory>
#include <queue>

#include "IMetadataService.h"
#include "IMediaLibrary.h"

class Parser : public IMetadataServiceCb
{
    public:
        Parser(DBConnection dbConnection);
        ~Parser();
        void addService(std::unique_ptr<IMetadataService> service );
        void parse( std::shared_ptr<Media> file, IMediaLibraryCb* cb );

    private:
        virtual void done( std::shared_ptr<Media> file, IMetadataService::Status status, void* data ) override;
        void run();
        // Queues all unparsed files for parsing.
        void restore();

    private:
        typedef std::unique_ptr<IMetadataService> ServicePtr;
        typedef std::vector<ServicePtr> ServiceList;
        struct Task
        {
            Task(std::shared_ptr<Media> file, ServiceList& serviceList , IMediaLibraryCb* metadataCb);
            std::shared_ptr<Media>   file;
            ServiceList::iterator   it;
            ServiceList::iterator   end;
            IMediaLibraryCb*        cb;
        };

    private:
        ServiceList m_services;
        std::queue<Task*> m_tasks;
        std::thread m_thread;
        std::mutex m_lock;
        std::condition_variable m_cond;
        std::atomic_bool m_stopParser;
        DBConnection m_dbConnection;
};

#endif // PARSER_H
