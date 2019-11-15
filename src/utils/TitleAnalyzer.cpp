/*****************************************************************************
 * Media Library
 *****************************************************************************
 * Copyright (C) 2019 Hugo Beauzée-Luyssen, Videolabs, VideoLAN
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

#if HAVE_CONFIG_H
# include "config.h"
#endif

#include "TitleAnalyzer.h"

#include <regex>

namespace medialibrary
{

namespace utils
{

namespace title
{

#define SEPARATORS "(\\.|-|_|\\+)"

std::string sanitize( const std::string& fileName )
{
    static const struct {
        std::regex pattern;
        const char* substitute;
    } replacePatterns[] =
    {
        // A small subset of patterns that need to be matched between separators
        // *excluding* spaces, but keeping the separators for now
        {
            std::regex{
                SEPARATORS
                "("
                    "MEMENTO"
                ")"
                SEPARATORS,
                std::regex_constants::icase | std::regex_constants::ECMAScript
            },
            "$1$3"
        },
        // Some specific patterns that we want to match in a case sensitive way, and
        // between specific separators
        {
            std::regex{
                "(\\b|" SEPARATORS ")"
                "("
                    "MeGusta|CRiMSON|Eclipse"
                ")"
                "(\\b|" SEPARATORS ")",
                std::regex_constants::ECMAScript
            },
            ""
        },
        // A small subset of patterns to remove that contain separators, and
        // that we want to match using those separators. For instance, "5.1"
        // would be changed to "5 1", and we don't want to remove a potentially
        // relevent string by assuming there was a dot before.
        {
            std::regex{
                "((\\b|" SEPARATORS ")"
                    "("
                        "5\\.1|Web(\\.|-)DL|HD.TS|AT-X|LOST-UGM|BD"
                    ")"
                "(\\b|" SEPARATORS "))|"
                // Attempt to match most <foo>-Raws anime teams
                "(\\[[a-z]+-raws\\])",
                std::regex_constants::icase | std::regex_constants::ECMAScript
            },
            ""
        },
        // Drop the extension:
        {
            std::regex{ "\\.[[:alnum:]]{2,4}$" },
            ""
        },
        {
            // File size, which we need to handle before removing a potential dot
            // We do not use \b before the size pattern to avoid considering
            // <something>.<number>.<nummber>GB as a size, we want a clean
            // <something unrelated><numerator>.<denominator><unit> pattern
            std::regex{
                "(\\s|-|_)(\\d{1,4}(\\.\\d{1,3})?(MB|GB))\\b",
                std::regex_constants::icase | std::regex_constants::ECMAScript
            },
            ""
        },

        // Replace '.' separating words by a space.
        // This is done before removing most of the common patterns, so the
        // word boundaries are still present
        {
            std::regex{ "(\\s|\\b|\\(|\\[|^)" SEPARATORS "(\\b|\\s|\\)|\\]|$)" },
            " "
        },
        {
            // Since this pattern ends with a '!', we can't use it with the other
            // list of patterns that are bound by `\b`. `\b` implies that the
            // current caracter is an alphanumerical character, which isn't the
            // case of '!'
            std::regex{
                "\\bPuyaSubs!",
                std::regex_constants::icase | std::regex_constants::ECMAScript
            },
            ""
        },
        {
            std::regex{
                "\\b("

                // Various patterns:
                "xvid|h264|dvd|rip|divx|x264|hdtv|aac|webrip|"
                "bluray|bdrip|brrip|dvdrip|ac3|HDTC|x265|h265|mp4|mkv|10\\s?bit(s)?|"
                "avi|HDRip|HEVC|YUV420P10|FLAC|"

                // Try to match most resolutions in one go:
                "([0-9]{3,4}(p|i))|"
                // And catch some hardcoded ones if specified without <number><p/i>
                "((7680|4096|4520|3840|2560|2048|2160|1920|1728|1280|720|460)"
                "x"
                "(4320|3072|2540|2160|1536|1440|1080|720|420|360|320))|"

                // Language/subs
                "(VOST( )?([a-z]{2})?)|"

                // Various TV channels
                "HBO|AMC|TX|"
                // AT-X contains a separator, so see above


                // Usually found team names:
                "ETTV|ETHD|DTOne|1337x|xrg|evo|yify|HorribleSubs|"
                "JiyuuNoFansub|ROVERS|YTS(\\s[A-Z]{2,})?|AMZN|RARBG|anoXmous(_){0,2}|"
                "BOKUTOX"
                // Ohys-Raws contains a separator so it's found in the corresponding
                // special rule above

                ")\\b",
                std::regex_constants::icase | std::regex_constants::ECMAScript
            },
            ""
        },
        // Trim spaces in parenthesis/brackets:
        {
            std::regex{
                "(\\(|\\[)\\s+|"    // Spaces after an opening ( or [
                "\\s+(\\)|\\])"     // Spaces before a closing ) or ]
            },
            "$1$2"
        },
        // In case some of the removed patterns were enclosed in [] or (), remove
        // the empty pairs now
        {
            std::regex{ "(\\(\\)|\\[\\])" },
            ""
        },
        // Now that we removed many elements, re-remove the separators since the
        // word boundaries have changed
        {
            std::regex{ "(\\s|\\b|\\(|\\[|^)" SEPARATORS "(\\b|\\s|\\)|\\]|$)" },
            " "
        },
        {
            // Trim the output. Leading & trailing spaces have no group so they will be
            // replaced by an empty string, any multiple space will be replaced by the
            // first group, which is a single space
            std::regex{ "^\\s+|\\s+$|"      // leading/trailing spaces: removed
                        "(\\s)\\s+"         // multiple spaces: merged into 1
            },
            "$1"
        },
    };
    auto res = fileName;
    for ( const auto& r : replacePatterns )
        res = std::regex_replace( res, r.pattern, r.substitute );

    // If we remove the entire content, it probably means we've been too greedy
    // Return
    if ( res.empty() == true )
        return fileName;

    return res;
}

}

}

}
