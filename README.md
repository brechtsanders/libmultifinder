libmultifinder
==========
Cross-platform C library for searching multiple patterns in data.

Description
-----------
The libmultifinder library is cross-platform C library that allows searching a stream of data for multiple search patterns.
The data to be searched is not to be loaded into memory, which make this a low memory footprint library.
Only literal search patterns are supported, not wildcards or regular expressions. However, a case insensitive match is included.

Goal
----
The library was written with the following goals in mind:
- written in standard C, but allows being used by C++
- speed
- small footprint
- portable across different platforms (Windows, Mac, *nix)
- no dependancies

Libraries
---------

The following libraries are provided:
- `-lmultifinder` - requires `#include <multifinder.h>`

Command line utilities
----------------------
Some command line utilities are included:
- `multifinder_count` - counts how much time a pattern appears
- `multifinder_replace` - replaces patterns with other patterns

Dependancies
------------
This project has no external depencancies.

Building from source
--------------------
Requirements:
- a C compiler like gcc or clang, on Windows MinGW and MinGW-w64 are supported
- a shell environment, on Windows MSYS is supported
- CMake version 2.6 or higher

Building with CMake
- configure by running `cmake -G"Unix Makefiles"` (or `cmake -G"MSYS Makefiles"` on Windows) optionally followed by:
  + `-DCMAKE_INSTALL_PREFIX:PATH=<path>` Base path were files will be installed
  + `-DBUILD_STATIC:BOOL=OFF` - Don't build static libraries
  + `-DBUILD_SHARED:BOOL=OFF` - Don't build shared libraries
  + `-DBUILD_TOOLS:BOOL=OFF` - Don't build tools (only libraries)
- build and install by running `make install` (or `make install/strip` to strip symbols)

For Windows prebuilt binaries are also available for download (both 32-bit and 64-bit)

License
-------
libmultifinder is released under the terms of the MIT License (MIT), see LICENSE.txt.

This means you are free to use libmultifinder in any of your projects, from open source to commercial.
