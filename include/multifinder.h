/*
Copyright (c) 2018 Brecht Sanders

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

/**
 * @file multifinder.h
 * @brief multifinder header file with main functions
 * @author Brecht Sanders
 *
 * This header file defines the functions needed search multiple patterns in text data.
 * The goal of this library is not to search for regular expressions or wildcards, only exact matches.
 * \sa     multifinder_process()
 */

#ifndef INCLUDED_MULTIFINDER_H
#define INCLUDED_MULTIFINDER_H

#include <stdlib.h>

/*! \cond PRIVATE */
#if !defined(DLL_EXPORT_MULTIFINDER)
# if defined(_WIN32) && defined(BUILD_MULTIFINDER_DLL)
#  define DLL_EXPORT_MULTIFINDER __declspec(dllexport)
# elif defined(_WIN32) && !defined(STATIC) && !defined(BUILD_MULTIFINDER_STATIC) && !defined(BUILD_MULTIFINDER)
#  define DLL_EXPORT_MULTIFINDER __declspec(dllimport)
# else
#  define DLL_EXPORT_MULTIFINDER
# endif
#endif
/*! \endcond */

/*! \brief version number constants
 * \sa     multifinder_get_version()
 * \sa     multifinder_get_version_string()
 * \name   MULTIFINDER_VERSION_*
 * \{
 */
/*! \brief major version number */
#define MULTIFINDER_VERSION_MAJOR 0
/*! \brief minor version number */
#define MULTIFINDER_VERSION_MINOR 1
/*! \brief micro version number */
#define MULTIFINDER_VERSION_MICRO 1
/*! @} */

/*! \cond PRIVATE */
#define MULTIFINDER_VERSION_STRINGIZE_(major, minor, micro) #major"."#minor"."#micro
#define MULTIFINDER_VERSION_STRINGIZE(major, minor, micro) MULTIFINDER_VERSION_STRINGIZE_(major, minor, micro)
/*! \endcond */

/*! \brief string with dotted version number \hideinitializer */
#define MULTIFINDER_VERSION_STRING MULTIFINDER_VERSION_STRINGIZE(MULTIFINDER_VERSION_MAJOR, MULTIFINDER_VERSION_MINOR, MULTIFINDER_VERSION_MICRO)

/*! \brief string with name of library */
#define MULTIFINDER_NAME "libmultifinder"

/*! \brief string with name and version of library \hideinitializer */
#define MULTIFINDER_FULLNAME MULTIFINDER_NAME " " MULTIFINDER_VERSION_STRING

