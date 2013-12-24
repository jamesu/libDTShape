//-----------------------------------------------------------------------------
// Copyright (c) 2012 GarageGames, LLC
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

#ifndef _PLATFORM_H_
#define _PLATFORM_H_

#include <stdlib.h>

#ifndef _TWISTFORKCONFIG_H_
#include "twistforkConfig.h"
#endif
#ifndef _TWISTFORK_TYPES_H_
#include "platform/types.h"
#endif
#ifndef _PLATFORMASSERT_H_
#include "platform/platformAssert.h"
#endif
#ifndef _TWISTFORK_SAFEDELETE_H_
#include "core/util/safeDelete.h"
#endif

#include <new>
#include <typeinfo>

#define BEGIN_NS(ns) namespace ns {
#define END_NS }

/// Global processor identifiers.
///
/// @note These enums must be globally scoped so that they work with the inline assembly
enum ProcessorType
{
   // x86
   CPU_X86Compatible,
   CPU_Intel_Unknown,
   CPU_Intel_486,
   CPU_Intel_Pentium,
   CPU_Intel_PentiumMMX,
   CPU_Intel_PentiumPro,
   CPU_Intel_PentiumII,
   CPU_Intel_PentiumCeleron,
   CPU_Intel_PentiumIII,
   CPU_Intel_Pentium4,
   CPU_Intel_PentiumM,
   CPU_Intel_Core,
   CPU_Intel_Core2,
   CPU_Intel_Corei7Xeon, // Core i7 or Xeon
   CPU_AMD_K6,
   CPU_AMD_K6_2,
   CPU_AMD_K6_3,
   CPU_AMD_Athlon,
   CPU_AMD_Unknown,
   CPU_Cyrix_6x86,
   CPU_Cyrix_MediaGX,
   CPU_Cyrix_6x86MX,
   CPU_Cyrix_GXm,          ///< Media GX w/ MMX
   CPU_Cyrix_Unknown,

   // PowerPC
   CPU_PowerPC_Unknown,
   CPU_PowerPC_601,
   CPU_PowerPC_603,
   CPU_PowerPC_603e,
   CPU_PowerPC_603ev,
   CPU_PowerPC_604,
   CPU_PowerPC_604e,
   CPU_PowerPC_604ev,
   CPU_PowerPC_G3,
   CPU_PowerPC_G4,
   CPU_PowerPC_G4_7450,
   CPU_PowerPC_G4_7455,
   CPU_PowerPC_G4_7447, 
   CPU_PowerPC_G5,

   // Xenon
   CPU_Xenon,

};

/// Properties for CPU.
enum ProcessorProperties
{ 
   CPU_PROP_C         = (1<<0),  ///< We should use C fallback math functions.
   CPU_PROP_FPU       = (1<<1),  ///< Has an FPU. (It better!)
   CPU_PROP_MMX       = (1<<2),  ///< Supports MMX instruction set extension.
   CPU_PROP_3DNOW     = (1<<3),  ///< Supports AMD 3dNow! instruction set extension.
   CPU_PROP_SSE       = (1<<4),  ///< Supports SSE instruction set extension.
   CPU_PROP_RDTSC     = (1<<5),  ///< Supports Read Time Stamp Counter op.
   CPU_PROP_SSE2      = (1<<6),  ///< Supports SSE2 instruction set extension.
   CPU_PROP_SSE3      = (1<<7),  ///< Supports SSE3 instruction set extension.  
   CPU_PROP_SSE3xt    = (1<<8),  ///< Supports extended SSE3 instruction set  
   CPU_PROP_SSE4_1    = (1<<9),  ///< Supports SSE4_1 instruction set extension.  
   CPU_PROP_SSE4_2    = (1<<10), ///< Supports SSE4_2 instruction set extension.  
   CPU_PROP_MP        = (1<<11), ///< This is a multi-processor system.
   CPU_PROP_LE        = (1<<12), ///< This processor is LITTLE ENDIAN.  
   CPU_PROP_64bit     = (1<<13), ///< This processor is 64-bit capable
   CPU_PROP_ALTIVEC   = (1<<14),  ///< Supports AltiVec instruction set extension (PPC only).
};

