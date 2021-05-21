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

/*
 * We don't know how the test macros will be invoked, and don't want to evaluate
 * the values more than once.
 * However:
 * - We can't just store them in a temporary directly since some parameters
 *   will only be valid during the expression lifetime, for instance something like
 *   value->fetchContent()->getAnotherValue() will trigger a use-after-free if
 *   fetchContent() returns by value and getAnotherValue() returns a reference,
 *   meaning we sometimes have to store a copy.
 * - We also can't just copy since some of the tested values won't be copyable
 *   mostly when we check a unique_ptr for nullity.
 *
 * So basically, if we can copy, we do so, if we can't, we return a reference
 */
namespace
{
template <typename T>
std::enable_if_t<std::is_copy_assignable<std::decay_t<T>>::value, T> getTestedValue( T t )
{
    return t;
}

template <typename T>
std::enable_if_t<!std::is_copy_assignable<std::decay_t<T>>::value, T&> getTestedValue( T& t )
{
    return t;
}
}

#define FAIL_TEST( lhsValue, rhsValue, lhsExp, rhsExp ) \
    throw TestFailed{ ( lhsValue ), ( rhsValue ), lhsExp, rhsExp, __FILE__, __LINE__ }

#define FAIL_TEST_MSG( msg ) \
    throw TestFailed{ msg, __FILE__, __LINE__ }

#define ASSERT_EQ( lhsExp, rhsExp ) \
    do { \
        auto&& lhsValue = getTestedValue( lhsExp ); \
        auto&& rhsValue = getTestedValue( rhsExp ); \
        if ( lhsValue != rhsValue ) { \
            FAIL_TEST( lhsValue, rhsValue, ( #lhsExp ), ( #rhsExp ) ); \
        } \
    } while ( 0 )

#define ASSERT_NE( lhsExp, rhsExp ) \
    do { \
        auto&& lhsValue = getTestedValue( lhsExp ); \
        auto&& rhsValue = getTestedValue( rhsExp ); \
        if ( lhsValue == rhsValue ) { \
            FAIL_TEST( std::forward<decltype(lhsValue)>( lhsValue ), \
                       std::forward<decltype(rhsValue)>( rhsValue ), \
                       ( #lhsExp ), ( #rhsExp ) ); \
        } \
    } while ( 0 )

#define ASSERT_TRUE( exp ) \
    do { \
        auto&& value = getTestedValue( exp ); \
        if ( ( value ) != true ) { \
            FAIL_TEST( std::forward<decltype(value)>( value ), true, \
                       ( #exp ), "true" ); \
        } \
    } while ( 0 )

#define ASSERT_FALSE( exp ) \
    do { \
        auto&& value = getTestedValue( exp ); \
        if ( ( value ) != false ) { \
            FAIL_TEST( std::forward<decltype(value)>( value ), false, \
                       ( #exp ), "false" ); \
        } \
    } while ( 0 )

#define ASSERT_NON_NULL( exp ) \
    do { \
        auto&& value = getTestedValue( exp ); \
        if ( value == nullptr ) { \
            FAIL_TEST( std::forward<decltype(value)>( value ), nullptr, \
                       ( #exp ), "nullptr" ); \
        } \
    } while ( 0 )

#define ASSERT_LE( lhsExp, rhsExp ) \
    do { \
        auto&& lhsValue = getTestedValue( lhsExp ); \
        auto&& rhsValue = getTestedValue( rhsExp ); \
        if ( lhsValue > rhsValue ) { \
            FAIL_TEST( std::forward<decltype(lhsValue)>( lhsValue ), \
                       std::forward<decltype(rhsValue)>( rhsValue ), \
                       ( #lhsExp ), ( #rhsExp ) ); \
        } \
    } while ( 0 )

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
