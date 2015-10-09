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

#include "Emotion.h"

#include <iostream>
#include <Ecore.h>
#include <Evas.h>
#include <Ecore_Evas.h>
#include <Emotion.h>

#include "IFile.h"

EmotionMetadataService::EmotionMetadataService()
    : m_initialized( false )
{
}

EmotionMetadataService::~EmotionMetadataService()
{
    if ( m_initialized )
        evas_shutdown();
}

bool EmotionMetadataService::initialize( IMetadataServiceCb *callback, IMediaLibrary *ml )
{
    m_cb = callback;
    m_ml = ml;
    if ( ecore_evas_init() == 0 )
    {
        throw std::runtime_error( "Failed to initialize Evas" );
    }
    m_initialized = true;
    return true;
}

unsigned int EmotionMetadataService::priority() const
{
    return 1000;
}

bool EmotionMetadataService::run( FilePtr file, void *data )
{
    auto evas = ecore_evas_new( nullptr, 0, 0, 10, 10, nullptr );
    if ( evas == nullptr )
        return false;
    std::unique_ptr<Ecore_Evas, decltype(&ecore_evas_free)> evas_u( evas, &ecore_evas_free );

    auto e = ecore_evas_get( evas );
    auto em = emotion_object_add( e );
    if ( emotion_object_init( em, "libvlc" ) != EINA_TRUE )
        return false;
    if ( emotion_object_file_set( em, file->mrl().c_str() ) != EINA_TRUE )
        return false;

    auto meta = emotion_object_meta_info_get( em, EMOTION_META_INFO_TRACK_ARTIST );
    std::cout << "track album: " << meta << std::endl;

    m_cb->done( file, StatusSuccess, data );
    return true;
}
