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

#include <limits>

#include "platform/platform.h"
#include "platform/threads/mutex.h"
#include "platform/typetraits.h"
#include "core/util/tVector.h"
#include "core/strings/stringFunctions.h"

#include "math/mRandom.h"
#include "core/module.h"


#include <unistd.h>
#include <signal.h>
#include <sys/time.h>
#include <stdio.h>

const F32 TypeTraits< F32 >::MIN = - F32_MAX;
const F32 TypeTraits< F32 >::MAX = F32_MAX;
const F32 TypeTraits< F32 >::ZERO = 0;
const F32 Float_Inf = std::numeric_limits< F32 >::infinity();

static MRandomLCG sgPlatRandom;

// The tools prefer to allow the CPU time to process
#ifndef TORQUE_TOOLS
S32 sgBackgroundProcessSleepTime = 25;
#else
S32 sgBackgroundProcessSleepTime = 200;
#endif
S32 sgTimeManagerProcessInterval = 1;

S32 Platform::getBackgroundSleepTime()
{
   return sgBackgroundProcessSleepTime;
}

void Platform::onFatalError(int code)
{
   exit(code);
}

//-----------------------------------------------------------------------------
void Platform::debugBreak()
{
   //kill(getpid(), SIGSEGV);
   kill(getpid(), SIGTRAP);
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

extern void shit();

MODULE_BEGIN( Platform )

MODULE_INIT
{
   shit();
   Processor::init();
}

MODULE_END;

