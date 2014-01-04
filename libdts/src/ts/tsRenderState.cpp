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

#include "platform/platform.h"
#include "ts/tsRenderState.h"
#include "ts/tsMesh.h"
#include "ts/tsMaterial.h"
#include "ts/tsShapeInstance.h"

//-----------------------------------------------------------------------------

BEGIN_NS(DTShape)

//-----------------------------------------------------------------------------

TSRenderState::TSRenderState()
   :  mState( NULL ),
      mWorldMatrix(1),
      mFadeOverride( 1.0f ),
      mNoRenderTranslucent( false ),
      mNoRenderNonTranslucent( false ),
      mMaterialHint( NULL ),
      mCuller( NULL ),
      mUseOriginSort( false ),
      smNodeCurrentRotations(__FILE__, __LINE__),
      smNodeCurrentTranslations(__FILE__, __LINE__),
      smNodeCurrentUniformScales(__FILE__, __LINE__),
      smNodeCurrentAlignedScales(__FILE__, __LINE__),
      smNodeCurrentArbitraryScales(__FILE__, __LINE__),
      smNodeLocalTransforms(__FILE__, __LINE__),
      smRotationThreads(__FILE__, __LINE__),
      smTranslationThreads(__FILE__, __LINE__),
      smScaleThreads(__FILE__, __LINE__)
{
   smDetailAdjust = 1.0f;
   smSmallestVisiblePixelSize = -1.0f;
   smNumSkipRenderDetails = 0;
   
   smLastScreenErrorTolerance = 0.0f;
   smLastScaledDistance = 0.0f;
   smLastPixelSize = 0.0f;
   
}

TSRenderState::TSRenderState( TSRenderState &state )
   :  mState( state.mState ),
      mWorldMatrix(1),
      mFadeOverride( state.mFadeOverride ),
      mNoRenderTranslucent( state.mNoRenderTranslucent ),
      mNoRenderNonTranslucent( state.mNoRenderNonTranslucent ),
      mMaterialHint( state.mMaterialHint ),
      mCuller( state.mCuller ),
      mUseOriginSort( state.mUseOriginSort )//,
      //mMeshRenderInfos( state.mMeshRenderInfos )
{
}

void TSRenderState::reset()
{
   mRenderInsts.clear();
   mTranslucentRenderInsts.clear();
   mChunker.clear();
   
   smNodeCurrentRotations.clear();
   smNodeCurrentTranslations.clear();
   smNodeCurrentUniformScales.clear();
   smNodeCurrentUniformScales.clear();
   smNodeCurrentAlignedScales.clear();
   smNodeCurrentArbitraryScales.clear();
   smNodeLocalTransforms.clear();
   smNodeLocalTransformDirty.clearAll();
   
   smRotationThreads.clear();
   smTranslationThreads.clear();
   smScaleThreads.clear();
}


/// Allocates a new TSRenderInst
TSRenderInst *TSRenderState::allocRenderInst()
{
   TSRenderInst *inst = mChunker.alloc<TSRenderInst>();
   inst->clear();
   return inst;
}

/// Allocates a new world matrix
MatrixF *TSRenderState::allocMatrix(const MatrixF &transform)
{
   MatrixF *mat = mChunker.alloc<MatrixF>();
   *mat = transform;
   return mat;
}

/// Adds a new TSRenderInst to the rendering pool
void TSRenderState::addRenderInst(TSRenderInst *inst)
{
   // Place in the correct bin
   if (!inst->translucentSort) {
      // Inverse sort
      const F32 invSortDistSq = F32_MAX - inst->sortDistSq;
      inst->defaultKey = *((U32*)&invSortDistSq);
      mRenderInsts.push_back(inst);
   } else {
      inst->defaultKey = inst->sortDistSq;
      mTranslucentRenderInsts.push_back(inst);
   }
   
   // Sort by material if present
   if (inst->matInst)
      inst->defaultKey2 = inst->matInst->getStateHint();
}

static S32 RenderStateSortFunc(const void *p1, const void *p2)
{
   const TSRenderInst* ri1 = (const TSRenderInst*)p1;
   const TSRenderInst* ri2 = (const TSRenderInst*)p2;
   
   S32 test1 = ri2->defaultKey - ri1->defaultKey;
   
   return ( test1 == 0 ) ? S32(ri1->defaultKey2) - S32(ri2->defaultKey2) : test1;
}

void TSRenderState::sortRenderInsts()
{
   dQsort(mRenderInsts.address(), mRenderInsts.size(), sizeof(TSRenderState*), RenderStateSortFunc);
   dQsort(mTranslucentRenderInsts.address(), mTranslucentRenderInsts.size(), sizeof(TSRenderState*), RenderStateSortFunc);
}

void TSRenderInst::clear()
{
   dMemset(this, '\0', sizeof(TSRenderInst));
}

void TSRenderInst::render(TSRenderState *renderState)
{
   mesh->mRenderer->doRenderInst(mesh, this, renderState);
}

//-----------------------------------------------------------------------------

END_NS
