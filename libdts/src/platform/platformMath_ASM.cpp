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
#include "math/mMath.h"

//-----------------------------------------------------------------------------

BEGIN_NS(DTShape)

#if defined(LIBDTSHAPE_SUPPORTS_VC_INLINE_X86_ASM)
static S32 m_mulDivS32_ASM(S32 a, S32 b, S32 c)
{  // a * b / c
   S32 r;
   _asm
   {
      mov   eax, a
      imul  b
      idiv  c
      mov   r, eax
   }
   return r;
}

//-----------------------------------------------------------------------------

static U32 m_mulDivU32_ASM(S32 a, S32 b, U32 c)
{  // a * b / c
   S32 r;
   _asm
   {
      mov   eax, a
      mov   edx, 0
      mul   b
      div   c
      mov   r, eax
   }
   return r;
}

//-----------------------------------------------------------------------------

static void m_sincos_ASM( F32 angle, F32 *s, F32 *c )
{
   _asm
   {
      fld     angle
      fsincos
      mov     eax, c
      fstp    dword ptr [eax]
      mov     eax, s
      fstp    dword ptr [eax]
   }
}

//-----------------------------------------------------------------------------

U32 Platform::getMathControlState()
{
   U16 cw;
   _asm
   {
      fstcw cw
   }
   return cw;
}

//-----------------------------------------------------------------------------

void Platform::setMathControlState(U32 state)
{
   U16 cw = state;
   _asm
   {
      fldcw cw
   }
}

//-----------------------------------------------------------------------------

void Platform::setMathControlStateKnown()
{
   U16 cw = 0x27F;
   _asm
   {
      fldcw cw
   }
}

//-----------------------------------------------------------------------------

#elif defined(LIBDTSHAPE_SUPPORTS_GCC_INLINE_X86_ASM)

U32 Platform::getMathControlState()
{
   U16 cw;
   __asm__ __volatile__(
      "fstcw %0;" : "=m" (cw)
   );
   return cw;
}

//-----------------------------------------------------------------------------

void Platform::setMathControlStateKnown()
{
   U16 cw = 0x27F;
   __asm__ __volatile__(
      "fldcw %0;" : "=m" (cw)
   );
}

//-----------------------------------------------------------------------------

void Platform::setMathControlState(U32 state)
{
   U16 cw = state;
   __asm__ __volatile__(
      "fldcw %0;" : "=m" (cw)
   );
}

//-----------------------------------------------------------------------------

static S32 m_mulDivS32_ASM(S32 a, S32 b, S32 c)
{  // a * b / c
   S32 r;
   
   __asm__ __volatile__(
      "imul  %2\n"
      "idiv  %3\n"
      : "=a" (r) : "a" (a) , "b" (b) , "c" (c) 
      );
   return r;
}   


static U32 m_mulDivU32_ASM(S32 a, S32 b, U32 c)
{  // a * b / c
   S32 r;
   __asm__ __volatile__(
      "mov   $0, %%edx\n"
      "mul   %2\n"
      "div   %3\n"
      : "=a" (r) : "a" (a) , "b" (b) , "c" (c) 
      );
   return r;
}

#else

U32 Platform::getMathControlState()
{
   return 0;
}

//-----------------------------------------------------------------------------

void Platform::setMathControlStateKnown()
{
}

//-----------------------------------------------------------------------------

void Platform::setMathControlState(U32 state)
{
}

#endif

//------------------------------------------------------------------------------
void mInstallLibrary_ASM()
{
#if defined(LIBDTSHAPE_SUPPORTS_VC_INLINE_X86_ASM)
   m_mulDivS32              = m_mulDivS32_ASM;
   m_mulDivU32              = m_mulDivU32_ASM;
   
   m_sincos = m_sincos_ASM;
#endif

#if defined(LIBDTSHAPE_SUPPORTS_GCC_INLINE_X86_ASM)
   m_mulDivS32              = m_mulDivS32_ASM;
   m_mulDivU32              = m_mulDivU32_ASM;
#endif
}

END_NS