#ifdef __cplusplus
extern "C" {
#endif

/*! \brief get multifinder version string
 * \param  pmajor        pointer to integer that will receive major version number
 * \param  pminor        pointer to integer that will receive minor version number
 * \param  pmicro        pointer to integer that will receive micro version number
 * \sa     multifinder_get_version_string()
 */
DLL_EXPORT_MULTIFINDER void multifinder_get_version (int* pmajor, int* pminor, int* pmicro);

/*! \brief get multifinder version string
 * \return version string
 * \sa     multifinder_get_version()
 */
DLL_EXPORT_MULTIFINDER const char* multifinder_get_version_string ();

/*! \brief type used as handle
 * \sa     multifinder_create
 * \sa     multifinder_free
 * \sa     multifinder_add_pattern
 * \sa     multifinder_add_allocated_pattern
 * \sa     multifinder_process
 */
typedef struct multifinder_struct* multifinder;

/*! \brief callback function called when a pattern is found
 * \param  handle                handle created with multifinder_create
 * \param  length                length of match
 * \param  patterncallbackdata   user data for matched pattern
 * \param  callbackdata          user data
 * \return 0 to continue processing or non-zero to abort
 * \sa     multifinder_create
 * \sa     multifinder_process
 * \sa     multifinder_add_pattern
 * \sa     multifinder_add_allocated_pattern
 */
typedef int (*multifinder_found_callback_fn)(multifinder handle, size_t length, void* patterncallbackdata, void* callbackdata);

/*! \brief callback function called for data that is not a match
 * \param  data                  data to be flushed (NULL at end of input stream)
 * \param  datalen               length of data to be flushed (zero at end of input stream)
 * \param  callbackdata          user data
 * \sa     multifinder_create
 * \sa     multifinder_process
 * \sa     multifinder_finalize
 */
typedef void (*multifinder_flush_callback_fn)(const char* data, size_t datalen, void* callbackdata);

/*! \brief initialize a new search
 * \sa     multifinder_free
 * \param  foundfunction         function to call for each match (can be NULL)
 * \param  flushfunction         function to call for all data that is not a match (can be NULL)
 * \param  callbackdata          user data to pass to callback functions
 * \return handle for a new search or NULL on error
 */
DLL_EXPORT_MULTIFINDER multifinder multifinder_create (multifinder_found_callback_fn foundfunction, multifinder_flush_callback_fn flushfunction, void* callbackdata);

/*! \brief clean up an existing search
 * \param  handle                handle created with multifinder_create
 * \sa     multifinder_create
 */
DLL_EXPORT_MULTIFINDER void multifinder_free (multifinder handle);

/*! \brief reset an existing search (e.g. to search a different data stream)
 * \param  handle                handle created with multifinder_create
 * \sa     multifinder_create
 */
DLL_EXPORT_MULTIFINDER void multifinder_reset (multifinder handle);

/*! \brief possible values for the flags parameter of multifinder_add_pattern() and multifinder_add_allocated_pattern
 * \sa     multifinder_add_pattern
 * \sa     multifinder_add_allocated_pattern
 * \name   MULTIFIND_PATTERN_*
 * \{
 */
/*! \brief case sensitive comparison (default) \hideinitializer */
#define MULTIFIND_PATTERN_CASE_SENSITIVE        0x00
/*! \brief case insensitive comparison \hideinitializer */
#define MULTIFIND_PATTERN_CASE_INSENSITIVE      0x01
/*! @} */

/*! \brief add a search pattern (patterns added earlier take precedence in simultaneous matches)
 * \param  handle                handle created with multifinder_create
 * \param  pattern               text to search (NULL terminated string)
 * \param  flags                 flags
 * \param  patterncallbackdata   user data to pass to callback function on each match
 * \sa     MULTIFIND_PATTERN_*
 * \sa     multifinder_add_allocated_pattern
 * \sa     multifinder_create
 */
DLL_EXPORT_MULTIFINDER void multifinder_add_pattern (multifinder handle, const char* pattern, unsigned int flags, void* patterncallbackdata);

/*! \brief add a search pattern (patterns added earlier take precedence in simultaneous matches)
 * \param  handle                handle created with multifinder_create
 * \param  pattern               text to search (NULL terminated string)
 * \param  patternlen            length of text to search
 * \param  patterncallbackdata   user data to pass to callback function on each match
 * \sa     MULTIFIND_PATTERN_*
 * \sa     multifinder_add_pattern
 * \sa     multifinder_create
 */
DLL_EXPORT_MULTIFINDER void multifinder_add_allocated_pattern (multifinder handle, char* pattern, size_t patternlen, unsigned int flags, void* patterncallbackdata);

/*! \brief get the total number of patterns
 * \param  handle                handle created with multifinder_create
 * \return number of patterns
 * \param  patterncallbackdata   user data to pass to callback function on each match
 * \sa     multifinder_add_pattern
 * \sa     multifinder_add_allocated_pattern
 * \sa     multifinder_create
 */
DLL_EXPORT_MULTIFINDER size_t multifinder_count_patterns (multifinder handle);

/*! \brief find patterns in data and call \p callbackfunction for each match
 * \param  handle                handle created with multifinder_create
 * \param  data                  text to search (does not need to be NULL terminated), will be freed by multifinder_free
 * \param  datalen               length text to search
 * \return number of matches found
 * \sa     multifinder_found_callback_fn
 * \sa     multifinder_finalize
 * \sa     multifinder_create
 * \sa     multifinder_add_pattern
 * \sa     multifinder_add_allocated_pattern
 */
DLL_EXPORT_MULTIFINDER size_t multifinder_process (multifinder handle, const char* data, size_t datalen);

/*! \brief finish finding patterns in data previously passed with \p multifinder_process and call \p callbackfunction for each match
 * \param  handle                handle created with multifinder_create
 * \return number of matches found
 * \sa     multifinder_found_callback_fn
 * \sa     multifinder_process
 * \sa     multifinder_create
 * \sa     multifinder_add_pattern
 * \sa     multifinder_add_allocated_pattern
 */
DLL_EXPORT_MULTIFINDER size_t multifinder_finalize (multifinder handle);

/*! \brief finish finding patterns in data previously passed with \p multifinder_process and call \p callbackfunction for each match
 * \param  handle                handle created with multifinder_create
 * \return returns the non-zero status code the callbackfunction returned if the search was aborted or 0 otherwise
 * \sa     multifinder_process
 * \sa     multifinder_finalize
 * \sa     multifinder_found_callback_fn
 * \sa     multifinder_create
 */
DLL_EXPORT_MULTIFINDER int multifinder_aborted (multifinder handle);

/*! \brief get the current position in the input sream provided by \b multifinder_process and \b multifinder_finalize
 * \param  handle                handle created with multifinder_create
 * \return returns the current position in the input stream
 * \sa     multifinder_process
 * \sa     multifinder_finalize
 * \sa     multifinder_found_callback_fn
 * \sa     multifinder_flush_callback_fn
 */
DLL_EXPORT_MULTIFINDER size_t multifinder_position (multifinder handle);

#ifdef __cplusplus
}
#endif

#endif //INCLUDED_MULTIFINDER_H
