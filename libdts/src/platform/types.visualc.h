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

#ifndef INCLUDED_TYPES_VISUALC_H
#define INCLUDED_TYPES_VISUALC_H


// For more information on VisualC++ predefined macros
// http://support.microsoft.com/default.aspx?scid=kb;EN-US;q65472

//--------------------------------------
// Types
typedef signed _int64   S64;
typedef unsigned _int64 U64;


//--------------------------------------
// Compiler Version
#define LIBDTSHAPE_COMPILER_VISUALC _MSC_VER

//--------------------------------------
// Identify the compiler string
#if _MSC_VER < 1200
   // No support for old compilers
#  error "VC: Minimum VisualC++ 6.0 or newer required"
#else _MSC_VER >= 1200
#  define LIBDTSHAPE_COMPILER_STRING "VisualC++"
#endif


//--------------------------------------
// Identify the Operating System
#if _XBOX_VER >= 200 
#  define LIBDTSHAPE_OS_STRING "Xenon"
#  ifndef LIBDTSHAPE_OS_XENON
#     define LIBDTSHAPE_OS_XENON
#  endif
#elif defined( _XBOX_VER )
#  define LIBDTSHAPE_OS_STRING "Xbox"
#  define LIBDTSHAPE_OS_XBOX
#elif defined(_WIN32)
#  define LIBDTSHAPE_OS_STRING "Win32"
#  define LIBDTSHAPE_OS_WIN32
#else 
#  error "VC: Unsupported Operating System"
#endif

//--------------------------------------
// Identify the CPU
#if defined(_M_IX86)
#  define LIBDTSHAPE_CPU_STRING "x86"
#  define LIBDTSHAPE_CPU_X86
#  define LIBDTSHAPE_LITTLE_ENDIAN
#  define LIBDTSHAPE_SUPPORTS_NASM
#  define LIBDTSHAPE_SUPPORTS_VC_INLINE_X86_ASM
#elif defined(LIBDTSHAPE_OS_XENON)
#  define LIBDTSHAPE_CPU_STRING "ppc"
#  define LIBDTSHAPE_CPU_PPC
#  define LIBDTSHAPE_BIG_ENDIAN
#else
#  error "VC: Unsupported Target CPU"
#endif

#ifndef FN_CDECL
#  define FN_CDECL __cdecl            ///< Calling convention
#endif

#if _MSC_VER < 1700
#define for if(false) {} else for   ///< Hack to work around Microsoft VC's non-C++ compliance on variable scoping
#endif

// disable warning caused by memory layer
// see msdn.microsoft.com "Compiler Warning (level 1) C4291" for more details
#pragma warning(disable: 4291) 


#endif // INCLUDED_TYPES_VISUALC_H

