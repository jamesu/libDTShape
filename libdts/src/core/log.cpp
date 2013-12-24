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

#include "platform/platform.h"
#include "core/log.h"
#include "core/stream/fileStream.h"
#include "core/dataChunker.h"

#include "core/frameAllocator.h"
#include "core/strings/stringFunctions.h"
#include "core/util/tVector.h"

//-----------------------------------------------------------------------------

BEGIN_NS(DTShape)

static Vector<ConsumerCallback> gConsumers(__FILE__, __LINE__);

LogEntry::Type LogEntry::General = 0;

namespace Log {

static bool active = false;
static bool useTimestamp = false;

void init()
{
   AssertFatal(active == false, "Log::init should only be called once.");
   
   // Set up general init values.
   active                        = true;
}

//--------------------------------------

void shutdown()
{
   AssertFatal(active == true, "Log::shutdown should only be called once.");
   active = false;
}

//------------------------------------------------------------------------------

static void _printf(LogEntry::Level level, LogEntry::Type type, const char* fmt, va_list argptr)
{
   if (!active)
	   return;
   Log::active = false;
   
   char buffer[8192];
   U32 offset = 0;
   
   if (useTimestamp)
   {
      static U32 startTime = Platform::getRealMilliseconds();
      U32 curTime = Platform::getRealMilliseconds() - startTime;
      offset += dSprintf(buffer + offset, sizeof(buffer) - offset, "[+%4d.%03d]", U32(curTime * 0.001), curTime % 1000);
   }
   dVsprintf(buffer + offset, sizeof(buffer) - offset, fmt, argptr);
   
   LogEntry entry;
   entry.mData = buffer;
   entry.mLevel = level;
   entry.mType = type;
   for(S32 i = 0; i < gConsumers.size(); i++)
      gConsumers[i](level, &entry);
   
   Log::active = true;
}

//------------------------------------------------------------------------------
void printf(const char* fmt,...)
{
   va_list argptr;
   va_start(argptr, fmt);
   _printf(LogEntry::Normal, LogEntry::General, fmt, argptr);
   va_end(argptr);
}

void warnf(LogEntry::Type type, const char* fmt,...)
{
   va_list argptr;
   va_start(argptr, fmt);
   _printf(LogEntry::Warning, type, fmt, argptr);
   va_end(argptr);
}

void errorf(LogEntry::Type type, const char* fmt,...)
{
   va_list argptr;
   va_start(argptr, fmt);
   _printf(LogEntry::Error, type, fmt, argptr);
   va_end(argptr);
}

void warnf(const char* fmt,...)
{
   va_list argptr;
   va_start(argptr, fmt);
   _printf(LogEntry::Warning, LogEntry::General, fmt, argptr);
   va_end(argptr);
}

void errorf(const char* fmt,...)
{
   va_list argptr;
   va_start(argptr, fmt);
   _printf(LogEntry::Error, LogEntry::General, fmt, argptr);
   va_end(argptr);
}

//---------------------------------------------------------------------------
void addConsumer(ConsumerCallback consumer)
{
   gConsumers.push_back(consumer);
}

void removeConsumer(ConsumerCallback consumer)
{
   S32 idx = gConsumers.indexOf(consumer);
   if (idx >= 0)
   {
      gConsumers.erase(idx);
   }
}

}

END_NS
