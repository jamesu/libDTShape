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

#include "libDTShapeConfig.h"
#include "platform/platform.h"
#include "core/fileio.h"

#include "core/strings/unicode.h"

#ifndef	WINVER
#define WINVER  0x0500      /* version 5.0 */
#endif	//!WINVER


// define this so that we can use WM_MOUSEWHEEL messages...
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0400
#endif

#include <windows.h>

// Microsoft VC++ has this POSIX header in the wrong directory
#if defined(TORQUE_COMPILER_VISUALC)
#include <sys/utime.h>
#elif defined (TORQUE_COMPILER_GCC)
#include <time.h>
#include <sys/utime.h>
#else
#include <utime.h>
#endif


template< typename T >
inline void forwardslashT( T *str )
{
   while(*str)
   {
      if(*str == '\\')
         *str = '/';
      str++;
   }
}

inline void forwardslash( char* str )
{
   forwardslashT< char >( str );
}
inline void forwardslash( WCHAR* str )
{
   forwardslashT< WCHAR >( str );
}

template< typename T >
inline void backslashT( T *str )
{
   while(*str)
   {
      if(*str == '/')
         *str = '\\';
      str++;
   }
}

inline void backslash( char* str )
{
   backslashT< char >( str );
}
inline void backslash( WCHAR* str )
{
   backslashT< WCHAR >( str );
}

//-----------------------------------------------------------------------------
bool Platform::fileDelete(const char *name)
{
   AssertFatal( name != NULL, "dFileDelete - NULL file name" );

   TempAlloc< TCHAR > buf( dStrlen( name ) + 1 );

#ifdef UNICODE
   convertUTF8toUTF16( name, (UTF16*)buf.ptr, buf.size );
#else
   dStrcpy( buf, name );
#endif

   backslash( buf );
   if( Platform::isFile( name ) )
      return DeleteFile( buf );
   else
      return RemoveDirectory( buf );
}

//-----------------------------------------------------------------------------
// Constructors & Destructor
//-----------------------------------------------------------------------------


class WinFile : public File
{
private:
   WinFile(const WinFile&);              ///< This is here to disable the copy constructor.
   WinFile& operator=(const WinFile&);   ///< This is here to disable assignment.
   
public:
   
   String name;
   void *handle;
   
   WinFile() : name(), handle(0)
   {
      handle = (void *)INVALID_HANDLE_VALUE;
   }
   
   virtual ~WinFile()
   {
      close();
   }
 
	//-----------------------------------------------------------------------------
	// Open a file in the mode specified by openMode (Read, Write, or ReadWrite).
	// Truncate the file if the mode is either Write or ReadWrite and truncate is
	// true.
	//
	// Sets capability appropriate to the openMode.
	// Returns the currentStatus of the file.
	//-----------------------------------------------------------------------------
	File::Status open(const char *filename, const AccessMode openMode)
	{
	   AssertFatal(NULL != filename, "File::open: NULL fname");
	   AssertWarn(INVALID_HANDLE_VALUE == (HANDLE)handle, "File::open: handle already valid");

	   TempAlloc< TCHAR > fname( dStrlen( filename ) + 1 );

	#ifdef UNICODE
	   convertUTF8toUTF16( filename, (UTF16*)fname.ptr, fname.size );
	#else
	   dStrcpy(fname, filename);
	#endif
	   backslash( fname );

	   // Close the file if it was already open...
	   if (Closed != currentStatus)
		  close();

	   // create the appropriate type of file...
	   switch (openMode)
	   {
	   case Read:
		  handle = (void *)CreateFile(fname,
			 GENERIC_READ,
			 FILE_SHARE_READ,
			 NULL,
			 OPEN_EXISTING,
			 FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
			 NULL);
		  break;
	   case Write:
		  handle = (void *)CreateFile(fname,
			 GENERIC_WRITE,
			 0,
			 NULL,
			 CREATE_ALWAYS,
			 FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
			 NULL);
		  break;
	   case ReadWrite:
		  handle = (void *)CreateFile(fname,
			 GENERIC_WRITE | GENERIC_READ,
			 0,
			 NULL,
			 OPEN_ALWAYS,
			 FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
			 NULL);
		  break;
	   case WriteAppend:
		  handle = (void *)CreateFile(fname,
			 GENERIC_WRITE,
			 0,
			 NULL,
			 OPEN_ALWAYS,
			 FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
			 NULL);
		  break;

	   default:
		  AssertFatal(false, "File::open: bad access mode");    // impossible
	   }

	   if (INVALID_HANDLE_VALUE == (HANDLE)handle)                // handle not created successfully
	   {
		  return setStatus();
	   }
	   else
	   {
		  // successfully created file, so set the file capabilities...
		  switch (openMode)
		  {
		  case Read:
			 capability = U32(FileRead);
			 break;
		  case Write:
		  case WriteAppend:
			 capability = U32(FileWrite);
			 break;
		  case ReadWrite:
			 capability = U32(FileRead)  |
				U32(FileWrite);
			 break;
		  default:
			 AssertFatal(false, "File::open: bad access mode");
		  }
		  return currentStatus = Ok;                                // success!
	   }
	}
   

