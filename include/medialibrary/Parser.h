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

#include <cstdint>

namespace medialibrary
{
namespace parser
{

enum class Status
{
    /// Default value.
    /// Also, having success = 0 is not the best idea ever.
    Unknown,
    /// All good
    Success,
    /// We can't compute this file for now (for instance the file was on a network drive which
    /// isn't connected anymore)
    TemporaryUnavailable,
    /// Something failed and we won't continue
    Fatal,
    /// The task must now be considered completed, regardless of the
    /// current step.
    Completed,
    /// The task should be discarded, regardless of its status
    /// This is likely to be used when trying to parse playlist items,
    /// as they already could have been queued before.
    Discarded,
};

enum class Step : uint8_t
{
    None = 0,
    MetadataExtraction = 1,
    MetadataAnalysis = 2,

    Completed = 1 | 2,
};

}
}