/// Processor info manager. 
struct Processor
{
   /// Gather processor state information.
   static void init();
};

#if defined(TWISTFORK_SUPPORTS_GCC_INLINE_X86_ASM)
#define TWISTFORK_DEBUGBREAK() { asm ( "int 3"); }
#elif defined (TWISTFORK_SUPPORTS_VC_INLINE_X86_ASM) // put this test second so that the __asm syntax doesn't break the Visual Studio Intellisense parser
#define TWISTFORK_DEBUGBREAK() { __asm { int 3 }; } 
#else
/// Macro to do in-line debug breaks, used for asserts.  Does inline assembly when possible.
#define TWISTFORK_DEBUGBREAK() Platform::debugBreak();
#endif

// Some forward declares for later.
class Point2I;
template<class T> class Vector;
template<typename Signature> class Signal;
struct InputEventInfo;

namespace Platform
{
   // Time
   struct LocalTime
   {
      U8  sec;        ///< Seconds after minute (0-59)
      U8  min;        ///< Minutes after hour (0-59)
      U8  hour;       ///< Hours after midnight (0-23)
      U8  month;      ///< Month (0-11; 0=january)
      U8  monthday;   ///< Day of the month (1-31)
      U8  weekday;    ///< Day of the week (0-6, 6=sunday)
      U16 year;       ///< Current year minus 1900
      U16 yearday;    ///< Day of year (0-365)
      bool isdst;     ///< True if daylight savings time is active
   };

   void getLocalTime(LocalTime &);
   
   /// Converts the local time to a formatted string appropriate
   /// for the current platform.
   String localTimeToString( const LocalTime &lt );
   
   U32  getTime();

   /// Returns the milliseconds since the system was started.  You should
   /// not depend on this for high precision timing.
   /// @see PlatformTimer
   U32 getRealMilliseconds();
   
   // Process control
   void sleep(U32 ms);

   // Debug
   void outputDebugString(const char *string, ...);
   void debugBreak();
   
   // Random
   float getRandom();
   
   // Return path to root (usually cwd)
   const char *getRootDir();
   
   void onFatalError(int code);
   
   bool isFile(const char *path);
   bool isDirectory(const char *path);
   
   bool isFullPath(const char *path);
   bool createPath(const char *pathName);
   String makeRelativePathName(const char *path, const char *to);
   char *makeFullPathName(const char *path, char *buffer, U32 size, const char *cwd = NULL);
   
   S32 compareFileTimes(const FileTime &a, const FileTime &b);
   bool getFileTimes(const char *filePath, FileTime *createTime, FileTime *modifyTime);
   
   bool fileDelete(const char *name);
   bool deletePath(const char *filename);

   // Alerts
   void AlertOK(const char *windowTitle, const char *message);
   bool AlertOKCancel(const char *windowTitle, const char *message);
   bool AlertRetry(const char *windowTitle, const char *message);

   struct SystemInfo_struct
   {
         struct Processor
         {
            ProcessorType  type;
            const char*    name;
            U32            mhz;
            bool           isMultiCore;
            bool           isHyperThreaded;
            U32            numLogicalProcessors;
            U32            numPhysicalProcessors;
            U32            numAvailableCores;
            U32            properties;      // CPU type specific enum
         } processor;
   };
   extern SystemInfo_struct  SystemInfo;
};

//------------------------------------------------------------------------------
// Unicode string conversions
// UNICODE is a windows platform API switching flag. Don't define it on other platforms.
#ifdef UNICODE
#define dT(s)    L##s
#else
#define dT(s)    s
#endif

//------------------------------------------------------------------------------
// Misc StdLib functions
#define QSORT_CALLBACK FN_CDECL
inline void dQsort(void *base, U32 nelem, U32 width, int (QSORT_CALLBACK *fcmp)(const void *, const void *))
{
   qsort(base, nelem, width, fcmp);
}

//-------------------------------------- Some all-around useful inlines and globals
//

///@defgroup ObjTrickery Object Management Trickery
///
/// These functions are to construct and destruct objects in memory
/// without causing a free or malloc call to occur. This is so that
/// we don't have to worry about allocating, say, space for a hundred
/// NetAddresses with a single malloc call, calling delete on a single
/// NetAdress, and having it try to free memory out from under us.
///
/// @{

