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

#include <cstring>
#include <memory>
#include <type_traits>
#include <sstream>

namespace
{

template <typename T, typename Enabled = void>
struct TestFailedDisplayer
{
    static T display( const T& t )
    {
        return t;
    }
};

template <typename T>
struct TestFailedDisplayer<T, std::enable_if_t<std::is_enum<std::decay_t<T>>::value>>
{
    using Enum = std::decay_t<T>;
    static std::underlying_type_t<Enum> display( T t )
    {
        return static_cast<std::underlying_type_t<Enum>>( t );
    }
};

template <typename T, typename Deleter>
struct TestFailedDisplayer<std::unique_ptr<T, Deleter>>
{
    static T* display( const std::unique_ptr<T, Deleter>& up )
    {
        return up.get();
    }
};

template <typename T>
struct TestFailedDisplayer<std::shared_ptr<T>>
{
    static T* display( const std::shared_ptr<T>& up )
    {
        return up.get();
    }
};

template <>
struct TestFailedDisplayer<std::nullptr_t>
{
    static const char* display( std::nullptr_t )
    {
        return "<nullptr>";
    }
};

}

struct TestFailed : public std::exception
{
    template <typename T1, typename T2>
    TestFailed( T1&& lhs, T2&& rhs,
                const char* lhsStr, const char* rhsStr,
                const char* file, int line )
    {
        std::stringstream s;
        s << "Invalid comparison at " << file << " at line " << line
             << "\n" << lhsStr << " != " << rhsStr
             << "\n\tActual values: "
             << TestFailedDisplayer<std::decay_t<T1>>::display( lhs )
             << " != "
             << TestFailedDisplayer<std::decay_t<T2>>::display( rhs );
        m_str = s.str();
    }

    TestFailed( const std::string& txt, const char* file, int line )
    {
        std::stringstream s;
        s << "Test failed: " << txt << " at " << file << " at line " << line;
        m_str = s.str();
    }

    const char* what() const noexcept override
    {
        return m_str.c_str();
    }

private:
    std::string m_str;
};

#define FAIL_TEST( lhs, rhs ) \
    throw TestFailed{ ( lhs ), ( rhs ), #lhs, #rhs, __FILE__, __LINE__ }

#define FAIL_TEST_MSG( msg ) \
    throw TestFailed{ msg, __FILE__, __LINE__ }

#define ASSERT_EQ( lhs, rhs ) \
    do { if ( ( lhs ) != ( rhs ) ) { FAIL_TEST( ( lhs ), ( rhs ) ); } } while ( 0 )

#define ASSERT_NE( lhs, rhs ) \
    do { if ( ( lhs ) == ( rhs ) ) { FAIL_TEST( ( lhs ), ( rhs ) ); } } while ( 0 )

#define ASSERT_TRUE( val ) \
    do { if ( ( val ) != true ) { FAIL_TEST( ( val ), true ); } } while ( 0 )

#define ASSERT_FALSE( val ) \
    do { if ( ( val ) != false ) { FAIL_TEST( ( val ), false ); } } while ( 0 )

#define ASSERT_NON_NULL( val ) \
    do { if ( val == nullptr ) { FAIL_TEST( ( val ), nullptr ); } } while ( 0 )

#define ASSERT_LE( lhs, rhs ) \
    do { if ( ( lhs ) > ( rhs ) ) { FAIL_TEST( ( lhs ), ( rhs ) ); } } while ( 0 )

#define ASSERT_THROW( stmt, ex_type ) \
    do { \
        auto thrown = false; \
        try { ( stmt ); } \
        catch ( const ex_type& ) { \
            thrown = true; \
        } catch ( ... ) { \
            FAIL_TEST_MSG( std::string{ "Expected exception of type " } + \
                #ex_type " but caught another" ); \
        } \
        if ( thrown == false ) { \
            FAIL_TEST_MSG( std::string{ "Expected exception: " } + #ex_type " wasn't thrown" ); \
        } \
    } while ( 0 )
