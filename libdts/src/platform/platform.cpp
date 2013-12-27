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

#include <limits>

#include "platform/platform.h"
#include "platform/typetraits.h"
#include "core/util/tVector.h"
#include "core/strings/stringFunctions.h"

#include "math/mRandom.h"

#include <signal.h>
#include <stdio.h>

#include "core/tempAlloc.h"
#include "core/util/tVector.h"


#if defined(WIN32)
#include <windows.h>
#endif

//-----------------------------------------------------------------------------

BEGIN_NS(DTShape)

//-----------------------------------------------------------------------------

const F32 TypeTraits< F32 >::MIN = - F32_MAX;
const F32 TypeTraits< F32 >::MAX = F32_MAX;
const F32 TypeTraits< F32 >::ZERO = 0;
const F32 Float_Inf = std::numeric_limits< F32 >::infinity();

void Platform::onFatalError(int code)
{
   exit(code);
}

//-----------------------------------------------------------------------------

#if defined(LIBDTSHAPE_OS_WIN32)
static inline void _resolveLeadingSlash(char* buf, U32 size)
{
   if(buf[0] != '/')
      return;

   AssertFatal(dStrlen(buf) + 2 < size, "Expanded path would be too long");
   dMemmove(buf + 2, buf, dStrlen(buf));
   buf[0] = 'c';
   buf[1] = ':';
}
#endif

//-----------------------------------------------------------------------------
void Platform::debugBreak()
{
#if defined(LIBDTSHAPE_OS_WIN32)
   DebugBreak();
#endif

#if defined(LIBDTSHAPE_OS_MAC)
   DebugStr("\pDEBUG_BREAK!");
#endif

#if defined(LIBDTSHAPE_OS_POSIX)
   kill(getpid(), SIGTRAP);
#endif
}

//-----------------------------------------------------------------------------
void Platform::outputDebugString(const char *string, ...)
{
   char buffer[2048];
   
   va_list args;
   va_start( args, string );
   
   dVsprintf( buffer, sizeof(buffer), string, args );
   va_end( args );
   
   U32 length = dStrlen(buffer);
   if( length == (sizeof(buffer) - 1 ) )
      length--;
   
   buffer[length++]  = '\n';
   buffer[length]    = '\0';
   
   fwrite(buffer, sizeof(char), length, stderr);
}


//------------------------------------------------------------------------------
void Platform::AlertOK(const char *windowTitle, const char *message)
{
   {
      dPrintf("Alert: %s %s\n", windowTitle, message);
   }
}

//------------------------------------------------------------------------------
bool Platform::AlertOKCancel(const char *windowTitle, const char *message)
{
   {
      dPrintf("Alert: %s %s\n", windowTitle, message);
      return false;
   }
}

//------------------------------------------------------------------------------
bool Platform::AlertRetry(const char *windowTitle, const char *message)
{
   {
      dPrintf("Alert: %s %s\n", windowTitle, message);
      return false;
   }
}


inline void catPath(char *dst, const char *src, U32 len)
{
   if(*dst != '/')
   {
      ++dst; --len;
      *dst = '/';
   }
   
   ++dst; --len;
   
   dStrncpy(dst, src, len);
   dst[len - 1] = 0;
}

static void makeCleanPathInPlace( char* path )
{
   U32 pathDepth = 0;
   char* fromPtr = path;
   char* toPtr = path;
   
   bool isAbsolute = false;
   if( *fromPtr == '/' )
   {
      fromPtr ++;
      toPtr ++;
      isAbsolute = true;
   }
   else if( fromPtr[ 0 ] != '\0' && fromPtr[ 1 ] == ':' )
   {
      toPtr += 3;
      fromPtr += 3;
      isAbsolute = true;
   }
   
   while( *fromPtr )
   {
      if( fromPtr[ 0 ] == '.' && fromPtr[ 1 ] == '.' && fromPtr[ 2 ] == '/' )
      {
         // Back up from '../'
         
         if( pathDepth > 0 )
         {
            pathDepth --;
            toPtr -= 2;
            while( toPtr >= path && *toPtr != '/' )
               toPtr --;
            toPtr ++;
         }
         else if( !isAbsolute )
         {
            dMemcpy( toPtr, fromPtr, 3 );
            toPtr += 3;
         }
         
         fromPtr += 3;
      }
      else if( fromPtr[ 0 ] == '.' && fromPtr[ 1 ] == '/' )
      {
         // Ignore.
         fromPtr += 2;
      }
      else
      {
         if( fromPtr[ 0 ] == '/' )
            pathDepth ++;
         
         *toPtr ++ = *fromPtr ++;
      }
   }
   
   *toPtr = '\0';
}



bool Platform::isFullPath(const char *path)
{
   // Quick way out
   if(path[0] == '/' || path[1] == ':')
      return true;
   
   return false;
}

