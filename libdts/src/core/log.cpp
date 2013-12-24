
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
