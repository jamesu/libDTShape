
#ifndef _CORE_LOG_H_
#define _CORE_LOG_H_

#include "platform/types.h"

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

#endif
