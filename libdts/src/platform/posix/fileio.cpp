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
#include "platform/fileio.h"
#include "core/strings/unicode.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <libgen.h>
#include <dirent.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/syslimits.h>

#if defined(__FreeBSD__)
#include <sys/types.h>
#endif

#include <string.h>

//-----------------------------------------------------------------------------

BEGIN_NS(DTShape)

//-----------------------------------------------------------------------------

const int MaxPath = PATH_MAX;

int x86UNIXOpen(const char *path, int oflag)
{
   return open(path, oflag, 0666);
}

int x86UNIXClose(int fd)
{
   return close(fd);
}

ssize_t x86UNIXRead(int fd, void *buf, size_t nbytes)
{
   return read(fd, buf, nbytes);
}

ssize_t x86UNIXWrite(int fd, const void *buf, size_t nbytes)
{
   return write(fd, buf, nbytes);
}

class PosixFile : public File
{
private:
   PosixFile(const File&);              ///< This is here to disable the copy constructor.
   PosixFile& operator=(const File&);   ///< This is here to disable assignment.
   
public:
   
   String name;
   void *handle;
   
   PosixFile() : name(), handle(0)
   {
      handle = (void *)NULL;
   }
   
   virtual ~PosixFile()
   {
      close();
   }
   
   File::Status open(const char *filename, const File::AccessMode openMode)
   {
      AssertFatal(NULL != filename, "File::open: NULL filename");
      AssertWarn(NULL == handle, "File::open: handle already valid");
      
      // Close the file if it was already open...
      if (File::Closed != currentStatus)
         close();
      
      int oflag;
      handle = (void *)malloc(sizeof(int));
      
      switch (openMode)
      {
         case Read:
            oflag = O_RDONLY;
            break;
         case Write:
            oflag = O_WRONLY | O_CREAT | O_TRUNC;
            break;
         case ReadWrite:
            oflag = O_RDWR | O_CREAT;
            break;
         case WriteAppend:
            oflag = O_WRONLY | O_CREAT | O_APPEND;
            break;
         default:
            AssertFatal(false, "File::open: bad access mode");    // impossible
      }
      
      // if we are writing, make sure output path exists
      if (openMode == File::Write || openMode == File::ReadWrite || openMode == File::WriteAppend)
         Platform::createPath(filename);
      
      int fd = -1;
      fd = x86UNIXOpen(filename, oflag);
      
      memcpy(handle, &fd, sizeof(int));
      
#ifdef DEBUG
      //   fprintf(stdout,"fd = %d, handle = %d\n", fd, *((int *)handle));
#endif
      
      if (*((int *)handle) == -1)
      {
         // handle not created successfully
         //Log::errorf("Can't open file: %s", filename);
         return setStatus();
      }
      else
      {
         name = filename;
         
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
         return currentStatus = File::Ok;                                // success!
      }
   }
   
   U32 getPosition() const
   {
      AssertFatal(File::Closed != currentStatus, "File::getPosition: file closed");
      AssertFatal(NULL != handle, "File::getPosition: invalid file handle");
      
#ifdef DEBUG
      //   fprintf(stdout, "handle = %d\n",*((int *)handle));fflush(stdout);
#endif
      return (U32) lseek(*((int *)handle), 0, SEEK_CUR);
   }
   
   File::Status setPosition(S32 position, File::SeekMode seekMode)
   {
      
      AssertFatal(Closed != currentStatus, "File::setPosition: file closed");
      AssertFatal(NULL != handle, "File::setPosition: invalid file handle");
      
      if (Ok != currentStatus && EOS != currentStatus)
         return currentStatus;
      
      U32 finalPos = 0;
      switch (seekMode)
      {
         case Begin:                                                    // absolute position
            AssertFatal(0 <= position, "File::setPosition: negative absolute position");
            
            // position beyond EOS is OK
            finalPos = lseek(*((int *)handle), position, SEEK_SET);
            break;
         case Current:                                                  // relative position
            AssertFatal((getPosition() >= (U32)abs(position) && 0 > position) || 0 <= position, "File::setPosition: negative relative position");
            
            // position beyond EOS is OK
            finalPos = lseek(*((int *)handle), position, SEEK_CUR);
            break;
         case End:                                                  // relative position
            
            // position beyond EOS is OK
            finalPos = lseek(*((int *)handle), position, SEEK_END);
            break;
      }
      
      if (0xffffffff == finalPos)
         return setStatus();                                        // unsuccessful
      else if (finalPos >= getSize())
         return currentStatus = EOS;                                // success, at end of file
      else
         return currentStatus = Ok;                                // success!
   }
   