	//-----------------------------------------------------------------------------
	// Get the current position of the file pointer.
	//-----------------------------------------------------------------------------
	U32 getPosition() const
	{
		AssertFatal(Closed != currentStatus, "File::getPosition: file closed");
		AssertFatal(INVALID_HANDLE_VALUE != (HANDLE)handle, "File::getPosition: invalid file handle");

		return SetFilePointer((HANDLE)handle,
							  0,                                    // how far to move
							  NULL,                                    // pointer to high word
							  FILE_CURRENT);                        // from what point
	}
   
	//-----------------------------------------------------------------------------
	// Set the position of the file pointer.
	// Absolute and relative positioning is supported via the absolutePos
	// parameter.
	//
	// If positioning absolutely, position MUST be positive - an IOError results if
	// position is negative.
	// Position can be negative if positioning relatively, however positioning
	// before the start of the file is an IOError.
	//
	// Returns the currentStatus of the file.
	//-----------------------------------------------------------------------------
	File::Status setPosition(S32 position, File::SeekMode seekMode)
	{
		AssertFatal(Closed != currentStatus, "File::setPosition: file closed");
		AssertFatal(INVALID_HANDLE_VALUE != (HANDLE)handle, "File::setPosition: invalid file handle");

		if (Ok != currentStatus && EOS != currentStatus)
			return currentStatus;
		
      U32 finalPos = 0;
      switch (seekMode)
      {
         case Begin:                                                    // absolute position
            AssertFatal(0 <= position, "File::setPosition: negative absolute position");
            
            // position beyond EOS is OK
            finalPos = SetFilePointer((HANDLE)handle,
									  position,
									  NULL,
									  FILE_BEGIN);
            break;
         case Current:                                                  // relative position
            AssertFatal((getPosition() >= (U32)abs(position) && 0 > position) || 0 <= position, "File::setPosition: negative relative position");
            
            // position beyond EOS is OK
			finalPos = SetFilePointer((HANDLE)handle,
									  position,
									  NULL,
									  FILE_CURRENT);
            break;
         case End:                                                  // relative position
            
            // position beyond EOS is OK
            finalPos = SetFilePointer((HANDLE)handle,
									  position,
									  NULL,
									  FILE_END);
            break;
      }

		if (0xffffffff == finalPos)
			return setStatus();                                        // unsuccessful
		else if (finalPos >= getSize())
			return currentStatus = EOS;                                // success, at end of file
		else
			return currentStatus = Ok;                                // success!
	}

   
	//-----------------------------------------------------------------------------
	// Get the size of the file in bytes.
	// It is an error to query the file size for a Closed file, or for one with an
	// error status.
	//-----------------------------------------------------------------------------
	U32 getSize() const
	{
		AssertWarn(Closed != currentStatus, "File::getSize: file closed");
		AssertFatal(INVALID_HANDLE_VALUE != (HANDLE)handle, "File::getSize: invalid file handle");

		if (Ok == currentStatus || EOS == currentStatus)
		{
			DWORD high;
			return GetFileSize((HANDLE)handle, &high);                // success!
		}
		else
			return 0;                                                // unsuccessful
	}

