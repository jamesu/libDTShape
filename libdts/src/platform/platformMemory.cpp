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

#include "platform/platform.h"
#include <stdlib.h>


#if defined(WIN32)
#include <malloc.h>
#elif defined(LIBDTSHAPE_OS_POSIX)
#else
#include <mm_malloc.h>
#endif

#include <string.h>

//-----------------------------------------------------------------------------

BEGIN_NS(DTShape)

//-----------------------------------------------------------------------------

void* dMemcpy(void *dst, const void *src, dsize_t size)
{
   return memcpy(dst,src,size);
}

//--------------------------------------
void* dMemmove(void *dst, const void *src, dsize_t size)
{
   return memmove(dst,src,size);
}


//--------------------------------------
void* dMemset(void *dst, S32 c, dsize_t size)
{
   return memset(dst,c,size);
}

//--------------------------------------
S32 dMemcmp(const void *ptr1, const void *ptr2, dsize_t len)
{
   return memcmp(ptr1, ptr2, len);
}

void* dRealMalloc(dsize_t s)
{
   return malloc(s);
}

void dRealFree(void* p)
{
   free(p);
}

void *dMalloc_aligned(dsize_t in_size, int alignment)
{
#if defined(WIN32)
   return _aligned_malloc(in_size, alignment);
#elif defined(LIBDTSHAPE_OS_POSIX)
   void *outPtr = NULL;
   int ret = posix_memalign(&outPtr, alignment, in_size);
   return outPtr;
#else
   return _mm_malloc(in_size, alignment);
#endif
}

void dFree_aligned(void* p)
{
#if defined(WIN32)
   return _aligned_free(p);
#elif defined(LIBDTSHAPE_OS_POSIX)
   return free(p);
#else
   return _mm_free(p);
#endif
}

//-----------------------------------------------------------------------------

END_NS
