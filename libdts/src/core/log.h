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

#ifndef _CORE_LOG_H_
#define _CORE_LOG_H_

#ifndef _PLATFORM_H_
#include "platform/platform.h"
#endif

//-----------------------------------------------------------------------------

BEGIN_NS(DTShape)

/// Represents an entry in the log.
struct LogEntry
{
   /// This field indicates the severity of the log entry.
   ///
   /// Log entries are filtered and displayed differently based on
   /// their severity. Errors are highlighted red, while normal entries
   /// are displayed as normal text. Often times, the engine will be
   /// configured to hide all log entries except warnings or errors,
   /// or to perform a special notification when it encounters an error.
   enum Level
   {
      Normal = 0,
      Warning,
      Error,
      Script,
      NUM_CLASS
   } mLevel;
   
   /// Line
   const char *mData;
   
   /// Type of entry
   typedef unsigned char Type;
   LogEntry::Type mType;
   
   static LogEntry::Type General;
};

typedef void (*ConsumerCallback)(U32 level, LogEntry *logEntry);

namespace Log
{
   /// @name Control Functions
   ///
   /// The Logging system must be initialized and shutdown appropriately during the
   /// lifetime of the app. These functions are used to manage this behavior.
   
   /// @{
   
   /// Initializes the logging.
   void init();
   
   /// Disables and shuts down the logging
   void shutdown();
   
   /// Is logging active at this time?
   bool isActive();
   
   /// @}
   
   /// @name Log Consumers
   ///
   /// The log distributes its output through by using
   /// consumers. Every time a new line is printed,
   /// all the ConsumerCallbacks registered using addConsumer are
   /// called, in order.
   ///
   /// @{
   
   ///
   void addConsumer(ConsumerCallback cb);
   void removeConsumer(ConsumerCallback cb);
   
   /// @}
   
   /// @name Miscellaneous
   /// @{
   
   /// @param _format   A stdlib printf style formatted out put string
   /// @param ...       Variables to be written
   void printf(const char *_format, ...);
   
   /// @param _format   A stdlib printf style formatted out put string
   /// @param ...       Variables to be written
   void warnf(const char *_format, ...);
   
   /// @param _format   A stdlib printf style formatted out put string
   /// @param ...       Variables to be written
   void errorf(const char *_format, ...);
   
   /// @param type      Allows you to associate the warning message with an internal module.
   /// @param _format   A stdlib printf style formatted out put string
   /// @param ...       Variables to be written
   /// @see Log::warnf()
   void warnf(LogEntry::Type type, const char *_format, ...);
   
   /// @param type      Allows you to associate the warning message with an internal module.
   /// @param _format   A stdlib printf style formatted out put string
   /// @param ...       Variables to be written
   /// @see Log::errorf()
   void errorf(LogEntry::Type type, const char *_format, ...);
}

END_NS

#endif