/// Constructs an object that already has memory allocated for it.
template <class T>
inline T* constructInPlace(T* p)
{
   return new ( p ) T;
}
template< class T >
inline T* constructArrayInPlace( T* p, U32 num )
{
   return new ( p ) T[ num ];
}

/// Copy constructs an object that already has memory allocated for it.
template <class T>
inline T* constructInPlace(T* p, const T* copy)
{
   return new ( p ) T( *copy );
}

template <class T, class T2> inline T* constructInPlace(T* ptr, T2 t2)
{
   return new ( ptr ) T( t2 );
}

template <class T, class T2, class T3> inline T* constructInPlace(T* ptr, T2 t2, T3 t3)
{
   return new ( ptr ) T( t2, t3 );
}

template <class T, class T2, class T3, class T4> inline T* constructInPlace(T* ptr, T2 t2, T3 t3, T4 t4)
{
   return new ( ptr ) T( t2, t3, t4 );
}

template <class T, class T2, class T3, class T4, class T5> inline T* constructInPlace(T* ptr, T2 t2, T3 t3, T4 t4, T5 t5)
{
   return new ( ptr ) T( t2, t3, t4, t5 );
}

/// Destructs an object without freeing the memory associated with it.
template <class T>
inline void destructInPlace(T* p)
{
   p->~T();
}


//------------------------------------------------------------------------------
/// Memory functions


#  define TWISTFORK_TMM_ARGS_DECL
#  define TWISTFORK_TMM_ARGS
#  define TWISTFORK_TMM_LOC

#define dMalloc(x) dMalloc_r(x, __FILE__, __LINE__)
#define dRealloc(x, y) dRealloc_r(x, y, __FILE__, __LINE__)

extern void  setBreakAlloc(dsize_t);
extern void  setMinimumAllocUnit(U32);
extern void* dMalloc_r(dsize_t in_size, const char*, const dsize_t);
extern void  dFree(void* in_pFree);
extern void* dRealloc_r(void* in_pResize, dsize_t in_size, const char*, const dsize_t);
extern void* dRealMalloc(dsize_t);
extern void  dRealFree(void*);

extern void *dMalloc_aligned(dsize_t in_size, int alignment);
extern void dFree_aligned(void *);


inline void dFree( const void* p )
{
   dFree( ( void* ) p );
}

// Helper function to copy one array into another of different type
template<class T,class S> void dCopyArray(T *dst, const S *src, dsize_t size)
{
   for (dsize_t i = 0; i < size; i++)
      dst[i] = (T)src[i];
}

extern void* dMemcpy(void *dst, const void *src, dsize_t size);
extern void* dMemmove(void *dst, const void *src, dsize_t size);
extern void* dMemset(void *dst, int c, dsize_t size);
extern int   dMemcmp(const void *ptr1, const void *ptr2, dsize_t size);

// Special case of the above function when the arrays are the same type (use memcpy)
template<class T> void dCopyArray(T *dst, const T *src, dsize_t size)
{
   dMemcpy(dst, src, size * sizeof(T));
}

/// The dALIGN macro ensures the passed declaration is
/// data aligned at 16 byte boundaries.
#if defined( TWISTFORK_COMPILER_VISUALC )
   #define dALIGN( decl ) __declspec( align( 16 ) ) decl
   #define dALIGN_BEGIN __declspec( align( 16 ) )
   #define dALIGN_END
#elif defined( TWISTFORK_COMPILER_GCC )
   #define dALIGN( decl ) decl __attribute__( ( aligned( 16 ) ) )
   #define dALIGN_BEGIN
   #define dALIGN_END __attribute__( ( aligned( 16 ) ) )
#else
   #define dALIGN( decl ) decl
   #define dALIGN_BEGIN()
   #define dALIGN_END()
#endif

//------------------------------------------------------------------------------
struct Math
{
   /// Initialize the math library with the appropriate libraries
   /// to support hardware acceleration features.
   ///
   /// @param properties Leave zero to detect available hardware. Otherwise,
   ///                   pass CPU instruction set flags that you want to load
   ///                   support for.
   static void init(U32 properties = 0);
};

/// @}

#endif


