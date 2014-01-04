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

#include "core/color.h"
#include "core/frameAllocator.h"

#include "core/stream/stream.h"

#include "core/strings/stringFunctions.h"

#include "core/util/endian.h"

//-----------------------------------------------------------------------------

BEGIN_NS(DTShape)

#define IMPLEMENT_OVERLOADED_READ(type)      \
   bool Stream::read(type* out_read)         \
   {                                         \
      return read(sizeof(type), out_read);   \
   }

#define IMPLEMENT_OVERLOADED_WRITE(type)        \
   bool Stream::write(type in_write)            \
   {                                            \
      return write(sizeof(type), &in_write);    \
   }

#define IMPLEMENT_ENDIAN_OVERLOADED_READ(type)  \
   bool Stream::read(type* out_read)            \
   {                                            \
      type temp;                                \
      bool success = read(sizeof(type), &temp); \
      *out_read = convertLEndianToHost(temp);   \
      return success;                           \
   }

#define IMPLEMENT_ENDIAN_OVERLOADED_WRITE(type)    \
   bool Stream::write(type in_write)               \
   {                                               \
      type temp = convertHostToLEndian(in_write);  \
      return write(sizeof(type), &temp);           \
   }

IMPLEMENT_OVERLOADED_WRITE(S8)
IMPLEMENT_OVERLOADED_WRITE(U8)

IMPLEMENT_ENDIAN_OVERLOADED_WRITE(S16)
IMPLEMENT_ENDIAN_OVERLOADED_WRITE(S32)
IMPLEMENT_ENDIAN_OVERLOADED_WRITE(U16)
IMPLEMENT_ENDIAN_OVERLOADED_WRITE(U32)
IMPLEMENT_ENDIAN_OVERLOADED_WRITE(U64)
IMPLEMENT_ENDIAN_OVERLOADED_WRITE(F32)
IMPLEMENT_ENDIAN_OVERLOADED_WRITE(F64)

IMPLEMENT_OVERLOADED_READ(S8)
IMPLEMENT_OVERLOADED_READ(U8)

IMPLEMENT_ENDIAN_OVERLOADED_READ(S16)
IMPLEMENT_ENDIAN_OVERLOADED_READ(S32)
IMPLEMENT_ENDIAN_OVERLOADED_READ(U16)
IMPLEMENT_ENDIAN_OVERLOADED_READ(U32)
IMPLEMENT_ENDIAN_OVERLOADED_READ(U64)
IMPLEMENT_ENDIAN_OVERLOADED_READ(F32)
IMPLEMENT_ENDIAN_OVERLOADED_READ(F64)


Stream::Stream()
 : m_streamStatus(Closed)
{
}

const char* Stream::getStatusString(const Status in_status)
{
   switch (in_status) {
      case Ok:
         return "StreamOk";
      case IOError:
         return "StreamIOError";
      case EOS:
         return "StreamEOS";
      case IllegalCall:
         return "StreamIllegalCall";
      case Closed:
         return "StreamClosed";
      case UnknownError:
         return "StreamUnknownError";

     default:
      return "Invalid Stream::Status";
   }
}

void Stream::writeString(const char *string, S32 maxLen)
{
   S32 len = string ? dStrlen(string) : 0;
   if(len > maxLen)
      len = maxLen;

   write(U8(len));
   if(len)
      write(len, string);
}

void Stream::readString(char buf[256])
{
   U8 len;
   read(&len);
   read(S32(len), buf);
   buf[len] = 0;
}

void Stream::readLongString(U32 maxStringLen, char *stringBuf)
{
   U32 len;
   read(&len);
   if(len > maxStringLen)
   {
      m_streamStatus = IOError;
      return;
   }
   read(len, stringBuf);
   stringBuf[len] = 0;
}

void Stream::writeLongString(U32 maxStringLen, const char *string)
{
   U32 len = dStrlen(string);
   if(len > maxStringLen)
      len = maxStringLen;
   write(len);
   write(len, string);
}

void Stream::readLine(U8 *buffer, U32 bufferSize)
{
   bufferSize--;  // account for NULL terminator
   U8 *buff = buffer;
   U8 *buffEnd = buff + bufferSize;
   *buff = '\r';

   // strip off preceding white space
   while ( *buff == '\r' )
   {
      if ( !read(buff) || *buff == '\n' )
      {
         *buff = 0;
         return;
      }
   }

   // read line
   while ( buff != buffEnd && read(++buff) && *buff != '\n' )
   {
      if ( *buff == '\r' )
      {

#if defined(LIBDTSHAPE_OS_MAC)
      U32 pushPos = getPosition(); // in case we need to back up.
      if (read(buff)) // feeling free to overwrite the \r as the NULL below will overwrite again...
	      if (*buff != '\n') // then push our position back.
	         setPosition(pushPos);
	   break; // we're always done after seeing the CR...
#else
      buff--; // 'erases' the CR of a CRLF
#endif

      }
   }
   *buff = 0;
}

void Stream::writeText(const char *text)
{
   if (text && text[0])
      write(dStrlen(text), text);
}

void Stream::writeLine(const U8 *buffer)
{
   write(dStrlen((const char *)buffer), buffer);
   write(2, "\r\n");
}

void Stream::_write(const String & str)
{
   U32 len = str.length();

   if (len<255)
      write(U8(len));
   else
   {
      // longer string, write full length
      write(U8(255));

      // fail if longer than 16 bits (will truncate string modulo 2^16)
      AssertFatal(len < (1<<16),"String too long");

      len &= (1<<16)-1;
      write(U16(len));
   }

   write(len,str.c_str());
}

bool Stream::write(const ColorI& rColor)
{
   bool success = write(rColor.red);
   success     |= write(rColor.green);
   success     |= write(rColor.blue);
   success     |= write(rColor.alpha);

   return success;
}

bool Stream::write(const ColorF& rColor)
{
   ColorI temp = rColor;
   return write(temp);
}

bool Stream::read(ColorI* pColor)
{
   bool success = read(&pColor->red);
   success     |= read(&pColor->green);
   success     |= read(&pColor->blue);
   success     |= read(&pColor->alpha);

   return success;
}

bool Stream::read(ColorF* pColor)
{
   ColorI temp;
   bool success = read(&temp);

   *pColor = temp;
   return success;
}

bool Stream::copyFrom(Stream *other)
{
   U8 buffer[1024];
   U32 numBytes = other->getStreamSize() - other->getPosition();
   while((other->getStatus() != Stream::EOS) && numBytes > 0)
   {
      U32 numRead = numBytes > sizeof(buffer) ? sizeof(buffer) : numBytes;
      if(! other->read(numRead, buffer))
         return false;

      if(! write(numRead, buffer))
         return false;

      numBytes -= numRead;
   }

   return true;
}

Stream* Stream::clone() const
{
   return NULL;
}

END_NS
