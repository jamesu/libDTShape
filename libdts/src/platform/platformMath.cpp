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
#include "core/log.h"
#include "math/mMath.h"
#include "core/strings/stringFunctions.h"

//-----------------------------------------------------------------------------

BEGIN_NS(DTShape)

//-----------------------------------------------------------------------------

extern void mInstallLibrary_C();
extern void mInstallLibrary_ASM();


extern void mInstall_AMD_Math();
extern void mInstall_Library_SSE();

//------------------------------------------------------------------------------
void Math::init(U32 properties)
{
   if (!properties)
      // detect what's available
      properties = Platform::SystemInfo.processor.properties;
   else
      // Make sure we're not asking for anything that's not supported
      properties &= Platform::SystemInfo.processor.properties;

   Log::printf("Math Init:");
   Log::printf("   Installing Standard C extensions");
   mInstallLibrary_C();

   Log::printf("   Installing Assembly extensions");
   mInstallLibrary_ASM();

   if (properties & CPU_PROP_FPU)
   {
      Log::printf("   Installing FPU extensions");
   }

   if (properties & CPU_PROP_MMX)
   {
      Log::printf("   Installing MMX extensions");
      if (properties & CPU_PROP_3DNOW)
      {
         Log::printf("   Installing 3DNow extensions");
         mInstall_AMD_Math();
      }
   }

#if !defined(__MWERKS__) || (__MWERKS__ >= 0x2400)
   if (properties & CPU_PROP_SSE)
   {
      Log::printf("   Installing SSE extensions");
      mInstall_Library_SSE();
   }
#endif //mwerks>2.4

   Log::printf(" ");
}


//------------------------------------------------------------------------------

static MRandomLCG sgPlatRandom;

F32 Platform::getRandom()
{
   return sgPlatRandom.randF();
}

END_NS
