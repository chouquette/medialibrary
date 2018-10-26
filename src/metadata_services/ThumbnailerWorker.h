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

#include "compat/ConditionVariable.h"
#include "compat/Thread.h"
#include "medialibrary/Types.h"
#include "medialibrary/IThumbnailer.h"
#include "Types.h"

#include <queue>
#include <atomic>

namespace medialibrary
{

class ThumbnailerWorker
{
public:
    explicit ThumbnailerWorker( MediaLibraryPtr ml );
    virtual ~ThumbnailerWorker();
    void requestThumbnail( MediaPtr media );
    void pause();
    void resume();

private:
    void run();
    void stop();

    bool generateThumbnail( MediaPtr task );

private:


private:
    MediaLibraryPtr m_ml;
    compat::Mutex m_mutex;
    compat::ConditionVariable m_cond;
    std::queue<MediaPtr> m_tasks;
    std::atomic_bool m_run;
    std::unique_ptr<IThumbnailer> m_generator;
    compat::Thread m_thread;
    bool m_paused;
};

}
