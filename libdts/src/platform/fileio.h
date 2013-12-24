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

#ifndef _FILEIO_H_
#define _FILEIO_H_

#ifndef _PLATFORM_H_
#include "platform/platform.h"
#endif

//-----------------------------------------------------------------------------

BEGIN_NS(DTShape)

//-----------------------------------------------------------------------------

class File
{
public:
   /// What is the status of our file handle?
   enum Status
   {
      Ok = 0,           ///< Ok!
      IOError,          ///< Read or Write error
      EOS,              ///< End of Stream reached (mostly for reads)
      IllegalCall,      ///< An unsupported operation used.  Always accompanied by AssertWarn
      Closed,           ///< Tried to operate on a closed stream (or detached filter)
      UnknownError      ///< Catchall
   };

   /// How are we accessing the file?
   enum AccessMode
   {
      Read         = 0,  ///< Open for read only, starting at beginning of file.
      Write        = 1,  ///< Open for write only, starting at beginning of file; will blast old contents of file.
      ReadWrite    = 2,  ///< Open for read-write.
      WriteAppend  = 3   ///< Write-only, starting at end of file.
   };
   
   enum SeekMode
   {
      Begin,               ///< Relative to the start of the file
      Current,             ///< Relative to the current position
      End,                 ///< Relative to the end of the file
   };

   /// Flags used to indicate what we can do to the file.
   enum Capability
   {
      FileRead         = BIT(0),
      FileWrite        = BIT(1)
   };

protected:
   Status currentStatus;   ///< Current status of the file (Ok, IOError, etc.).
   U32 capability;         ///< Keeps track of file capabilities.

   File(const File&);              ///< This is here to disable the copy constructor.
   File& operator=(const File&);   ///< This is here to disable assignment.

public:
   File() : currentStatus(Closed), capability(0) {;}
   virtual ~File() {}

   /// Opens a file for access using the specified AccessMode
   ///
   /// @returns The status of the file
   virtual Status open(const char *filename, const AccessMode openMode) = 0;

   /// Gets the current position in the file
   ///
   /// This is in bytes from the beginning of the file.
   virtual U32 getPosition() const = 0;

   /// Sets the current position in the file.
   ///
   /// You can set either a relative or absolute position to go to in the file.
   ///
   /// @code
   /// File *foo;
   ///
   /// ... set up file ...
   ///
   /// // Go to byte 32 in the file...
   /// foo->setPosition(32);
   ///
   /// // Now skip back 20 bytes...
   /// foo->setPosition(-20, false);
   ///
   /// // And forward 17...
   /// foo->setPosition(17, false);
   /// @endcode
   ///
   /// @returns The status of the file
   virtual Status setPosition(S32 position, SeekMode seekMode=File::Begin) = 0;

   /// Returns the size of the file
   virtual U32 getSize() const = 0;

   /// Make sure everything that's supposed to be written to the file gets written.
   ///
   /// @returns The status of the file.
   virtual Status flush() = 0;

   /// Closes the file
   ///
   /// @returns The status of the file.
   virtual Status close() = 0;

   /// Gets the status of the file
   File::Status getStatus() const
   {
      return currentStatus;
   }

   /// Reads "size" bytes from the file, and dumps data into "dst".
   /// The number of actual bytes read is returned in bytesRead
   /// @returns The status of the file
   virtual File::Status read(U32 size, char *dst, U32 *bytesRead = NULL) = 0;

   /// Writes "size" bytes into the file from the pointer "src".
   /// The number of actual bytes written is returned in bytesWritten
   /// @returns The status of the file
   virtual File::Status write(U32 size, const char *src, U32 *bytesWritten = NULL) = 0;

   /// Returns whether or not this file is capable of the given function.
   bool hasCapability(Capability cap) const
   {
      return (0 != (U32(cap) & capability));
   }
   
   virtual File *clone() = 0;

protected:
   virtual Status setStatus() = 0;                 ///< Called after error encountered.
   File::Status setStatus(File::Status status)
   {
      return currentStatus = status;
   }
   
public:
   // Opens a file
   static File *openFile(const String &file, File::AccessMode mode);
};

END_NS

#endif // _FILE_IO_H_
