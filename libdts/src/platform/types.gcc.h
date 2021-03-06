//-----------------------------------------------------------------------------
// Copyright (c) 2012 GarageGames, LLC
// Portions Copyright (C) 2013 James S Urquhart
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.
//-----------------------------------------------------------------------------

#ifndef _TYPESGCC_H
#define _TYPESGCC_H


// For additional information on GCC predefined macros
// http://gcc.gnu.org/onlinedocs/gcc-3.0.2/cpp.html


//--------------------------------------
// Types
typedef signed long long    S64;
typedef unsigned long long  U64;


//--------------------------------------
// Compiler Version
#define LIBDTSHAPE_COMPILER_GCC (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__)


//--------------------------------------
// Identify the compiler string

#if defined(__MINGW32__)
#  define LIBDTSHAPE_COMPILER_STRING "GCC (MinGW)"
#  define LIBDTSHAPE_COMPILER_MINGW
#elif defined(__CYGWIN__)
#  define LIBDTSHAPE_COMPILER_STRING "GCC (Cygwin)"
#  define LIBDTSHAPE_COMPILER_MINGW
#else
#  define LIBDTSHAPE_COMPILER_STRING "GCC "
#endif


//--------------------------------------
// Identify the Operating System
#if defined(__WIN32__) || defined(_WIN32)
#  define LIBDTSHAPE_OS_STRING "Win32"
#  define LIBDTSHAPE_OS_WIN32
#  define LIBDTSHAPE_SUPPORTS_NASM
#  define LIBDTSHAPE_SUPPORTS_GCC_INLINE_X86_ASM

#elif defined(_WIN64)
#  define LIBDTSHAPE_OS_STRING "Win64"
#  define LIBDTSHAPE_OS_WIN32
#  define LIBDTSHAPE_OS_WIN64

#elif defined(SN_TARGET_PS3)
#  define LIBDTSHAPE_OS_STRING "PS3"
#  define LIBDTSHAPE_OS_PS3

#elif defined(linux)
#  define LIBDTSHAPE_OS_STRING "Linux"
#  define LIBDTSHAPE_OS_LINUX
#  define LIBDTSHAPE_OS_POSIX

#elif defined(__OpenBSD__)
#  define LIBDTSHAPE_OS_STRING "OpenBSD"
#  define LIBDTSHAPE_OS_OPENBSD
#  define LIBDTSHAPE_OS_POSIX
#  define LIBDTSHAPE_SUPPORTS_NASM
#  define LIBDTSHAPE_SUPPORTS_GCC_INLINE_X86_ASM

#elif defined(__FreeBSD__)
#  define LIBDTSHAPE_OS_STRING "FreeBSD"
#  define LIBDTSHAPE_OS_FREEBSD
#  define LIBDTSHAPE_OS_POSIX
#  define LIBDTSHAPE_SUPPORTS_NASM
#  define LIBDTSHAPE_SUPPORTS_GCC_INLINE_X86_ASM

#elif defined(__APPLE__)
#  define LIBDTSHAPE_OS_STRING "MacOS X"
#  define LIBDTSHAPE_OS_MAC
#  define LIBDTSHAPE_OS_POSIX
#  define LIBDTSHAPE_OS_DARWIN
#  if defined(i386) || defined(__amd64__)
#  define LIBDTSHAPE_SUPPORTS_GCC_INLINE_X86_ASM
#  endif
#else 
#  error "GCC: Unsupported Operating System"
#endif

//--------------------------------------
// Identify the CPU
#if defined(i386)
#  define LIBDTSHAPE_CPU_STRING "Intel x86"
#  define LIBDTSHAPE_CPU_X86
#  define LIBDTSHAPE_LITTLE_ENDIAN

#if defined(linux)
#  define LIBDTSHAPE_SUPPORTS_GCC_INLINE_X86_ASM
#endif

#elif defined(__amd64__)
#  define LIBDTSHAPE_CPU_STRING "Intel x86-64"
#  define LIBDTSHAPE_CPU_X86_64
#  define LIBDTSHAPE_LITTLE_ENDIAN
#  define LIBDTSHAPE_64

#if defined(linux)
#  define LIBDTSHAPE_SUPPORTS_GCC_INLINE_X86_ASM
#endif

#elif defined(__ppc__)
#  define LIBDTSHAPE_CPU_STRING "PowerPC"
#  define LIBDTSHAPE_CPU_PPC
#  define LIBDTSHAPE_BIG_ENDIAN

#elif defined(__arm__)
#  define LIBDTSHAPE_CPU_STRING "ARM"
#  define LIBDTSHAPE_CPU_ARM
#  define LIBDTSHAPE_LITTLE_ENDIAN

#elif defined(SN_TARGET_PS3)
#  define LIBDTSHAPE_CPU_STRING "PowerPC"
#  define LIBDTSHAPE_CPU_PPC
#  define LIBDTSHAPE_BIG_ENDIAN

#else
#  error "GCC: Unsupported Target CPU"
#endif

#ifndef Offset
/// Offset macro:
/// Calculates the location in memory of a given member x of class cls from the
/// start of the class.  Need several definitions to account for various
/// flavors of GCC.

// now, for each compiler type, define the Offset macros that should be used.
// The Engine code usually uses the Offset macro, but OffsetNonConst is needed
// when a variable is used in the indexing of the member field (see
// TSShapeConstructor::initPersistFields for an example)

// compiler is non-GCC, or gcc < 3
#if (__GNUC__ < 3)
#define Offset(x, cls) _Offset_Normal(x, cls)
#define OffsetNonConst(x, cls) _Offset_Normal(x, cls)

// compiler is GCC 3 with minor version less than 4
#elif defined(LIBDTSHAPE_COMPILER_GCC) && (__GNUC__ == 3) && (__GNUC_MINOR__ < 4)
#define Offset(x, cls) _Offset_Variant_1(x, cls)
#define OffsetNonConst(x, cls) _Offset_Variant_1(x, cls)

// compiler is GCC 3 with minor version greater than 4
#elif defined(LIBDTSHAPE_COMPILER_GCC) && (__GNUC__ == 3) && (__GNUC_MINOR__ >= 4)
#include <stddef.h>
#define Offset(x, cls) _Offset_Variant_2(x, cls)
#define OffsetNonConst(x, cls) _Offset_Variant_1(x, cls)

// compiler is GCC 4
#elif defined(LIBDTSHAPE_COMPILER_GCC) && (__GNUC__ == 4)
#include <stddef.h>
#define Offset(x, cls) _Offset_Normal(x, cls)
#define OffsetNonConst(x, cls) _Offset_Variant_1(x, cls)

#endif
#endif

#endif // INCLUDED_TYPES_GCC_H