	//-----------------------------------------------------------------------------
	// Flush the file.
	// It is an error to flush a read-only file.
	// Returns the currentStatus of the file.
	//-----------------------------------------------------------------------------
	File::Status flush()
	{
		AssertFatal(Closed != currentStatus, "File::flush: file closed");
		AssertFatal(INVALID_HANDLE_VALUE != (HANDLE)handle, "File::flush: invalid file handle");
		AssertFatal(true == hasCapability(FileWrite), "File::flush: cannot flush a read-only file");

		if (0 != FlushFileBuffers((HANDLE)handle))
			return setStatus();                                        // unsuccessful
		else
			return currentStatus = Ok;                                // success!
	}

	//-----------------------------------------------------------------------------
	// Close the File.
	//
	// Returns the currentStatus
	//-----------------------------------------------------------------------------
	File::Status close()
	{
		// check if it's already closed...
		if (Closed == currentStatus)
			return currentStatus;

		// it's not, so close it...
		if (INVALID_HANDLE_VALUE != (HANDLE)handle)
		{
			if (0 == CloseHandle((HANDLE)handle))
				return setStatus();                                    // unsuccessful
		}
		handle = (void *)INVALID_HANDLE_VALUE;
		return currentStatus = Closed;
	}

	//-----------------------------------------------------------------------------
	// Self-explanatory.
	//-----------------------------------------------------------------------------
	File::Status getStatus() const
	{
		return currentStatus;
	}

	//-----------------------------------------------------------------------------
	// Sets and returns the currentStatus to status.
	//-----------------------------------------------------------------------------
	File::Status setStatus(File::Status status)
	{
		return currentStatus = status;
	}

	//-----------------------------------------------------------------------------
	// Read from a file.
	// The number of bytes to read is passed in size, the data is returned in src.
	// The number of bytes read is available in bytesRead if a non-Null pointer is
	// provided.
	//-----------------------------------------------------------------------------
	File::Status read(U32 size, char *dst, U32 *bytesRead)
	{
		AssertFatal(Closed != currentStatus, "File::read: file closed");
		AssertFatal(INVALID_HANDLE_VALUE != (HANDLE)handle, "File::read: invalid file handle");
		AssertFatal(NULL != dst, "File::read: NULL destination pointer");
		AssertFatal(true == hasCapability(FileRead), "File::read: file lacks capability");
		AssertWarn(0 != size, "File::read: size of zero");

		if (Ok != currentStatus || 0 == size)
			return currentStatus;
		else
		{
			DWORD lastBytes;
			DWORD *bytes = (NULL == bytesRead) ? &lastBytes : (DWORD *)bytesRead;
			if (0 != ReadFile((HANDLE)handle, dst, size, bytes, NULL))
			{
				if(*((U32 *)bytes) != size)
					return currentStatus = EOS;                        // end of stream
			}
			else
				return setStatus();                                    // unsuccessful
		}
		return currentStatus = Ok;                                    // successfully read size bytes
	}