String Platform::makeRelativePathName(const char *path, const char *to)
{
   // Make sure 'to' is a proper absolute path terminated with a forward slash.
   
   char buffer[ 2048 ];
   if( !to )
   {
      dSprintf( buffer, sizeof( buffer ), "%s/", Platform::getRootDir() );
      to = buffer;
   }
   else if( !Platform::isFullPath( to ) )
   {
      dSprintf( buffer, sizeof( buffer ), "%s/%s/", Platform::getRootDir(), to );
      makeCleanPathInPlace( buffer );
      to = buffer;
   }
   else if( to[ dStrlen( to ) - 1 ] != '/' )
   {
      U32 length = getMin( (U32)dStrlen( to ), sizeof( buffer ) - 2 );
      dMemcpy( buffer, to, length );
      buffer[ length ] = '/';
      buffer[ length + 1 ] = '\0';
      to = buffer;
   }
   
   // If 'path' isn't absolute, make it now.  Let's us use a single
   // absolute/absolute merge path from here on.
   
   char buffer2[ 1024 ];
   if( !Platform::isFullPath( path ) )
   {
      dSprintf( buffer2, sizeof( buffer2 ), "%s/%s", Platform::getRootDir(), path );
      makeCleanPathInPlace( buffer2 );
      path = buffer2;
   }
   
   // First, find the common prefix and see where 'path' branches off from 'to'.
   
   const char *pathPtr, *toPtr, *branch = path;
   for(pathPtr = path, toPtr = to;*pathPtr && *toPtr && dTolower(*pathPtr) == dTolower(*toPtr);++pathPtr, ++toPtr)
   {
      if(*pathPtr == '/')
         branch = pathPtr;
   }
   
   // If there's no common part, the two paths are on different drives and
   // there's nothing we can do.
   
   if( pathPtr == path )
      return path;
   
   // If 'path' and 'to' are identical (minus trailing slash or so), we can just return './'.
   
   else if((*pathPtr == 0 || (*pathPtr == '/' && *(pathPtr + 1) == 0)) &&
           (*toPtr == 0 || (*toPtr == '/' && *(toPtr + 1) == 0)))
   {
      char* bufPtr = buffer;
      *bufPtr ++ = '.';
      
      if(*pathPtr == '/' || *(pathPtr - 1) == '/')
         *bufPtr++ = '/';
      
      *bufPtr = 0;
      return buffer;
   }
   
   // If 'to' is a proper prefix of 'path', the remainder of 'path' is our relative path.
   
   else if( *toPtr == '\0' && toPtr[ -1 ] == '/' )
      return pathPtr;
   
   // Otherwise have to step up the remaining directories in 'to' and then
   // append the remainder of 'path'.
   
   else
   {
      if((*pathPtr == 0 && *toPtr == '/') || (*toPtr == '/' && *pathPtr == 0))
         branch = pathPtr;
      
      // Allocate a new temp so we aren't prone to buffer overruns.
      
      TempAlloc< char > temp( dStrlen( toPtr ) + dStrlen( branch ) + 1 );
      char* bufPtr = temp;
      
      // Figure out parent dirs
      
      for(toPtr = to + (branch - path);*toPtr;++toPtr)
      {
         if(*toPtr == '/' && *(toPtr + 1) != 0)
         {
            *bufPtr++ = '.';
            *bufPtr++ = '.';
            *bufPtr++ = '/';
         }
      }
      *bufPtr = 0;
      
      // Copy the rest
      if(*branch)
         dStrcpy(bufPtr, branch + 1);
      else
         *--bufPtr = 0;
      
      return temp.ptr;
   }
}


char * Platform::makeFullPathName(const char *path, char *buffer, U32 size, const char *cwd /* = NULL */)
{
   char bspath[1024];
   dStrncpy(bspath, path, sizeof(bspath));
   bspath[sizeof(bspath)-1] = 0;
   
   for(S32 i = 0;i < dStrlen(bspath);++i)
   {
      if(bspath[i] == '\\')
         bspath[i] = '/';
   }
   
   if(Platform::isFullPath(bspath))
   {
      // Already a full path
#if defined(LIBDTSHAPE_OS_WIN32)
      _resolveLeadingSlash(bspath, sizeof(bspath));
#endif
      dStrncpy(buffer, bspath, size);
      buffer[size-1] = 0;
      return buffer;
   }
   
   if(cwd == NULL)
      cwd = getRootDir();
   
   dStrncpy(buffer, cwd, size);
   buffer[size-1] = 0;
   
   char *ptr = bspath;
   char *slash = NULL;
   char *endptr = buffer + dStrlen(buffer) - 1;
   
   do
   {
      slash = dStrchr(ptr, '/');
      if(slash)
      {
         *slash = 0;
         
         // Directory
         
         if(dStrcmp(ptr, "..") == 0)
         {
            // Parent
            endptr = dStrrchr(buffer, '/');
            if (endptr)
               *endptr-- = 0;
         }
         else if(dStrcmp(ptr, ".") == 0)
         {
            // Current dir
         }
         else if(endptr)
         {
            catPath(endptr, ptr, size - (endptr - buffer));
            endptr += dStrlen(endptr) - 1;
         }
         
         ptr = slash + 1;
      }
      else if(endptr)
      {
         // File
         
         catPath(endptr, ptr, size - (endptr - buffer));
         endptr += dStrlen(endptr) - 1;
      }
      
   } while(slash);
   
   return buffer;
}


//------------------------------------------------------------------------------

void* dMalloc_r(dsize_t in_size, const char* fileName, const dsize_t line)
{
   return malloc(in_size);
}

void dFree(void* in_pFree)
{
   free(in_pFree);
}

void* dRealloc_r(void* in_pResize, dsize_t in_size, const char* fileName, const dsize_t line)
{
   return realloc(in_pResize,in_size);
}

//-----------------------------------------------------------------------------

END_NS

