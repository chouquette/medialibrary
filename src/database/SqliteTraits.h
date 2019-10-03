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

#include <sqlite3.h>
#include <tuple>
#include <atomic>
#include <utility>

#include "SqliteErrors.h"

namespace medialibrary
{

namespace sqlite
{

struct ForeignKey
{
    constexpr explicit ForeignKey(int64_t v) : value(v) {}
    int64_t value;
};

struct NullableString
{
    explicit NullableString( std::string str ) : s( std::move( str ) ) {}
    std::string s;
};

template <typename ToCheck, typename T>
using IsSameDecay = std::is_same<typename std::decay<ToCheck>::type, T>;

template <typename T, typename Enable = void>
struct Traits;

template <typename T>
struct Traits<T, typename std::enable_if<
        std::is_integral<typename std::decay<T>::type>::value
        && ! IsSameDecay<T, int64_t>::value
    >::type>
{
    static constexpr
    int (*Bind)(sqlite3_stmt *, int, int) = &sqlite3_bind_int;

    static constexpr
    int (*Load)(sqlite3_stmt *, int) = &sqlite3_column_int;
};

template <typename T>
struct Traits<T, typename std::enable_if<IsSameDecay<T, ForeignKey>::value>::type>
{
    static int Bind( sqlite3_stmt *stmt, int pos, ForeignKey fk)
    {
        if ( fk.value != 0 )
            return Traits<unsigned int>::Bind( stmt, pos, fk.value );
        return sqlite3_bind_null( stmt, pos );
    }
};

template <typename T>
struct Traits<T, typename std::enable_if<IsSameDecay<T, std::string>::value ||
                                         IsSameDecay<T, const char*>::value ||
                                         IsSameDecay<T, char*>::value>::type>
{
    static int Bind(sqlite3_stmt* stmt, int pos, const std::string& value )
    {
        return sqlite3_bind_text( stmt, pos, value.c_str(), -1, SQLITE_STATIC );
    }

    static std::string Load( sqlite3_stmt* stmt, int pos )
    {
        auto tmp = reinterpret_cast<const char*>( sqlite3_column_text( stmt, pos ) );
        if ( tmp != nullptr )
            return std::string( tmp );
        return std::string();
    }
};

template <typename T>
struct Traits<T, typename std::enable_if<std::is_floating_point<
        typename std::decay<T>::type
    >::value>::type>
{
        static constexpr int
        (*Bind)(sqlite3_stmt *, int, double) = &sqlite3_bind_double;

        static constexpr double
        (*Load)(sqlite3_stmt *, int) = &sqlite3_column_double;
};

template <>
struct Traits<std::nullptr_t>
{
    static int Bind(sqlite3_stmt* stmt, int idx, std::nullptr_t)
    {
        return sqlite3_bind_null( stmt, idx );
    }
};

template <typename T>
struct Traits<T, typename std::enable_if<std::is_enum<
        typename std::decay<T>::type
    >::value>::type>
{
    using type_t = typename std::underlying_type<typename std::decay<T>::type>::type;
    static int Bind(sqlite3_stmt* stmt, int pos, T value )
    {
        return sqlite3_bind_int( stmt, pos, static_cast<type_t>( value ) );
    }

    static T Load( sqlite3_stmt* stmt, int pos )
    {
        return static_cast<T>( sqlite3_column_int( stmt, pos ) );
    }
};


template <typename T>
struct Traits<T, typename std::enable_if<IsSameDecay<T, int64_t>::value>::type>
{
    static constexpr int
    (*Bind)(sqlite3_stmt *, int, sqlite_int64) = &sqlite3_bind_int64;