	//-----------------------------------------------------------------------------
	// Write to a file.
	// The number of bytes to write is passed in size, the data is passed in src.
	// The number of bytes written is available in bytesWritten if a non-Null
	// pointer is provided.
	//-----------------------------------------------------------------------------
	File::Status write(U32 size, const char *src, U32 *bytesWritten)
	{
		AssertFatal(Closed != currentStatus, "File::write: file closed");
		AssertFatal(INVALID_HANDLE_VALUE != (HANDLE)handle, "File::write: invalid file handle");
		AssertFatal(NULL != src, "File::write: NULL source pointer");
		AssertFatal(true == hasCapability(FileWrite), "File::write: file lacks capability");
		AssertWarn(0 != size, "File::write: size of zero");

		if ((Ok != currentStatus && EOS != currentStatus) || 0 == size)
			return currentStatus;
		else
		{
			DWORD lastBytes;
			DWORD *bytes = (NULL == bytesWritten) ? &lastBytes : (DWORD *)bytesWritten;
			if (0 != WriteFile((HANDLE)handle, src, size, bytes, NULL))
				return currentStatus = Ok;                            // success!
			else
				return setStatus();                                    // unsuccessful
		}
	}

	//-----------------------------------------------------------------------------
	// Self-explanatory.
	//-----------------------------------------------------------------------------
	bool hasCapability(Capability cap) const
	{
		return (0 != (U32(cap) & capability));
	}
   
   virtual File *clone()
   {
      File::AccessMode openMode = File::Read;
      
      if (capability & File::FileWrite)
         openMode = File::WriteAppend;
      else
         openMode = File::Read;
      
      WinFile *file = new WinFile();
      if (file->open(name.c_str(), openMode))
      {
         return file;
      }
      
      delete file;
      return NULL;
   }
   
protected:
	//-----------------------------------------------------------------------------
	// Sets and returns the currentStatus when an error has been encountered.
	//-----------------------------------------------------------------------------
	virtual File::Status setStatus()
	{
		switch (GetLastError())
		{
		case ERROR_INVALID_HANDLE:
		case ERROR_INVALID_ACCESS:
		case ERROR_TOO_MANY_OPEN_FILES:
		case ERROR_FILE_NOT_FOUND:
		case ERROR_SHARING_VIOLATION:
		case ERROR_HANDLE_DISK_FULL:
			  return currentStatus = IOError;

		default:
			  return currentStatus = UnknownError;
		}
	}

};

File *File::openFile(const String &file, File::AccessMode mode)
{
   WinFile *instance = new WinFile();
   if (instance->open(file.c_str(), mode) == File::Ok)
   {
      return instance;
   }
   
   delete instance;
   return NULL;
}

S32 Platform::compareFileTimes(const FileTime &a, const FileTime &b)
{
   if(a.v2 > b.v2)
      return 1;
   if(a.v2 < b.v2)
      return -1;
   if(a.v1 > b.v1)
      return 1;
   if(a.v1 < b.v1)
      return -1;
   return 0;
}

//--------------------------------------

bool Platform::getFileTimes(const char *filePath, FileTime *createTime, FileTime *modifyTime)
{
   WIN32_FIND_DATA findData;

   TempAlloc< TCHAR > fp( dStrlen( filePath ) + 1 );

#ifdef UNICODE
   convertUTF8toUTF16( filePath, (UTF16*)fp.ptr, fp.size );
#else
   dStrcpy( fp, filePath );
#endif

   backslash( fp );

   HANDLE h = FindFirstFile(fp, &findData);
   if(h == INVALID_HANDLE_VALUE)
      return false;

   if(createTime)
   {
      createTime->v1 = findData.ftCreationTime.dwLowDateTime;
      createTime->v2 = findData.ftCreationTime.dwHighDateTime;
   }
   if(modifyTime)
   {
      modifyTime->v1 = findData.ftLastWriteTime.dwLowDateTime;
      modifyTime->v2 = findData.ftLastWriteTime.dwHighDateTime;
   }
   FindClose(h);
   return true;
}