   /// Returns the size of the file
   U32 getSize() const
   {
      AssertWarn(Closed != currentStatus, "File::getSize: file closed");
      AssertFatal(NULL != handle, "File::getSize: invalid file handle");
      
      if (Ok == currentStatus || EOS == currentStatus)
      {
         long	currentOffset = getPosition();                  // keep track of our current position
         long	fileSize;
         lseek(*((int *)handle), 0, SEEK_END);                     // seek to the end of the file
         fileSize = getPosition();                               // get the file size
         lseek(*((int *)handle), currentOffset, SEEK_SET);         // seek back to our old offset
         return fileSize;                                        // success!
      }
      else
         return 0;                                               // unsuccessful
   }
   
   /// Make sure everything that's supposed to be written to the file gets written.
   ///
   /// @returns The status of the file.
   File::Status flush()
   {
      AssertFatal(Closed != currentStatus, "File::flush: file closed");
      AssertFatal(NULL != handle, "File::flush: invalid file handle");
      AssertFatal(true == hasCapability(FileWrite), "File::flush: cannot flush a read-only file");
      
      if (fsync(*((int *)handle)) == 0)
         return currentStatus = Ok;                                // success!
      else
         return setStatus();                                       // unsuccessful
   }
   
   /// Closes the file
   ///
   /// @returns The status of the file.
   File::Status close()
   {
      // if the handle is non-NULL, close it if necessary and free it
      if (NULL != handle)
      {
         // make a local copy of the handle value and
         // free the handle
         int handleVal = *((int *)handle);
         free(handle);
         handle = (void *)NULL;
         
         // close the handle if it is valid
         if (handleVal != -1 && x86UNIXClose(handleVal) != 0)
            return setStatus();                                    // unsuccessful
      }
      // Set the status to closed
      return currentStatus = Closed;
   }
   
   /// Reads "size" bytes from the file, and dumps data into "dst".
   /// The number of actual bytes read is returned in bytesRead
   /// @returns The status of the file
   File::Status read(U32 size, char *dst, U32 *bytesRead)
   {
#ifdef DEBUG
      //   fprintf(stdout,"reading %d bytes\n",size);fflush(stdout);
#endif
      AssertFatal(Closed != currentStatus, "File::read: file closed");
      AssertFatal(NULL != handle, "File::read: invalid file handle");
      AssertFatal(NULL != dst, "File::read: NULL destination pointer");
      AssertFatal(true == hasCapability(FileRead), "File::read: file lacks capability");
      AssertWarn(0 != size, "File::read: size of zero");
      
      /* show stats for this file */
#ifdef DEBUG
      //struct stat st;
      //fstat(*((int *)handle), &st);
      //fprintf(stdout,"file size = %d\n", st.st_size);
#endif
      /****************************/
      long lastBytes=0;
      File::Status lastStatus = File::Ok;

      if (Ok != currentStatus || 0 == size) {
         lastStatus = currentStatus;
	  } else
      {
         long *bytes = &lastBytes;
         if ( (*((U32 *)bytes) = x86UNIXRead(*((int *)handle), dst, size)) == -1)
         {
#ifdef DEBUG
            //   fprintf(stdout,"unsuccessful: %d\n", *((U32 *)bytes));fflush(stdout);
#endif
            setStatus();                                    // unsuccessful
            lastStatus = currentStatus;
         } else {
            //            dst[*((U32 *)bytes)] = '\0';
            if (*((U32 *)bytes) != size || *((U32 *)bytes) == 0) {
#ifdef DEBUG
               //  fprintf(stdout,"end of stream: %d\n", *((U32 *)bytes));fflush(stdout);
#endif
               currentStatus = EOS;                        // end of stream
               lastStatus = currentStatus;
            }
         }
      }
      //    dst[*bytesRead] = '\0';
#ifdef DEBUG
      //fprintf(stdout, "We read:\n");
      //fprintf(stdout, "====================================================\n");
      //fprintf(stdout, "%s\n",dst);
      //fprintf(stdout, "====================================================\n");
      //fprintf(stdout,"read ok: %d\n", *bytesRead);fflush(stdout);
#endif

	  // if bytesRead is a valid pointer, put number of bytes read there.
	  if(bytesRead)
         *bytesRead = lastBytes;

      currentStatus = lastStatus;
      return currentStatus;                                    // successfully read size bytes
   }
   