    static constexpr sqlite_int64
    (*Load)(sqlite3_stmt *, int) = &sqlite3_column_int64;
};

template <typename T>
struct Traits<T, typename std::enable_if<IsSameDecay<T, NullableString>::value>::type>
{
    static int Bind( sqlite3_stmt *stmt, int pos, const NullableString& ns )
    {
        if ( ns.s.empty() == false )
            return Traits<std::string>::Bind( stmt, pos, ns.s );
        return sqlite3_bind_null( stmt, pos );
    }
};

template <typename Gen, template <typename...> class Inst>
struct is_instanciation_of : std::false_type {};

template <template <typename...> class T, typename... Args>
struct is_instanciation_of<T<Args...>, T> : std::true_type
{
};

static_assert( is_instanciation_of<std::tuple<int, float, double>, std::tuple>::value,
               "Invalid is_instanciation_of helper implementation" );

template <size_t... Ns>
struct IndexSequence
{
    using type = IndexSequence<Ns..., sizeof...(Ns)>;
};

template <size_t N>
struct MakeIndexSequence
{
    // This will recurse down to IndexSequence<> which yields type = IndexSequence<0>
    // Then, from Seq<0>::type, up to Seq<N-1>::type, which ends
    // up generating Seq<0, 1, ... N - 1>
    using type = typename MakeIndexSequence<N - 1>::type::type;
};

template <>
struct MakeIndexSequence<0>
{
    using type = IndexSequence<>;
};

template <typename T>
struct Traits<T, typename std::enable_if<
        is_instanciation_of<typename std::decay<T>::type, std::tuple>::value &&
        (std::tuple_size<typename std::decay<T>::type>::value > 0)
    >::type>
{
private:
    template <typename Value>
    static bool bind_inner( sqlite3_stmt* stmt, int& pos, Value&& value )
    {
        int res = Traits<Value>::Bind( stmt, pos, std::forward<Value>( value ) );
        if ( res != SQLITE_OK )
            errors::mapToException( sqlite3_sql( stmt ), "Failed to bind parameter", res );
        ++pos;
        return true;
    }

    template <typename Tuple, size_t... Indices>
    static void for_each_bind_tuple( sqlite3_stmt* stmt, int& pos, Tuple&& t, IndexSequence<Indices...> )
    {
        (void)std::initializer_list<bool> {
            bind_inner( stmt, pos, std::get<Indices>( std::forward<Tuple>( t ) ) )...
        };
    }

public:
    template <typename Tuple>
    static int Bind(sqlite3_stmt* stmt, int& pos, Tuple&& values )
    {
        static_assert(std::is_same<typename std::decay<Tuple>::type,
                                   typename std::decay<T>::type>
                      ::value, "Incompatible types");
        constexpr auto TupleSize = std::tuple_size<typename std::decay<T>::type>::value;
        for_each_bind_tuple( stmt, pos, std::forward<Tuple>( values ),
                             typename MakeIndexSequence<TupleSize>::type{} );
        // Decrement the position since the original SqliteTools::_bind call will
        // increment the position for each parameter.
        assert(pos >= 1);
        --pos;
        return SQLITE_OK;
    }
};

// Provide a specialization for empty tuples
template <typename T>
struct Traits<T, typename std::enable_if<
        std::is_same<typename std::decay<T>::type, std::tuple<>>::value>::type
    >
{
    static int Bind(sqlite3_stmt*, int& pos, std::tuple<> )
    {
        // Decrement the position since the original SqliteTools::_bind call will
        // increment the position for each parameter.
        assert(pos >= 1);
        --pos;
        return SQLITE_OK;
    }
};

template <typename T>
struct Traits<T, typename std::enable_if<
        is_instanciation_of<typename std::decay<T>::type, std::atomic>::value>::type
    >
{
    // std::atomic::value_type is C++17 only
    using value_type = decltype( std::declval<T>().load() );

    static int Bind( sqlite3_stmt* stmt, int pos, const T& value )
    {
        return Traits<value_type>::Bind( stmt, pos, value.load() );
    }

    static value_type Load( sqlite3_stmt* stmt, int pos )
    {
        return Traits<value_type>::Load( stmt, pos );
    }
};

static_assert( is_instanciation_of<std::atomic_uint, std::atomic>::value == true,
               "std::atomic_uint should be an instanciation of std::atomic" );

} // namespace sqlite

}