//--------------------------------------
bool Platform::createPath(const char *file)
{
   char pathbuf[1024];
   const char *dir;
   pathbuf[0] = 0;
   U32 pathLen = 0;

   while((dir = dStrchr(file, '/')) != NULL)
   {
      dStrncpy(pathbuf + pathLen, file, dir - file);
      pathbuf[pathLen + dir-file] = 0;
#ifdef UNICODE
      UTF16 b[1024];
      convertUTF8toUTF16((UTF8 *)pathbuf, b, sizeof(b));
      bool ret = CreateDirectory(b, NULL);
#else
      bool ret = CreateDirectory(pathbuf, NULL);
#endif
      pathLen += dir - file;
      pathbuf[pathLen++] = '/';
      file = dir + 1;
   }
   return true;
}

//-------------------------------------

static char sCwd[2048];
static bool sGenCwd = false;

const char* Platform::getRootDir()
{
	UTF16 buf[2048];

	if (sGenCwd)
		return sCwd;

#ifdef USE_CWD
   GetCurrentDirectory( 2048, buf );

#ifdef UNICODE
   char* utf8 = convertUTF16toUTF8( buf );
   dStrncpy(sCwd, utf8, 2048);
   forwardslash( sCwd );
   delete utf8;
   sGenCwd = true;
   return sCwd;
#else
   dStrncpy(sCwd, 2048, buf);
   sGenCwd = true;
   return sCwd;
#endif

#else
   {
      GetModuleFileName( NULL, buf, 2048);
      char* utf8 = convertUTF16toUTF8( buf );

      forwardslash(utf8);

      char *delimiter = dStrrchr( utf8, '/' );

      if( delimiter != NULL )
         *delimiter = 0x00;

	  dStrncpy(sCwd, utf8, 2048);
	  sGenCwd = true;
	  return sCwd;
   }

#endif
}


//--------------------------------------
bool Platform::isFile(const char *pFilePath)
{
   if (!pFilePath || !*pFilePath)
      return false;

   // Get file info
   WIN32_FIND_DATA findData;
#ifdef UNICODE
   UTF16 b[512];
   convertUTF8toUTF16((UTF8 *)pFilePath, b, sizeof(b));
   HANDLE handle = FindFirstFile(b, &findData);
#else
   HANDLE handle = FindFirstFile(pFilePath, &findData);
#endif

   // [neo, 4/6/2007]
   // This used to be after the FindClose() call 
   if(handle == INVALID_HANDLE_VALUE)
      return false;

   FindClose(handle);

   // if the file is a Directory, Offline, System or Temporary then FALSE
   if (findData.dwFileAttributes &
       (FILE_ATTRIBUTE_DIRECTORY|
        FILE_ATTRIBUTE_OFFLINE|
        FILE_ATTRIBUTE_SYSTEM|
        FILE_ATTRIBUTE_TEMPORARY) )
      return false;

   // must be a real file then
   return true;
}

//--------------------------------------
bool Platform::isDirectory(const char *pDirPath)
{
   if (!pDirPath || !*pDirPath)
      return false;

   // Get file info
   WIN32_FIND_DATA findData;
#ifdef UNICODE
   UTF16 b[512];
   convertUTF8toUTF16((UTF8 *)pDirPath, b, sizeof(b));
   HANDLE handle = FindFirstFile(b, &findData);
#else
   HANDLE handle = FindFirstFile(pDirPath, &findData);
#endif

   // [neo, 5/15/2007]
   // This check was AFTER FindClose for some reason - this is most probably the
   // original intent.
   if(handle == INVALID_HANDLE_VALUE)
      return false;

   FindClose(handle);
   
   // if the file is a Directory, Offline, System or Temporary then FALSE
   if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
   {
      // make sure it's a valid game directory
      if (findData.dwFileAttributes & (FILE_ATTRIBUTE_OFFLINE|FILE_ATTRIBUTE_SYSTEM) )
         return false;

      // must be a directory
      return true;
   }

   return false;
}


//------------------------------------------------------------------------------