   /// Writes "size" bytes into the file from the pointer "src".
   /// The number of actual bytes written is returned in bytesWritten
   /// @returns The status of the file
   virtual File::Status write(U32 size, const char *src, U32 *bytesWritten)
   {
      // JMQ: despite the U32 parameters, the maximum filesize supported by this
      // function is probably the max value of S32, due to the unix syscall
      // api.
      AssertFatal(Closed != currentStatus, "File::write: file closed");
      AssertFatal(NULL != handle, "File::write: invalid file handle");
      AssertFatal(NULL != src, "File::write: NULL source pointer");
      AssertFatal(true == hasCapability(FileWrite), "File::write: file lacks capability");
      AssertWarn(0 != size, "File::write: size of zero");

      long lastBytes=0;
      File::Status lastStatus = File::Ok;
      
      if ((Ok != currentStatus && EOS != currentStatus) || 0 == size) {
		  lastStatus = currentStatus;
	  } else
      {
         lastBytes = x86UNIXWrite(*((int *)handle), src, size);
         if (lastBytes < 0)
         {
			lastBytes = 0;
            setStatus();
            lastStatus = currentStatus;
         }
         else
         {
            lastStatus = Ok;
         }
      }

	  // if bytesWritten is a valid pointer, put number of bytes read there.
	  if(bytesWritten)
         *bytesWritten = lastBytes;

	  currentStatus = lastStatus;
	  return currentStatus;
   }
   
   virtual File *clone()
   {
      File::AccessMode openMode = File::Read;
      
      if (capability & File::FileWrite)
         openMode = File::WriteAppend;
      else
         openMode = File::Read;
      
      PosixFile *file = new PosixFile();
      if (file->open(name.c_str(), openMode))
      {
         return file;
      }
      
      delete file;
      return NULL;
   }
   
protected:
   virtual File::Status setStatus()
   {
      //Log::printf("File IO error: %s", strerror(errno));
      return currentStatus = IOError;
   }
};

File *File::openFile(const String &file, File::AccessMode mode)
{
   PosixFile *instance = new PosixFile();
   if (instance->open(file.c_str(), mode) == File::Ok)
   {
      return instance;
   }
   
   delete instance;
   return NULL;
}

bool Platform::createPath(const char * filename)
{
   return false; // TODO
}

bool Platform::deletePath(const char * filename)
{
   return false; // TODO
}

bool Platform::fileDelete(const char *name)
{
   return(remove(name) == 0);
}

S32 Platform::compareFileTimes(const FileTime &a, const FileTime &b)
{
   if(a > b)
      return 1;
   if(a < b)
      return -1;
   return 0;
}

//-----------------------------------------------------------------------------
bool Platform::isFile(const char *path)
{
   if (!path || !*path)
      return false;
   
   // make sure we can stat the file
   struct stat statData;
   if( stat(path, &statData) < 0 )
      return false;
   
   // now see if it's a regular file
   if( (statData.st_mode & S_IFMT) == S_IFREG)
      return true;
   
   return false;
}

//-----------------------------------------------------------------------------
bool Platform::isDirectory(const char *path)
{
   if (!path || !*path)
      return false;
   
   // make sure we can stat the file
   struct stat statData;
   if( stat(path, &statData) < 0 )
      return false;
   
   // now see if it's a directory
   if( (statData.st_mode & S_IFMT) == S_IFDIR)
      return true;
   
   return false;
}


//-----------------------------------------------------------------------------
static bool GetFileTimes(const char *filePath, FileTime *createTime, FileTime *modifyTime)
{
   struct stat fStat;
   
   if (stat(filePath, &fStat) == -1)
      return false;
   
   if(createTime)
   {
      // no where does SysV/BSD UNIX keep a record of a file's
      // creation time.  instead of creation time I'll just use
      // changed time for now.
      *createTime = fStat.st_ctime;
   }
   if(modifyTime)
   {
      *modifyTime = fStat.st_mtime;
   }
   
   return true;
}

//-----------------------------------------------------------------------------
bool Platform::getFileTimes(const char *filePath, FileTime *createTime, FileTime *modifyTime)
{
   // if it starts with cwd, we need to strip that off so that we can look for
   // the file in the pref dir
   char cwd[MaxPath];
   getcwd(cwd, MaxPath);
   if (strstr(filePath, cwd) == filePath)
      filePath = filePath + strlen(cwd) + 1;
   
   struct stat fStat;
   
   if (stat(filePath, &fStat) == -1)
      return false;
   
   if(createTime)
   {
      // no where does SysV/BSD UNIX keep a record of a file's
      // creation time.  instead of creation time I'll just use
      // changed time for now.
      *createTime = fStat.st_ctime;
   }
   if(modifyTime)
   {
      *modifyTime = fStat.st_mtime;
   }
   
   return true;
}

char sRootDir[PATH_MAX];// Return path to root (usually cwd)
bool sGotCwd = false;

const char *Platform::getRootDir()
{
	if (sGotCwd) {
		return sRootDir;
	} else {
		getcwd(sRootDir, MaxPath);
		return sRootDir;
	}
}

END_NS
